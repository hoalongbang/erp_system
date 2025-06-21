// UI/Supplier/SupplierManagementWidget.cpp
#include "SupplierManagementWidget.h" // Đã rút gọn include
#include "Supplier.h"          // Đã rút gọn include
#include "SupplierService.h"   // Đã rút gọn include
#include "ISecurityManager.h"  // Đã rút gọn include
#include "Logger.h"            // Đã rút gọn include
#include "ErrorHandler.h"      // Đã rút gọn include
#include "Common.h"            // Đã rút gọn include
#include "DateUtils.h"         // Đã rút gọn include
#include "StringUtils.h"       // Đã rút gọn include
#include "CustomMessageBox.h"  // Đã rút gọn include
#include "UserService.h"       // For getting current user roles from SecurityManager
#include "ContactPersonDTO.h"  // Đã rút gọn include
#include "AddressDTO.h"        // Đã rút gọn include

#include <QInputDialog> // For getting input from user
#include <QDoubleValidator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace ERP {
namespace UI {
namespace Supplier {

SupplierManagementWidget::SupplierManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::ISupplierService> supplierService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      supplierService_(supplierService),
      securityManager_(securityManager) {

    if (!supplierService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ nhà cung cấp hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("SupplierManagementWidget: Initialized with null dependencies.");
        return;
    }

    auto authService = securityManager_->getAuthenticationService();
    if (authService) {
        std::string dummySessionId = "current_session_id"; // Placeholder
        std::optional<ERP::Security::DTO::SessionDTO> currentSession = authService->validateSession(dummySessionId);
        if (currentSession) {
            currentUserId_ = currentSession->userId;
            currentUserRoleIds_ = securityManager_->getUserService()->getUserRoles(currentUserId_, {});
        } else {
            currentUserId_ = "system_user";
            currentUserRoleIds_ = {"anonymous"};
            ERP::Logger::Logger::getInstance().warning("SupplierManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("SupplierManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadSuppliers();
    updateButtonsState();
}

SupplierManagementWidget::~SupplierManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void SupplierManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên nhà cung cấp...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &SupplierManagementWidget::onSearchSupplierClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    supplierTable_ = new QTableWidget(this);
    supplierTable_->setColumnCount(6); // ID, Tên, Mã số thuế, Ghi chú, Điều khoản TT, Điều khoản GH, Trạng thái
    supplierTable_->setHorizontalHeaderLabels({"ID", "Tên", "Mã số thuế", "Ghi chú", "Điều khoản TT", "Điều khoản GH", "Trạng thái"});
    supplierTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    supplierTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    supplierTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    supplierTable_->horizontalHeader()->setStretchLastSection(true);
    connect(supplierTable_, &QTableWidget::itemClicked, this, &SupplierManagementWidget::onSupplierTableItemClicked);
    mainLayout->addWidget(supplierTable_);

    // Form elements for editing/adding
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    nameLineEdit_ = new QLineEdit(this);
    taxIdLineEdit_ = new QLineEdit(this);
    notesLineEdit_ = new QLineEdit(this);
    defaultPaymentTermsLineEdit_ = new QLineEdit(this);
    defaultDeliveryTermsLineEdit_ = new QLineEdit(this);
    statusComboBox_ = new QComboBox(this);
    statusComboBox_->addItem("Active", static_cast<int>(ERP::Common::EntityStatus::ACTIVE));
    statusComboBox_->addItem("Inactive", static_cast<int>(ERP::Common::EntityStatus::INACTIVE));
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Common::EntityStatus::PENDING));
    statusComboBox_->addItem("Deleted", static_cast<int>(ERP::Common::EntityStatus::DELETED));

    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Tên:*", this), 1, 0); formLayout->addWidget(nameLineEdit_, 1, 1);
    formLayout->addWidget(new QLabel("Mã số thuế:", this), 2, 0); formLayout->addWidget(taxIdLineEdit_, 2, 1);
    formLayout->addWidget(new QLabel("Ghi chú:", this), 3, 0); formLayout->addWidget(notesLineEdit_, 3, 1);
    formLayout->addWidget(new QLabel("Điều khoản TT mặc định:", this), 4, 0); formLayout->addWidget(defaultPaymentTermsLineEdit_, 4, 1);
    formLayout->addWidget(new QLabel("Điều khoản GH mặc định:", this), 5, 0); formLayout->addWidget(defaultDeliveryTermsLineEdit_, 5, 1);
    formLayout->addWidget(new QLabel("Trạng thái:", this), 6, 0); formLayout->addWidget(statusComboBox_, 6, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addSupplierButton_ = new QPushButton("Thêm mới", this); connect(addSupplierButton_, &QPushButton::clicked, this, &SupplierManagementWidget::onAddSupplierClicked);
    editSupplierButton_ = new QPushButton("Sửa", this); connect(editSupplierButton_, &QPushButton::clicked, this, &SupplierManagementWidget::onEditSupplierClicked);
    deleteSupplierButton_ = new QPushButton("Xóa", this); connect(deleteSupplierButton_, &QPushButton::clicked, this, &SupplierManagementWidget::onDeleteSupplierClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &SupplierManagementWidget::onUpdateSupplierStatusClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &SupplierManagementWidget::clearForm);
    buttonLayout->addWidget(addSupplierButton_);
    buttonLayout->addWidget(editSupplierButton_);
    buttonLayout->addWidget(deleteSupplierButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void SupplierManagementWidget::loadSuppliers() {
    ERP::Logger::Logger::getInstance().info("SupplierManagementWidget: Loading suppliers...");
    supplierTable_->setRowCount(0);

    std::vector<ERP::Supplier::DTO::SupplierDTO> suppliers = supplierService_->getAllSuppliers({}, currentUserId_, currentUserRoleIds_);

    supplierTable_->setRowCount(suppliers.size());
    for (int i = 0; i < suppliers.size(); ++i) {
        const auto& supplier = suppliers[i];
        supplierTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(supplier.id)));
        supplierTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(supplier.name)));
        supplierTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(supplier.taxId.value_or(""))));
        supplierTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(supplier.notes.value_or(""))));
        supplierTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(supplier.defaultPaymentTerms.value_or(""))));
        supplierTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(supplier.defaultDeliveryTerms.value_or(""))));
        supplierTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(supplier.status))));
    }
    supplierTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("SupplierManagementWidget: Suppliers loaded successfully.");
}

