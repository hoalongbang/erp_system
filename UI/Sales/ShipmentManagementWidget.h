// UI/Sales/ShipmentManagementWidget.h
#ifndef UI_SALES_SHIPMENTMANAGEMENTWIDGET_H
#define UI_SALES_SHIPMENTMANAGEMENTWIDGET_H
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
#include "ShipmentService.h"        // Dịch vụ vận chuyển
#include "SalesOrderService.h"      // Dịch vụ đơn hàng bán (cho validation)
#include "CustomerService.h"        // Dịch vụ khách hàng (cho validation/display)
#include "WarehouseService.h"       // Dịch vụ kho hàng (cho details)
#include "ProductService.h"         // Dịch vụ sản phẩm (cho details)
#include "ISecurityManager.h"       // Dịch vụ bảo mật
#include "Logger.h"                 // Logging
#include "ErrorHandler.h"           // Xử lý lỗi
#include "Common.h"                 // Các enum chung
#include "DateUtils.h"              // Xử lý ngày tháng
#include "StringUtils.h"            // Xử lý chuỗi
#include "CustomMessageBox.h"       // Hộp thoại thông báo tùy chỉnh
#include "Shipment.h"               // Shipment DTO
#include "ShipmentDetail.h"         // ShipmentDetail DTO
#include "SalesOrder.h"             // SalesOrder DTO (for display)
#include "Customer.h"               // Customer DTO (for display)
#include "Product.h"                // Product DTO (for display)
#include "Warehouse.h"              // Warehouse DTO (for display)
#include "Location.h"               // Location DTO (for display)


namespace ERP {
namespace UI {
namespace Sales {

/**
 * @brief ShipmentManagementWidget class provides a UI for managing Sales Shipments.
 * This widget allows viewing, creating, updating, deleting, and changing shipment status.
 * It also supports managing shipment details.
 */
class ShipmentManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for ShipmentManagementWidget.
     * @param shipmentService Shared pointer to IShipmentService.
     * @param salesOrderService Shared pointer to ISalesOrderService.
     * @param customerService Shared pointer to ICustomerService.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param productService Shared pointer to IProductService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit ShipmentManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IShipmentService> shipmentService = nullptr,
        std::shared_ptr<Services::ISalesOrderService> salesOrderService = nullptr,
        std::shared_ptr<Customer::Services::ICustomerService> customerService = nullptr,
        std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService = nullptr,
        std::shared_ptr<Product::Services::IProductService> productService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~ShipmentManagementWidget();

private slots:
    void loadShipments();
    void onAddShipmentClicked();
    void onEditShipmentClicked();
    void onDeleteShipmentClicked();
    void onUpdateShipmentStatusClicked();
    void onSearchShipmentClicked();
    void onShipmentTableItemClicked(int row, int column);
    void clearForm();

    void onManageDetailsClicked(); // New slot for managing shipment details

private:
    std::shared_ptr<Services::IShipmentService> shipmentService_;
    std::shared_ptr<Services::ISalesOrderService> salesOrderService_;
    std::shared_ptr<Customer::Services::ICustomerService> customerService_;
    std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService_;
    std::shared_ptr<Product::Services::IProductService> productService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *shipmentTable_;
    QPushButton *addShipmentButton_;
    QPushButton *editShipmentButton_;
    QPushButton *deleteShipmentButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;
    QPushButton *manageDetailsButton_;

    // Form elements for editing/adding shipments
    QLineEdit *idLineEdit_;
    QLineEdit *shipmentNumberLineEdit_;
    QComboBox *salesOrderComboBox_; // Sales Order ID, display number
    QComboBox *customerComboBox_; // Customer ID, display name
    QLineEdit *shippedByLineEdit_; // User ID, display name
    QDateTimeEdit *shipmentDateEdit_;
    QDateTimeEdit *deliveryDateEdit_;
    QComboBox *typeComboBox_; // ShipmentType
    QComboBox *statusComboBox_; // ShipmentStatus
    QLineEdit *carrierNameLineEdit_;
    QLineEdit *trackingNumberLineEdit_;
    QLineEdit *deliveryAddressLineEdit_;
    QLineEdit *notesLineEdit_;

    // Helper functions
    void setupUI();
    void populateSalesOrderComboBox();
    void populateCustomerComboBox();
    void populateTypeComboBox();
    void populateStatusComboBox(); // For shipment status
    void populateWarehouseComboBox(); // For details
    void populateLocationComboBox(); // For details
    void showShipmentInputDialog(ERP::Sales::DTO::ShipmentDTO* shipment = nullptr);
    void showManageDetailsDialog(ERP::Sales::DTO::ShipmentDTO* shipment);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Sales
} // namespace UI
} // namespace ERP

#endif // UI_SALES_SHIPMENTMANAGEMENTWIDGET_H