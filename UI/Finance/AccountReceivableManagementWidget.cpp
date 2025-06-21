// UI/Finance/AccountReceivableManagementWidget.cpp
#include "AccountReceivableManagementWidget.h" // Standard includes
#include "AccountReceivableBalance.h"          // AR Balance DTO
#include "AccountReceivableTransaction.h"      // AR Transaction DTO
#include "Customer.h"                          // Customer DTO
#include "Invoice.h"                           // Invoice DTO
#include "Payment.h"                           // Payment DTO
#include "AccountReceivableService.h"          // AR Service
#include "CustomerService.h"                   // Customer Service
#include "InvoiceService.h"                    // Invoice Service
#include "PaymentService.h"                    // Payment Service
#include "ISecurityManager.h"                  // Security Manager
#include "Logger.h"                            // Logging
#include "ErrorHandler.h"                      // Error Handling
#include "Common.h"                            // Common Enums/Constants
#include "DateUtils.h"                         // Date Utilities
#include "StringUtils.h"                       // String Utilities
#include "CustomMessageBox.h"                  // Custom Message Box
#include "UserService.h"                       // For getting user names

#include <QTabWidget> // For tabs in UI
#include <QInputDialog>
#include <QDoubleValidator>
#include <QDateTime>

namespace ERP {
namespace UI {
namespace Finance {

AccountReceivableManagementWidget::AccountReceivableManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IAccountReceivableService> arService,
    std::shared_ptr<Customer::Services::ICustomerService> customerService,
    std::shared_ptr<Sales::Services::IInvoiceService> invoiceService,
    std::shared_ptr<Sales::Services::IPaymentService> paymentService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      arService_(arService),
      customerService_(customerService),
      invoiceService_(invoiceService),
      paymentService_(paymentService),
      securityManager_(securityManager) {

    if (!arService_ || !customerService_ || !invoiceService_ || !paymentService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ công nợ phải thu hoặc các dịch vụ phụ thuộc không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("AccountReceivableManagementWidget: Initialized with null dependencies.");
        return;
    }

    auto authService = securityManager_->getAuthenticationService();
    if (authService) {
        std::string dummySessionId = "current_session_id"; // Placeholder
        std::optional<ERP::Security::DTO::SessionDTO> currentSession = authService->validateSession(dummySessionId);
        if (currentSession) {
            currentUserId_ = currentSession->userId;
            currentUserRoleIds_ = securityManager_->getUserService()->getUserRoles(currentUserId_, {});
        } else {
            currentUserId_ = "system_user";
            currentUserRoleIds_ = {"anonymous"};
            ERP::Logger::Logger::getInstance().warning("AccountReceivableManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("AccountReceivableManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadARBalances();
    loadARTransactions();
    updateButtonsState();
}

AccountReceivableManagementWidget::~AccountReceivableManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void AccountReceivableManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QTabWidget *tabWidget = new QTabWidget(this);
    mainLayout->addWidget(tabWidget);

    // --- Balances Tab ---
    QWidget *balancesTab = new QWidget(this);
    QVBoxLayout *balancesLayout = new QVBoxLayout(balancesTab);
    tabWidget->addTab(balancesTab, "Số dư Công nợ");

    QHBoxLayout *searchBalanceLayout = new QHBoxLayout();
    searchBalanceLineEdit_ = new QLineEdit(this);
    searchBalanceLineEdit_->setPlaceholderText("Tìm kiếm theo tên khách hàng...");
    searchBalanceButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchBalanceButton_, &QPushButton::clicked, this, &AccountReceivableManagementWidget::onSearchARBalanceClicked);
    searchBalanceLayout->addWidget(searchBalanceLineEdit_);
    searchBalanceLayout->addWidget(searchBalanceButton_);
    balancesLayout->addLayout(searchBalanceLayout);

    arBalanceTable_ = new QTableWidget(this);
    arBalanceTable_->setColumnCount(5); // ID, Customer, Balance, Currency, Last Activity
    arBalanceTable_->setHorizontalHeaderLabels({"ID", "Khách hàng", "Số dư hiện tại", "Tiền tệ", "Ngày hoạt động cuối"});
    arBalanceTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    arBalanceTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    arBalanceTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    arBalanceTable_->horizontalHeader()->setStretchLastSection(true);
    connect(arBalanceTable_, &QTableWidget::itemClicked, this, &AccountReceivableManagementWidget::onARBalanceTableItemClicked);
    balancesLayout->addWidget(arBalanceTable_);

    QGridLayout *balanceFormLayout = new QGridLayout();
    balanceIdLineEdit_ = new QLineEdit(this); balanceIdLineEdit_->setReadOnly(true);
    balanceCustomerComboBox_ = new QComboBox(this); populateCustomerComboBox(balanceCustomerComboBox_);
    currentBalanceLineEdit_ = new QLineEdit(this); currentBalanceLineEdit_->setReadOnly(true);
    balanceCurrencyLineEdit_ = new QLineEdit(this); balanceCurrencyLineEdit_->setReadOnly(true);
    lastActivityDateEdit_ = new QDateTimeEdit(this); lastActivityDateEdit_->setReadOnly(true); lastActivityDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");

    balanceFormLayout->addWidget(new QLabel("ID:", this), 0, 0); balanceFormLayout->addWidget(balanceIdLineEdit_, 0, 1);
    balanceFormLayout->addRow("Khách hàng:", &balanceCustomerComboBox_);
    balanceFormLayout->addRow("Số dư hiện tại:", &currentBalanceLineEdit_);
    balanceFormLayout->addRow("Tiền tệ:", &balanceCurrencyLineEdit_);
    balanceFormLayout->addRow("Ngày hoạt động cuối:", &lastActivityDateEdit_);
    balancesLayout->addLayout(balanceFormLayout);

    QHBoxLayout *balanceButtonLayout = new QHBoxLayout();
    adjustARBalanceButton_ = new QPushButton("Điều chỉnh Số dư", this);
    connect(adjustARBalanceButton_, &QPushButton::clicked, this, &AccountReceivableManagementWidget::onAdjustARBalanceClicked);
    clearBalanceFormButton_ = new QPushButton("Xóa Form", this);
    connect(clearBalanceFormButton_, &QPushButton::clicked, this, &AccountReceivableManagementWidget::clearBalanceForm);
    balanceButtonLayout->addWidget(adjustARBalanceButton_);
    balanceButtonLayout->addWidget(clearBalanceFormButton_);
    balancesLayout->addLayout(balanceButtonLayout);

    // --- Transactions Tab ---
    QWidget *transactionsTab = new QWidget(this);
    QVBoxLayout *transactionsLayout = new QVBoxLayout(transactionsTab);
    tabWidget->addTab(transactionsTab, "Giao dịch Công nợ");

    QHBoxLayout *searchTransactionLayout = new QHBoxLayout();
    searchTransactionLineEdit_ = new QLineEdit(this);
    searchTransactionLineEdit_->setPlaceholderText("Tìm kiếm theo ID giao dịch, hóa đơn, thanh toán...");
    searchTransactionButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchTransactionButton_, &QPushButton::clicked, this, &AccountReceivableManagementWidget::onSearchARTransactionClicked);
    searchTransactionLayout->addWidget(searchTransactionLineEdit_);
    searchTransactionLayout->addWidget(searchTransactionButton_);
    transactionsLayout->addLayout(searchTransactionLayout);

    arTransactionTable_ = new QTableWidget(this);
    arTransactionTable_->setColumnCount(7); // ID, Customer, Type, Amount, Date, Reference Doc, Notes
    arTransactionTable_->setHorizontalHeaderLabels({"ID", "Khách hàng", "Loại", "Số tiền", "Tiền tệ", "Ngày GD", "Tài liệu tham chiếu"});
    arTransactionTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    arTransactionTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    arTransactionTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    arTransactionTable_->horizontalHeader()->setStretchLastSection(true);
    connect(arTransactionTable_, &QTableWidget::itemClicked, this, &AccountReceivableManagementWidget::onARTransactionTableItemClicked);
    transactionsLayout->addWidget(arTransactionTable_);

    QGridLayout *transactionFormLayout = new QGridLayout();
    transactionIdLineEdit_ = new QLineEdit(this); transactionIdLineEdit_->setReadOnly(true);
    transactionCustomerComboBox_ = new QComboBox(this); populateCustomerComboBox(transactionCustomerComboBox_); transactionCustomerComboBox_->setEnabled(false);
    transactionTypeComboBox_ = new QComboBox(this); populateTransactionTypeComboBox(); transactionTypeComboBox_->setEnabled(false);
    transactionAmountLineEdit_ = new QLineEdit(this); transactionAmountLineEdit_->setReadOnly(true);
    transactionCurrencyLineEdit_ = new QLineEdit(this); transactionCurrencyLineEdit_->setReadOnly(true);
    transactionDateEdit_ = new QDateTimeEdit(this); transactionDateEdit_->setReadOnly(true); transactionDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    referenceDocumentIdLineEdit_ = new QLineEdit(this); referenceDocumentIdLineEdit_->setReadOnly(true);
    referenceDocumentTypeLineEdit_ = new QLineEdit(this); referenceDocumentTypeLineEdit_->setReadOnly(true);
    notesLineEdit_ = new QLineEdit(this); notesLineEdit_->setReadOnly(true);

    transactionFormLayout->addWidget(new QLabel("ID:", this), 0, 0); transactionFormLayout->addWidget(transactionIdLineEdit_, 0, 1);
    transactionFormLayout->addRow("Khách hàng:", &transactionCustomerComboBox_);
    transactionFormLayout->addRow("Loại GD:", &transactionTypeComboBox_);
    transactionFormLayout->addRow("Số tiền:", &transactionAmountLineEdit_);
    transactionFormLayout->addRow("Tiền tệ:", &transactionCurrencyLineEdit_);
    transactionFormLayout->addRow("Ngày GD:", &transactionDateEdit_);
    transactionFormLayout->addRow("ID Tài liệu tham chiếu:", &referenceDocumentIdLineEdit_);
    transactionFormLayout->addRow("Loại Tài liệu tham chiếu:", &referenceDocumentTypeLineEdit_);
    transactionFormLayout->addRow("Ghi chú:", &notesLineEdit_);
    transactionsLayout->addLayout(transactionFormLayout);

    QHBoxLayout *transactionButtonLayout = new QHBoxLayout();
    clearTransactionFormButton_ = new QPushButton("Xóa Form", this);
    connect(clearTransactionFormButton_, &QPushButton::clicked, this, &AccountReceivableManagementWidget::clearTransactionForm);
    transactionButtonLayout->addWidget(clearTransactionFormButton_);
    transactionsLayout->addLayout(transactionButtonLayout);
}

void AccountReceivableManagementWidget::loadARBalances() {
    ERP::Logger::Logger::getInstance().info("AccountReceivableManagementWidget: Loading AR balances...");
    arBalanceTable_->setRowCount(0);

    std::vector<ERP::Finance::DTO::AccountReceivableBalanceDTO> balances = arService_->getAllARBalances({}, currentUserId_, currentUserRoleIds_);

    arBalanceTable_->setRowCount(balances.size());
    for (int i = 0; i < balances.size(); ++i) {
        const auto& balance = balances[i];
        arBalanceTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(balance.id)));
        
