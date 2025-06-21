// UI/Catalog/PermissionManagementWidget.cpp
#include "PermissionManagementWidget.h" // Đã rút gọn include
#include "Permission.h"        // Đã rút gọn include
#include "PermissionService.h" // Đã rút gọn include
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

PermissionManagementWidget::PermissionManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IPermissionService> permissionService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      permissionService_(permissionService),
      securityManager_(securityManager) {

    if (!permissionService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ quyền hạn hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("PermissionManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("PermissionManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("PermissionManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadPermissions();
    updateButtonsState();
}

PermissionManagementWidget::~PermissionManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void PermissionManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên quyền hạn...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &PermissionManagementWidget::onSearchPermissionClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    permissionTable_ = new QTableWidget(this);
    permissionTable_->setColumnCount(5); // ID, Tên, Module, Action, Mô tả, Trạng thái
    permissionTable_->setHorizontalHeaderLabels({"ID", "Tên", "Module", "Hành động", "Mô tả", "Trạng thái"});
    permissionTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    permissionTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    permissionTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    permissionTable_->horizontalHeader()->setStretchLastSection(true);
    connect(permissionTable_, &QTableWidget::itemClicked, this, &PermissionManagementWidget::onPermissionTableItemClicked);
    mainLayout->addWidget(permissionTable_);

    // Form elements for editing/adding
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    nameLineEdit_ = new QLineEdit(this); // Permission Name (e.g., "Sales.CreateOrder")
    moduleLineEdit_ = new QLineEdit(this); // Module (e.g., "Sales")
    actionLineEdit_ = new QLineEdit(this); // Action (e.g., "CreateOrder")
    descriptionLineEdit_ = new QLineEdit(this);
    statusComboBox_ = new QComboBox(this);
    statusComboBox_->addItem("Active", static_cast<int>(ERP::Common::EntityStatus::ACTIVE));
    statusComboBox_->addItem("Inactive", static_cast<int>(ERP::Common::EntityStatus::INACTIVE));
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Common::EntityStatus::PENDING));
    statusComboBox_->addItem("Deleted", static_cast<int>(ERP::Common::EntityStatus::DELETED));

    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Tên quyền hạn:*", this), 1, 0); formLayout->addWidget(nameLineEdit_, 1, 1);
    formLayout->addWidget(new QLabel("Module:*", this), 2, 0); formLayout->addWidget(moduleLineEdit_, 2, 1);
    formLayout->addWidget(new QLabel("Hành động:*", this), 3, 0); formLayout->addWidget(actionLineEdit_, 3, 1);
    formLayout->addWidget(new QLabel("Mô tả:", this), 4, 0); formLayout->addWidget(descriptionLineEdit_, 4, 1);
    formLayout->addWidget(new QLabel("Trạng thái:", this), 5, 0); formLayout->addWidget(statusComboBox_, 5, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addPermissionButton_ = new QPushButton("Thêm mới", this); connect(addPermissionButton_, &QPushButton::clicked, this, &PermissionManagementWidget::onAddPermissionClicked);
    editPermissionButton_ = new QPushButton("Sửa", this); connect(editPermissionButton_, &QPushButton::clicked, this, &PermissionManagementWidget::onEditPermissionClicked);
    deletePermissionButton_ = new QPushButton("Xóa", this); connect(deletePermissionButton_, &QPushButton::clicked, this, &PermissionManagementWidget::onDeletePermissionClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &PermissionManagementWidget::onUpdatePermissionStatusClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &PermissionManagementWidget::clearForm);
    buttonLayout->addWidget(addPermissionButton_);
    buttonLayout->addWidget(editPermissionButton_);
    buttonLayout->addWidget(deletePermissionButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void PermissionManagementWidget::loadPermissions() {
    ERP::Logger::Logger::getInstance().info("PermissionManagementWidget: Loading permissions...");
    permissionTable_->setRowCount(0);

    std::vector<ERP::Catalog::DTO::PermissionDTO> permissions = permissionService_->getAllPermissions({}, currentUserId_, currentUserRoleIds_);

    permissionTable_->setRowCount(permissions.size());
    for (int i = 0; i < permissions.size(); ++i) {
        const auto& perm = permissions[i];
        permissionTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(perm.id)));
        permissionTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(perm.name)));
        permissionTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(perm.module)));
        permissionTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(perm.action)));
        permissionTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(perm.description.value_or(""))));
        permissionTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(perm.status))));
    }
    permissionTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("PermissionManagementWidget: Permissions loaded successfully.");
}