void SupplierManagementWidget::onAddSupplierClicked() {
    if (!hasPermission("Supplier.CreateSupplier")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm nhà cung cấp.", QMessageBox::Warning);
        return;
    }
    clearForm();
    showSupplierInputDialog();
}

void SupplierManagementWidget::onEditSupplierClicked() {
    if (!hasPermission("Supplier.UpdateSupplier")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa nhà cung cấp.", QMessageBox::Warning);
        return;
    }

    int selectedRow = supplierTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Nhà Cung Cấp", "Vui lòng chọn một nhà cung cấp để sửa.", QMessageBox::Information);
        return;
    }

    QString supplierId = supplierTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Supplier::DTO::SupplierDTO> supplierOpt = supplierService_->getSupplierById(supplierId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (supplierOpt) {
        showSupplierInputDialog(&(*supplierOpt));
    } else {
        showMessageBox("Sửa Nhà Cung Cấp", "Không tìm thấy nhà cung cấp để sửa.", QMessageBox::Critical);
    }
}

void SupplierManagementWidget::onDeleteSupplierClicked() {
    if (!hasPermission("Supplier.DeleteSupplier")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa nhà cung cấp.", QMessageBox::Warning);
        return;
    }

    int selectedRow = supplierTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Nhà Cung Cấp", "Vui lòng chọn một nhà cung cấp để xóa.", QMessageBox::Information);
        return;
    }

    QString supplierId = supplierTable_->item(selectedRow, 0)->text();
    QString supplierName = supplierTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Nhà Cung Cấp");
    confirmBox.setText("Bạn có chắc chắn muốn xóa nhà cung cấp '" + supplierName + "' (ID: " + supplierId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (supplierService_->deleteSupplier(supplierId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Nhà Cung Cấp", "Nhà cung cấp đã được xóa thành công.", QMessageBox::Information);
            loadSuppliers();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa nhà cung cấp. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void SupplierManagementWidget::onUpdateSupplierStatusClicked() {
    if (!hasPermission("Supplier.UpdateSupplierStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái nhà cung cấp.", QMessageBox::Warning);
        return;
    }

    int selectedRow = supplierTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một nhà cung cấp để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString supplierId = supplierTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Supplier::DTO::SupplierDTO> supplierOpt = supplierService_->getSupplierById(supplierId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!supplierOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy nhà cung cấp để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Supplier::DTO::SupplierDTO currentSupplier = *supplierOpt;
    ERP::Common::EntityStatus newStatus = (currentSupplier.status == ERP::Common::EntityStatus::ACTIVE) ?
                                          ERP::Common::EntityStatus::INACTIVE :
                                          ERP::Common::EntityStatus::ACTIVE;

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Cập nhật trạng thái nhà cung cấp");
    confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái nhà cung cấp '" + QString::fromStdString(currentSupplier.name) + "' thành " + QString::fromStdString(ERP::Common::entityStatusToString(newStatus)) + "?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (supplierService_->updateSupplierStatus(supplierId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Cập nhật trạng thái", "Trạng thái nhà cung cấp đã được cập nhật thành công.", QMessageBox::Information);
            loadSuppliers();
            clearForm();
        } else {
            showMessageBox("Lỗi", "Không thể cập nhật trạng thái nhà cung cấp. Vui lòng kiểm tra log.", QMessageBox::Critical);
        }
    }
}


void SupplierManagementWidget::onSearchSupplierClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["name_contains"] = searchText.toStdString(); // Assuming service supports "contains" filter
    }
    supplierTable_->setRowCount(0);
    std::vector<ERP::Supplier::DTO::SupplierDTO> suppliers = supplierService_->getAllSuppliers(filter, currentUserId_, currentUserRoleIds_);

    supplierTable_->setRowCount(suppliers.size());
    for (int i = 0; i < suppliers.size(); ++i) {
        const auto& supplier = suppliers[i];
        supplierTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(supplier.id)));
        supplierTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(supplier.name)));
        supplierTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(supplier.taxId.value_or(""))));
        supplierTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(supplier.notes.value_or(""))));
        supplierTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(supplier.defaultPaymentTerms.value_or(""))));
        supplierTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(supplier.defaultDeliveryTerms.value_or(""))));
        supplierTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(supplier.status))));
    }
    supplierTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("SupplierManagementWidget: Search completed.");
}

