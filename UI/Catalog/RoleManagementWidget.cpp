// UI/Catalog/RoleManagementWidget.cpp
#include "RoleManagementWidget.h" // Đã rút gọn include
#include "Role.h"              // Đã rút gọn include
#include "Permission.h"        // Đã rút gọn include
#include "RoleService.h"       // Đã rút gọn include
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
#include <QListWidget>
#include <QListWidgetItem>

namespace ERP {
namespace UI {
namespace Catalog {

RoleManagementWidget::RoleManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IRoleService> roleService,
    std::shared_ptr<Services::IPermissionService> permissionService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      roleService_(roleService),
      permissionService_(permissionService),
      securityManager_(securityManager) {

    if (!roleService_ || !permissionService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ vai trò, quyền hạn hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("RoleManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("RoleManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("RoleManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadRoles();
    updateButtonsState();
}

RoleManagementWidget::~RoleManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void RoleManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên vai trò...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &RoleManagementWidget::onSearchRoleClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    roleTable_ = new QTableWidget(this);
    roleTable_->setColumnCount(4); // ID, Tên, Mô tả, Trạng thái
    roleTable_->setHorizontalHeaderLabels({"ID", "Tên", "Mô tả", "Trạng thái"});
    roleTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    roleTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    roleTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    roleTable_->horizontalHeader()->setStretchLastSection(true);
    connect(roleTable_, &QTableWidget::itemClicked, this, &RoleManagementWidget::onRoleTableItemClicked);
    mainLayout->addWidget(roleTable_);

    // Form elements for editing/adding
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    nameLineEdit_ = new QLineEdit(this);
    descriptionLineEdit_ = new QLineEdit(this);
    statusComboBox_ = new QComboBox(this);
    statusComboBox_->addItem("Active", static_cast<int>(ERP::Common::EntityStatus::ACTIVE));
    statusComboBox_->addItem("Inactive", static_cast<int>(ERP::Common::EntityStatus::INACTIVE));
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Common::EntityStatus::PENDING));
    statusComboBox_->addItem("Deleted", static_cast<int>(ERP::Common::EntityStatus::DELETED));

    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Tên:*", this), 1, 0); formLayout->addWidget(nameLineEdit_, 1, 1);
    formLayout->addWidget(new QLabel("Mô tả:", this), 2, 0); formLayout->addWidget(descriptionLineEdit_, 2, 1);
    formLayout->addWidget(new QLabel("Trạng thái:", this), 3, 0); formLayout->addWidget(statusComboBox_, 3, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addRoleButton_ = new QPushButton("Thêm mới", this); connect(addRoleButton_, &QPushButton::clicked, this, &RoleManagementWidget::onAddRoleClicked);
    editRoleButton_ = new QPushButton("Sửa", this); connect(editRoleButton_, &QPushButton::clicked, this, &RoleManagementWidget::onEditRoleClicked);
    deleteRoleButton_ = new QPushButton("Xóa", this); connect(deleteRoleButton_, &QPushButton::clicked, this, &RoleManagementWidget::onDeleteRoleClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &RoleManagementWidget::onUpdateRoleStatusClicked);
    managePermissionsButton_ = new QPushButton("Quản lý quyền hạn", this); connect(managePermissionsButton_, &QPushButton::clicked, this, &RoleManagementWidget::onManagePermissionsClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &RoleManagementWidget::clearForm);
    buttonLayout->addWidget(addRoleButton_);
    buttonLayout->addWidget(editRoleButton_);
    buttonLayout->addWidget(deleteRoleButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(managePermissionsButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void RoleManagementWidget::loadRoles() {
    ERP::Logger::Logger::getInstance().info("RoleManagementWidget: Loading roles...");
    roleTable_->setRowCount(0);

    std::vector<ERP::Catalog::DTO::RoleDTO> roles = roleService_->getAllRoles({}, currentUserId_, currentUserRoleIds_);

    roleTable_->setRowCount(roles.size());
    for (int i = 0; i < roles.size(); ++i) {
        const auto& role = roles[i];
        roleTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(role.id)));
        roleTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(role.name)));
        roleTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(role.description.value_or(""))));
        roleTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(role.status))));
    }
    roleTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("RoleManagementWidget: Roles loaded successfully.");
}

