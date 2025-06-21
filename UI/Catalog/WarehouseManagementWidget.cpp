// UI/Catalog/WarehouseManagementWidget.cpp
#include "WarehouseManagementWidget.h" // Đã rút gọn include
#include "Warehouse.h"         // Đã rút gọn include
#include "Location.h"          // Đã rút gọn include
#include "WarehouseService.h"  // Đã rút gọn include
#include "LocationService.h"   // Đã rút gọn include
#include "ISecurityManager.h"  // Đã rút gọn include
#include "Logger.h"            // Đã rút gọn include
#include "ErrorHandler.h"      // Đã rút gọn include
#include "Common.h"            // Đã rút gọn include
#include "DateUtils.h"         // Đã rút gọn include
#include "StringUtils.h"       // Đã rút gọn include
#include "CustomMessageBox.h"  // Đã rút gọn include
#include "UserService.h"       // For getting current user roles from SecurityManager
#include "InventoryManagementService.h" // For deletion check


#include <QInputDialog> // For getting input from user

namespace ERP {
namespace UI {
namespace Catalog {

WarehouseManagementWidget::WarehouseManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IWarehouseService> warehouseService,
    std::shared_ptr<Services::ILocationService> locationService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      warehouseService_(warehouseService),
      locationService_(locationService),
      securityManager_(securityManager) {

    if (!warehouseService_ || !locationService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ kho hàng, vị trí kho hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("WarehouseManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("WarehouseManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("WarehouseManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadWarehouses();
    updateButtonsState();
}

WarehouseManagementWidget::~WarehouseManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void WarehouseManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên kho hàng...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &WarehouseManagementWidget::onSearchWarehouseClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    warehouseTable_ = new QTableWidget(this);
    warehouseTable_->setColumnCount(6); // ID, Tên, Địa điểm, Người liên hệ, Điện thoại, Email, Trạng thái
    warehouseTable_->setHorizontalHeaderLabels({"ID", "Tên", "Địa điểm", "Người liên hệ", "Điện thoại", "Email", "Trạng thái"});
    warehouseTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    warehouseTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    warehouseTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    warehouseTable_->horizontalHeader()->setStretchLastSection(true);
    connect(warehouseTable_, &QTableWidget::itemClicked, this, &WarehouseManagementWidget::onWarehouseTableItemClicked);
    mainLayout->addWidget(warehouseTable_);

    // Form elements for editing/adding
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    nameLineEdit_ = new QLineEdit(this);
    locationLineEdit_ = new QLineEdit(this);
    contactPersonLineEdit_ = new QLineEdit(this);
    contactPhoneLineEdit_ = new QLineEdit(this);
    emailLineEdit_ = new QLineEdit(this);
    statusComboBox_ = new QComboBox(this);
    statusComboBox_->addItem("Active", static_cast<int>(ERP::Common::EntityStatus::ACTIVE));
    statusComboBox_->addItem("Inactive", static_cast<int>(ERP::Common::EntityStatus::INACTIVE));
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Common::EntityStatus::PENDING));
    statusComboBox_->addItem("Deleted", static_cast<int>(ERP::Common::EntityStatus::DELETED));

    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Tên:*", this), 1, 0); formLayout->addWidget(nameLineEdit_, 1, 1);
    formLayout->addWidget(new QLabel("Địa điểm:", this), 2, 0); formLayout->addWidget(locationLineEdit_, 2, 1);
    formLayout->addWidget(new QLabel("Người liên hệ:", this), 3, 0); formLayout->addWidget(contactPersonLineEdit_, 3, 1);
    formLayout->addWidget(new QLabel("Điện thoại liên hệ:", this), 4, 0); formLayout->addWidget(contactPhoneLineEdit_, 4, 1);
    formLayout->addWidget(new QLabel("Email:", this), 5, 0); formLayout->addWidget(emailLineEdit_, 5, 1);
    formLayout->addWidget(new QLabel("Trạng thái:", this), 6, 0); formLayout->addWidget(statusComboBox_, 6, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addWarehouseButton_ = new QPushButton("Thêm mới", this); connect(addWarehouseButton_, &QPushButton::clicked, this, &WarehouseManagementWidget::onAddWarehouseClicked);
    editWarehouseButton_ = new QPushButton("Sửa", this); connect(editWarehouseButton_, &QPushButton::clicked, this, &WarehouseManagementWidget::onEditWarehouseClicked);
    deleteWarehouseButton_ = new QPushButton("Xóa", this); connect(deleteWarehouseButton_, &QPushButton::clicked, this, &WarehouseManagementWidget::onDeleteWarehouseClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &WarehouseManagementWidget::onUpdateWarehouseStatusClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &WarehouseManagementWidget::clearForm);
    buttonLayout->addWidget(addWarehouseButton_);
    buttonLayout->addWidget(editWarehouseButton_);
    buttonLayout->addWidget(deleteWarehouseButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void WarehouseManagementWidget::loadWarehouses() {
    ERP::Logger::Logger::getInstance().info("WarehouseManagementWidget: Loading warehouses...");
    warehouseTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Catalog::DTO::WarehouseDTO> warehouses = warehouseService_->getAllWarehouses({}, currentUserId_, currentUserRoleIds_);

    warehouseTable_->setRowCount(warehouses.size());
    for (int i = 0; i < warehouses.size(); ++i) {
        const auto& warehouse = warehouses[i];
        warehouseTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(warehouse.id)));
        warehouseTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(warehouse.name)));
        warehouseTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(warehouse.location.value_or(""))));
        warehouseTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(warehouse.contactPerson.value_or(""))));
        warehouseTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(warehouse.contactPhone.value_or(""))));
        warehouseTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(warehouse.email.value_or(""))));
        warehouseTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(warehouse.status))));
    }
    warehouseTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("WarehouseManagementWidget: Warehouses loaded successfully.");
}

