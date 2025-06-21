// UI/Sales/ReturnManagementWidget.h
#ifndef UI_SALES_RETURNMANAGEMENTWIDGET_H
#define UI_SALES_RETURNMANAGEMENTWIDGET_H

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
#include <QDialog> // For input dialogs

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>
#include <chrono>

// Rút gọn các include paths
#include "ReturnService.h"        // Dịch vụ trả hàng
#include "SalesOrderService.h"    // Dịch vụ đơn hàng bán
#include "CustomerService.h"      // Dịch vụ khách hàng
#include "WarehouseService.h"     // Dịch vụ kho hàng
#include "ProductService.h"       // Dịch vụ sản phẩm
#include "InventoryManagementService.h" // Dịch vụ quản lý tồn kho
#include "ISecurityManager.h"     // Dịch vụ bảo mật
#include "Logger.h"               // Logging
#include "ErrorHandler.h"         // Xử lý lỗi
#include "Common.h"               // Các enum chung
#include "DateUtils.h"            // Xử lý ngày tháng
#include "StringUtils.h"          // Xử lý chuỗi
#include "CustomMessageBox.h"     // Hộp thoại thông báo tùy chỉnh
#include "Return.h"               // Return DTO
#include "ReturnDetail.h"         // ReturnDetail DTO
#include "SalesOrder.h"           // SalesOrder DTO
#include "Customer.h"             // Customer DTO
#include "Warehouse.h"            // Warehouse DTO
#include "Product.h"              // Product DTO
#include "UnitOfMeasure.h"        // UnitOfMeasure DTO (dependency of Product)
#include "UserService.h"          // For getting user roles

namespace ERP {
namespace UI {
namespace Sales {

/**
 * @brief ReturnManagementWidget class provides a UI for managing Sales Returns.
 * This widget allows viewing, creating, updating, deleting, and changing return status.
 */
class ReturnManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for ReturnManagementWidget.
     * @param parent Parent widget.
     * @param returnService Shared pointer to IReturnService.
     * @param salesOrderService Shared pointer to ISalesOrderService.
     * @param customerService Shared pointer to ICustomerService.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param productService Shared pointer to IProductService.
     * @param inventoryManagementService Shared pointer to IInventoryManagementService.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    explicit ReturnManagementWidget(
        QWidget* parent = nullptr,
        std::shared_ptr<Services::IReturnService> returnService = nullptr,
        std::shared_ptr<Services::ISalesOrderService> salesOrderService = nullptr,
        std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService = nullptr,
        std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService = nullptr,
        std::shared_ptr<ERP::Product::Services::IProductService> productService = nullptr,
        std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~ReturnManagementWidget();

private slots:
    void loadReturns();
    void onAddReturnClicked();
    void onEditReturnClicked();
    void onDeleteReturnClicked();
    void onUpdateReturnStatusClicked();
    void onSearchReturnClicked();
    void onReturnTableItemClicked(int row, int column);
    void clearForm();
    void onAddReturnDetailClicked(); // To add a new detail line in dialog
    void onRemoveReturnDetailClicked(); // To remove a detail line in dialog

private:
    std::shared_ptr<Services::IReturnService> returnService_;
    std::shared_ptr<Services::ISalesOrderService> salesOrderService_;
    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService_;
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService_;
    std::shared_ptr<ERP::Product::Services::IProductService> productService_;
    std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService_;
    std::shared_ptr<Security::ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget* returnTable_;
    QPushButton* addReturnButton_;
    QPushButton* editReturnButton_;
    QPushButton* deleteReturnButton_;
    QPushButton* updateStatusButton_;
    QPushButton* searchButton_;
    QLineEdit* searchLineEdit_;
    QPushButton* clearFormButton_;

    // Form elements for editing/adding
    QLineEdit* idLineEdit_;
    QLineEdit* returnNumberLineEdit_;
    QComboBox* salesOrderComboBox_; // For selecting sales order
    QComboBox* customerComboBox_;   // For selecting customer
    QDateTimeEdit* returnDateEdit_;
    QLineEdit* reasonLineEdit_;
    QLineEdit* totalAmountLineEdit_; // Display only, calculated from details
    QComboBox* statusComboBox_;      // ReturnStatus
    QComboBox* warehouseComboBox_;   // For selecting warehouse for return
    QLineEdit* notesLineEdit_;

    // Nested table for return details
    QTableWidget* detailsTable_;
    QPushButton* addDetailButton_;
    QPushButton* removeDetailButton_;

    // Helper functions
    void setupUI();
    void populateSalesOrderComboBox(QComboBox* comboBox);
    void populateCustomerComboBox(QComboBox* comboBox);
    void populateWarehouseComboBox(QComboBox* comboBox);
    void populateStatusComboBox();
    void showReturnInputDialog(ERP::Sales::DTO::ReturnDTO* returnObj = nullptr);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState(); // Control button enable/disable based on selection and permissions
    void updateDetailTable(const std::vector<ERP::Sales::DTO::ReturnDetailDTO>& details); // Update the nested details table

    // Permission checking helper
    bool hasPermission(const std::string& permission);
};

} // namespace Sales
} // namespace UI
} // namespace ERP

#endif // UI_SALES_RETURNMANAGEMENTWIDGET_H