void RoleManagementWidget::onAddRoleClicked() {
    if (!hasPermission("Catalog.CreateRole")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm vai trò.", QMessageBox::Warning);
        return;
    }
    clearForm();
    showRoleInputDialog();
}

void RoleManagementWidget::onEditRoleClicked() {
    if (!hasPermission("Catalog.UpdateRole")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa vai trò.", QMessageBox::Warning);
        return;
    }

    int selectedRow = roleTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Vai Trò", "Vui lòng chọn một vai trò để sửa.", QMessageBox::Information);
        return;
    }

    QString roleId = roleTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Catalog::DTO::RoleDTO> roleOpt = roleService_->getRoleById(roleId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (roleOpt) {
        showRoleInputDialog(&(*roleOpt));
    } else {
        showMessageBox("Sửa Vai Trò", "Không tìm thấy vai trò để sửa.", QMessageBox::Critical);
    }
}

void RoleManagementWidget::onDeleteRoleClicked() {
    if (!hasPermission("Catalog.DeleteRole")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa vai trò.", QMessageBox::Warning);
        return;
    }

    int selectedRow = roleTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Vai Trò", "Vui lòng chọn một vai trò để xóa.", QMessageBox::Information);
        return;
    }

    QString roleId = roleTable_->item(selectedRow, 0)->text();
    QString roleName = roleTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Vai Trò");
    confirmBox.setText("Bạn có chắc chắn muốn xóa vai trò '" + roleName + "' (ID: " + roleId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (roleService_->deleteRole(roleId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Vai Trò", "Vai trò đã được xóa thành công.", QMessageBox::Information);
            loadRoles();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa vai trò. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void RoleManagementWidget::onUpdateRoleStatusClicked() {
    if (!hasPermission("Catalog.ChangeRoleStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái vai trò.", QMessageBox::Warning);
        return;
    }

    int selectedRow = roleTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một vai trò để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString roleId = roleTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Catalog::DTO::RoleDTO> roleOpt = roleService_->getRoleById(roleId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!roleOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy vai trò để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Catalog::DTO::RoleDTO currentRole = *roleOpt;
    ERP::Common::EntityStatus newStatus = (currentRole.status == ERP::Common::EntityStatus::ACTIVE) ?
                                          ERP::Common::EntityStatus::INACTIVE :
                                          ERP::Common::EntityStatus::ACTIVE;

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Cập nhật trạng thái vai trò");
    confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái vai trò '" + QString::fromStdString(currentRole.name) + "' thành " + QString::fromStdString(ERP::Common::entityStatusToString(newStatus)) + "?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (roleService_->updateRoleStatus(roleId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Cập nhật trạng thái", "Trạng thái vai trò đã được cập nhật thành công.", QMessageBox::Information);
            loadRoles();
            clearForm();
        } else {
            showMessageBox("Lỗi", "Không thể cập nhật trạng thái vai trò. Vui lòng kiểm tra log.", QMessageBox::Critical);
        }
    }
}


void RoleManagementWidget::onSearchRoleClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["name_contains"] = searchText.toStdString(); // Assuming service supports "contains" filter
    }
    roleTable_->setRowCount(0);
    std::vector<ERP::Catalog::DTO::RoleDTO> roles = roleService_->getAllRoles(filter, currentUserId_, currentUserRoleIds_);

    roleTable_->setRowCount(roles.size());
    for (int i = 0; i < roles.size(); ++i) {
        const auto& role = roles[i];
        roleTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(role.id)));
        roleTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(role.name)));
        roleTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(role.description.value_or(""))));
        roleTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(role.status))));
    }
    roleTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("RoleManagementWidget: Search completed.");
}