void WarehouseManagementWidget::onAddWarehouseClicked() {
    if (!hasPermission("Catalog.CreateWarehouse")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm kho hàng.", QMessageBox::Warning);
        return;
    }
    clearForm();
    showWarehouseInputDialog();
}

void WarehouseManagementWidget::onEditWarehouseClicked() {
    if (!hasPermission("Catalog.UpdateWarehouse")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa kho hàng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = warehouseTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Kho Hàng", "Vui lòng chọn một kho hàng để sửa.", QMessageBox::Information);
        return;
    }

    QString warehouseId = warehouseTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouseOpt = warehouseService_->getWarehouseById(warehouseId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (warehouseOpt) {
        showWarehouseInputDialog(&(*warehouseOpt));
    } else {
        showMessageBox("Sửa Kho Hàng", "Không tìm thấy kho hàng để sửa.", QMessageBox::Critical);
    }
}

void WarehouseManagementWidget::onDeleteWarehouseClicked() {
    if (!hasPermission("Catalog.DeleteWarehouse")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa kho hàng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = warehouseTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Kho Hàng", "Vui lòng chọn một kho hàng để xóa.", QMessageBox::Information);
        return;
    }

    QString warehouseId = warehouseTable_->item(selectedRow, 0)->text();
    QString warehouseName = warehouseTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Kho Hàng");
    confirmBox.setText("Bạn có chắc chắn muốn xóa kho hàng '" + warehouseName + "' (ID: " + warehouseId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (warehouseService_->deleteWarehouse(warehouseId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Kho Hàng", "Kho hàng đã được xóa thành công.", QMessageBox::Information);
            loadWarehouses();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa kho hàng. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void WarehouseManagementWidget::onUpdateWarehouseStatusClicked() {
    if (!hasPermission("Catalog.ChangeWarehouseStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái kho hàng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = warehouseTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một kho hàng để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString warehouseId = warehouseTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouseOpt = warehouseService_->getWarehouseById(warehouseId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!warehouseOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy kho hàng để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Catalog::DTO::WarehouseDTO currentWarehouse = *warehouseOpt;
    ERP::Common::EntityStatus newStatus = (currentWarehouse.status == ERP::Common::EntityStatus::ACTIVE) ?
                                          ERP::Common::EntityStatus::INACTIVE :
                                          ERP::Common::EntityStatus::ACTIVE;

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Cập nhật trạng thái kho hàng");
    confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái kho hàng '" + QString::fromStdString(currentWarehouse.name) + "' thành " + QString::fromStdString(ERP::Common::entityStatusToString(newStatus)) + "?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (warehouseService_->updateWarehouseStatus(warehouseId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Cập nhật trạng thái", "Trạng thái kho hàng đã được cập nhật thành công.", QMessageBox::Information);
            loadWarehouses();
            clearForm();
        } else {
            showMessageBox("Lỗi", "Không thể cập nhật trạng thái kho hàng. Vui lòng kiểm tra log.", QMessageBox::Critical);
        }
    }
}


void WarehouseManagementWidget::onSearchWarehouseClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["name_contains"] = searchText.toStdString(); // Assuming service supports "contains" filter
    }
    warehouseTable_->setRowCount(0);
    std::vector<ERP::Catalog::DTO::WarehouseDTO> warehouses = warehouseService_->getAllWarehouses(filter, currentUserId_, currentUserRoleIds_);

    warehouseTable_->setRowCount(warehouses.size());
    for (int i = 0; i < warehouses.size(); ++i) {
        const auto& warehouse = warehouses[i];
        warehouseTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(warehouse.id)));
        warehouseTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(warehouse.name)));
        warehouseTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(warehouse.location.value_or(""))));
        warehouseTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(warehouse.contactPerson.value_or(""))));
        warehouseTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(warehouse.contactPhone.value_or(""))));
        warehouseTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(warehouse.email.value_or(""))));
        warehouseTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(warehouse.status))));
    }
    warehouseTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("WarehouseManagementWidget: Search completed.");
}

