// UI/Warehouse/StocktakeRequestManagementWidget.h
#ifndef UI_WAREHOUSE_STOCKTAKEREQUESTMANAGEMENTWIDGET_H
#define UI_WAREHOUSE_STOCKTAKEREQUESTMANAGEMENTWIDGET_H
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
#include "StocktakeService.h"           // Dịch vụ kiểm kê
#include "InventoryManagementService.h" // Dịch vụ quản lý tồn kho
#include "WarehouseService.h"           // Dịch vụ kho hàng
#include "ProductService.h"             // Dịch vụ sản phẩm
#include "ISecurityManager.h"           // Dịch vụ bảo mật
#include "Logger.h"                     // Logging
#include "ErrorHandler.h"               // Xử lý lỗi
#include "Common.h"                     // Các enum chung
#include "DateUtils.h"                  // Xử lý ngày tháng
#include "StringUtils.h"                // Xử lý chuỗi
#include "CustomMessageBox.h"           // Hộp thoại thông báo tùy chỉnh
#include "StocktakeRequest.h"           // StocktakeRequest DTO
#include "StocktakeDetail.h"            // StocktakeDetail DTO
#include "Warehouse.h"                  // Warehouse DTO (for display)
#include "Location.h"                   // Location DTO (for display)
#include "Product.h"                    // Product DTO (for display)


namespace ERP {
namespace UI {
namespace Warehouse {

/**
 * @brief StocktakeRequestManagementWidget class provides a UI for managing Stocktake Requests.
 * This widget allows viewing, creating, updating, deleting, and changing request status.
 * It also supports managing details, recording counted quantities, and reconciling stocktakes.
 */
class StocktakeRequestManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for StocktakeRequestManagementWidget.
     * @param stocktakeService Shared pointer to IStocktakeService.
     * @param inventoryManagementService Shared pointer to IInventoryManagementService.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit StocktakeRequestManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IStocktakeService> stocktakeService = nullptr,
        std::shared_ptr<Warehouse::Services::IInventoryManagementService> inventoryManagementService = nullptr,
        std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~StocktakeRequestManagementWidget();

private slots:
    void loadStocktakeRequests();
    void onAddRequestClicked();
    void onEditRequestClicked();
    void onDeleteRequestClicked();
    void onUpdateRequestStatusClicked();
    void onSearchRequestClicked();
    void onRequestTableItemClicked(int row, int column);
    void clearForm();

    void onManageDetailsClicked(); // New slot for managing details
    void onRecordCountedQuantityClicked(); // New slot for recording quantities
    void onReconcileStocktakeClicked(); // New slot for reconciling

private:
    std::shared_ptr<Services::IStocktakeService> stocktakeService_;
    std::shared_ptr<Warehouse::Services::IInventoryManagementService> inventoryManagementService_;
    std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService_;
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
    QPushButton *recordCountedQuantityButton_;
    QPushButton *reconcileStocktakeButton_;

    // Form elements for editing/adding requests
    QLineEdit *idLineEdit_;
    QComboBox *warehouseComboBox_;
    QComboBox *locationComboBox_; // Optional: specific location for stocktake
    QLineEdit *requestedByLineEdit_; // User ID, display name
    QLineEdit *countedByLineEdit_; // User ID, display name
    QDateTimeEdit *countDateEdit_;
    QComboBox *statusComboBox_; // Request Status
    QLineEdit *notesLineEdit_;

    // Helper functions
    void setupUI();
    void populateWarehouseComboBox();
    void populateLocationComboBox(const std::string& warehouseId = "");
    void populateUserComboBox(QComboBox* comboBox); // For requestedBy/countedBy
    void populateStatusComboBox(); // For stocktake request status
    void showRequestInputDialog(ERP::Warehouse::DTO::StocktakeRequestDTO* request = nullptr);
    void showManageDetailsDialog(ERP::Warehouse::DTO::StocktakeRequestDTO* request);
    void showRecordCountedQuantityDialog(ERP::Warehouse::DTO::StocktakeDetailDTO* detail, ERP::Warehouse::DTO::StocktakeRequestDTO* parentRequest);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Warehouse
} // namespace UI
} // namespace ERP

#endif // UI_WAREHOUSE_STOCKTAKEREQUESTMANAGEMENTWIDGET_H