// UI/Sales/InvoiceManagementWidget.h
#ifndef UI_SALES_INVOICEMANAGEMENTWIDGET_H
#define UI_SALES_INVOICEMANAGEMENTWIDGET_H
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
#include "InvoiceService.h"         // Dịch vụ hóa đơn
#include "SalesOrderService.h"      // Dịch vụ đơn hàng bán (cho validation)
#include "CustomerService.h"        // Dịch vụ khách hàng (cho validation/display)
#include "ProductService.h"         // Dịch vụ sản phẩm (cho detail display)
#include "ISecurityManager.h"       // Dịch vụ bảo mật
#include "Logger.h"                 // Logging
#include "ErrorHandler.h"           // Xử lý lỗi
#include "Common.h"                 // Các enum chung
#include "DateUtils.h"              // Xử lý ngày tháng
#include "StringUtils.h"            // Xử lý chuỗi
#include "CustomMessageBox.h"       // Hộp thoại thông báo tùy chỉnh
#include "Invoice.h"                // Invoice DTO
#include "InvoiceDetail.h"          // InvoiceDetail DTO
#include "SalesOrder.h"             // SalesOrder DTO (for display)
#include "Customer.h"               // Customer DTO (for display)
#include "Product.h"                // Product DTO (for display)

namespace ERP {
namespace UI {
namespace Sales {

/**
 * @brief InvoiceManagementWidget class provides a UI for managing Sales Invoices.
 * This widget allows viewing, creating, updating, deleting, and changing invoice status.
 * It also supports managing invoice details.
 */
class InvoiceManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for InvoiceManagementWidget.
     * @param invoiceService Shared pointer to IInvoiceService.
     * @param salesOrderService Shared pointer to ISalesOrderService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit InvoiceManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IInvoiceService> invoiceService = nullptr,
        std::shared_ptr<Services::ISalesOrderService> salesOrderService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~InvoiceManagementWidget();

private slots:
    void loadInvoices();
    void onAddInvoiceClicked();
    void onEditInvoiceClicked();
    void onDeleteInvoiceClicked();
    void onUpdateInvoiceStatusClicked();
    void onSearchInvoiceClicked();
    void onInvoiceTableItemClicked(int row, int column);
    void clearForm();

    void onManageDetailsClicked(); // New slot for managing invoice details

private:
    std::shared_ptr<Services::IInvoiceService> invoiceService_;
    std::shared_ptr<Services::ISalesOrderService> salesOrderService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *invoiceTable_;
    QPushButton *addInvoiceButton_;
    QPushButton *editInvoiceButton_;
    QPushButton *deleteInvoiceButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;
    QPushButton *manageDetailsButton_;

    // Form elements for editing/adding invoices
    QLineEdit *idLineEdit_;
    QLineEdit *invoiceNumberLineEdit_;
    QComboBox *customerComboBox_; // Customer ID, display name
    QComboBox *salesOrderComboBox_; // Sales Order ID, display number
    QComboBox *typeComboBox_; // InvoiceType
    QDateTimeEdit *invoiceDateEdit_;
    QDateTimeEdit *dueDateEdit_;
    QComboBox *statusComboBox_; // InvoiceStatus
    QLineEdit *totalAmountLineEdit_;
    QLineEdit *totalDiscountLineEdit_;
    QLineEdit *totalTaxLineEdit_;
    QLineEdit *netAmountLineEdit_;
    QLineEdit *amountPaidLineEdit_;
    QLineEdit *amountDueLineEdit_;
    QLineEdit *currencyLineEdit_;
    QLineEdit *notesLineEdit_;

    // Helper functions
    void setupUI();
    void populateCustomerComboBox();
    void populateSalesOrderComboBox();
    void populateTypeComboBox();
    void populateStatusComboBox(); // For invoice status
    void showInvoiceInputDialog(ERP::Sales::DTO::InvoiceDTO* invoice = nullptr);
    void showManageDetailsDialog(ERP::Sales::DTO::InvoiceDTO* invoice);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Sales
} // namespace UI
} // namespace ERP

#endif // UI_SALES_INVOICEMANAGEMENTWIDGET_H