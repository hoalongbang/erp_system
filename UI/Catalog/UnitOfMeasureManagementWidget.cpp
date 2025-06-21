// UI/Catalog/UnitOfMeasureManagementWidget.cpp
#include "UnitOfMeasureManagementWidget.h" // Đã rút gọn include
#include "UnitOfMeasure.h"     // Đã rút gọn include
#include "UnitOfMeasureService.h" // Đã rút gọn include
#include "ISecurityManager.h"  // Đã rút gọn include
#include "Logger.h"            // Đã rút gọn include
#include "ErrorHandler.h"      // Đã rút gọn include
#include "Common.h"            // Đã rút gọn include
#include "DateUtils.h"         // Đã rút gọn include
#include "StringUtils.h"       // Đã rút gọn include
#include "CustomMessageBox.h"  // Đã rút gọn include
#include "UserService.h"       // For getting current user roles from SecurityManager

#include <QInputDialog> // For getting input from user

namespace ERP {
namespace UI {
namespace Catalog {

UnitOfMeasureManagementWidget::UnitOfMeasureManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IUnitOfMeasureService> unitOfMeasureService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      unitOfMeasureService_(unitOfMeasureService),
      securityManager_(securityManager) {

    if (!unitOfMeasureService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ đơn vị đo hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("UnitOfMeasureManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("UnitOfMeasureManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("UnitOfMeasureManagementWidget: Authentication Service not available. Running with limited privileges.");
    }


    setupUI();
    loadUnitsOfMeasure();
    updateButtonsState();
}

UnitOfMeasureManagementWidget::~UnitOfMeasureManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void UnitOfMeasureManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên hoặc ký hiệu...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &UnitOfMeasureManagementWidget::onSearchUnitOfMeasureClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    unitOfMeasureTable_ = new QTableWidget(this);
    unitOfMeasureTable_->setColumnCount(5); // ID, Tên, Ký hiệu, Mô tả, Trạng thái
    unitOfMeasureTable_->setHorizontalHeaderLabels({"ID", "Tên", "Ký hiệu", "Mô tả", "Trạng thái"});
    unitOfMeasureTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    unitOfMeasureTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    unitOfMeasureTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    unitOfMeasureTable_->horizontalHeader()->setStretchLastSection(true);
    connect(unitOfMeasureTable_, &QTableWidget::itemClicked, this, &UnitOfMeasureManagementWidget::onUnitOfMeasureTableItemClicked);
    mainLayout->addWidget(unitOfMeasureTable_);

    // Form for Add/Edit
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    nameLineEdit_ = new QLineEdit(this);
    symbolLineEdit_ = new QLineEdit(this);
    descriptionLineEdit_ = new QLineEdit(this);
    statusComboBox_ = new QComboBox(this);
    statusComboBox_->addItem("Active", static_cast<int>(ERP::Common::EntityStatus::ACTIVE));
    statusComboBox_->addItem("Inactive", static_cast<int>(ERP::Common::EntityStatus::INACTIVE));
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Common::EntityStatus::PENDING));
    statusComboBox_->addItem("Deleted", static_cast<int>(ERP::Common::EntityStatus::DELETED));

    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Tên:*", this), 1, 0); formLayout->addWidget(nameLineEdit_, 1, 1);
    formLayout->addWidget(new QLabel("Ký hiệu:*", this), 2, 0); formLayout->addWidget(symbolLineEdit_, 2, 1);
    formLayout->addWidget(new QLabel("Mô tả:", this), 3, 0); formLayout->addWidget(descriptionLineEdit_, 3, 1);
    formLayout->addWidget(new QLabel("Trạng thái:", this), 4, 0); formLayout->addWidget(statusComboBox_, 4, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addUnitOfMeasureButton_ = new QPushButton("Thêm mới", this); connect(addUnitOfMeasureButton_, &QPushButton::clicked, this, &UnitOfMeasureManagementWidget::onAddUnitOfMeasureClicked);
    editUnitOfMeasureButton_ = new QPushButton("Sửa", this); connect(editUnitOfMeasureButton_, &QPushButton::clicked, this, &UnitOfMeasureManagementWidget::onEditUnitOfMeasureClicked);
    deleteUnitOfMeasureButton_ = new QPushButton("Xóa", this); connect(deleteUnitOfMeasureButton_, &QPushButton::clicked, this, &UnitOfMeasureManagementWidget::onDeleteUnitOfMeasureClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &UnitOfMeasureManagementWidget::onUpdateUnitOfMeasureStatusClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &UnitOfMeasureManagementWidget::clearForm);
    buttonLayout->addWidget(addUnitOfMeasureButton_);
    buttonLayout->addWidget(editUnitOfMeasureButton_);
    buttonLayout->addWidget(deleteUnitOfMeasureButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void UnitOfMeasureManagementWidget::loadUnitsOfMeasure() {
    ERP::Logger::Logger::getInstance().info("UnitOfMeasureManagementWidget: Loading units of measure...");
    unitOfMeasureTable_->setRowCount(0);

    std::vector<ERP::Catalog::DTO::UnitOfMeasureDTO> uoms = unitOfMeasureService_->getAllUnitsOfMeasure({}, currentUserId_, currentUserRoleIds_);

    unitOfMeasureTable_->setRowCount(uoms.size());
    for (int i = 0; i < uoms.size(); ++i) {
        const auto& uom = uoms[i];
        unitOfMeasureTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(uom.id)));
        unitOfMeasureTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(uom.name)));
        unitOfMeasureTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(uom.symbol)));
        unitOfMeasureTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(uom.description.value_or(""))));
        unitOfMeasureTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(uom.status))));
    }
    unitOfMeasureTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("UnitOfMeasureManagementWidget: Units of measure loaded successfully.");
}

