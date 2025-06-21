// UI/Sales/SalesOrderManagementWidget.h
#ifndef UI_SALES_SALESORDERMANAGEMENTWIDGET_H
#define UI_SALES_SALESORDERMANAGEMENTWIDGET_H
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
#include "SalesOrderService.h"      // Dịch vụ đơn hàng bán
#include "CustomerService.h"        // Dịch vụ khách hàng
#include "WarehouseService.h"       // Dịch vụ kho hàng
#include "ProductService.h"         // Dịch vụ sản phẩm (cho details)
#include "ISecurityManager.h"       // Dịch vụ bảo mật
#include "Logger.h"                 // Logging
#include "ErrorHandler.h"           // Xử lý lỗi
#include "Common.h"                 // Các enum chung
#include "DateUtils.h"              // Xử lý ngày tháng
#include "StringUtils.h"            // Xử lý chuỗi
#include "CustomMessageBox.h"       // Hộp thoại thông báo tùy chỉnh
#include "SalesOrder.h"             // SalesOrder DTO
#include "SalesOrderDetail.h"       // SalesOrderDetail DTO
#include "Customer.h"               // Customer DTO (for display)
#include "Warehouse.h"              // Warehouse DTO (for display)
#include "Product.h"                // Product DTO (for display)


namespace ERP {
namespace UI {
namespace Sales {

/**
 * @brief SalesOrderManagementWidget class provides a UI for managing Sales Orders.
 * This widget allows viewing, creating, updating, deleting, and changing order status.
 * It also supports managing order details.
 */
class SalesOrderManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for SalesOrderManagementWidget.
     * @param salesOrderService Shared pointer to ISalesOrderService.
     * @param customerService Shared pointer to ICustomerService.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param productService Shared pointer to IProductService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit SalesOrderManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::ISalesOrderService> salesOrderService = nullptr,
        std::shared_ptr<Customer::Services::ICustomerService> customerService = nullptr,
        std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService = nullptr,
        std::shared_ptr<Product::Services::IProductService> productService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~SalesOrderManagementWidget();

private slots:
    void loadSalesOrders();
    void onAddOrderClicked();
    void onEditOrderClicked();
    void onDeleteOrderClicked();
    void onUpdateOrderStatusClicked();
    void onSearchOrderClicked();
    void onOrderTableItemClicked(int row, int column);
    void clearForm();

    void onManageDetailsClicked(); // New slot for managing order details

private:
    std::shared_ptr<Services::ISalesOrderService> salesOrderService_;
    std::shared_ptr<Customer::Services::ICustomerService> customerService_;
    std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService_;
    std::shared_ptr<Product::Services::IProductService> productService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *orderTable_;
    QPushButton *addOrderButton_;
    QPushButton *editOrderButton_;
    QPushButton *deleteOrderButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;
    QPushButton *manageDetailsButton_;

    // Form elements for editing/adding orders
    QLineEdit *idLineEdit_;
    QLineEdit *orderNumberLineEdit_;
    QComboBox *customerComboBox_; // Customer ID, display name
    QLineEdit *requestedByLineEdit_; // User ID, display name
    QLineEdit *approvedByLineEdit_; // User ID, display name
    QDateTimeEdit *orderDateEdit_;
    QDateTimeEdit *requiredDeliveryDateEdit_;
    QComboBox *statusComboBox_; // SalesOrderStatus
    QLineEdit *totalAmountLineEdit_; // Calculated
    QLineEdit *totalDiscountLineEdit_; // Calculated
    QLineEdit *totalTaxLineEdit_; // Calculated
    QLineEdit *netAmountLineEdit_; // Calculated
    QLineEdit *amountPaidLineEdit_; // Updated by payments
    QLineEdit *amountDueLineEdit_; // Calculated
    QLineEdit *currencyLineEdit_;
    QLineEdit *paymentTermsLineEdit_;
    QLineEdit *deliveryAddressLineEdit_;
    QLineEdit *notesLineEdit_;
    QComboBox *warehouseComboBox_; // Default warehouse for the order
    QLineEdit *quotationIdLineEdit_; // Link to Quotation, read-only

    // Helper functions
    void setupUI();
    void populateCustomerComboBox();
    void populateWarehouseComboBox();
    void populateStatusComboBox(); // For sales order status
    void populateUserComboBox(QComboBox* comboBox); // Helper for requestedBy/approvedBy
    void showOrderInputDialog(ERP::Sales::DTO::SalesOrderDTO* order = nullptr);
    void showManageDetailsDialog(ERP::Sales::DTO::SalesOrderDTO* order);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Sales
} // namespace UI
} // namespace ERP

#endif // UI_SALES_SALESORDERMANAGEMENTWIDGET_H