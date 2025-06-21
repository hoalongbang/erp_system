// UI/Sales/QuotationManagementWidget.h
#ifndef UI_SALES_QUOTATIONMANAGEMENTWIDGET_H
#define UI_SALES_QUOTATIONMANAGEMENTWIDGET_H
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
#include "QuotationService.h"       // Dịch vụ báo giá
#include "CustomerService.h"        // Dịch vụ khách hàng
#include "ProductService.h"         // Dịch vụ sản phẩm
#include "UnitOfMeasureService.h"   // Dịch vụ đơn vị đo
#include "SalesOrderService.h"      // Dịch vụ đơn hàng bán (cho chuyển đổi)
#include "ISecurityManager.h"       // Dịch vụ bảo mật
#include "Logger.h"                 // Logging
#include "ErrorHandler.h"           // Xử lý lỗi
#include "Common.h"                 // Các enum chung
#include "DateUtils.h"              // Xử lý ngày tháng
#include "StringUtils.h"            // Xử lý chuỗi
#include "CustomMessageBox.h"       // Hộp thoại thông báo tùy chỉnh
#include "Quotation.h"              // Quotation DTO
#include "QuotationDetail.h"        // QuotationDetail DTO
#include "Customer.h"               // Customer DTO (for display)
#include "Product.h"                // Product DTO (for display)
#include "SalesOrder.h"             // SalesOrder DTO (for conversion result)


namespace ERP {
namespace UI {
namespace Sales {

/**
 * @brief QuotationManagementWidget class provides a UI for managing Sales Quotations.
 * This widget allows viewing, creating, updating, deleting, and changing quotation status.
 * It also supports managing quotation details and converting to sales orders.
 */
class QuotationManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for QuotationManagementWidget.
     * @param quotationService Shared pointer to IQuotationService.
     * @param customerService Shared pointer to ICustomerService.
     * @param productService Shared pointer to IProductService.
     * @param unitOfMeasureService Shared pointer to IUnitOfMeasureService.
     * @param salesOrderService Shared pointer to ISalesOrderService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit QuotationManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IQuotationService> quotationService = nullptr,
        std::shared_ptr<Customer::Services::ICustomerService> customerService = nullptr,
        std::shared_ptr<Product::Services::IProductService> productService = nullptr,
        std::shared_ptr<Catalog::Services::IUnitOfMeasureService> unitOfMeasureService = nullptr,
        std::shared_ptr<Services::ISalesOrderService> salesOrderService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~QuotationManagementWidget();

private slots:
    void loadQuotations();
    void onAddQuotationClicked();
    void onEditQuotationClicked();
    void onDeleteQuotationClicked();
    void onUpdateQuotationStatusClicked();
    void onSearchQuotationClicked();
    void onQuotationTableItemClicked(int row, int column);
    void clearForm();

    void onManageDetailsClicked(); // New slot for managing quotation details
    void onConvertToSalesOrderClicked(); // New slot for converting quotation to sales order

private:
    std::shared_ptr<Services::IQuotationService> quotationService_;
    std::shared_ptr<Customer::Services::ICustomerService> customerService_;
    std::shared_ptr<Product::Services::IProductService> productService_;
    std::shared_ptr<Catalog::Services::IUnitOfMeasureService> unitOfMeasureService_;
    std::shared_ptr<Services::ISalesOrderService> salesOrderService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *quotationTable_;
    QPushButton *addQuotationButton_;
    QPushButton *editQuotationButton_;
    QPushButton *deleteQuotationButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;
    QPushButton *manageDetailsButton_;
    QPushButton *convertToSalesOrderButton_;

    // Form elements for editing/adding quotations
    QLineEdit *idLineEdit_;
    QLineEdit *quotationNumberLineEdit_;
    QComboBox *customerComboBox_; // Customer ID, display name
    QLineEdit *requestedByLineEdit_; // User ID, display name
    QDateTimeEdit *quotationDateEdit_;
    QDateTimeEdit *validUntilDateEdit_;
    QComboBox *statusComboBox_; // QuotationStatus
    QLineEdit *totalAmountLineEdit_; // Calculated
    QLineEdit *totalDiscountLineEdit_; // Calculated
    QLineEdit *totalTaxLineEdit_; // Calculated
    QLineEdit *netAmountLineEdit_; // Calculated
    QLineEdit *currencyLineEdit_;
    QLineEdit *paymentTermsLineEdit_;
    QLineEdit *deliveryTermsLineEdit_;
    QLineEdit *notesLineEdit_;

    // Helper functions
    void setupUI();
    void populateCustomerComboBox();
    void populateProductComboBox(); // For details
    void populateUnitOfMeasureComboBox(); // For details
    void populateStatusComboBox(); // For quotation status
    void showQuotationInputDialog(ERP::Sales::DTO::QuotationDTO* quotation = nullptr);
    void showManageDetailsDialog(ERP::Sales::DTO::QuotationDTO* quotation);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Sales
} // namespace UI
} // namespace ERP

#endif // UI_SALES_QUOTATIONMANAGEMENTWIDGET_H