void WarehouseManagementWidget::onWarehouseTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString warehouseId = warehouseTable_->item(row, 0)->text();
    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouseOpt = warehouseService_->getWarehouseById(warehouseId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (warehouseOpt) {
        idLineEdit_->setText(QString::fromStdString(warehouseOpt->id));
        nameLineEdit_->setText(QString::fromStdString(warehouseOpt->name));
        locationLineEdit_->setText(QString::fromStdString(warehouseOpt->location.value_or("")));
        contactPersonLineEdit_->setText(QString::fromStdString(warehouseOpt->contactPerson.value_or("")));
        contactPhoneLineEdit_->setText(QString::fromStdString(warehouseOpt->contactPhone.value_or("")));
        emailLineEdit_->setText(QString::fromStdString(warehouseOpt->email.value_or("")));
        
        int statusIndex = statusComboBox_->findData(static_cast<int>(warehouseOpt->status));
        if (statusIndex != -1) {
            statusComboBox_->setCurrentIndex(statusIndex);
        }

    } else {
        showMessageBox("Thông tin Kho Hàng", "Không thể tải chi tiết kho hàng đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void WarehouseManagementWidget::clearForm() {
    idLineEdit_->clear();
    nameLineEdit_->clear();
    locationLineEdit_->clear();
    contactPersonLineEdit_->clear();
    contactPhoneLineEdit_->clear();
    emailLineEdit_->clear();
    statusComboBox_->setCurrentIndex(0); // Default to Active
    warehouseTable_->clearSelection();
    updateButtonsState();
}


void WarehouseManagementWidget::showWarehouseInputDialog(ERP::Catalog::DTO::WarehouseDTO* warehouse) {
    QDialog dialog(this);
    dialog.setWindowTitle(warehouse ? "Sửa Kho Hàng" : "Thêm Kho Hàng Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit nameEdit(this);
    QLineEdit locationEdit(this);
    QLineEdit contactPersonEdit(this);
    QLineEdit contactPhoneEdit(this);
    QLineEdit emailEdit(this);

    if (warehouse) {
        nameEdit.setText(QString::fromStdString(warehouse->name));
        locationEdit.setText(QString::fromStdString(warehouse->location.value_or("")));
        contactPersonEdit.setText(QString::fromStdString(warehouse->contactPerson.value_or("")));
        contactPhoneEdit.setText(QString::fromStdString(warehouse->contactPhone.value_or("")));
        emailEdit.setText(QString::fromStdString(warehouse->email.value_or("")));
    }

    formLayout->addRow("Tên:*", &nameEdit);
    formLayout->addRow("Địa điểm:", &locationEdit);
    formLayout->addRow("Người liên hệ:", &contactPersonEdit);
    formLayout->addRow("Điện thoại liên hệ:", &contactPhoneEdit);
    formLayout->addRow("Email:", &emailEdit);

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(warehouse ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Catalog::DTO::WarehouseDTO newWarehouseData;
        if (warehouse) {
            newWarehouseData = *warehouse;
        }

        newWarehouseData.name = nameEdit.text().toStdString();
        newWarehouseData.location = locationEdit.text().isEmpty() ? std::nullopt : std::make_optional(locationEdit.text().toStdString());
        newWarehouseData.contactPerson = contactPersonEdit.text().isEmpty() ? std::nullopt : std::make_optional(contactPersonEdit.text().toStdString());
        newWarehouseData.contactPhone = contactPhoneEdit.text().isEmpty() ? std::nullopt : std::make_optional(contactPhoneEdit.text().toStdString());
        newWarehouseData.email = emailEdit.text().isEmpty() ? std::nullopt : std::make_optional(emailEdit.text().toStdString());
        newWarehouseData.status = ERP::Common::EntityStatus::ACTIVE; // Always set to active on creation/update via form

        bool success = false;
        if (warehouse) {
            success = warehouseService_->updateWarehouse(newWarehouseData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Kho Hàng", "Kho hàng đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật kho hàng. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Catalog::DTO::WarehouseDTO> createdWarehouse = warehouseService_->createWarehouse(newWarehouseData, currentUserId_, currentUserRoleIds_);
            if (createdWarehouse) {
                showMessageBox("Thêm Kho Hàng", "Kho hàng mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm kho hàng mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadWarehouses();
            clearForm();
        }
    }
}


void WarehouseManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool WarehouseManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void WarehouseManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Catalog.CreateWarehouse");
    bool canUpdate = hasPermission("Catalog.UpdateWarehouse");
    bool canDelete = hasPermission("Catalog.DeleteWarehouse");
    bool canChangeStatus = hasPermission("Catalog.ChangeWarehouseStatus");

    addWarehouseButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Catalog.ViewWarehouses"));

    bool isRowSelected = warehouseTable_->currentRow() >= 0;
    editWarehouseButton_->setEnabled(isRowSelected && canUpdate);
    deleteWarehouseButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);

    bool enableForm = isRowSelected && canUpdate;
    nameLineEdit_->setEnabled(enableForm);
    locationLineEdit_->setEnabled(enableForm);
    contactPersonLineEdit_->setEnabled(enableForm);
    contactPhoneLineEdit_->setEnabled(enableForm);
    emailLineEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        nameLineEdit_->clear();
        locationLineEdit_->clear();
        contactPersonLineEdit_->clear();
        contactPhoneLineEdit_->clear();
        emailLineEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
    }
}


} // namespace Catalog
} // namespace UI
} // namespace ERP