void UnitOfMeasureManagementWidget::onAddUnitOfMeasureClicked() {
    if (!hasPermission("Catalog.CreateUnitOfMeasure")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm đơn vị đo.", QMessageBox::Warning);
        return;
    }
    clearForm();
    showUnitOfMeasureInputDialog();
}

void UnitOfMeasureManagementWidget::onEditUnitOfMeasureClicked() {
    if (!hasPermission("Catalog.UpdateUnitOfMeasure")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa đơn vị đo.", QMessageBox::Warning);
        return;
    }

    int selectedRow = unitOfMeasureTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Đơn Vị Đo", "Vui lòng chọn một đơn vị đo để sửa.", QMessageBox::Information);
        return;
    }

    QString uomId = unitOfMeasureTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> uomOpt = unitOfMeasureService_->getUnitOfMeasureById(uomId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (uomOpt) {
        showUnitOfMeasureInputDialog(&(*uomOpt));
    } else {
        showMessageBox("Sửa Đơn Vị Đo", "Không tìm thấy đơn vị đo để sửa.", QMessageBox::Critical);
    }
}

void UnitOfMeasureManagementWidget::onDeleteUnitOfMeasureClicked() {
    if (!hasPermission("Catalog.DeleteUnitOfMeasure")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa đơn vị đo.", QMessageBox::Warning);
        return;
    }

    int selectedRow = unitOfMeasureTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Đơn Vị Đo", "Vui lòng chọn một đơn vị đo để xóa.", QMessageBox::Information);
        return;
    }

    QString uomId = unitOfMeasureTable_->item(selectedRow, 0)->text();
    QString uomName = unitOfMeasureTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Đơn Vị Đo");
    confirmBox.setText("Bạn có chắc chắn muốn xóa đơn vị đo '" + uomName + "' (ID: " + uomId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (unitOfMeasureService_->deleteUnitOfMeasure(uomId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Đơn Vị Đo", "Đơn vị đo đã được xóa thành công.", QMessageBox::Information);
            loadUnitsOfMeasure();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa đơn vị đo. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void UnitOfMeasureManagementWidget::onUpdateUnitOfMeasureStatusClicked() {
    if (!hasPermission("Catalog.ChangeUnitOfMeasureStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái đơn vị đo.", QMessageBox::Warning);
        return;
    }

    int selectedRow = unitOfMeasureTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một đơn vị đo để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString uomId = unitOfMeasureTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> uomOpt = unitOfMeasureService_->getUnitOfMeasureById(uomId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!uomOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy đơn vị đo để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Catalog::DTO::UnitOfMeasureDTO currentUoM = *uomOpt;
    ERP::Common::EntityStatus newStatus = (currentUoM.status == ERP::Common::EntityStatus::ACTIVE) ?
                                          ERP::Common::EntityStatus::INACTIVE :
                                          ERP::Common::EntityStatus::ACTIVE;

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Cập nhật trạng thái đơn vị đo");
    confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái đơn vị đo '" + QString::fromStdString(currentUoM.name) + "' thành " + QString::fromStdString(ERP::Common::entityStatusToString(newStatus)) + "?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (unitOfMeasureService_->updateUnitOfMeasureStatus(uomId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Cập nhật trạng thái", "Trạng thái đơn vị đo đã được cập nhật thành công.", QMessageBox::Information);
            loadUnitsOfMeasure();
            clearForm();
        } else {
            showMessageBox("Lỗi", "Không thể cập nhật trạng thái đơn vị đo. Vui lòng kiểm tra log.", QMessageBox::Critical);
        }
    }
}


void UnitOfMeasureManagementWidget::onSearchUnitOfMeasureClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["name_or_symbol_contains"] = searchText.toStdString(); // Assuming service supports "contains" filter
    }
    unitOfMeasureTable_->setRowCount(0);
    std::vector<ERP::Catalog::DTO::UnitOfMeasureDTO> uoms = unitOfMeasureService_->getAllUnitsOfMeasure(filter, currentUserId_, currentUserRoleIds_);

    unitOfMeasureTable_->setRowCount(uoms.size());
    for (int i = 0; i < uoms.size(); ++i) {
        const auto& uom = uoms[i];
        unitOfMeasureTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(uom.id)));
        unitOfMeasureTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(uom.name)));
        unitOfMeasureTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(uom.symbol)));
        unitOfMeasureTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(uom.description.value_or(""))));
        unitOfMeasureTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(uom.status))));
    }
    unitOfMeasureTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("UnitOfMeasureManagementWidget: Search completed.");
}