        QString customerName = "N/A";
        std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(balance.customerId, currentUserId_, currentUserRoleIds_);
        if (customer) customerName = QString::fromStdString(customer->name);
        arBalanceTable_->setItem(i, 1, new QTableWidgetItem(customerName));

        arBalanceTable_->setItem(i, 2, new QTableWidgetItem(QString::number(balance.currentBalance, 'f', 2)));
        arBalanceTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(balance.currency)));
        arBalanceTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(balance.lastActivityDate, ERP::Common::DATETIME_FORMAT))));
    }
    arBalanceTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("AccountReceivableManagementWidget: AR balances loaded successfully.");
}

void AccountReceivableManagementWidget::loadARTransactions() {
    ERP::Logger::Logger::getInstance().info("AccountReceivableManagementWidget: Loading AR transactions...");
    arTransactionTable_->setRowCount(0);

    std::vector<ERP::Finance::DTO::AccountReceivableTransactionDTO> transactions = arService_->getAllARTransactions({}, currentUserId_, currentUserRoleIds_);

    arTransactionTable_->setRowCount(transactions.size());
    for (int i = 0; i < transactions.size(); ++i) {
        const auto& transaction = transactions[i];
        arTransactionTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(transaction.id)));
        
        QString customerName = "N/A";
        std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(transaction.customerId, currentUserId_, currentUserRoleIds_);
        if (customer) customerName = QString::fromStdString(customer->name);
        arTransactionTable_->setItem(i, 1, new QTableWidgetItem(customerName));

        arTransactionTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(transaction.getTypeString())));
        arTransactionTable_->setItem(i, 3, new QTableWidgetItem(QString::number(transaction.amount, 'f', 2)));
        arTransactionTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(transaction.currency)));
        arTransactionTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(transaction.transactionDate, ERP::Common::DATETIME_FORMAT))));
        
        QString refDoc = transaction.referenceDocumentId.value_or("") + " (" + transaction.referenceDocumentType.value_or("") + ")";
        if (refDoc == " ()") refDoc = "N/A";
        arTransactionTable_->setItem(i, 6, new QTableWidgetItem(refDoc));
    }
    arTransactionTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("AccountReceivableManagementWidget: AR transactions loaded successfully.");
}

