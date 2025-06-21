// UI/Catalog/RoleManagementWidget.h
#ifndef UI_CATALOG_ROLEMANAGEMENTWIDGET_H
#define UI_CATALOG_ROLEMANAGEMENTWIDGET_H
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
#include <QDialog> // For role input dialog
#include <QListWidget> // For permissions list
#include <QSet> // For QSet<QString>

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "RoleService.h"       // Dịch vụ quản lý vai trò
#include "PermissionService.h.h" // Dịch vụ quản lý quyền hạn
#include "ISecurityManager.h"  // Dịch vụ bảo mật
#include "Logger.h"            // Logging
#include "ErrorHandler.h"    // Xử lý lỗi
#include "Common.h"          // Các enum chung
#include "DateUtils.h"       // Xử lý ngày tháng
#include "StringUtils.h"     // Xử lý chuỗi
#include "CustomMessageBox.h" // Hộp thoại thông báo tùy chỉnh
#include "Role.h"            // Role DTO (for enums etc.)
#include "Permission.h"      // Permission DTO (for enums etc.)


namespace ERP {
namespace UI {
namespace Catalog {

/**
 * @brief RoleManagementWidget class provides a UI for managing Roles.
 * This widget allows viewing, creating, updating, deleting, and changing role status.
 * It also supports managing permissions assigned to roles.
 */
class RoleManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for RoleManagementWidget.
     * @param roleService Shared pointer to IRoleService.
     * @param permissionService Shared pointer to IPermissionService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit RoleManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IRoleService> roleService = nullptr,
        std::shared_ptr<Services::IPermissionService> permissionService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~RoleManagementWidget();

private slots:
    void loadRoles();
    void onAddRoleClicked();
    void onEditRoleClicked();
    void onDeleteRoleClicked();
    void onUpdateRoleStatusClicked();
    void onSearchRoleClicked();
    void onRoleTableItemClicked(int row, int column);
    void clearForm();

    void onManagePermissionsClicked(); // New slot for managing permissions

private:
    std::shared_ptr<Services::IRoleService> roleService_;
    std::shared_ptr<Services::IPermissionService> permissionService_;
    std::shared_ptr<Security::ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *roleTable_;
    QPushButton *addRoleButton_;
    QPushButton *editRoleButton_;
    QPushButton *deleteRoleButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;
    QPushButton *managePermissionsButton_; // New button

    // Form elements for editing/adding
    QLineEdit *idLineEdit_;
    QLineEdit *nameLineEdit_;
    QLineEdit *descriptionLineEdit_;
    QComboBox *statusComboBox_; // For status selection

    // Helper functions
    void setupUI();
    void showRoleInputDialog(ERP::Catalog::DTO::RoleDTO* role = nullptr);
    void showManagePermissionsDialog(ERP::Catalog::DTO::RoleDTO* role); // New dialog for permissions
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState(); // Control button enable/disable based on selection and permissions
    
    // Permission checking helper
    bool hasPermission(const std::string& permission);
};

} // namespace Catalog
} // namespace UI
} // namespace ERP

#endif // UI_CATALOG_ROLEMANAGEMENTWIDGET_H