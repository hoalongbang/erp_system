// UI/Warehouse/PickingRequestManagementWidget.h
#ifndef UI_WAREHOUSE_PICKINGREQUESTMANAGEMENTWIDGET_H
#define UI_WAREHOUSE_PICKINGREQUESTMANAGEMENTWIDGET_H
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
#include "PickingService.h"             // Dịch vụ lấy hàng
#include "SalesOrderService.h"          // Dịch vụ đơn hàng bán
#include "InventoryManagementService.h" // Dịch vụ quản lý tồn kho
#include "ProductService.h"             // Dịch vụ sản phẩm
#include "WarehouseService.h"           // Dịch vụ kho hàng
#include "ISecurityManager.h"           // Dịch vụ bảo mật
#include "Logger.h"                     // Logging
#include "ErrorHandler.h"               // Xử lý lỗi
#include "Common.h"                     // Các enum chung
#include "DateUtils.h"                  // Xử lý ngày tháng
#include "StringUtils.h"                // Xử lý chuỗi
##include "CustomMessageBox.h"           // Hộp thoại thông báo tùy chỉnh
#include "PickingRequest.h"             // PickingRequest DTO
#include "PickingDetail.h"              // PickingDetail DTO
#include "SalesOrder.h"                 // SalesOrder DTO (for display)
#include "Product.h"                    // Product DTO (for display)
#include "Warehouse.h"                  // Warehouse DTO (for display)
#include "Location.h"                   // Location DTO (for display)


namespace ERP {
namespace UI {
namespace Warehouse {

/**
 * @brief PickingRequestManagementWidget class provides a UI for managing Picking Requests.
 * This widget allows viewing, creating, updating, deleting, and changing request status.
 * It also supports managing picking details and recording picked quantities.
 */
class PickingRequestManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for PickingRequestManagementWidget.
     * @param pickingService Shared pointer to IPickingService.
     * @param salesOrderService Shared pointer to ISalesOrderService.
     * @param inventoryManagementService Shared pointer to IInventoryManagementService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit PickingRequestManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IPickingService> pickingService = nullptr,
        std::shared_ptr<Sales::Services::ISalesOrderService> salesOrderService = nullptr,
        std::shared_ptr<Warehouse::Services::IInventoryManagementService> inventoryManagementService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~PickingRequestManagementWidget();

private slots:
    void loadPickingRequests();
    void onAddRequestClicked();
    void onEditRequestClicked();
    void onDeleteRequestClicked();
    void onUpdateRequestStatusClicked();
    void onSearchRequestClicked();
    void onRequestTableItemClicked(int row, int column);
    void clearForm();

    void onManageDetailsClicked(); // New slot for managing picking details
    void onRecordPickedQuantityClicked(); // New slot for recording quantities

private:
    std::shared_ptr<Services::IPickingService> pickingService_;
    std::shared_ptr<Sales::Services::ISalesOrderService> salesOrderService_;
    std::shared_ptr<Warehouse::Services::IInventoryManagementService> inventoryManagementService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *requestTable_;
    QPushButton *addRequestButton_;
    QPushButton *editRequestButton_;
    QPushButton *deleteRequestButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;
    QPushButton *manageDetailsButton_;
    QPushButton *recordPickedQuantityButton_;

    // Form elements for editing/adding requests
    QLineEdit *idLineEdit_;
    QComboBox *salesOrderComboBox_; // Sales Order ID, display number
    QLineEdit *requestedByLineEdit_; // User ID, display name
    QLineEdit *pickedByLineEdit_; // User ID, display name
    QComboBox *statusComboBox_; // Request Status
    QDateTimeEdit *pickStartTimeEdit_;
    QDateTimeEdit *pickEndTimeEdit_;
    QLineEdit *notesLineEdit_;

    // Helper functions
    void setupUI();
    void populateSalesOrderComboBox();
    void populateStatusComboBox(); // For picking request status
    void populateUserComboBox(QComboBox* comboBox); // For requestedBy/pickedBy
    void populateProductComboBox(QComboBox* comboBox); // For details
    void populateWarehouseComboBox(QComboBox* comboBox); // For details
    void populateLocationComboBox(QComboBox* comboBox, const std::string& warehouseId = ""); // For details
    void showRequestInputDialog(ERP::Warehouse::DTO::PickingRequestDTO* request = nullptr);
    void showManageDetailsDialog(ERP::Warehouse::DTO::PickingRequestDTO* request);
    void showRecordPickedQuantityDialog(ERP::Warehouse::DTO::PickingDetailDTO* detail, ERP::Warehouse::DTO::PickingRequestDTO* parentRequest);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Warehouse
} // namespace UI
} // namespace ERP

#endif // UI_WAREHOUSE_PICKINGREQUESTMANAGEMENTWIDGET_H