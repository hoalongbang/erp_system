// UI/Material/MaterialIssueSlipManagementWidget.h
#ifndef UI_MATERIAL_MATERIALISSUESLIPMANAGEMENTWIDGET_H
#define UI_MATERIAL_MATERIALISSUESLIPMANAGEMENTWIDGET_H
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
#include "MaterialIssueSlipService.h" // Dịch vụ phiếu xuất vật tư sản xuất
#include "ProductionOrderService.h"   // Dịch vụ lệnh sản xuất
#include "ProductService.h"           // Dịch vụ sản phẩm
#include "WarehouseService.h"         // Dịch vụ kho hàng
#include "InventoryManagementService.h" // Dịch vụ quản lý tồn kho
#include "ISecurityManager.h"         // Dịch vụ bảo mật
#include "Logger.h"                   // Logging
#include "ErrorHandler.h"             // Xử lý lỗi
#include "Common.h"                   // Các enum chung
#include "DateUtils.h"                // Xử lý ngày tháng
#include "StringUtils.h"              // Xử lý chuỗi
#include "CustomMessageBox.h"         // Hộp thoại thông báo tùy chỉnh
#include "MaterialIssueSlip.h"        // MaterialIssueSlip DTO
#include "MaterialIssueSlipDetail.h"  // MaterialIssueSlipDetail DTO
#include "ProductionOrder.h"          // ProductionOrder DTO (for display)
#include "Product.h"                  // Product DTO (for display)
#include "Warehouse.h"                // Warehouse DTO (for display)

namespace ERP {
namespace UI {
namespace Material {

/**
 * @brief MaterialIssueSlipManagementWidget class provides a UI for managing Material Issue Slips for Manufacturing.
 * This widget allows viewing, creating, updating, deleting, and changing slip status.
 * It also supports managing slip details and recording issued quantities for manufacturing.
 */
class MaterialIssueSlipManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for MaterialIssueSlipManagementWidget.
     * @param materialIssueSlipService Shared pointer to IMaterialIssueSlipService.
     * @param productionOrderService Shared pointer to IProductionOrderService.
     * @param productService Shared pointer to IProductService.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param inventoryManagementService Shared pointer to IInventoryManagementService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit MaterialIssueSlipManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IMaterialIssueSlipService> materialIssueSlipService = nullptr,
        std::shared_ptr<Manufacturing::Services::IProductionOrderService> productionOrderService = nullptr,
        std::shared_ptr<Product::Services::IProductService> productService = nullptr,
        std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService = nullptr,
        std::shared_ptr<Warehouse::Services::IInventoryManagementService> inventoryManagementService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~MaterialIssueSlipManagementWidget();

private slots:
    void loadMaterialIssueSlips();
    void onAddSlipClicked();
    void onEditSlipClicked();
    void onDeleteSlipClicked();
    void onUpdateSlipStatusClicked();
    void onSearchSlipClicked();
    void onSlipTableItemClicked(int row, int column);
    void clearForm();

    void onManageDetailsClicked(); // New slot for managing slip details
    void onRecordIssuedQuantityClicked(); // New slot for recording quantities

private:
    std::shared_ptr<Services::IMaterialIssueSlipService> materialIssueSlipService_;
    std::shared_ptr<Manufacturing::Services::IProductionOrderService> productionOrderService_;
    std::shared_ptr<Product::Services::IProductService> productService_;
    std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService_;
    std::shared_ptr<Warehouse::Services::IInventoryManagementService> inventoryManagementService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *slipTable_;
    QPushButton *addSlipButton_;
    QPushButton *editSlipButton_;
    QPushButton *deleteSlipButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;
    QPushButton *manageDetailsButton_;
    QPushButton *recordIssuedQuantityButton_;

    // Form elements for editing/adding slips
    QLineEdit *idLineEdit_;
    QLineEdit *issueNumberLineEdit_;
    QComboBox *productionOrderComboBox_;
    QComboBox *warehouseComboBox_;
    QLineEdit *issuedByLineEdit_; // User ID, display name
    QDateTimeEdit *issueDateEdit_;
    QComboBox *statusComboBox_; // Slip Status
    QLineEdit *notesLineEdit_;

    // Helper functions
    void setupUI();
    void populateProductionOrderComboBox();
    void populateWarehouseComboBox();
    void populateStatusComboBox(); // For material issue slip status
    void showSlipInputDialog(ERP::Material::DTO::MaterialIssueSlipDTO* slip = nullptr);
    void showManageDetailsDialog(ERP::Material::DTO::MaterialIssueSlipDTO* slip);
    void showRecordIssuedQuantityDialog(ERP::Material::DTO::MaterialIssueSlipDetailDTO* detail, ERP::Material::DTO::MaterialIssueSlipDTO* parentSlip);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Material
} // namespace UI
} // namespace ERP

#endif // UI_MATERIAL_MATERIALISSUESLIPMANAGEMENTWIDGET_H