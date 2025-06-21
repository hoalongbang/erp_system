// UI/Finance/AccountReceivableManagementWidget.h
#ifndef UI_FINANCE_ACCOUNTRECEIVABLEMANAGEMENTWIDGET_H
#define UI_FINANCE_ACCOUNTRECEIVABLEMANAGEMENTWIDGET_H
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
#include "AccountReceivableService.h" // Dịch vụ công nợ phải thu
#include "CustomerService.h"          // Dịch vụ khách hàng
#include "InvoiceService.h"           // Dịch vụ hóa đơn
#include "PaymentService.h"           // Dịch vụ thanh toán
#include "ISecurityManager.h"         // Dịch vụ bảo mật
#include "Logger.h"                   // Logging
#include "ErrorHandler.h"             // Xử lý lỗi
#include "Common.h"                   // Các enum chung
#include "DateUtils.h"                // Xử lý ngày tháng
#include "StringUtils.h"              // Xử lý chuỗi
#include "CustomMessageBox.h"         // Hộp thoại thông báo tùy chỉnh
#include "AccountReceivableBalance.h" // AR Balance DTO
#include "AccountReceivableTransaction.h" // AR Transaction DTO
#include "Customer.h"                 // Customer DTO (for display)
#include "Invoice.h"                  // Invoice DTO (for display)
#include "Payment.h"                  // Payment DTO (for display)


namespace ERP {
namespace UI {
namespace Finance {

/**
 * @brief AccountReceivableManagementWidget class provides a UI for managing Accounts Receivable (AR).
 * This widget allows viewing AR balances and transactions, and manually adjusting balances.
 */
class AccountReceivableManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for AccountReceivableManagementWidget.
     * @param arService Shared pointer to IAccountReceivableService.
     * @param customerService Shared pointer to ICustomerService.
     * @param invoiceService Shared pointer to IInvoiceService.
     * @param paymentService Shared pointer to IPaymentService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit AccountReceivableManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IAccountReceivableService> arService = nullptr,
        std::shared_ptr<Customer::Services::ICustomerService> customerService = nullptr,
        std::shared_ptr<Sales::Services::IInvoiceService> invoiceService = nullptr,
        std::shared_ptr<Sales::Services::IPaymentService> paymentService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~AccountReceivableManagementWidget();

private slots:
    void loadARBalances();
    void loadARTransactions();
    void onAdjustARBalanceClicked();
    void onSearchARBalanceClicked();
    void onSearchARTransactionClicked();
    void onARBalanceTableItemClicked(int row, int column);
    void onARTransactionTableItemClicked(int row, int column);
    void clearBalanceForm();
    void clearTransactionForm();

private:
    std::shared_ptr<Services::IAccountReceivableService> arService_;
    std::shared_ptr<Customer::Services::ICustomerService> customerService_;
    std::shared_ptr<Sales::Services::IInvoiceService> invoiceService_;
    std::shared_ptr<Sales::Services::IPaymentService> paymentService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    // UI elements for Balances tab
    QTableWidget *arBalanceTable_;
    QPushButton *adjustARBalanceButton_;
    QLineEdit *searchBalanceLineEdit_;
    QPushButton *searchBalanceButton_;
    QPushButton *clearBalanceFormButton_;

    QLineEdit *balanceIdLineEdit_;
    QComboBox *balanceCustomerComboBox_;
    QLineEdit *currentBalanceLineEdit_;
    QLineEdit *balanceCurrencyLineEdit_;
    QDateTimeEdit *lastActivityDateEdit_;

    // UI elements for Transactions tab
    QTableWidget *arTransactionTable_;
    QLineEdit *searchTransactionLineEdit_;
    QPushButton *searchTransactionButton_;
    QPushButton *clearTransactionFormButton_;

    QLineEdit *transactionIdLineEdit_;
    QComboBox *transactionCustomerComboBox_;
    QComboBox *transactionTypeComboBox_;
    QLineEdit *transactionAmountLineEdit_;
    QLineEdit *transactionCurrencyLineEdit_;
    QDateTimeEdit *transactionDateEdit_;
    QLineEdit *referenceDocumentIdLineEdit_;
    QLineEdit *referenceDocumentTypeLineEdit_;
    QLineEdit *notesLineEdit_;

    // Helper functions
    void setupUI();
    void populateCustomerComboBox(QComboBox* comboBox);
    void populateTransactionTypeComboBox();
    void showAdjustARBalanceDialog();
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Finance
} // namespace UI
} // namespace ERP

#endif // UI_FINANCE_ACCOUNTRECEIVABLEMANAGEMENTWIDGET_H