// UI/Catalog/PermissionManagementWidget.h
#ifndef UI_CATALOG_PERMISSIONMANAGEMENTWIDGET_H
#define UI_CATALOG_PERMISSIONMANAGEMENTWIDGET_H
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
#include <QDialog> // For permission input dialog

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "PermissionService.h" // Dịch vụ quản lý quyền hạn
#include "ISecurityManager.h"  // Dịch vụ bảo mật
#include "Logger.h"            // Logging
#include "ErrorHandler.h"    // Xử lý lỗi
#include "Common.h"          // Các enum chung
#include "DateUtils.h"       // Xử lý ngày tháng
#include "StringUtils.h"     // Xử lý chuỗi
#include "CustomMessageBox.h" // Hộp thoại thông báo tùy chỉnh
#include "Permission.h"      // Permission DTO (for enums etc.)


namespace ERP {
namespace UI {
namespace Catalog {

/**
 * @brief PermissionManagementWidget class provides a UI for managing Permissions.
 * This widget allows viewing, creating, updating, deleting, and changing permission status.
 */
class PermissionManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for PermissionManagementWidget.
     * @param permissionService Shared pointer to IPermissionService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit PermissionManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IPermissionService> permissionService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~PermissionManagementWidget();

private slots:
    void loadPermissions();
    void onAddPermissionClicked();
    void onEditPermissionClicked();
    void onDeletePermissionClicked();
    void onUpdatePermissionStatusClicked();
    void onSearchPermissionClicked();
    void onPermissionTableItemClicked(int row, int column);
    void clearForm();

private:
    std::shared_ptr<Services::IPermissionService> permissionService_;
    std::shared_ptr<Security::ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *permissionTable_;
    QPushButton *addPermissionButton_;
    QPushButton *editPermissionButton_;
    QPushButton *deletePermissionButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;

    // Form elements for editing/adding
    QLineEdit *idLineEdit_;
    QLineEdit *nameLineEdit_;
    QLineEdit *moduleLineEdit_;
    QLineEdit *actionLineEdit_;
    QLineEdit *descriptionLineEdit_;
    QComboBox *statusComboBox_; // For status selection

    // Helper functions
    void setupUI();
    void showPermissionInputDialog(ERP::Catalog::DTO::PermissionDTO* permission = nullptr);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState(); // Control button enable/disable based on selection and permissions
    
    // Permission checking helper
    bool hasPermission(const std::string& permission);
};

} // namespace Catalog
} // namespace UI
} // namespace ERP

#endif // UI_CATALOG_PERMISSIONMANAGEMENTWIDGET_H