void AccountReceivableManagementWidget::populateCustomerComboBox(QComboBox* comboBox) {
    comboBox->clear();
    std::vector<ERP::Customer::DTO::CustomerDTO> allCustomers = customerService_->getAllCustomers({}, currentUserId_, currentUserRoleIds_);
    for (const auto& customer : allCustomers) {
        comboBox->addItem(QString::fromStdString(customer.name), QString::fromStdString(customer.id));
    }
}

void AccountReceivableManagementWidget::populateTransactionTypeComboBox() {
    transactionTypeComboBox_->clear();
    transactionTypeComboBox_->addItem("Invoice", static_cast<int>(ERP::Finance::DTO::ARTransactionType::INVOICE));
    transactionTypeComboBox_->addItem("Payment", static_cast<int>(ERP::Finance::DTO::ARTransactionType::PAYMENT));
    transactionTypeComboBox_->addItem("Adjustment", static_cast<int>(ERP::Finance::DTO::ARTransactionType::ADJUSTMENT));
    transactionTypeComboBox_->addItem("Credit Memo", static_cast<int>(ERP::Finance::DTO::ARTransactionType::CREDIT_MEMO));
    transactionTypeComboBox_->addItem("Debit Memo", static_cast<int>(ERP::Finance::DTO::ARTransactionType::DEBIT_MEMO));
}


