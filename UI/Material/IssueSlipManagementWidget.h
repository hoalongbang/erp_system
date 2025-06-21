// UI/Material/IssueSlipManagementWidget.h
#ifndef UI_MATERIAL_ISSUESLIPMANAGEMENTWIDGET_H
#define UI_MATERIAL_ISSUESLIPMANAGEMENTWIDGET_H
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
#include "IssueSlipService.h"           // Dịch vụ phiếu xuất kho
#include "ProductService.h"             // Dịch vụ sản phẩm
#include "WarehouseService.h"           // Dịch vụ kho hàng
#include "InventoryManagementService.h" // Dịch vụ quản lý tồn kho
#include "MaterialRequestService.h"     // Dịch vụ yêu cầu vật tư (cho liên kết)
#include "ISecurityManager.h"           // Dịch vụ bảo mật
#include "Logger.h"                     // Logging
#include "ErrorHandler.h"               // Xử lý lỗi
#include "Common.h"                     // Các enum chung
#include "DateUtils.h"                  // Xử lý ngày tháng
#include "StringUtils.h"                // Xử lý chuỗi
#include "CustomMessageBox.h"           // Hộp thoại thông báo tùy chỉnh
#include "IssueSlip.h"                  // IssueSlip DTO
#include "IssueSlipDetail.h"            // IssueSlipDetail DTO
#include "Product.h"                    // Product DTO (for display)
#include "Warehouse.h"                  // Warehouse DTO (for display)
#include "Location.h"                   // Location DTO (for display)
#include "MaterialRequestSlip.h"        // MaterialRequestSlip DTO (for display)


namespace ERP {
namespace UI {
namespace Material {

/**
 * @brief IssueSlipManagementWidget class provides a UI for managing Material Issue Slips.
 * This widget allows viewing, creating, updating, deleting, and changing slip status.
 * It also supports managing slip details and recording issued quantities.
 */
class IssueSlipManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for IssueSlipManagementWidget.
     * @param issueSlipService Shared pointer to IIssueSlipService.
     * @param productService Shared pointer to IProductService.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param inventoryManagementService Shared pointer to IInventoryManagementService.
     * @param materialRequestService Shared pointer to IMaterialRequestService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit IssueSlipManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IIssueSlipService> issueSlipService = nullptr,
        std::shared_ptr<Product::Services::IProductService> productService = nullptr,
        std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService = nullptr,
        std::shared_ptr<Warehouse::Services::IInventoryManagementService> inventoryManagementService = nullptr,
        std::shared_ptr<Services::IMaterialRequestService> materialRequestService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~IssueSlipManagementWidget();

private slots:
    void loadIssueSlips();
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
    std::shared_ptr<Services::IIssueSlipService> issueSlipService_;
    std::shared_ptr<Product::Services::IProductService> productService_;
    std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService_;
    std::shared_ptr<Warehouse::Services::IInventoryManagementService> inventoryManagementService_;
    std::shared_ptr<Services::IMaterialRequestService> materialRequestService_;
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
    QComboBox *warehouseComboBox_;
    QLineEdit *issuedByLineEdit_; // User ID, display name
    QDateTimeEdit *issueDateEdit_;
    QComboBox *materialRequestSlipComboBox_; // Link to MRS
    QComboBox *statusComboBox_; // Slip Status
    QLineEdit *referenceDocumentIdLineEdit_;
    QLineEdit *referenceDocumentTypeLineEdit_;
    QLineEdit *notesLineEdit_;

    // Helper functions
    void setupUI();
    void populateWarehouseComboBox();
    void populateMaterialRequestSlipComboBox();
    void populateStatusComboBox(); // For issue slip status
    void showSlipInputDialog(ERP::Material::DTO::IssueSlipDTO* slip = nullptr);
    void showManageDetailsDialog(ERP::Material::DTO::IssueSlipDTO* slip);
    void showRecordIssuedQuantityDialog(ERP::Material::DTO::IssueSlipDetailDTO* detail, ERP::Material::DTO::IssueSlipDTO* parentSlip);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Material
} // namespace UI
} // namespace ERP

#endif // UI_MATERIAL_ISSUESLIPMANAGEMENTWIDGET_H