// UI/Material/ReceiptSlipManagementWidget.h
#ifndef UI_MATERIAL_RECEIPTSLIPMANAGEMENTWIDGET_H
#define UI_MATERIAL_RECEIPTSLIPMANAGEMENTWIDGET_H
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
#include "ReceiptSlipService.h"         // Dịch vụ phiếu nhập kho
#include "ProductService.h"             // Dịch vụ sản phẩm
#include "WarehouseService.h"           // Dịch vụ kho hàng
#include "InventoryManagementService.h" // Dịch vụ quản lý tồn kho
#include "ISecurityManager.h"           // Dịch vụ bảo mật
#include "Logger.h"                     // Logging
#include "ErrorHandler.h"               // Xử lý lỗi
#include "Common.h"                     // Các enum chung
#include "DateUtils.h"                  // Xử lý ngày tháng
#include "StringUtils.h"                // Xử lý chuỗi
#include "CustomMessageBox.h"           // Hộp thoại thông báo tùy chỉnh
#include "ReceiptSlip.h"                // ReceiptSlip DTO
#include "ReceiptSlipDetail.h"          // ReceiptSlipDetail DTO
#include "Product.h"                    // Product DTO (for display)
#include "Warehouse.h"                  // Warehouse DTO (for display)
#include "Location.h"                   // Location DTO (for display)

namespace ERP {
namespace UI {
namespace Material {

/**
 * @brief ReceiptSlipManagementWidget class provides a UI for managing Material Receipt Slips.
 * This widget allows viewing, creating, updating, deleting, and changing slip status.
 * It also supports managing slip details and recording received quantities.
 */
class ReceiptSlipManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for ReceiptSlipManagementWidget.
     * @param receiptSlipService Shared pointer to IReceiptSlipService.
     * @param productService Shared pointer to IProductService.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param inventoryManagementService Shared pointer to IInventoryManagementService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit ReceiptSlipManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IReceiptSlipService> receiptSlipService = nullptr,
        std::shared_ptr<Product::Services::IProductService> productService = nullptr,
        std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService = nullptr,
        std::shared_ptr<Warehouse::Services::IInventoryManagementService> inventoryManagementService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~ReceiptSlipManagementWidget();

private slots:
    void loadReceiptSlips();
    void onAddSlipClicked();
    void onEditSlipClicked();
    void onDeleteSlipClicked();
    void onUpdateSlipStatusClicked();
    void onSearchSlipClicked();
    void onSlipTableItemClicked(int row, int column);
    void clearForm();

    void onManageDetailsClicked(); // New slot for managing slip details
    void onRecordReceivedQuantityClicked(); // New slot for recording quantities

private:
    std::shared_ptr<Services::IReceiptSlipService> receiptSlipService_;
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
    QPushButton *recordReceivedQuantityButton_;

    // Form elements for editing/adding slips
    QLineEdit *idLineEdit_;
    QLineEdit *receiptNumberLineEdit_;
    QComboBox *warehouseComboBox_;
    QLineEdit *receivedByLineEdit_; // User ID, display name
    QDateTimeEdit *receiptDateEdit_;
    QComboBox *statusComboBox_; // Slip Status
    QLineEdit *referenceDocumentIdLineEdit_;
    QLineEdit *referenceDocumentTypeLineEdit_;
    QLineEdit *notesLineEdit_;

    // Helper functions
    void setupUI();
    void populateWarehouseComboBox();
    void populateStatusComboBox(); // For receipt slip status
    void showSlipInputDialog(ERP::Material::DTO::ReceiptSlipDTO* slip = nullptr);
    void showManageDetailsDialog(ERP::Material::DTO::ReceiptSlipDTO* slip);
    void showRecordReceivedQuantityDialog(ERP::Material::DTO::ReceiptSlipDetailDTO* detail, ERP::Material::DTO::ReceiptSlipDTO* parentSlip);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Material
} // namespace UI
} // namespace ERP

#endif // UI_MATERIAL_RECEIPTSLIPMANAGEMENTWIDGET_H