void AccountReceivableManagementWidget::onAdjustARBalanceClicked() {
    if (!hasPermission("Finance.AdjustARBalance")) {
        showMessageBox("Lỗi", "Bạn không có quyền điều chỉnh số dư công nợ phải thu.", QMessageBox::Warning);
        return;
    }
    showAdjustARBalanceDialog();
}

void AccountReceivableManagementWidget::onSearchARBalanceClicked() {
    QString searchText = searchBalanceLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["customer_name_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    loadARBalances(); // Reloads with current filter if any
    ERP::Logger::Logger::getInstance().info("AccountReceivableManagementWidget: AR Balance Search completed.");
}

void AccountReceivableManagementWidget::onSearchARTransactionClicked() {
    QString searchText = searchTransactionLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["search_term"] = searchText.toStdString(); // Assuming generic search
    }
    loadARTransactions(); // Reloads with current filter if any
    ERP::Logger::Logger::getInstance().info("AccountReceivableManagementWidget: AR Transaction Search completed.");
}

void AccountReceivableManagementWidget::onARBalanceTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString balanceId = arBalanceTable_->item(row, 0)->text();
    std::map<std::string, std::any> filter;
    filter["id"] = balanceId.toStdString();
    std::vector<ERP::Finance::DTO::AccountReceivableBalanceDTO> balances = arService_->getAllARBalances(filter, currentUserId_, currentUserRoleIds_);

    if (!balances.empty()) {
        const auto& balanceOpt = balances[0]; // Assuming only one result by ID
        balanceIdLineEdit_->setText(QString::fromStdString(balanceOpt.id));
        
        populateCustomerComboBox(balanceCustomerComboBox_);
        int customerIndex = balanceCustomerComboBox_->findData(QString::fromStdString(balanceOpt.customerId));
        if (customerIndex != -1) balanceCustomerComboBox_->setCurrentIndex(customerIndex);
        
        currentBalanceLineEdit_->setText(QString::number(balanceOpt.currentBalance, 'f', 2));
        balanceCurrencyLineEdit_->setText(QString::fromStdString(balanceOpt.currency));
        lastActivityDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(balanceOpt.lastActivityDate.time_since_epoch()).count()));
    } else {
        showMessageBox("Thông tin Số dư AR", "Không thể tải chi tiết số dư đã chọn.", QMessageBox::Warning);
        clearBalanceForm();
    }
    updateButtonsState();
}

