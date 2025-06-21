// UI/Manufacturing/ProductionOrderManagementWidget.h
#ifndef UI_MANUFACTURING_PRODUCTIONORDERMANAGEMENTWIDGET_H
#define UI_MANUFACTURING_PRODUCTIONORDERMANAGEMENTWIDGET_H
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
#include "ProductionOrderService.h" // Dịch vụ lệnh sản xuất
#include "ProductService.h"         // Dịch vụ sản phẩm
#include "BillOfMaterialService.h"  // Dịch vụ BOM
#include "ProductionLineService.h"  // Dịch vụ dây chuyền sản xuất
#include "ISecurityManager.h"       // Dịch vụ bảo mật
#include "Logger.h"                 // Logging
#include "ErrorHandler.h"           // Xử lý lỗi
#include "Common.h"                 // Các enum chung
#include "DateUtils.h"              // Xử lý ngày tháng
#include "StringUtils.h"            // Xử lý chuỗi
#include "CustomMessageBox.h"       // Hộp thoại thông báo tùy chỉnh
#include "ProductionOrder.h"        // ProductionOrder DTO
#include "Product.h"                // Product DTO
#include "BillOfMaterial.h"         // BOM DTO
#include "ProductionLine.h"         // ProductionLine DTO

namespace ERP {
namespace UI {
namespace Manufacturing {

/**
 * @brief ProductionOrderManagementWidget class provides a UI for managing Production Orders.
 * This widget allows viewing, creating, updating, deleting, and changing order status.
 * It also supports recording actual produced quantity.
 */
class ProductionOrderManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for ProductionOrderManagementWidget.
     * @param productionOrderService Shared pointer to IProductionOrderService.
     * @param productService Shared pointer to IProductService.
     * @param bomService Shared pointer to IBillOfMaterialService.
     * @param productionLineService Shared pointer to IProductionLineService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit ProductionOrderManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IProductionOrderService> productionOrderService = nullptr,
        std::shared_ptr<Product::Services::IProductService> productService = nullptr,
        std::shared_ptr<Services::IBillOfMaterialService> bomService = nullptr,
        std::shared_ptr<Services::IProductionLineService> productionLineService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~ProductionOrderManagementWidget();

private slots:
    void loadProductionOrders();
    void onAddOrderClicked();
    void onEditOrderClicked();
    void onDeleteOrderClicked();
    void onUpdateOrderStatusClicked();
    void onRecordActualQuantityClicked(); // New slot for recording actual quantity
    void onSearchOrderClicked();
    void onOrderTableItemClicked(int row, int column);
    void clearForm();

private:
    std::shared_ptr<Services::IProductionOrderService> productionOrderService_;
    std::shared_ptr<Product::Services::IProductService> productService_;
    std::shared_ptr<Services::IBillOfMaterialService> bomService_;
    std::shared_ptr<Services::IProductionLineService> productionLineService_;
    std::shared_ptr<Security::ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *orderTable_;
    QPushButton *addOrderButton_;
    QPushButton *editOrderButton_;
    QPushButton *deleteOrderButton_;
    QPushButton *updateStatusButton_;
    QPushButton *recordActualQuantityButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;

    // Form elements for editing/adding orders
    QLineEdit *idLineEdit_;
    QLineEdit *orderNumberLineEdit_;
    QComboBox *productComboBox_; // Product to be manufactured
    QLineEdit *plannedQuantityLineEdit_;
    QComboBox *unitOfMeasureComboBox_; // Unit for planned quantity
    QComboBox *statusComboBox_; // Order Status
    QComboBox *bomComboBox_; // Bill of Material
    QComboBox *productionLineComboBox_; // Production Line
    QDateTimeEdit *plannedStartDateEdit_;
    QDateTimeEdit *plannedEndDateEdit_;
    QDateTimeEdit *actualStartDateEdit_;
    QDateTimeEdit *actualEndDateEdit_;
    QLineEdit *actualQuantityProducedLineEdit_; // Read-only or updated via specific action
    QLineEdit *notesLineEdit_;

    // Helper functions
    void setupUI();
    void populateProductComboBox();
    void populateUnitOfMeasureComboBox();
    void populateStatusComboBox(); // For order status
    void populateBOMComboBox();
    void populateProductionLineComboBox();
    void showOrderInputDialog(ERP::Manufacturing::DTO::ProductionOrderDTO* order = nullptr);
    void showRecordActualQuantityDialog(ERP::Manufacturing::DTO::ProductionOrderDTO* order);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Manufacturing
} // namespace UI
} // namespace ERP

#endif // UI_MANUFACTURING_PRODUCTIONORDERMANAGEMENTWIDGET_H