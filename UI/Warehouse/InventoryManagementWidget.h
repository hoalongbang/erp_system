// UI/Warehouse/InventoryManagementWidget.h
#ifndef UI_WAREHOUSE_INVENTORYMANAGEMENTWIDGET_H
#define UI_WAREHOUSE_INVENTORYMANAGEMENTWIDGET_H
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
#include <QDialog>
#include <QDateTimeEdit>
#include <QDoubleValidator>

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "InventoryManagementService.h" // Dịch vụ quản lý tồn kho
#include "ProductService.h"             // Dịch vụ sản phẩm
#include "WarehouseService.h"           // Dịch vụ kho hàng
#include "ISecurityManager.h"           // Dịch vụ bảo mật
#include "Logger.h"                     // Logging
#include "ErrorHandler.h"               // Xử lý lỗi
#include "Common.h"                     // Các enum chung
#include "DateUtils.h"                  // Xử lý ngày tháng
#include "StringUtils.h"                // Xử lý chuỗi
#include "CustomMessageBox.h"           // Hộp thoại thông báo tùy chỉnh
#include "Inventory.h"                  // Inventory DTO
#include "InventoryTransaction.h"       // InventoryTransaction DTO
#include "Product.h"                    // Product DTO (for display)
#include "Warehouse.h"                  // Warehouse DTO (for display)
#include "Location.h"                   // Location DTO (for display)


namespace ERP {
namespace UI {
namespace Warehouse {

/**
 * @brief InventoryManagementWidget class provides a UI for managing Inventory.
 * This widget allows viewing inventory levels, recording goods movements (receipt, issue, adjustment, transfer).
 */
class InventoryManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for InventoryManagementWidget.
     * @param inventoryService Shared pointer to IInventoryManagementService.
     * @param productService Shared pointer to IProductService.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit InventoryManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IInventoryManagementService> inventoryService = nullptr,
        std::shared_ptr<Product::Services::IProductService> productService = nullptr,
        std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~InventoryManagementWidget();

private slots:
    void loadInventory();
    void onRecordGoodsReceiptClicked();
    void onRecordGoodsIssueClicked();
    void onAdjustInventoryClicked();
    void onTransferStockClicked();
    void onSearchInventoryClicked();
    void onInventoryTableItemClicked(int row, int column);
    void clearForm();

private:
    std::shared_ptr<Services::IInventoryManagementService> inventoryService_;
    std::shared_ptr<Product::Services::IProductService> productService_;
    std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *inventoryTable_;
    QPushButton *recordGoodsReceiptButton_;
    QPushButton *recordGoodsIssueButton_;
    QPushButton *adjustInventoryButton_;
    QPushButton *transferStockButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;

    // Form elements for displaying inventory details (read-only)
    QLineEdit *idLineEdit_;
    QLineEdit *productIdLineEdit_;
    QLineEdit *productNameLineEdit_; // Display name
    QLineEdit *warehouseIdLineEdit_;
    QLineEdit *warehouseNameLineEdit_; // Display name
    QLineEdit *locationIdLineEdit_;
    QLineEdit *locationNameLineEdit_; // Display name
    QLineEdit *quantityLineEdit_;
    QLineEdit *reservedQuantityLineEdit_;
    QLineEdit *availableQuantityLineEdit_;
    QLineEdit *unitCostLineEdit_;
    QLineEdit *lotNumberLineEdit_;
    QLineEdit *serialNumberLineEdit_;
    QDateTimeEdit *manufactureDateEdit_;
    QDateTimeEdit *expirationDateEdit_;
    QLineEdit *reorderLevelLineEdit_;
    QLineEdit *reorderQuantityLineEdit_;

    // Helper functions
    void setupUI();
    void populateProductComboBox(QComboBox* comboBox);
    void populateWarehouseComboBox(QComboBox* comboBox);
    void populateLocationComboBox(QComboBox* comboBox, const std::string& warehouseId = "");
    void showGoodsMovementDialog(ERP::Warehouse::DTO::InventoryTransactionType type);
    void showTransferStockDialog();
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Warehouse
} // namespace UI
} // namespace ERP

#endif // UI_WAREHOUSE_INVENTORYMANAGEMENTWIDGET_H