void UnitOfMeasureManagementWidget::onUnitOfMeasureTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString uomId = unitOfMeasureTable_->item(row, 0)->text();
    std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> uomOpt = unitOfMeasureService_->getUnitOfMeasureById(uomId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (uomOpt) {
        idLineEdit_->setText(QString::fromStdString(uomOpt->id));
        nameLineEdit_->setText(QString::fromStdString(uomOpt->name));
        symbolLineEdit_->setText(QString::fromStdString(uomOpt->symbol));
        descriptionLineEdit_->setText(QString::fromStdString(uomOpt->description.value_or("")));
        
        int statusIndex = statusComboBox_->findData(static_cast<int>(uomOpt->status));
        if (statusIndex != -1) {
            statusComboBox_->setCurrentIndex(statusIndex);
        }

    } else {
        showMessageBox("Thông tin Đơn Vị Đo", "Không thể tải chi tiết đơn vị đo đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void UnitOfMeasureManagementWidget::clearForm() {
    idLineEdit_->clear();
    nameLineEdit_->clear();
    symbolLineEdit_->clear();
    descriptionLineEdit_->clear();
    statusComboBox_->setCurrentIndex(0); // Default to Active
    unitOfMeasureTable_->clearSelection();
    updateButtonsState();
}


void UnitOfMeasureManagementWidget::showUnitOfMeasureInputDialog(ERP::Catalog::DTO::UnitOfMeasureDTO* uom) {
    QDialog dialog(this);
    dialog.setWindowTitle(uom ? "Sửa Đơn Vị Đo" : "Thêm Đơn Vị Đo Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit nameEdit(this);
    QLineEdit symbolEdit(this);
    QLineEdit descriptionEdit(this);

    if (uom) {
        nameEdit.setText(QString::fromStdString(uom->name));
        symbolEdit.setText(QString::fromStdString(uom->symbol));
        descriptionEdit.setText(QString::fromStdString(uom->description.value_or("")));
    }

    formLayout->addRow("Tên:*", &nameEdit);
    formLayout->addRow("Ký hiệu:*", &symbolEdit);
    formLayout->addRow("Mô tả:", &descriptionEdit);

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(uom ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Catalog::DTO::UnitOfMeasureDTO newUoMData;
        if (uom) {
            newUoMData = *uom;
        }

        newUoMData.name = nameEdit.text().toStdString();
        newUoMData.symbol = symbolEdit.text().toStdString();
        newUoMData.description = descriptionEdit.text().isEmpty() ? std::nullopt : std::make_optional(descriptionEdit.text().toStdString());
        newUoMData.status = ERP::Common::EntityStatus::ACTIVE; // Always set to active on creation/update via form

        bool success = false;
        if (uom) {
            success = unitOfMeasureService_->updateUnitOfMeasure(newUoMData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Đơn Vị Đo", "Đơn vị đo đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật đơn vị đo. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> createdUoM = unitOfMeasureService_->createUnitOfMeasure(newUoMData, currentUserId_, currentUserRoleIds_);
            if (createdUoM) {
                showMessageBox("Thêm Đơn Vị Đo", "Đơn vị đo mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm đơn vị đo mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadUnitsOfMeasure();
            clearForm();
        }
    }
}


void UnitOfMeasureManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool UnitOfMeasureManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void UnitOfMeasureManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Catalog.CreateUnitOfMeasure");
    bool canUpdate = hasPermission("Catalog.UpdateUnitOfMeasure");
    bool canDelete = hasPermission("Catalog.DeleteUnitOfMeasure");
    bool canChangeStatus = hasPermission("Catalog.ChangeUnitOfMeasureStatus");

    addUnitOfMeasureButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Catalog.ViewUnitsOfMeasure"));

    bool isRowSelected = unitOfMeasureTable_->currentRow() >= 0;
    editUnitOfMeasureButton_->setEnabled(isRowSelected && canUpdate);
    deleteUnitOfMeasureButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);

    bool enableForm = isRowSelected && canUpdate;
    nameLineEdit_->setEnabled(enableForm);
    symbolLineEdit_->setEnabled(enableForm);
    descriptionLineEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        nameLineEdit_->clear();
        symbolLineEdit_->clear();
        descriptionLineEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
    }
}


} // namespace Catalog
} // namespace UI
} // namespace ERP