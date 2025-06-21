// UI/User/UserManagementWidget.cpp
#include "UserManagementWidget.h" // Đã rút gọn include
#include "User.h"              // Đã rút gọn include
#include "UserProfile.h"       // Đã rút gọn include
#include "Role.h"              // Đã rút gọn include
#include "UserService.h"       // Đã rút gọn include
#include "RoleService.h"       // Đã rút gọn include
#include "ISecurityManager.h"  // Đã rút gọn include
#include "Logger.h"            // Đã rút gọn include
#include "ErrorHandler.h"      // Đã rút gọn include
#include "Common.h"            // Đã rút gọn include
#include "DateUtils.h"         // Đã rút gọn include
#include "StringUtils.h"       // Đã rút gọn include
#include "CustomMessageBox.h"  // Đã rút gọn include

#include <QInputDialog> // For getting input from user
#include <QListWidget>
#include <QListWidgetItem>
#include <QSet> // For QSet

namespace ERP {
namespace UI {
namespace User {

UserManagementWidget::UserManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IUserService> userService,
    std::shared_ptr<Catalog::Services::IRoleService> roleService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      userService_(userService),
      roleService_(roleService),
      securityManager_(securityManager) {

    if (!userService_ || !roleService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ người dùng, vai trò hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("UserManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("UserManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("UserManagementWidget: Authentication Service not available. Running with limited privileges.");
    }


    setupUI();
    loadUsers();
    updateButtonsState();
}

UserManagementWidget::~UserManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void UserManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên đăng nhập hoặc email...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &UserManagementWidget::onSearchUserClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    userTable_ = new QTableWidget(this);
    userTable_->setColumnCount(10); // ID, Tên đăng nhập, Email, Tên, Họ, Điện thoại, Loại, Vai trò, Trạng thái, Lần cuối đăng nhập
    userTable_->setHorizontalHeaderLabels({"ID", "Tên đăng nhập", "Email", "Tên", "Họ", "Điện thoại", "Loại", "Vai trò", "Trạng thái", "Lần cuối ĐN"});
    userTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    userTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    userTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    userTable_->horizontalHeader()->setStretchLastSection(true);
    connect(userTable_, &QTableWidget::itemClicked, this, &UserManagementWidget::onUserTableItemClicked);
    mainLayout->addWidget(userTable_);

    // Form elements for editing/adding
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    usernameLineEdit_ = new QLineEdit(this);
    emailLineEdit_ = new QLineEdit(this);
    firstNameLineEdit_ = new QLineEdit(this);
    lastNameLineEdit_ = new QLineEdit(this);
    phoneNumberLineEdit_ = new QLineEdit(this);
    typeComboBox_ = new QComboBox(this);
    roleComboBox_ = new QComboBox(this);
    lastLoginTimeEdit_ = new QDateTimeEdit(this); lastLoginTimeEdit_->setReadOnly(true); lastLoginTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    lastLoginIpLineEdit_ = new QLineEdit(this); lastLoginIpLineEdit_->setReadOnly(true);
    isLockedCheckBox_ = new QCheckBox("Tài khoản bị khóa", this); isLockedCheckBox_->setEnabled(false); // Managed by system
    failedLoginAttemptsLineEdit_ = new QLineEdit(this); failedLoginAttemptsLineEdit_->setReadOnly(true);
    lockUntilTimeEdit_ = new QDateTimeEdit(this); lockUntilTimeEdit_->setReadOnly(true); lockUntilTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");


    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Tên đăng nhập:*", this), 1, 0); formLayout->addWidget(usernameLineEdit_, 1, 1);
    formLayout->addWidget(new QLabel("Email:", this), 2, 0); formLayout->addWidget(emailLineEdit_, 2, 1);
    formLayout->addWidget(new QLabel("Tên:", this), 3, 0); formLayout->addWidget(firstNameLineEdit_, 3, 1);
    formLayout->addWidget(new QLabel("Họ:", this), 4, 0); formLayout->addWidget(lastNameLineEdit_, 4, 1);
    formLayout->addWidget(new QLabel("Điện thoại:", this), 5, 0); formLayout->addWidget(phoneNumberLineEdit_, 5, 1);
    formLayout->addWidget(new QLabel("Loại người dùng:", this), 6, 0); formLayout->addWidget(typeComboBox_, 6, 1);
    formLayout->addWidget(new QLabel("Vai trò chính:", this), 7, 0); formLayout->addWidget(roleComboBox_, 7, 1);
    formLayout->addWidget(new QLabel("Lần cuối ĐN:", this), 8, 0); formLayout->addWidget(lastLoginTimeEdit_, 8, 1);
    formLayout->addWidget(new QLabel("IP cuối ĐN:", this), 9, 0); formLayout->addWidget(lastLoginIpLineEdit_, 9, 1);
    formLayout->addWidget(new QLabel("Bị khóa:", this), 10, 0); formLayout->addWidget(isLockedCheckBox_, 10, 1);
    formLayout->addWidget(new QLabel("Số lần ĐN sai:", this), 11, 0); formLayout->addWidget(failedLoginAttemptsLineEdit_, 11, 1);
    formLayout->addWidget(new QLabel("Khóa đến:", this), 12, 0); formLayout->addWidget(lockUntilTimeEdit_, 12, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addUserButton_ = new QPushButton("Thêm mới", this); connect(addUserButton_, &QPushButton::clicked, this, &UserManagementWidget::onAddUserClicked);
    editUserButton_ = new QPushButton("Sửa", this); connect(editUserButton_, &QPushButton::clicked, this, &UserManagementWidget::onEditUserClicked);
    deleteUserButton_ = new QPushButton("Xóa", this); connect(deleteUserButton_, &QPushButton::clicked, this, &UserManagementWidget::onDeleteUserClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &UserManagementWidget::onUpdateUserStatusClicked);
    changePasswordButton_ = new QPushButton("Đổi mật khẩu", this); connect(changePasswordButton_, &QPushButton::clicked, this, &UserManagementWidget::onChangePasswordClicked);
    manageRolesButton_ = new QPushButton("Quản lý vai trò", this); connect(manageRolesButton_, &QPushButton::clicked, this, &UserManagementWidget::onManageRolesClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &UserManagementWidget::clearForm);
    buttonLayout->addWidget(addUserButton_);
    buttonLayout->addWidget(editUserButton_);
    buttonLayout->addWidget(deleteUserButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(changePasswordButton_);
    buttonLayout->addWidget(manageRolesButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void UserManagementWidget::loadUsers() {
    ERP::Logger::Logger::getInstance().info("UserManagementWidget: Loading users...");
    userTable_->setRowCount(0);

    std::vector<ERP::User::DTO::UserDTO> users = userService_->getAllUsers({}, currentUserId_, currentUserRoleIds_);

    userTable_->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        const auto& user = users[i];
        userTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(user.id)));
        userTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(user.username)));
        userTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(user.email.value_or(""))));
        userTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(user.firstName.value_or(""))));
        userTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(user.lastName.value_or(""))));
        userTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(user.phoneNumber.value_or(""))));
        userTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(user.getTypeString()))); // UserType string
        
        // Resolve Role Name
        QString roleName = "N/A";
        std::optional<ERP::Catalog::DTO::RoleDTO> role = roleService_->getRoleById(user.roleId, currentUserId_, currentUserRoleIds_);
        if (role) roleName = QString::fromStdString(role->name);
        userTable_->setItem(i, 7, new QTableWidgetItem(roleName));

        userTable_->setItem(i, 8, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(user.status))));
        userTable_->setItem(i, 9, new QTableWidgetItem(user.lastLoginTime ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*user.lastLoginTime, ERP::Common::DATETIME_FORMAT)) : "N/A"));
    }
    userTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("UserManagementWidget: Users loaded successfully.");
}