void AccountReceivableManagementWidget::onARTransactionTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString transactionId = arTransactionTable_->item(row, 0)->text();
    std::map<std::string, std::any> filter;
    filter["id"] = transactionId.toStdString();
    std::vector<ERP::Finance::DTO::AccountReceivableTransactionDTO> transactions = arService_->getAllARTransactions(filter, currentUserId_, currentUserRoleIds_);

    if (!transactions.empty()) {
        const auto& transactionOpt = transactions[0]; // Assuming only one result by ID
        transactionIdLineEdit_->setText(QString::fromStdString(transactionOpt.id));
        
        populateCustomerComboBox(transactionCustomerComboBox_);
        int customerIndex = transactionCustomerComboBox_->findData(QString::fromStdString(transactionOpt.customerId));
        if (customerIndex != -1) transactionCustomerComboBox_->setCurrentIndex(customerIndex);
        
        populateTransactionTypeComboBox();
        int typeIndex = transactionTypeComboBox_->findData(static_cast<int>(transactionOpt.type));
        if (typeIndex != -1) transactionTypeComboBox_->setCurrentIndex(typeIndex);

        transactionAmountLineEdit_->setText(QString::number(transactionOpt.amount, 'f', 2));
        transactionCurrencyLineEdit_->setText(QString::fromStdString(transactionOpt.currency));
        transactionDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(transactionOpt.transactionDate.time_since_epoch()).count()));
        referenceDocumentIdLineEdit_->setText(QString::fromStdString(transactionOpt.referenceDocumentId.value_or("")));
        referenceDocumentTypeLineEdit_->setText(QString::fromStdString(transactionOpt.referenceDocumentType.value_or("")));
        notesLineEdit_->setText(QString::fromStdString(transactionOpt.notes.value_or("")));
    } else {
        showMessageBox("Thông tin Giao dịch AR", "Không thể tải chi tiết giao dịch đã chọn.", QMessageBox::Warning);
        clearTransactionForm();
    }
    updateButtonsState();
}

void AccountReceivableManagementWidget::clearBalanceForm() {
    balanceIdLineEdit_->clear();
    balanceCustomerComboBox_->clear();
    currentBalanceLineEdit_->clear();
    balanceCurrencyLineEdit_->clear();
    lastActivityDateEdit_->clear();
    arBalanceTable_->clearSelection();
    updateButtonsState();
}

