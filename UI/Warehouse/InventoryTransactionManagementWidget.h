// UI/Warehouse/InventoryTransactionManagementWidget.h
#ifndef UI_WAREHOUSE_INVENTORYTRANSACTIONMANAGEMENTWIDGET_H
#define UI_WAREHOUSE_INVENTORYTRANSACTIONMANAGEMENTWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "InventoryTransactionService.h" // Dịch vụ giao dịch tồn kho
#include "ProductService.h"            // Dịch vụ sản phẩm
#include "WarehouseService.h"          // Dịch vụ kho hàng
#include "LocationService.h"           // Dịch vụ vị trí
#include "ISecurityManager.h"          // Dịch vụ bảo mật
#include "Logger.h"                    // Logging
#include "ErrorHandler.h"              // Xử lý lỗi
#include "Common.h"                    // Các enum chung
#include "DateUtils.h"                 // Xử lý ngày tháng
#include "StringUtils.h"               // Xử lý chuỗi
#include "CustomMessageBox.h"          // Hộp thoại thông báo tùy chỉnh
#include "InventoryTransaction.h"      // InventoryTransaction DTO
#include "Product.h"                   // Product DTO
#include "Warehouse.h"                 // Warehouse DTO
#include "Location.h"                  // Location DTO
#include "UserService.h"               // For getting user roles

namespace ERP {
namespace UI {
namespace Warehouse {

/**
 * @brief InventoryTransactionManagementWidget class provides a UI for viewing Inventory Transactions.
 * This widget allows filtering and displaying various inventory movements.
 */
class InventoryTransactionManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for InventoryTransactionManagementWidget.
     * @param parent Parent widget.
     * @param inventoryTransactionService Shared pointer to IInventoryTransactionService.
     * @param productService Shared pointer to IProductService.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param locationService Shared pointer to ILocationService.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    explicit InventoryTransactionManagementWidget(
        QWidget* parent = nullptr,
        std::shared_ptr<ERP::Warehouse::Services::IInventoryTransactionService> inventoryTransactionService = nullptr,
        std::shared_ptr<ERP::Product::Services::IProductService> productService = nullptr,
        std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService = nullptr,
        std::shared_ptr<ERP::Catalog::Services::ILocationService> locationService = nullptr,
        std::shared_ptr<ERP::Security::ISecurityManager> securityManager = nullptr);

private slots:
    /**
     * @brief Slot to handle loading inventory transactions based on filters.
     */
    void loadInventoryTransactions();

private:
    std::shared_ptr<ERP::Warehouse::Services::IInventoryTransactionService> inventoryTransactionService_;
    std::shared_ptr<ERP::Product::Services::IProductService> productService_;
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService_;
    std::shared_ptr<ERP::Catalog::Services::ILocationService> locationService_;
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager_;
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    // UI Elements
    QTableWidget* transactionsTable_;

    QLineEdit* filterProductNameInput_;
    QLineEdit* filterLotNumberInput_;    // NEW: Added Lot/Serial Filters
    QLineEdit* filterSerialNumberInput_; // NEW
    QComboBox* filterTransactionTypeComboBox_;
    QLineEdit* filterReferenceDocumentIdInput_;
    QComboBox* filterWarehouseComboBox_;
    QComboBox* filterLocationComboBox_;
    QDateTimeEdit* filterStartDateEdit_;
    QDateTimeEdit* filterEndDateEdit_;
    QPushButton* loadTransactionsButton_;

    // Helper functions
    void setupUI();
    void populateWarehouseFilterComboBox();
    void populateLocationFilterComboBox(const QString& warehouseId = "");
    void populateTransactionTypeFilterComboBox();
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
};

} // namespace Warehouse
} // namespace UI
} // namespace ERP

#endif // UI_WAREHOUSE_INVENTORYTRANSACTIONMANAGEMENTWIDGET_H