void RoleManagementWidget::onRoleTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString roleId = roleTable_->item(row, 0)->text();
    std::optional<ERP::Catalog::DTO::RoleDTO> roleOpt = roleService_->getRoleById(roleId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (roleOpt) {
        idLineEdit_->setText(QString::fromStdString(roleOpt->id));
        nameLineEdit_->setText(QString::fromStdString(roleOpt->name));
        descriptionLineEdit_->setText(QString::fromStdString(roleOpt->description.value_or("")));
        
        int statusIndex = statusComboBox_->findData(static_cast<int>(roleOpt->status));
        if (statusIndex != -1) {
            statusComboBox_->setCurrentIndex(statusIndex);
        }

    } else {
        showMessageBox("Thông tin Vai Trò", "Không thể tải chi tiết vai trò đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void RoleManagementWidget::clearForm() {
    idLineEdit_->clear();
    nameLineEdit_->clear();
    descriptionLineEdit_->clear();
    statusComboBox_->setCurrentIndex(0); // Default to Active
    roleTable_->clearSelection();
    updateButtonsState();
}

void RoleManagementWidget::onManagePermissionsClicked() {
    if (!hasPermission("Catalog.ManageRolePermissions")) {
        showMessageBox("Lỗi", "Bạn không có quyền quản lý quyền hạn của vai trò.", QMessageBox::Warning);
        return;
    }

    int selectedRow = roleTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Quản Lý Quyền Hạn", "Vui lòng chọn một vai trò để quản lý quyền hạn.", QMessageBox::Information);
        return;
    }

    QString roleId = roleTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Catalog::DTO::RoleDTO> roleOpt = roleService_->getRoleById(roleId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (roleOpt) {
        showManagePermissionsDialog(&(*roleOpt));
    } else {
        showMessageBox("Quản Lý Quyền Hạn", "Không tìm thấy vai trò để quản lý quyền hạn.", QMessageBox::Critical);
    }
}

void RoleManagementWidget::showRoleInputDialog(ERP::Catalog::DTO::RoleDTO* role) {
    QDialog dialog(this);
    dialog.setWindowTitle(role ? "Sửa Vai Trò" : "Thêm Vai Trò Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit nameEdit(this);
    QLineEdit descriptionEdit(this);

    if (role) {
        nameEdit.setText(QString::fromStdString(role->name));
        descriptionEdit.setText(QString::fromStdString(role->description.value_or("")));
    }

    formLayout->addRow("Tên:*", &nameEdit);
    formLayout->addRow("Mô tả:", &descriptionEdit);

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(role ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Catalog::DTO::RoleDTO newRoleData;
        if (role) {
            newRoleData = *role;
        }

        newRoleData.name = nameEdit.text().toStdString();
        newRoleData.description = descriptionEdit.text().isEmpty() ? std::nullopt : std::make_optional(descriptionEdit.text().toStdString());
        newRoleData.status = ERP::Common::EntityStatus::ACTIVE; // Always set to active on creation/update via form

        bool success = false;
        if (role) {
            success = roleService_->updateRole(newRoleData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Vai Trò", "Vai trò đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật vai trò. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Catalog::DTO::RoleDTO> createdRole = roleService_->createRole(newRoleData, currentUserId_, currentUserRoleIds_);
            if (createdRole) {
                showMessageBox("Thêm Vai Trò", "Vai trò mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm vai trò mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadRoles();
            clearForm();
        }
    }
}

void RoleManagementWidget::showManagePermissionsDialog(ERP::Catalog::DTO::RoleDTO* role) {
    if (!role) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Quản lý quyền hạn cho vai trò: " + QString::fromStdString(role->name));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    // List of all available permissions
    QListWidget *allPermissionsList = new QListWidget(&dialog);
    allPermissionsList->setSelectionMode(QAbstractItemView::MultiSelection);
    dialogLayout->addWidget(new QLabel("Tất cả quyền hạn có sẵn:", &dialog));
    dialogLayout->addWidget(allPermissionsList);

    // Populate all available permissions
    std::vector<ERP::Catalog::DTO::PermissionDTO> allPermissions = permissionService_->getAllPermissions({}, currentUserId_, currentUserRoleIds_);
    for (const auto& perm : allPermissions) {
        QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(perm.name), allPermissionsList);
        item->setData(Qt::UserRole, QString::fromStdString(perm.id)); // Store ID in user data
        allPermissionsList->addItem(item);
    }

    // Select currently assigned permissions
    std::set<std::string> assignedPermissions = roleService_->getRolePermissions(role->id, currentUserId_, currentUserRoleIds_);
    for (int i = 0; i < allPermissionsList->count(); ++i) {
        QListWidgetItem *item = allPermissionsList->item(i);
        if (assignedPermissions.count(item->text().toStdString())) {
            item->setSelected(true);
        }
    }

    QPushButton *saveButton = new QPushButton("Lưu", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Collect selected permissions
        QSet<QString> newlySelectedPermissions;
        for (int i = 0; i < allPermissionsList->count(); ++i) {
            QListWidgetItem *item = allPermissionsList->item(i);
            if (item->isSelected()) {
                newlySelectedPermissions.insert(item->text());
            }
        }

        bool transactionSuccess = true;
        Common::CustomMessageBox processMsgBox(this);
        processMsgBox.setWindowTitle("Cập nhật quyền hạn");
        processMsgBox.setText("Đang cập nhật quyền hạn. Vui lòng đợi...");
        processMsgBox.setStandardButtons(QMessageBox::NoButton);
        processMsgBox.show();
        QCoreApplication::processEvents();

        // Remove unselected permissions
        for (const auto& permName : assignedPermissions) {
            if (!newlySelectedPermissions.contains(QString::fromStdString(permName))) {
                if (!roleService_->removePermissionFromRole(role->id, permName, currentUserId_, currentUserRoleIds_)) {
                    transactionSuccess = false;
                    ERP::Logger::Logger::getInstance().error("RoleManagementWidget: Failed to remove permission " + permName + " from role " + role->id + ".");
                    break;
                }
            }
        }

        // Add newly selected permissions
        if (transactionSuccess) {
            for (const auto& newPermName : newlySelectedPermissions) {
                if (!assignedPermissions.count(newPermName.toStdString())) {
                    if (!roleService_->assignPermissionToRole(role->id, newPermName.toStdString(), currentUserId_, currentUserRoleIds_)) {
                        transactionSuccess = false;
                        ERP::Logger::Logger::getInstance().error("RoleManagementWidget: Failed to assign permission " + newPermName.toStdString() + " to role " + role->id + ".");
                        break;
                    }
                }
            }
        }
        processMsgBox.close();

        if (transactionSuccess) {
            showMessageBox("Quản lý Quyền Hạn", "Quyền hạn đã được cập nhật thành công cho vai trò.", QMessageBox::Information);
            // Re-select the role in the main table to refresh displayed details if needed
            // Note: This does not update the current form if it's open, only the table.
            loadRoles();
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật quyền hạn. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void RoleManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool RoleManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void RoleManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Catalog.CreateRole");
    bool canUpdate = hasPermission("Catalog.UpdateRole");
    bool canDelete = hasPermission("Catalog.DeleteRole");
    bool canChangeStatus = hasPermission("Catalog.ChangeRoleStatus");
    bool canManagePermissions = hasPermission("Catalog.ManageRolePermissions");

    addRoleButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Catalog.ViewRoles"));

    bool isRowSelected = roleTable_->currentRow() >= 0;
    editRoleButton_->setEnabled(isRowSelected && canUpdate);
    deleteRoleButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    managePermissionsButton_->setEnabled(isRowSelected && canManagePermissions);

    bool enableForm = isRowSelected && canUpdate;
    nameLineEdit_->setEnabled(enableForm);
    descriptionLineEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        nameLineEdit_->clear();
        descriptionLineEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
    }
}


} // namespace Catalog
} // namespace UI
} // namespace ERP