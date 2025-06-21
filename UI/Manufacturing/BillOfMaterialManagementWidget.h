// UI/Manufacturing/BillOfMaterialManagementWidget.h
#ifndef UI_MANUFACTURING_BILLOFMATERIALMANAGEMENTWIDGET_H
#define UI_MANUFACTURING_BILLOFMATERIALMANAGEMENTWIDGET_H
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
#include <QDoubleValidator>
#include <QCheckBox>

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "BillOfMaterialService.h"  // Dịch vụ BOM
#include "ProductService.h"         // Dịch vụ sản phẩm
#include "UnitOfMeasureService.h"   // Dịch vụ đơn vị đo
#include "ISecurityManager.h"       // Dịch vụ bảo mật
#include "Logger.h"                 // Logging
#include "ErrorHandler.h"           // Xử lý lỗi
#include "Common.h"                 // Các enum chung
#include "DateUtils.h"              // Xử lý ngày tháng
#include "StringUtils.h"            // Xử lý chuỗi
#include "CustomMessageBox.h"       // Hộp thoại thông báo tùy chỉnh
#include "BillOfMaterial.h"         // BOM DTO
#include "BillOfMaterialItem.h"     // BOM Item DTO

namespace ERP {
namespace UI {
namespace Manufacturing {

/**
 * @brief BillOfMaterialManagementWidget class provides a UI for managing Bills of Material (BOMs).
 * This widget allows viewing, creating, updating, deleting, and changing BOM status.
 * It also supports managing BOM items.
 */
class BillOfMaterialManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for BillOfMaterialManagementWidget.
     * @param bomService Shared pointer to IBillOfMaterialService.
     * @param productService Shared pointer to IProductService.
     * @param unitOfMeasureService Shared pointer to IUnitOfMeasureService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit BillOfMaterialManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IBillOfMaterialService> bomService = nullptr,
        std::shared_ptr<Product::Services::IProductService> productService = nullptr,
        std::shared_ptr<Catalog::Services::IUnitOfMeasureService> unitOfMeasureService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~BillOfMaterialManagementWidget();

private slots:
    void loadBOMs();
    void onAddBOMClicked();
    void onEditBOMClicked();
    void onDeleteBOMClicked();
    void onUpdateBOMStatusClicked();
    void onSearchBOMClicked();
    void onBOMTableItemClicked(int row, int column);
    void clearForm();

    void onManageBOMItemsClicked(); // New slot for managing BOM items

private:
    std::shared_ptr<Services::IBillOfMaterialService> bomService_;
    std::shared_ptr<Product::Services::IProductService> productService_;
    std::shared_ptr<Catalog::Services::IUnitOfMeasureService> unitOfMeasureService_;
    std::shared_ptr<Security::ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *bomTable_;
    QPushButton *addBOMButton_;
    QPushButton *editBOMButton_;
    QPushButton *deleteBOMButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;
    QPushButton *manageBOMItemsButton_;

    // Form elements for editing/adding
    QLineEdit *idLineEdit_;
    QLineEdit *bomNameLineEdit_;
    QComboBox *productComboBox_; // Product to be manufactured
    QLineEdit *descriptionLineEdit_;
    QLineEdit *baseQuantityLineEdit_;
    QComboBox *baseQuantityUnitComboBox_;
    QComboBox *statusComboBox_; // BOM Status
    QLineEdit *versionLineEdit_;

    // Helper functions
    void setupUI();
    void populateProductComboBox();
    void populateUnitOfMeasureComboBox();
    void populateStatusComboBox(); // For BOM status
    void showBOMInputDialog(ERP::Manufacturing::DTO::BillOfMaterialDTO* bom = nullptr);
    void showManageBOMItemsDialog(ERP::Manufacturing::DTO::BillOfMaterialDTO* bom);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState(); // Control button enable/disable based on selection and permissions
    
    // Permission checking helper
    bool hasPermission(const std::string& permission);
};

} // namespace Manufacturing
} // namespace UI
} // namespace ERP

#endif // UI_MANUFACTURING_BILLOFMATERIALMANAGEMENTWIDGET_H