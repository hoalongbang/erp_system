// UI/Catalog/WarehouseManagementWidget.h
#ifndef UI_CATALOG_WAREHOUSEMANAGEMENTWIDGET_H
#define UI_CATALOG_WAREHOUSEMANAGEMENTWIDGET_H
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
#include <QDialog> // For warehouse input dialog

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "WarehouseService.h"  // Dịch vụ quản lý kho hàng
#include "LocationService.h"   // Dịch vụ quản lý vị trí kho
#include "ISecurityManager.h"  // Dịch vụ bảo mật
#include "Logger.h"            // Logging
#include "ErrorHandler.h"    // Xử lý lỗi
#include "Common.h"          // Các enum chung
#include "DateUtils.h"       // Xử lý ngày tháng
#include "StringUtils.h"     // Xử lý chuỗi
#include "CustomMessageBox.h" // Hộp thoại thông báo tùy chỉnh
#include "Warehouse.h"       // Warehouse DTO (for enums etc.)


namespace ERP {
namespace UI {
namespace Catalog {

/**
 * @brief WarehouseManagementWidget class provides a UI for managing Warehouses.
 * This widget allows viewing, creating, updating, deleting, and changing warehouse status.
 */
class WarehouseManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for WarehouseManagementWidget.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param locationService Shared pointer to ILocationService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit WarehouseManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IWarehouseService> warehouseService = nullptr,
        std::shared_ptr<Services::ILocationService> locationService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~WarehouseManagementWidget();

private slots:
    void loadWarehouses();
    void onAddWarehouseClicked();
    void onEditWarehouseClicked();
    void onDeleteWarehouseClicked();
    void onUpdateWarehouseStatusClicked();
    void onSearchWarehouseClicked();
    void onWarehouseTableItemClicked(int row, int column);
    void clearForm();

private:
    std::shared_ptr<Services::IWarehouseService> warehouseService_;
    std::shared_ptr<Services::ILocationService> locationService_;
    std::shared_ptr<Security::ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *warehouseTable_;
    QPushButton *addWarehouseButton_;
    QPushButton *editWarehouseButton_;
    QPushButton *deleteWarehouseButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;

    // Form elements for editing/adding
    QLineEdit *idLineEdit_;
    QLineEdit *nameLineEdit_;
    QLineEdit *locationLineEdit_; // Physical location description
    QLineEdit *contactPersonLineEdit_;
    QLineEdit *contactPhoneLineEdit_;
    QLineEdit *emailLineEdit_;
    QComboBox *statusComboBox_; // For status selection

    // Helper functions
    void setupUI();
    void showWarehouseInputDialog(ERP::Catalog::DTO::WarehouseDTO* warehouse = nullptr);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState(); // Control button enable/disable based on selection and permissions
    
    // Permission checking helper
    bool hasPermission(const std::string& permission);
};

} // namespace Catalog
} // namespace UI
} // namespace ERP

#endif // UI_CATALOG_WAREHOUSEMANAGEMENTWIDGET_H