// UI/User/UserManagementWidget.h
#ifndef UI_USER_USERMANAGEMENTWIDGET_H
#define UI_USER_USERMANAGEMENTWIDGET_H
#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QDialog> // For user input dialog
#include <QCheckBox> // For is_locked
#include <QDateTimeEdit> // For date/time selection

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "UserService.h"     // Dịch vụ người dùng
#include "RoleService.h"     // Dịch vụ vai trò (để hiển thị/chọn vai trò)
#include "ISecurityManager.h"  // Dịch vụ bảo mật
#include "Logger.h"            // Logging
#include "ErrorHandler.h"    // Xử lý lỗi
#include "Common.h"          // Các enum chung
#include "DateUtils.h"       // Xử lý ngày tháng
#include "StringUtils.h"     // Xử lý chuỗi
#include "CustomMessageBox.h" // Hộp thoại thông báo tùy chỉnh
#include "User.h"            // User DTO (for enums etc.)
#include "UserProfile.h"     // UserProfile DTO (for enums etc.)


namespace ERP {
namespace UI {
namespace User {

/**
 * @brief UserManagementWidget class provides a UI for managing User accounts.
 * This widget allows viewing, creating, updating, deleting, and changing user status.
 * It also includes basic profile management.
 */
class UserManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for UserManagementWidget.
     * @param userService Shared pointer to IUserService.
     * @param roleService Shared pointer to IRoleService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit UserManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IUserService> userService = nullptr,
        std::shared_ptr<Catalog::Services::IRoleService> roleService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~UserManagementWidget();

private slots:
    void loadUsers();
    void onAddUserClicked();
    void onEditUserClicked();
    void onDeleteUserClicked();
    void onUpdateUserStatusClicked();
    void onChangePasswordClicked(); // New for password change
    void onManageRolesClicked(); // New for managing user roles
    void onSearchUserClicked();
    void onUserTableItemClicked(int row, int column);
    void clearForm();

private:
    std::shared_ptr<Services::IUserService> userService_;
    std::shared_ptr<Catalog::Services::IRoleService> roleService_;
    std::shared_ptr<Security::ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *userTable_;
    QPushButton *addUserButton_;
    QPushButton *editUserButton_;
    QPushButton *deleteUserButton_;
    QPushButton *updateStatusButton_;
    QPushButton *changePasswordButton_;
    QPushButton *manageRolesButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;

    // Form elements for editing/adding
    QLineEdit *idLineEdit_;
    QLineEdit *usernameLineEdit_;
    QLineEdit *emailLineEdit_;
    QLineEdit *firstNameLineEdit_;
    QLineEdit *lastNameLineEdit_;
    QLineEdit *phoneNumberLineEdit_;
    QComboBox *typeComboBox_; // UserType
    QComboBox *roleComboBox_; // Role selection
    QDateTimeEdit *lastLoginTimeEdit_; // Read-only
    QLineEdit *lastLoginIpLineEdit_; // Read-only
    QCheckBox *isLockedCheckBox_; // Read-only or managed by special action
    QLineEdit *failedLoginAttemptsLineEdit_; // Read-only
    QDateTimeEdit *lockUntilTimeEdit_; // Read-only

    // Helper functions
    void setupUI();
    void populateRoleComboBox();
    void populateTypeComboBox();
    void showUserInputDialog(ERP::User::DTO::UserDTO* user = nullptr);
    void showChangePasswordDialog(ERP::User::DTO::UserDTO* user);
    void showManageUserRolesDialog(ERP::User::DTO::UserDTO* user);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState(); // Control button enable/disable based on selection and permissions
    
    // Permission checking helper
    bool hasPermission(const std::string& permission);
};

} // namespace User
} // namespace UI
} // namespace ERP

#endif // UI_USER_USERMANAGEMENTWIDGET_H