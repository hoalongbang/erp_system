// UI/Product/ProductManagementWidget.h
#ifndef UI_PRODUCT_PRODUCTMANAGEMENTWIDGET_H
#define UI_PRODUCT_PRODUCTMANAGEMENTWIDGET_H
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
#include <QDialog> // For product input dialog
#include <QDoubleValidator> // For price, weight input
#include <QCheckBox> // For is_active

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "ProductService.h"     // Dịch vụ sản phẩm
#include "CategoryService.h"    // Dịch vụ danh mục
#include "UnitOfMeasureService.h" // Dịch vụ đơn vị đo
#include "ISecurityManager.h"   // Dịch vụ bảo mật
#include "Logger.h"             // Logging
#include "ErrorHandler.h"       // Xử lý lỗi
#include "Common.h"             // Các enum chung
#include "DateUtils.h"          // Xử lý ngày tháng
#include "StringUtils.h"        // Xử lý chuỗi
#include "CustomMessageBox.h"   // Hộp thoại thông báo tùy chỉnh
#include "Product.h"            // Product DTO (for enums etc.)

namespace ERP {
namespace UI {
namespace Product {

/**
 * @brief ProductManagementWidget class provides a UI for managing Products.
 * This widget allows viewing, creating, updating, deleting, and changing product status.
 */
class ProductManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for ProductManagementWidget.
     * @param productService Shared pointer to IProductService.
     * @param categoryService Shared pointer to ICategoryService.
     * @param unitOfMeasureService Shared pointer to IUnitOfMeasureService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit ProductManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IProductService> productService = nullptr,
        std::shared_ptr<Catalog::Services::ICategoryService> categoryService = nullptr,
        std::shared_ptr<Catalog::Services::IUnitOfMeasureService> unitOfMeasureService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~ProductManagementWidget();

private slots:
    void loadProducts();
    void onAddProductClicked();
    void onEditProductClicked();
    void onDeleteProductClicked();
    void onUpdateProductStatusClicked();
    void onSearchProductClicked();
    void onProductTableItemClicked(int row, int column);
    void clearForm();

private:
    std::shared_ptr<Services::IProductService> productService_;
    std::shared_ptr<Catalog::Services::ICategoryService> categoryService_;
    std::shared_ptr<Catalog::Services::IUnitOfMeasureService> unitOfMeasureService_;
    std::shared_ptr<Security::ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *productTable_;
    QPushButton *addProductButton_;
    QPushButton *editProductButton_;
    QPushButton *deleteProductButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;

    // Form elements for editing/adding
    QLineEdit *idLineEdit_;
    QLineEdit *nameLineEdit_;
    QLineEdit *productCodeLineEdit_;
    QComboBox *categoryComboBox_;
    QComboBox *baseUnitOfMeasureComboBox_;
    QLineEdit *descriptionLineEdit_;
    QLineEdit *purchasePriceLineEdit_;
    QLineEdit *purchaseCurrencyLineEdit_;
    QLineEdit *salePriceLineEdit_;
    QLineEdit *saleCurrencyLineEdit_;
    QLineEdit *imageUrlLineEdit_;
    QLineEdit *weightLineEdit_;
    QLineEdit *weightUnitLineEdit_;
    QComboBox *typeComboBox_; // ProductType
    QLineEdit *manufacturerLineEdit_;
    QLineEdit *supplierIdLineEdit_; // Needs to be a combo box or search for actual UI
    QLineEdit *barcodeLineEdit_;
    QComboBox *statusComboBox_; // EntityStatus
    QCheckBox *isActiveCheckBox_; // For BaseDTO status management

    // Helper functions
    void setupUI();
    void populateCategoryComboBox();
    void populateUnitOfMeasureComboBox();
    void populateTypeComboBox();
    void showProductInputDialog(ERP::Product::DTO::ProductDTO* product = nullptr);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState(); // Control button enable/disable based on selection and permissions
    
    // Permission checking helper
    bool hasPermission(const std::string& permission);
};

} // namespace Product
} // namespace UI
} // namespace ERP

#endif // UI_PRODUCT_PRODUCTMANAGEMENTWIDGET_H