void PermissionManagementWidget::onAddPermissionClicked() {
    if (!hasPermission("Catalog.CreatePermission")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm quyền hạn.", QMessageBox::Warning);
        return;
    }
    clearForm();
    showPermissionInputDialog();
}

void PermissionManagementWidget::onEditPermissionClicked() {
    if (!hasPermission("Catalog.UpdatePermission")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa quyền hạn.", QMessageBox::Warning);
        return;
    }

    int selectedRow = permissionTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Quyền Hạn", "Vui lòng chọn một quyền hạn để sửa.", QMessageBox::Information);
        return;
    }

    QString permissionId = permissionTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Catalog::DTO::PermissionDTO> permissionOpt = permissionService_->getPermissionById(permissionId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (permissionOpt) {
        showPermissionInputDialog(&(*permissionOpt));
    } else {
        showMessageBox("Sửa Quyền Hạn", "Không tìm thấy quyền hạn để sửa.", QMessageBox::Critical);
    }
}

void PermissionManagementWidget::onDeletePermissionClicked() {
    if (!hasPermission("Catalog.DeletePermission")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa quyền hạn.", QMessageBox::Warning);
        return;
    }

    int selectedRow = permissionTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Quyền Hạn", "Vui lòng chọn một quyền hạn để xóa.", QMessageBox::Information);
        return;
    }

    QString permissionId = permissionTable_->item(selectedRow, 0)->text();
    QString permissionName = permissionTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Quyền Hạn");
    confirmBox.setText("Bạn có chắc chắn muốn xóa quyền hạn '" + permissionName + "' (ID: " + permissionId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (permissionService_->deletePermission(permissionId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Quyền Hạn", "Quyền hạn đã được xóa thành công.", QMessageBox::Information);
            loadPermissions();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa quyền hạn. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void PermissionManagementWidget::onUpdatePermissionStatusClicked() {
    if (!hasPermission("Catalog.ChangePermissionStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái quyền hạn.", QMessageBox::Warning);
        return;
    }

    int selectedRow = permissionTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một quyền hạn để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString permissionId = permissionTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Catalog::DTO::PermissionDTO> permissionOpt = permissionService_->getPermissionById(permissionId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!permissionOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy quyền hạn để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Catalog::DTO::PermissionDTO currentPermission = *permissionOpt;
    ERP::Common::EntityStatus newStatus = (currentPermission.status == ERP::Common::EntityStatus::ACTIVE) ?
                                          ERP::Common::EntityStatus::INACTIVE :
                                          ERP::Common::EntityStatus::ACTIVE;

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Cập nhật trạng thái quyền hạn");
    confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái quyền hạn '" + QString::fromStdString(currentPermission.name) + "' thành " + QString::fromStdString(ERP::Common::entityStatusToString(newStatus)) + "?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (permissionService_->updatePermissionStatus(permissionId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Cập nhật trạng thái", "Trạng thái quyền hạn đã được cập nhật thành công.", QMessageBox::Information);
            loadPermissions();
            clearForm();
        } else {
            showMessageBox("Lỗi", "Không thể cập nhật trạng thái quyền hạn. Vui lòng kiểm tra log.", QMessageBox::Critical);
        }
    }
}


void PermissionManagementWidget::onSearchPermissionClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["name_contains"] = searchText.toStdString(); // Assuming service supports "contains" filter
    }
    permissionTable_->setRowCount(0);
    std::vector<ERP::Catalog::DTO::PermissionDTO> permissions = permissionService_->getAllPermissions(filter, currentUserId_, currentUserRoleIds_);

    permissionTable_->setRowCount(permissions.size());
    for (int i = 0; i < permissions.size(); ++i) {
        const auto& perm = permissions[i];
        permissionTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(perm.id)));
        permissionTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(perm.name)));
        permissionTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(perm.module)));
        permissionTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(perm.action)));
        permissionTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(perm.description.value_or(""))));
        permissionTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(perm.status))));
    }
    permissionTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("PermissionManagementWidget: Search completed.");
}