void SupplierManagementWidget::onSupplierTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString supplierId = supplierTable_->item(row, 0)->text();
    std::optional<ERP::Supplier::DTO::SupplierDTO> supplierOpt = supplierService_->getSupplierById(supplierId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (supplierOpt) {
        idLineEdit_->setText(QString::fromStdString(supplierOpt->id));
        nameLineEdit_->setText(QString::fromStdString(supplierOpt->name));
        taxIdLineEdit_->setText(QString::fromStdString(supplierOpt->taxId.value_or("")));
        notesLineEdit_->setText(QString::fromStdString(supplierOpt->notes.value_or("")));
        defaultPaymentTermsLineEdit_->setText(QString::fromStdString(supplierOpt->defaultPaymentTerms.value_or("")));
        defaultDeliveryTermsLineEdit_->setText(QString::fromStdString(supplierOpt->defaultDeliveryTerms.value_or("")));

        int statusIndex = statusComboBox_->findData(static_cast<int>(supplierOpt->status));
        if (statusIndex != -1) {
            statusComboBox_->setCurrentIndex(statusIndex);
        }

    } else {
        showMessageBox("Thông tin Nhà Cung Cấp", "Không thể tải chi tiết nhà cung cấp đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void SupplierManagementWidget::clearForm() {
    idLineEdit_->clear();
    nameLineEdit_->clear();
    taxIdLineEdit_->clear();
    notesLineEdit_->clear();
    defaultPaymentTermsLineEdit_->clear();
    defaultDeliveryTermsLineEdit_->clear();
    statusComboBox_->setCurrentIndex(0); // Default to Active
    supplierTable_->clearSelection();
    updateButtonsState();
}


void SupplierManagementWidget::showSupplierInputDialog(ERP::Supplier::DTO::SupplierDTO* supplier) {
    QDialog dialog(this);
    dialog.setWindowTitle(supplier ? "Sửa Nhà Cung Cấp" : "Thêm Nhà Cung Cấp Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit nameEdit(this);
    QLineEdit taxIdEdit(this);
    QLineEdit notesEdit(this);
    QLineEdit defaultPaymentTermsEdit(this);
    QLineEdit defaultDeliveryTermsEdit(this);

    if (supplier) {
        nameEdit.setText(QString::fromStdString(supplier->name));
        taxIdEdit.setText(QString::fromStdString(supplier->taxId.value_or("")));
        notesEdit.setText(QString::fromStdString(supplier->notes.value_or("")));
        defaultPaymentTermsEdit.setText(QString::fromStdString(supplier->defaultPaymentTerms.value_or("")));
        defaultDeliveryTermsEdit.setText(QString::fromStdString(supplier->defaultDeliveryTerms.value_or("")));
    }

    formLayout->addRow("Tên:*", &nameEdit);
    formLayout->addRow("Mã số thuế:", &taxIdEdit);
    formLayout->addRow("Ghi chú:", &notesEdit);
    formLayout->addRow("Điều khoản TT mặc định:", &defaultPaymentTermsEdit);
    formLayout->addRow("Điều khoản GH mặc định:", &defaultDeliveryTermsEdit);

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(supplier ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Supplier::DTO::SupplierDTO newSupplierData;
        if (supplier) {
            newSupplierData = *supplier;
        }

        newSupplierData.name = nameEdit.text().toStdString();
        newSupplierData.taxId = taxIdEdit.text().isEmpty() ? std::nullopt : std::make_optional(taxIdEdit.text().toStdString());
        newSupplierData.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());
        newSupplierData.defaultPaymentTerms = defaultPaymentTermsEdit.text().isEmpty() ? std::nullopt : std::make_optional(defaultPaymentTermsEdit.text().toStdString());
        newSupplierData.defaultDeliveryTerms = defaultDeliveryTermsEdit.text().isEmpty() ? std::nullopt : std::make_optional(defaultDeliveryTermsEdit.text().toStdString());
        newSupplierData.status = ERP::Common::EntityStatus::ACTIVE; // Always set to active on creation/update via form

        bool success = false;
        if (supplier) {
            success = supplierService_->updateSupplier(newSupplierData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Nhà Cung Cấp", "Nhà cung cấp đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật nhà cung cấp. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Supplier::DTO::SupplierDTO> createdSupplier = supplierService_->createSupplier(newSupplierData, currentUserId_, currentUserRoleIds_);
            if (createdSupplier) {
                showMessageBox("Thêm Nhà Cung Cấp", "Nhà cung cấp mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm nhà cung cấp mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadSuppliers();
            clearForm();
        }
    }
}


void SupplierManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool SupplierManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void SupplierManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Supplier.CreateSupplier");
    bool canUpdate = hasPermission("Supplier.UpdateSupplier");
    bool canDelete = hasPermission("Supplier.DeleteSupplier");
    bool canChangeStatus = hasPermission("Supplier.ChangeSupplierStatus");

    addSupplierButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Supplier.ViewSuppliers"));

    bool isRowSelected = supplierTable_->currentRow() >= 0;
    editSupplierButton_->setEnabled(isRowSelected && canUpdate);
    deleteSupplierButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);

    bool enableForm = isRowSelected && canUpdate;
    nameLineEdit_->setEnabled(enableForm);
    taxIdLineEdit_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);
    defaultPaymentTermsLineEdit_->setEnabled(enableForm);
    defaultDeliveryTermsLineEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        nameLineEdit_->clear();
        taxIdLineEdit_->clear();
        notesLineEdit_->clear();
        defaultPaymentTermsLineEdit_->clear();
        defaultDeliveryTermsLineEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
    }
}


} // namespace Supplier
} // namespace UI
} // namespace ERP