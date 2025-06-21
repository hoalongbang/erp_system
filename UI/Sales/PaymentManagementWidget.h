// UI/Sales/PaymentManagementWidget.h
#ifndef UI_SALES_PAYMENTMANAGEMENTWIDGET_H
#define UI_SALES_PAYMENTMANAGEMENTWIDGET_H
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
#include "PaymentService.h"         // Dịch vụ thanh toán
#include "CustomerService.h"        // Dịch vụ khách hàng (cho validation/display)
#include "InvoiceService.h"         // Dịch vụ hóa đơn (cho validation/display)
#include "ISecurityManager.h"       // Dịch vụ bảo mật
#include "Logger.h"                 // Logging
#include "ErrorHandler.h"           // Xử lý lỗi
#include "Common.h"                 // Các enum chung
#include "DateUtils.h"              // Xử lý ngày tháng
#include "StringUtils.h"            // Xử lý chuỗi
#include "CustomMessageBox.h"       // Hộp thoại thông báo tùy chỉnh
#include "Payment.h"                // Payment DTO
#include "Customer.h"               // Customer DTO (for display)
#include "Invoice.h"                // Invoice DTO (for display)

namespace ERP {
namespace UI {
namespace Sales {

/**
 * @brief PaymentManagementWidget class provides a UI for managing Payments.
 * This widget allows viewing, creating, updating, deleting, and changing payment status.
 */
class PaymentManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for PaymentManagementWidget.
     * @param paymentService Shared pointer to IPaymentService.
     * @param customerService Shared pointer to ICustomerService.
     * @param invoiceService Shared pointer to IInvoiceService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit PaymentManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IPaymentService> paymentService = nullptr,
        std::shared_ptr<Customer::Services::ICustomerService> customerService = nullptr,
        std::shared_ptr<Services::IInvoiceService> invoiceService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~PaymentManagementWidget();

private slots:
    void loadPayments();
    void onAddPaymentClicked();
    void onEditPaymentClicked();
    void onDeletePaymentClicked();
    void onUpdatePaymentStatusClicked();
    void onSearchPaymentClicked();
    void onPaymentTableItemClicked(int row, int column);
    void clearForm();

private:
    std::shared_ptr<Services::IPaymentService> paymentService_;
    std::shared_ptr<Customer::Services::ICustomerService> customerService_;
    std::shared_ptr<Services::IInvoiceService> invoiceService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *paymentTable_;
    QPushButton *addPaymentButton_;
    QPushButton *editPaymentButton_;
    QPushButton *deletePaymentButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;

    // Form elements for editing/adding payments
    QLineEdit *idLineEdit_;
    QLineEdit *paymentNumberLineEdit_;
    QComboBox *customerComboBox_; // Customer ID, display name
    QComboBox *invoiceComboBox_; // Invoice ID, display number
    QLineEdit *amountLineEdit_;
    QDateTimeEdit *paymentDateEdit_;
    QComboBox *methodComboBox_; // PaymentMethod
    QComboBox *statusComboBox_; // PaymentStatus
    QLineEdit *transactionIdLineEdit_;
    QLineEdit *notesLineEdit_;
    QLineEdit *currencyLineEdit_;

    // Helper functions
    void setupUI();
    void populateCustomerComboBox();
    void populateInvoiceComboBox();
    void populateMethodComboBox();
    void populateStatusComboBox(); // For payment status
    void showPaymentInputDialog(ERP::Sales::DTO::PaymentDTO* payment = nullptr);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Sales
} // namespace UI
} // namespace ERP

#endif // UI_SALES_PAYMENTMANAGEMENTWIDGET_H