void UserManagementWidget::populateRoleComboBox() {
    roleComboBox_->clear();
    std::vector<ERP::Catalog::DTO::RoleDTO> allRoles = roleService_->getAllRoles({}, currentUserId_, currentUserRoleIds_);
    for (const auto& role : allRoles) {
        roleComboBox_->addItem(QString::fromStdString(role.name), QString::fromStdString(role.id));
    }
}

void UserManagementWidget::populateTypeComboBox() {
    typeComboBox_->clear();
    typeComboBox_->addItem("Admin", static_cast<int>(ERP::User::DTO::UserType::ADMIN));
    typeComboBox_->addItem("Employee", static_cast<int>(ERP::User::DTO::UserType::EMPLOYEE));
    typeComboBox_->addItem("Customer Portal", static_cast<int>(ERP::User::DTO::UserType::CUSTOMER_PORTAL));
    typeComboBox_->addItem("Supplier Portal", static_cast<int>(ERP::User::DTO::UserType::SUPPLIER_PORTAL));
    typeComboBox_->addItem("Other", static_cast<int>(ERP::User::DTO::UserType::OTHER));
    typeComboBox_->addItem("Unknown", static_cast<int>(ERP::User::DTO::UserType::UNKNOWN));
}

void UserManagementWidget::onAddUserClicked() {
    if (!hasPermission("User.CreateUser")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm người dùng.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateRoleComboBox();
    populateTypeComboBox();
    showUserInputDialog();
}

void UserManagementWidget::onEditUserClicked() {
    if (!hasPermission("User.UpdateUser")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa người dùng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = userTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Người Dùng", "Vui lòng chọn một người dùng để sửa.", QMessageBox::Information);
        return;
    }

    QString userId = userTable_->item(selectedRow, 0)->text();
    std::optional<ERP::User::DTO::UserDTO> userOpt = userService_->getUserById(userId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (userOpt) {
        populateRoleComboBox();
        populateTypeComboBox();
        showUserInputDialog(&(*userOpt));
    } else {
        showMessageBox("Sửa Người Dùng", "Không tìm thấy người dùng để sửa.", QMessageBox::Critical);
    }
}

void UserManagementWidget::onDeleteUserClicked() {
    if (!hasPermission("User.DeleteUser")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa người dùng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = userTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Người Dùng", "Vui lòng chọn một người dùng để xóa.", QMessageBox::Information);
        return;
    }

    QString userId = userTable_->item(selectedRow, 0)->text();
    QString username = userTable_->item(selectedRow, 1)->text();

    if (userId.toStdString() == currentUserId_) {
        showMessageBox("Lỗi Xóa", "Bạn không thể xóa tài khoản của chính mình.", QMessageBox::Warning);
        return;
    }

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Người Dùng");
    confirmBox.setText("Bạn có chắc chắn muốn xóa người dùng '" + username + "' (ID: " + userId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (userService_->deleteUser(userId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Người Dùng", "Người dùng đã được xóa thành công.", QMessageBox::Information);
            loadUsers();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa người dùng. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void UserManagementWidget::onUpdateUserStatusClicked() {
    if (!hasPermission("User.ChangeUserStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái người dùng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = userTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một người dùng để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString userId = userTable_->item(selectedRow, 0)->text();
    std::optional<ERP::User::DTO::UserDTO> userOpt = userService_->getUserById(userId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!userOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy người dùng để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::User::DTO::UserDTO currentUser = *userOpt;
    ERP::Common::EntityStatus newStatus = (currentUser.status == ERP::Common::EntityStatus::ACTIVE) ?
                                          ERP::Common::EntityStatus::INACTIVE :
                                          ERP::Common::EntityStatus::ACTIVE;

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Cập nhật trạng thái người dùng");
    confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái người dùng '" + QString::fromStdString(currentUser.username) + "' thành " + QString::fromStdString(ERP::Common::entityStatusToString(newStatus)) + "?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (userService_->updateUserStatus(userId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Cập nhật trạng thái", "Trạng thái người dùng đã được cập nhật thành công.", QMessageBox::Information);
            loadUsers();
            clearForm();
        } else {
            showMessageBox("Lỗi", "Không thể cập nhật trạng thái người dùng. Vui lòng kiểm tra log.", QMessageBox::Critical);
        }
    }
}

void UserManagementWidget::onChangePasswordClicked() {
    if (!hasPermission("User.ChangeAnyPassword")) { // Permission to change any password
        // If user can only change own password, check userId == currentUserId
        // if (selectedUserId != currentUserId_) { show message }
        showMessageBox("Lỗi", "Bạn không có quyền đổi mật khẩu người dùng khác.", QMessageBox::Warning);
        return;
    }

    int selectedRow = userTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Đổi Mật Khẩu", "Vui lòng chọn một người dùng để đổi mật khẩu.", QMessageBox::Information);
        return;
    }

    QString userId = userTable_->item(selectedRow, 0)->text();
    std::optional<ERP::User::DTO::UserDTO> userOpt = userService_->getUserById(userId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (userOpt) {
        showChangePasswordDialog(&(*userOpt));
    } else {
        showMessageBox("Đổi Mật Khẩu", "Không tìm thấy người dùng để đổi mật khẩu.", QMessageBox::Critical);
    }
}

void UserManagementWidget::onManageRolesClicked() {
    if (!hasPermission("User.ManageRoles")) { // Assuming this permission exists
        showMessageBox("Lỗi", "Bạn không có quyền quản lý vai trò người dùng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = userTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Quản Lý Vai Trò", "Vui lòng chọn một người dùng để quản lý vai trò.", QMessageBox::Information);
        return;
    }

    QString userId = userTable_->item(selectedRow, 0)->text();
    std::optional<ERP::User::DTO::UserDTO> userOpt = userService_->getUserById(userId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (userOpt) {
        showManageUserRolesDialog(&(*userOpt));
    } else {
        showMessageBox("Quản Lý Vai Trò", "Không tìm thấy người dùng để quản lý vai trò.", QMessageBox::Critical);
    }
}


void UserManagementWidget::onSearchUserClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["username_or_email_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    userTable_->setRowCount(0);
    std::vector<ERP::User::DTO::UserDTO> users = userService_->getAllUsers(filter, currentUserId_, currentUserRoleIds_);

    userTable_->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        const auto& user = users[i];
        userTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(user.id)));
        userTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(user.username)));
        userTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(user.email.value_or(""))));
        userTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(user.firstName.value_or(""))));
        userTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(user.lastName.value_or(""))));
        userTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(user.phoneNumber.value_or(""))));
        userTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(user.getTypeString())));
        
        QString roleName = "N/A";
        std::optional<ERP::Catalog::DTO::RoleDTO> role = roleService_->getRoleById(user.roleId, currentUserId_, currentUserRoleIds_);
        if (role) roleName = QString::fromStdString(role->name);
        userTable_->setItem(i, 7, new QTableWidgetItem(roleName));

        userTable_->setItem(i, 8, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(user.status))));
        userTable_->setItem(i, 9, new QTableWidgetItem(user.lastLoginTime ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*user.lastLoginTime, ERP::Common::DATETIME_FORMAT)) : "N/A"));
    }
    userTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("UserManagementWidget: Search completed.");
}