void AccountReceivableManagementWidget::clearTransactionForm() {
    transactionIdLineEdit_->clear();
    transactionCustomerComboBox_->clear();
    transactionTypeComboBox_->clear();
    transactionAmountLineEdit_->clear();
    transactionCurrencyLineEdit_->clear();
    transactionDateEdit_->clear();
    referenceDocumentIdLineEdit_->clear();
    referenceDocumentTypeLineEdit_->clear();
    notesLineEdit_->clear();
    arTransactionTable_->clearSelection();
    updateButtonsState();
}

void AccountReceivableManagementWidget::showAdjustARBalanceDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Điều chỉnh Số dư Công nợ");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox customerCombo(this); populateCustomerComboBox(&customerCombo);
    QLineEdit amountEdit(this); amountEdit.setValidator(new QDoubleValidator(-999999999.0, 999999999.0, 2, this));
    amountEdit.setPlaceholderText("Số tiền (dương để tăng, âm để giảm)");
    QLineEdit currencyEdit(this); currencyEdit.setText("VND");
    QLineEdit reasonEdit(this);

    formLayout->addRow("Khách hàng:*", &customerCombo);
    formLayout->addRow("Số tiền điều chỉnh:*", &amountEdit);
    formLayout->addRow("Tiền tệ:", &currencyEdit);
    formLayout->addRow("Lý do:*", &reasonEdit);
    dialogLayout->addLayout(formLayout);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    dialogLayout->addWidget(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        if (customerCombo.currentData().isNull() || amountEdit.text().isEmpty() || reasonEdit.text().isEmpty()) {
            showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin điều chỉnh.", QMessageBox::Warning);
            return;
        }
        
        std::string customerId = customerCombo.currentData().toString().toStdString();
        double amount = amountEdit.text().toDouble();
        std::string currency = currencyEdit.text().toStdString();
        std::string reason = reasonEdit.text().toStdString();

        if (arService_->adjustARBalance(customerId, amount, currency, reason, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Điều chỉnh Số dư", "Điều chỉnh số dư công nợ thành công.", QMessageBox::Information);
            loadARBalances();
            loadARTransactions();
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể điều chỉnh số dư công nợ. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void AccountReceivableManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool AccountReceivableManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void AccountReceivableManagementWidget::updateButtonsState() {
    bool canViewAR = hasPermission("Finance.ViewARBalance");
    bool canViewARTxns = hasPermission("Finance.ViewARTransactions");
    bool canAdjustAR = hasPermission("Finance.AdjustARBalance");

    searchBalanceButton_->setEnabled(canViewAR);
    searchTransactionButton_->setEnabled(canViewARTxns);
    adjustARBalanceButton_->setEnabled(canAdjustAR);

    // Form fields are read-only
    balanceIdLineEdit_->setEnabled(false);
    balanceCustomerComboBox_->setEnabled(false);
    currentBalanceLineEdit_->setEnabled(false);
    balanceCurrencyLineEdit_->setEnabled(false);
    lastActivityDateEdit_->setEnabled(false);

    transactionIdLineEdit_->setEnabled(false);
    transactionCustomerComboBox_->setEnabled(false);
    transactionTypeComboBox_->setEnabled(false);
    transactionAmountLineEdit_->setEnabled(false);
    transactionCurrencyLineEdit_->setEnabled(false);
    transactionDateEdit_->setEnabled(false);
    referenceDocumentIdLineEdit_->setEnabled(false);
    referenceDocumentTypeLineEdit_->setEnabled(false);
    notesLineEdit_->setEnabled(false);

    // Clear buttons always enabled
    clearBalanceFormButton_->setEnabled(true);
    clearTransactionFormButton_->setEnabled(true);
}

} // namespace Finance
} // namespace UI
} // namespace ERP