void PermissionManagementWidget::onPermissionTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString permissionId = permissionTable_->item(row, 0)->text();
    std::optional<ERP::Catalog::DTO::PermissionDTO> permissionOpt = permissionService_->getPermissionById(permissionId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (permissionOpt) {
        idLineEdit_->setText(QString::fromStdString(permissionOpt->id));
        nameLineEdit_->setText(QString::fromStdString(permissionOpt->name));
        moduleLineEdit_->setText(QString::fromStdString(permissionOpt->module));
        actionLineEdit_->setText(QString::fromStdString(permissionOpt->action));
        descriptionLineEdit_->setText(QString::fromStdString(permissionOpt->description.value_or("")));
        
        int statusIndex = statusComboBox_->findData(static_cast<int>(permissionOpt->status));
        if (statusIndex != -1) {
            statusComboBox_->setCurrentIndex(statusIndex);
        }

    } else {
        showMessageBox("Thông tin Quyền Hạn", "Không thể tải chi tiết quyền hạn đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void PermissionManagementWidget::clearForm() {
    idLineEdit_->clear();
    nameLineEdit_->clear();
    moduleLineEdit_->clear();
    actionLineEdit_->clear();
    descriptionLineEdit_->clear();
    statusComboBox_->setCurrentIndex(0); // Default to Active
    permissionTable_->clearSelection();
    updateButtonsState();
}


void PermissionManagementWidget::showPermissionInputDialog(ERP::Catalog::DTO::PermissionDTO* permission) {
    QDialog dialog(this);
    dialog.setWindowTitle(permission ? "Sửa Quyền Hạn" : "Thêm Quyền Hạn Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit nameEdit(this);
    QLineEdit moduleEdit(this);
    QLineEdit actionEdit(this);
    QLineEdit descriptionEdit(this);

    if (permission) {
        nameEdit.setText(QString::fromStdString(permission->name));
        moduleEdit.setText(QString::fromStdString(permission->module));
        actionEdit.setText(QString::fromStdString(permission->action));
        descriptionEdit.setText(QString::fromStdString(permission->description.value_or("")));
    }

    formLayout->addRow("Tên quyền hạn:*", &nameEdit);
    formLayout->addRow("Module:*", &moduleEdit);
    formLayout->addRow("Hành động:*", &actionEdit);
    formLayout->addRow("Mô tả:", &descriptionEdit);

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(permission ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Catalog::DTO::PermissionDTO newPermissionData;
        if (permission) {
            newPermissionData = *permission;
        }

        newPermissionData.name = nameEdit.text().toStdString();
        newPermissionData.module = moduleEdit.text().toStdString();
        newPermissionData.action = actionEdit.text().toStdString();
        newPermissionData.description = descriptionEdit.text().isEmpty() ? std::nullopt : std::make_optional(descriptionEdit.text().toStdString());
        newPermissionData.status = ERP::Common::EntityStatus::ACTIVE; // Always set to active on creation/update via form

        bool success = false;
        if (permission) {
            success = permissionService_->updatePermission(newPermissionData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Quyền Hạn", "Quyền hạn đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật quyền hạn. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Catalog::DTO::PermissionDTO> createdPermission = permissionService_->createPermission(newPermissionData, currentUserId_, currentUserRoleIds_);
            if (createdPermission) {
                showMessageBox("Thêm Quyền Hạn", "Quyền hạn mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm quyền hạn mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadPermissions();
            clearForm();
        }
    }
}


void PermissionManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool PermissionManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void PermissionManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Catalog.CreatePermission");
    bool canUpdate = hasPermission("Catalog.UpdatePermission");
    bool canDelete = hasPermission("Catalog.DeletePermission");
    bool canChangeStatus = hasPermission("Catalog.ChangePermissionStatus");

    addPermissionButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Catalog.ViewPermissions"));

    bool isRowSelected = permissionTable_->currentRow() >= 0;
    editPermissionButton_->setEnabled(isRowSelected && canUpdate);
    deletePermissionButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);

    bool enableForm = isRowSelected && canUpdate;
    nameLineEdit_->setEnabled(enableForm);
    moduleLineEdit_->setEnabled(enableForm);
    actionLineEdit_->setEnabled(enableForm);
    descriptionLineEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        nameLineEdit_->clear();
        moduleLineEdit_->clear();
        actionLineEdit_->clear();
        descriptionLineEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
    }
}


} // namespace Catalog
} // namespace UI
} // namespace ERP