void UserManagementWidget::onUserTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString userId = userTable_->item(row, 0)->text();
    std::optional<ERP::User::DTO::UserDTO> userOpt = userService_->getUserById(userId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (userOpt) {
        idLineEdit_->setText(QString::fromStdString(userOpt->id));
        usernameLineEdit_->setText(QString::fromStdString(userOpt->username));
        emailLineEdit_->setText(QString::fromStdString(userOpt->email.value_or("")));
        firstNameLineEdit_->setText(QString::fromStdString(userOpt->firstName.value_or("")));
        lastNameLineEdit_->setText(QString::fromStdString(userOpt->lastName.value_or("")));
        phoneNumberLineEdit_->setText(QString::fromStdString(userOpt->phoneNumber.value_or("")));
        
        populateTypeComboBox();
        int typeIndex = typeComboBox_->findData(static_cast<int>(userOpt->type));
        if (typeIndex != -1) typeComboBox_->setCurrentIndex(typeIndex);

        populateRoleComboBox();
        int roleIndex = roleComboBox_->findData(QString::fromStdString(userOpt->roleId));
        if (roleIndex != -1) roleComboBox_->setCurrentIndex(roleIndex);

        if (userOpt->lastLoginTime) lastLoginTimeEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(userOpt->lastLoginTime->time_since_epoch()).count()));
        else lastLoginTimeEdit_->clear();

        lastLoginIpLineEdit_->setText(QString::fromStdString(userOpt->lastLoginIp.value_or("")));
        isLockedCheckBox_->setChecked(userOpt->isLocked);
        failedLoginAttemptsLineEdit_->setText(QString::number(userOpt->failedLoginAttempts));
        if (userOpt->lockUntilTime) lockUntilTimeEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(userOpt->lockUntilTime->time_since_epoch()).count()));
        else lockUntilTimeEdit_->clear();

    } else {
        showMessageBox("Thông tin Người Dùng", "Không thể tải chi tiết người dùng đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void UserManagementWidget::clearForm() {
    idLineEdit_->clear();
    usernameLineEdit_->clear();
    emailLineEdit_->clear();
    firstNameLineEdit_->clear();
    lastNameLineEdit_->clear();
    phoneNumberLineEdit_->clear();
    typeComboBox_->setCurrentIndex(0); // Default to Admin or first
    roleComboBox_->clear(); // Will be repopulated
    lastLoginTimeEdit_->clear();
    lastLoginIpLineEdit_->clear();
    isLockedCheckBox_->setChecked(false);
    failedLoginAttemptsLineEdit_->clear();
    lockUntilTimeEdit_->clear();
    userTable_->clearSelection();
    updateButtonsState();
}


void UserManagementWidget::showUserInputDialog(ERP::User::DTO::UserDTO* user) {
    QDialog dialog(this);
    dialog.setWindowTitle(user ? "Sửa Người Dùng" : "Thêm Người Dùng Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit usernameEdit(this);
    QLineEdit passwordEdit(this); passwordEdit.setEchoMode(QLineEdit::Password);
    QLineEdit confirmPasswordEdit(this); confirmPasswordEdit.setEchoMode(QLineEdit::Password);
    QLineEdit emailEdit(this);
    QLineEdit firstNameEdit(this);
    QLineEdit lastNameEdit(this);
    QLineEdit phoneNumberEdit(this);
    QComboBox typeCombo(this); populateTypeComboBox();
    for (int i = 0; i < typeComboBox_->count(); ++i) typeCombo.addItem(typeComboBox_->itemText(i), typeComboBox_->itemData(i));
    QComboBox roleCombo(this); populateRoleComboBox();
    for (int i = 0; i < roleComboBox_->count(); ++i) roleCombo.addItem(roleComboBox_->itemText(i), roleComboBox_->itemData(i));


    if (user) {
        usernameEdit.setText(QString::fromStdString(user->username));
        // Password fields are not populated for security reasons when editing
        emailEdit.setText(QString::fromStdString(user->email.value_or("")));
        firstNameEdit.setText(QString::fromStdString(user->firstName.value_or("")));
        lastNameEdit.setText(QString::fromStdString(user->lastName.value_or("")));
        phoneNumberEdit.setText(QString::fromStdString(user->phoneNumber.value_or("")));
        
        int typeIndex = typeCombo.findData(static_cast<int>(user->type));
        if (typeIndex != -1) typeCombo.setCurrentIndex(typeIndex);
        
        int roleIndex = roleCombo.findData(QString::fromStdString(user->roleId));
        if (roleIndex != -1) roleCombo.setCurrentIndex(roleIndex);

        // Disable username editing for existing users
        usernameEdit.setReadOnly(true);
    }

    formLayout->addRow("Tên đăng nhập:*", &usernameEdit);
    // Password fields only shown for new user or via dedicated change password dialog
    if (!user) {
        formLayout->addRow("Mật khẩu:*", &passwordEdit);
        formLayout->addRow("Xác nhận mật khẩu:*", &confirmPasswordEdit);
    }
    formLayout->addRow("Email:", &emailEdit);
    formLayout->addRow("Tên:", &firstNameEdit);
    formLayout->addRow("Họ:", &lastNameEdit);
    formLayout->addRow("Điện thoại:", &phoneNumberEdit);
    formLayout->addRow("Loại người dùng:", &typeCombo);
    formLayout->addRow("Vai trò chính:*", &roleCombo);

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(user ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::User::DTO::UserDTO newUserData;
        if (user) {
            newUserData = *user; // Start with existing data for update
        } else {
            // New user, validate passwords
            if (passwordEdit.text().isEmpty() || passwordEdit.text() != confirmPasswordEdit.text()) {
                showMessageBox("Lỗi", "Mật khẩu không khớp hoặc trống.", QMessageBox::Warning);
                return; // Do not proceed with operation
            }
            if (passwordEdit.text().length() < 8) {
                showMessageBox("Lỗi", "Mật khẩu phải có ít nhất 8 ký tự.", QMessageBox::Warning);
                return; // Do not proceed
            }
            newUserData.passwordHash = passwordEdit.text().toStdString(); // Will be hashed by service
            // Salt will be generated by service
        }

        newUserData.username = usernameEdit.text().toStdString();
        newUserData.email = emailEdit.text().isEmpty() ? std::nullopt : std::make_optional(emailEdit.text().toStdString());
        newUserData.firstName = firstNameEdit.text().isEmpty() ? std::nullopt : std::make_optional(firstNameEdit.text().toStdString());
        newUserData.lastName = lastNameEdit.text().isEmpty() ? std::nullopt : std::make_optional(lastNameEdit.text().toStdString());
        newUserData.phoneNumber = phoneNumberEdit.text().isEmpty() ? std::nullopt : std::make_optional(phoneNumberEdit.text().toStdString());
        newUserData.type = static_cast<ERP::User::DTO::UserType>(typeCombo.currentData().toInt());
        newUserData.roleId = roleCombo.currentData().toString().toStdString(); // Selected role ID
        
        newUserData.status = ERP::Common::EntityStatus::ACTIVE; // Always set to active on creation/update via form

        bool success = false;
        if (user) {
            success = userService_->updateUser(newUserData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Người Dùng", "Người dùng đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật người dùng. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::User::DTO::UserDTO> createdUser = userService_->createUser(newUserData, currentUserId_, currentUserRoleIds_);
            if (createdUser) {
                showMessageBox("Thêm Người Dùng", "Người dùng mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm người dùng mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadUsers();
            clearForm();
        }
    }
}

void UserManagementWidget::showChangePasswordDialog(ERP::User::DTO::UserDTO* user) {
    if (!user) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Đổi Mật Khẩu cho: " + QString::fromStdString(user->username));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit newPasswordEdit(this);
    newPasswordEdit.setEchoMode(QLineEdit::Password);
    QLineEdit confirmNewPasswordEdit(this);
    confirmNewPasswordEdit.setEchoMode(QLineEdit::Password);

    formLayout->addRow("Mật khẩu mới:*", &newPasswordEdit);
    formLayout->addRow("Xác nhận mật khẩu mới:*", &confirmNewPasswordEdit);
    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton("Đổi Mật Khẩu", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString newPassword = newPasswordEdit.text();
        QString confirmNewPassword = confirmNewPasswordEdit.text();

        if (newPassword.isEmpty() || newPassword != confirmNewPassword) {
            showMessageBox("Lỗi", "Mật khẩu không khớp hoặc trống.", QMessageBox::Warning);
            return;
        }
        if (newPassword.length() < 8) {
            showMessageBox("Lỗi", "Mật khẩu phải có ít nhất 8 ký tự.", QMessageBox::Warning);
            return;
        }

        if (userService_->changePassword(user->id, newPassword.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Đổi Mật Khẩu", "Mật khẩu đã được đổi thành công.", QMessageBox::Information);
            loadUsers(); // Refresh data in table
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể đổi mật khẩu. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}

void UserManagementWidget::showManageUserRolesDialog(ERP::User::DTO::UserDTO* user) {
    if (!user) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Quản lý vai trò cho người dùng: " + QString::fromStdString(user->username));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QListWidget *allRolesList = new QListWidget(&dialog);
    allRolesList->setSelectionMode(QAbstractItemView::MultiSelection);
    dialogLayout->addWidget(new QLabel("Tất cả vai trò có sẵn:", &dialog));
    dialogLayout->addWidget(allRolesList);

    // Populate all available roles
    std::vector<ERP::Catalog::DTO::RoleDTO> allRoles = roleService_->getAllRoles({}, currentUserId_, currentUserRoleIds_);
    for (const auto& role : allRoles) {
        QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(role.name), allRolesList);
        item->setData(Qt::UserRole, QString::fromStdString(role.id)); // Store ID in user data
        allRolesList->addItem(item);
    }

    // Select currently assigned roles
    std::vector<std::string> assignedRoleIds = userService_->getUserRoles(user->id, currentUserId_, currentUserRoleIds_);
    QSet<QString> assignedRoleNames; // To quickly check assigned roles
    for (const auto& roleId : assignedRoleIds) {
        std::optional<ERP::Catalog::DTO::RoleDTO> role = roleService_->getRoleById(roleId, currentUserId_, currentUserRoleIds_);
        if (role) assignedRoleNames.insert(QString::fromStdString(role->name));
    }

    for (int i = 0; i < allRolesList->count(); ++i) {
        QListWidgetItem *item = allRolesList->item(i);
        if (assignedRoleNames.contains(item->text())) {
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
        QSet<QString> newlySelectedRoles;
        for (int i = 0; i < allRolesList->count(); ++i) {
            QListWidgetItem *item = allRolesList->item(i);
            if (item->isSelected()) {
                newlySelectedRoles.insert(item->text());
            }
        }

        bool transactionSuccess = true;
        Common::CustomMessageBox processMsgBox(this);
        processMsgBox.setWindowTitle("Cập nhật vai trò");
        processMsgBox.setText("Đang cập nhật vai trò. Vui lòng đợi...");
        processMsgBox.setStandardButtons(QMessageBox::NoButton);
        processMsgBox.show();
        QCoreApplication::processEvents();

        // Remove unselected roles
        for (const auto& roleName : assignedRoleNames) {
            if (!newlySelectedRoles.contains(roleName)) {
                // Get role ID from name (or store ID in item data)
                std::optional<ERP::Catalog::DTO::RoleDTO> roleToRemove = roleService_->getRoleByName(roleName.toStdString(), currentUserId_, currentUserRoleIds_);
                if (roleToRemove) {
                    if (!userService_->removeUserRole(user->id, roleToRemove->id, currentUserId_, currentUserRoleIds_)) {
                        transactionSuccess = false;
                        ERP::Logger::Logger::getInstance().error("UserManagementWidget: Failed to remove role " + roleToRemove->name + " from user " + user->id + ".");
                        break;
                    }
                }
            }
        }

        // Add newly selected roles
        if (transactionSuccess) {
            for (const auto& newRoleName : newlySelectedRoles) {
                if (!assignedRoleNames.contains(newRoleName)) {
                    std::optional<ERP::Catalog::DTO::RoleDTO> roleToAdd = roleService_->getRoleByName(newRoleName.toStdString(), currentUserId_, currentUserRoleIds_);
                    if (roleToAdd) {
                        if (!userService_->assignUserRole(user->id, roleToAdd->id, currentUserId_, currentUserRoleIds_)) {
                            transactionSuccess = false;
                            ERP::Logger::Logger::getInstance().error("UserManagementWidget: Failed to assign role " + roleToAdd->name + " to user " + user->id + ".");
                            break;
                        }
                    }
                }
            }
        }
        processMsgBox.close();

        if (transactionSuccess) {
            showMessageBox("Quản lý Vai Trò", "Vai trò đã được cập nhật thành công cho người dùng.", QMessageBox::Information);
            loadUsers(); // Refresh the main table to show updated roles
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật vai trò. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void UserManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool UserManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void UserManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("User.CreateUser");
    bool canUpdate = hasPermission("User.UpdateUser");
    bool canDelete = hasPermission("User.DeleteUser");
    bool canChangeStatus = hasPermission("User.ChangeUserStatus");
    bool canChangePassword = hasPermission("User.ChangeAnyPassword");
    bool canManageRoles = hasPermission("User.ManageRoles");

    addUserButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("User.ViewUsers"));

    bool isRowSelected = userTable_->currentRow() >= 0;
    editUserButton_->setEnabled(isRowSelected && canUpdate);
    deleteUserButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    changePasswordButton_->setEnabled(isRowSelected && canChangePassword);
    manageRolesButton_->setEnabled(isRowSelected && canManageRoles);

    bool enableForm = isRowSelected && canUpdate;
    usernameLineEdit_->setEnabled(enableForm && (!isRowSelected || hasPermission("User.UpdateUsername"))); // Username might be protected
    emailLineEdit_->setEnabled(enableForm);
    firstNameLineEdit_->setEnabled(enableForm);
    lastNameLineEdit_->setEnabled(enableForm);
    phoneNumberLineEdit_->setEnabled(enableForm);
    typeComboBox_->setEnabled(enableForm);
    roleComboBox_->setEnabled(enableForm);

    // Read-only fields remain read-only
    lastLoginTimeEdit_->setEnabled(false);
    lastLoginIpLineEdit_->setEnabled(false);
    isLockedCheckBox_->setEnabled(false);
    failedLoginAttemptsLineEdit_->setEnabled(false);
    lockUntilTimeEdit_->setEnabled(false);


    if (!isRowSelected) {
        idLineEdit_->clear();
        usernameLineEdit_->clear();
        emailLineEdit_->clear();
        firstNameLineEdit_->clear();
        lastNameLineEdit_->clear();
        phoneNumberLineEdit_->clear();
        typeComboBox_->setCurrentIndex(0);
        roleComboBox_->clear();
        lastLoginTimeEdit_->clear();
        lastLoginIpLineEdit_->clear();
        isLockedCheckBox_->setChecked(false);
        failedLoginAttemptsLineEdit_->clear();
        lockUntilTimeEdit_->clear();
    }
}


} // namespace User
} // namespace UI
} // namespace ERP