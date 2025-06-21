// UI/Sales/PaymentManagementWidget.cpp
#include "PaymentManagementWidget.h" // Standard includes
#include "Payment.h"                 // Payment DTO
#include "Customer.h"                // Customer DTO
#include "Invoice.h"                 // Invoice DTO
#include "PaymentService.h"          // Payment Service
#include "CustomerService.h"         // Customer Service
#include "InvoiceService.h"          // Invoice Service
#include "ISecurityManager.h"        // Security Manager
#include "Logger.h"                  // Logging
#include "ErrorHandler.h"            // Error Handling
#include "Common.h"                  // Common Enums/Constants
#include "DateUtils.h"               // Date Utilities
#include "StringUtils.h"             // String Utilities
#include "CustomMessageBox.h"        // Custom Message Box
#include "UserService.h"             // For getting user names

#include <QInputDialog>
#include <QDoubleValidator>
#include <QDateTime>

namespace ERP {
namespace UI {
namespace Sales {

PaymentManagementWidget::PaymentManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IPaymentService> paymentService,
    std::shared_ptr<Customer::Services::ICustomerService> customerService,
    std::shared_ptr<Services::IInvoiceService> invoiceService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      paymentService_(paymentService),
      customerService_(customerService),
      invoiceService_(invoiceService),
      securityManager_(securityManager) {

    if (!paymentService_ || !customerService_ || !invoiceService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ thanh toán, khách hàng, hóa đơn hoặc bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("PaymentManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("PaymentManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("PaymentManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadPayments();
    updateButtonsState();
}

PaymentManagementWidget::~PaymentManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void PaymentManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo số thanh toán...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &PaymentManagementWidget::onSearchPaymentClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    paymentTable_ = new QTableWidget(this);
    paymentTable_->setColumnCount(8); // ID, Số thanh toán, Khách hàng, Hóa đơn, Số tiền, Ngày thanh toán, Phương thức, Trạng thái
    paymentTable_->setHorizontalHeaderLabels({"ID", "Số TT", "Khách hàng", "Hóa đơn", "Số tiền", "Ngày TT", "Phương thức", "Trạng thái"});
    paymentTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    paymentTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    paymentTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    paymentTable_->horizontalHeader()->setStretchLastSection(true);
    connect(paymentTable_, &QTableWidget::itemClicked, this, &PaymentManagementWidget::onPaymentTableItemClicked);
    mainLayout->addWidget(paymentTable_);

    // Form elements for editing/adding payments
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    paymentNumberLineEdit_ = new QLineEdit(this);
    customerComboBox_ = new QComboBox(this); populateCustomerComboBox();
    invoiceComboBox_ = new QComboBox(this); populateInvoiceComboBox();
    amountLineEdit_ = new QLineEdit(this); amountLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    paymentDateEdit_ = new QDateTimeEdit(this); paymentDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    methodComboBox_ = new QComboBox(this); populateMethodComboBox();
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();
    transactionIdLineEdit_ = new QLineEdit(this);
    notesLineEdit_ = new QLineEdit(this);
    currencyLineEdit_ = new QLineEdit(this);


    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("Số Thanh toán:*", &paymentNumberLineEdit_);
    formLayout->addRow("Khách hàng:*", &customerComboBox_);
    formLayout->addRow("Hóa đơn:*", &invoiceComboBox_);
    formLayout->addRow("Số tiền:*", &amountLineEdit_);
    formLayout->addRow("Ngày Thanh toán:*", &paymentDateEdit_);
    formLayout->addRow("Phương thức:*", &methodComboBox_);
    formLayout->addRow("Trạng thái:*", &statusComboBox_);
    formLayout->addRow("ID Giao dịch:", &transactionIdLineEdit_);
    formLayout->addRow("Ghi chú:", &notesLineEdit_);
    formLayout->addRow("Tiền tệ:*", &currencyLineEdit_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addPaymentButton_ = new QPushButton("Thêm mới", this); connect(addPaymentButton_, &QPushButton::clicked, this, &PaymentManagementWidget::onAddPaymentClicked);
    editPaymentButton_ = new QPushButton("Sửa", this); connect(editPaymentButton_, &QPushButton::clicked, this, &PaymentManagementWidget::onEditPaymentClicked);
    deletePaymentButton_ = new QPushButton("Xóa", this); connect(deletePaymentButton_, &QPushButton::clicked, this, &PaymentManagementWidget::onDeletePaymentClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &PaymentManagementWidget::onUpdatePaymentStatusClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &PaymentManagementWidget::onSearchPaymentClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &PaymentManagementWidget::clearForm);
    
    buttonLayout->addWidget(addPaymentButton_);
    buttonLayout->addWidget(editPaymentButton_);
    buttonLayout->addWidget(deletePaymentButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void PaymentManagementWidget::loadPayments() {
    ERP::Logger::Logger::getInstance().info("PaymentManagementWidget: Loading payments...");
    paymentTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Sales::DTO::PaymentDTO> payments = paymentService_->getAllPayments({}, currentUserId_, currentUserRoleIds_);

    paymentTable_->setRowCount(payments.size());
    for (int i = 0; i < payments.size(); ++i) {
        const auto& payment = payments[i];
        paymentTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(payment.id)));
        paymentTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(payment.paymentNumber)));
        
        QString customerName = "N/A";
        std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(payment.customerId, currentUserId_, currentUserRoleIds_);
        if (customer) customerName = QString::fromStdString(customer->name);
        paymentTable_->setItem(i, 2, new QTableWidgetItem(customerName));

        QString invoiceNumber = "N/A";
        std::optional<ERP::Sales::DTO::InvoiceDTO> invoice = invoiceService_->getInvoiceById(payment.invoiceId, currentUserId_, currentUserRoleIds_);
        if (invoice) invoiceNumber = QString::fromStdString(invoice->invoiceNumber);
        paymentTable_->setItem(i, 3, new QTableWidgetItem(invoiceNumber));

        paymentTable_->setItem(i, 4, new QTableWidgetItem(QString::number(payment.amount, 'f', 2) + " " + QString::fromStdString(payment.currency)));
        paymentTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(payment.paymentDate, ERP::Common::DATETIME_FORMAT))));
        paymentTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(payment.getMethodString())));
        paymentTable_->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(payment.getStatusString())));
    }
    paymentTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("PaymentManagementWidget: Payments loaded successfully.");
}

void PaymentManagementWidget::populateCustomerComboBox() {
    customerComboBox_->clear();
    std::vector<ERP::Customer::DTO::CustomerDTO> allCustomers = customerService_->getAllCustomers({}, currentUserId_, currentUserRoleIds_);
    for (const auto& customer : allCustomers) {
        customerComboBox_->addItem(QString::fromStdString(customer.name), QString::fromStdString(customer.id));
    }
}

void PaymentManagementWidget::populateInvoiceComboBox() {
    invoiceComboBox_->clear();
    std::vector<ERP::Sales::DTO::InvoiceDTO> allInvoices = invoiceService_->getAllInvoices({}, currentUserId_, currentUserRoleIds_);
    for (const auto& invoice : allInvoices) {
        invoiceComboBox_->addItem(QString::fromStdString(invoice.invoiceNumber), QString::fromStdString(invoice.id));
    }
}

void PaymentManagementWidget::populateMethodComboBox() {
    methodComboBox_->clear();
    methodComboBox_->addItem("Cash", static_cast<int>(ERP::Sales::DTO::PaymentMethod::CASH));
    methodComboBox_->addItem("Bank Transfer", static_cast<int>(ERP::Sales::DTO::PaymentMethod::BANK_TRANSFER));
    methodComboBox_->addItem("Credit Card", static_cast<int>(ERP::Sales::DTO::PaymentMethod::CREDIT_CARD));
    methodComboBox_->addItem("Online Payment", static_cast<int>(ERP::Sales::DTO::PaymentMethod::ONLINE_PAYMENT));
    methodComboBox_->addItem("Other", static_cast<int>(ERP::Sales::DTO::PaymentMethod::OTHER));
}

void PaymentManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Sales::DTO::PaymentStatus::PENDING));
    statusComboBox_->addItem("Completed", static_cast<int>(ERP::Sales::DTO::PaymentStatus::COMPLETED));
    statusComboBox_->addItem("Failed", static_cast<int>(ERP::Sales::DTO::PaymentStatus::FAILED));
    statusComboBox_->addItem("Refunded", static_cast<int>(ERP::Sales::DTO::PaymentStatus::REFUNDED));
    statusComboBox_->addItem("Cancelled", static_cast<int>(ERP::Sales::DTO::PaymentStatus::CANCELLED));
}


void PaymentManagementWidget::onAddPaymentClicked() {
    if (!hasPermission("Sales.CreatePayment")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm thanh toán.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateCustomerComboBox();
    populateInvoiceComboBox();
    populateMethodComboBox();
    showPaymentInputDialog();
}

void PaymentManagementWidget::onEditPaymentClicked() {
    if (!hasPermission("Sales.UpdatePayment")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa thanh toán.", QMessageBox::Warning);
        return;
    }

    int selectedRow = paymentTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Thanh Toán", "Vui lòng chọn một thanh toán để sửa.", QMessageBox::Information);
        return;
    }

    QString paymentId = paymentTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::PaymentDTO> paymentOpt = paymentService_->getPaymentById(paymentId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (paymentOpt) {
        populateCustomerComboBox();
        populateInvoiceComboBox();
        populateMethodComboBox();
        showPaymentInputDialog(&(*paymentOpt));
    } else {
        showMessageBox("Sửa Thanh Toán", "Không tìm thấy thanh toán để sửa.", QMessageBox::Critical);
    }
}

void PaymentManagementWidget::onDeletePaymentClicked() {
    if (!hasPermission("Sales.DeletePayment")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa thanh toán.", QMessageBox::Warning);
        return;
    }

    int selectedRow = paymentTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Thanh Toán", "Vui lòng chọn một thanh toán để xóa.", QMessageBox::Information);
        return;
    }

    QString paymentId = paymentTable_->item(selectedRow, 0)->text();
    QString paymentNumber = paymentTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Thanh Toán");
    confirmBox.setText("Bạn có chắc chắn muốn xóa thanh toán '" + paymentNumber + "' (ID: " + paymentId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (paymentService_->deletePayment(paymentId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Thanh Toán", "Thanh toán đã được xóa thành công.", QMessageBox::Information);
            loadPayments();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa thanh toán. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void PaymentManagementWidget::onUpdatePaymentStatusClicked() {
    if (!hasPermission("Sales.UpdatePaymentStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái thanh toán.", QMessageBox::Warning);
        return;
    }

    int selectedRow = paymentTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một thanh toán để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString paymentId = paymentTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::PaymentDTO> paymentOpt = paymentService_->getPaymentById(paymentId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!paymentOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy thanh toán để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Sales::DTO::PaymentDTO currentPayment = *paymentOpt;
    // Use a QInputDialog or custom dialog to get the new status
    QDialog statusDialog(this);
    statusDialog.setWindowTitle("Chọn Trạng Thái Mới");
    QVBoxLayout *layout = new QVBoxLayout(&statusDialog);
    QComboBox newStatusCombo;
    populateStatusComboBox(); // Populate it
    for(int i = 0; i < statusComboBox_->count(); ++i) { // Copy items
        newStatusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));
    }
    // Set current status as default selected
    int currentStatusIndex = newStatusCombo.findData(static_cast<int>(currentPayment.status));
    if (currentStatusIndex != -1) newStatusCombo.setCurrentIndex(currentStatusIndex);

    layout->addWidget(new QLabel("Chọn trạng thái mới:", &statusDialog));
    layout->addWidget(&newStatusCombo);
    QPushButton *okButton = new QPushButton("Cập nhật", &statusDialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &statusDialog);
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(okButton);
    btnLayout->addWidget(cancelButton);
    layout->addLayout(btnLayout);

    connect(okButton, &QPushButton::clicked, &statusDialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (statusDialog.exec() == QDialog::Accepted) {
        ERP::Sales::DTO::PaymentStatus newStatus = static_cast<ERP::Sales::DTO::PaymentStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái thanh toán");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái thanh toán '" + QString::fromStdString(currentPayment.paymentNumber) + "' thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (paymentService_->updatePaymentStatus(paymentId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái thanh toán đã được cập nhật thành công.", QMessageBox::Information);
                loadPayments();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái thanh toán. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}


void PaymentManagementWidget::onSearchPaymentClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["payment_number_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    paymentTable_->setRowCount(0);
    std::vector<ERP::Sales::DTO::PaymentDTO> payments = paymentService_->getAllPayments(filter, currentUserId_, currentUserRoleIds_);

    paymentTable_->setRowCount(payments.size());
    for (int i = 0; i < payments.size(); ++i) {
        const auto& payment = payments[i];
        paymentTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(payment.id)));
        paymentTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(payment.paymentNumber)));
        
        QString customerName = "N/A";
        std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(payment.customerId, currentUserId_, currentUserRoleIds_);
        if (customer) customerName = QString::fromStdString(customer->name);
        paymentTable_->setItem(i, 2, new QTableWidgetItem(customerName));

        QString invoiceNumber = "N/A";
        std::optional<ERP::Sales::DTO::InvoiceDTO> invoice = invoiceService_->getInvoiceById(payment.invoiceId, currentUserId_, currentUserRoleIds_);
        if (invoice) invoiceNumber = QString::fromStdString(invoice->invoiceNumber);
        paymentTable_->setItem(i, 3, new QTableWidgetItem(invoiceNumber));

        paymentTable_->setItem(i, 4, new QTableWidgetItem(QString::number(payment.amount, 'f', 2) + " " + QString::fromStdString(payment.currency)));
        paymentTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(payment.paymentDate, ERP::Common::DATETIME_FORMAT))));
        paymentTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(payment.getMethodString())));
        paymentTable_->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(payment.getStatusString())));
    }
    paymentTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("PaymentManagementWidget: Search completed.");
}

void PaymentManagementWidget::onPaymentTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString paymentId = paymentTable_->item(row, 0)->text();
    std::optional<ERP::Sales::DTO::PaymentDTO> paymentOpt = paymentService_->getPaymentById(paymentId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (paymentOpt) {
        idLineEdit_->setText(QString::fromStdString(paymentOpt->id));
        paymentNumberLineEdit_->setText(QString::fromStdString(paymentOpt->paymentNumber));
        
        populateCustomerComboBox();
        int customerIndex = customerComboBox_->findData(QString::fromStdString(paymentOpt->customerId));
        if (customerIndex != -1) customerComboBox_->setCurrentIndex(customerIndex);

        populateInvoiceComboBox();
        int invoiceIndex = invoiceComboBox_->findData(QString::fromStdString(paymentOpt->invoiceId));
        if (invoiceIndex != -1) invoiceComboBox_->setCurrentIndex(invoiceIndex);

        amountLineEdit_->setText(QString::number(paymentOpt->amount, 'f', 2));
        paymentDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(paymentOpt->paymentDate.time_since_epoch()).count()));
        
        populateMethodComboBox();
        int methodIndex = methodComboBox_->findData(static_cast<int>(paymentOpt->method));
        if (methodIndex != -1) methodComboBox_->setCurrentIndex(methodIndex);

        populateStatusComboBox();
        int statusIndex = statusComboBox_->findData(static_cast<int>(paymentOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);

        transactionIdLineEdit_->setText(QString::fromStdString(paymentOpt->transactionId.value_or("")));
        notesLineEdit_->setText(QString::fromStdString(paymentOpt->notes.value_or("")));
        currencyLineEdit_->setText(QString::fromStdString(paymentOpt->currency));

    } else {
        showMessageBox("Thông tin Thanh Toán", "Không tìm thấy thanh toán đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void PaymentManagementWidget::clearForm() {
    idLineEdit_->clear();
    paymentNumberLineEdit_->clear();
    customerComboBox_->clear();
    invoiceComboBox_->clear();
    amountLineEdit_->clear();
    paymentDateEdit_->clear();
    methodComboBox_->setCurrentIndex(0);
    statusComboBox_->setCurrentIndex(0);
    transactionIdLineEdit_->clear();
    notesLineEdit_->clear();
    currencyLineEdit_->clear();
    paymentTable_->clearSelection();
    updateButtonsState();
}


void PaymentManagementWidget::showPaymentInputDialog(ERP::Sales::DTO::PaymentDTO* payment) {
    QDialog dialog(this);
    dialog.setWindowTitle(payment ? "Sửa Thanh Toán" : "Thêm Thanh Toán Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit paymentNumberEdit(this);
    QComboBox customerCombo(this); populateCustomerComboBox();
    for(int i = 0; i < customerComboBox_->count(); ++i) customerCombo.addItem(customerComboBox_->itemText(i), customerComboBox_->itemData(i));
    
    QComboBox invoiceCombo(this); populateInvoiceComboBox();
    for(int i = 0; i < invoiceComboBox_->count(); ++i) invoiceCombo.addItem(invoiceComboBox_->itemText(i), invoiceComboBox_->itemData(i));
    
    QLineEdit amountEdit(this); amountEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    QDateTimeEdit paymentDateEdit(this); paymentDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QComboBox methodCombo(this); populateMethodComboBox();
    for(int i = 0; i < methodComboBox_->count(); ++i) methodCombo.addItem(methodComboBox_->itemText(i), methodComboBox_->itemData(i));
    
    QLineEdit transactionIdEdit(this);
    QLineEdit notesEdit(this);
    QLineEdit currencyEdit(this);

    if (payment) {
        paymentNumberEdit.setText(QString::fromStdString(payment->paymentNumber));
        int customerIndex = customerCombo.findData(QString::fromStdString(payment->customerId));
        if (customerIndex != -1) customerCombo.setCurrentIndex(customerIndex);
        int invoiceIndex = invoiceCombo.findData(QString::fromStdString(payment->invoiceId));
        if (invoiceIndex != -1) invoiceCombo.setCurrentIndex(invoiceIndex);
        amountEdit.setText(QString::number(payment->amount, 'f', 2));
        paymentDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(payment->paymentDate.time_since_epoch()).count()));
        int methodIndex = methodCombo.findData(static_cast<int>(payment->method));
        if (methodIndex != -1) methodCombo.setCurrentIndex(methodIndex);
        transactionIdEdit.setText(QString::fromStdString(payment->transactionId.value_or("")));
        notesEdit.setText(QString::fromStdString(payment->notes.value_or("")));
        currencyEdit.setText(QString::fromStdString(payment->currency));

        paymentNumberEdit.setReadOnly(true); // Don't allow changing payment number for existing
    } else {
        paymentNumberEdit.setText("PAY-" + QString::fromStdString(ERP::Utils::generateUUID().substr(0, 8))); // Auto-generate
        amountEdit.setText("0.00");
        paymentDateEdit.setDateTime(QDateTime::currentDateTime());
        currencyEdit.setText("VND");
    }

    formLayout->addRow("Số Thanh toán:*", &paymentNumberEdit);
    formLayout->addRow("Khách hàng:*", &customerCombo);
    formLayout->addRow("Hóa đơn:*", &invoiceCombo);
    formLayout->addRow("Số tiền:*", &amountEdit);
    formLayout->addRow("Ngày Thanh toán:*", &paymentDateEdit);
    formLayout->addRow("Phương thức:*", &methodCombo);
    formLayout->addRow("ID Giao dịch:", &transactionIdEdit);
    formLayout->addRow("Ghi chú:", &notesEdit);
    formLayout->addRow("Tiền tệ:*", &currencyEdit);
    
    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(payment ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Sales::DTO::PaymentDTO newPaymentData;
        if (payment) {
            newPaymentData = *payment;
        }

        newPaymentData.paymentNumber = paymentNumberEdit.text().toStdString();
        newPaymentData.customerId = customerCombo.currentData().toString().toStdString();
        newPaymentData.invoiceId = invoiceCombo.currentData().toString().toStdString();
        newPaymentData.amount = amountEdit.text().toDouble();
        newPaymentData.paymentDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(paymentDateEdit.dateTime());
        newPaymentData.method = static_cast<ERP::Sales::DTO::PaymentMethod>(methodCombo.currentData().toInt());
        newPaymentData.transactionId = transactionIdEdit.text().isEmpty() ? std::nullopt : std::make_optional(transactionIdEdit.text().toStdString());
        newPaymentData.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());
        newPaymentData.currency = currencyEdit.text().toStdString();
        newPaymentData.status = ERP::Sales::DTO::PaymentStatus::PENDING; // Default status, service will finalize

        bool success = false;
        if (payment) {
            success = paymentService_->updatePayment(newPaymentData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Thanh Toán", "Thanh toán đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật thanh toán. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Sales::DTO::PaymentDTO> createdPayment = paymentService_->createPayment(newPaymentData, currentUserId_, currentUserRoleIds_);
            if (createdPayment) {
                showMessageBox("Thêm Thanh Toán", "Thanh toán mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm thanh toán mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadPayments();
            clearForm();
        }
    }
}


void PaymentManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool PaymentManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void PaymentManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Sales.CreatePayment");
    bool canUpdate = hasPermission("Sales.UpdatePayment");
    bool canDelete = hasPermission("Sales.DeletePayment");
    bool canChangeStatus = hasPermission("Sales.UpdatePaymentStatus");

    addPaymentButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Sales.ViewPayments"));

    bool isRowSelected = paymentTable_->currentRow() >= 0;
    editPaymentButton_->setEnabled(isRowSelected && canUpdate);
    deletePaymentButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);


    bool enableForm = isRowSelected && canUpdate;
    paymentNumberLineEdit_->setEnabled(enableForm); // Will be read-only for existing anyway
    customerComboBox_->setEnabled(enableForm);
    invoiceComboBox_->setEnabled(enableForm);
    amountLineEdit_->setEnabled(enableForm);
    paymentDateEdit_->setEnabled(enableForm);
    methodComboBox_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    transactionIdLineEdit_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);
    currencyLineEdit_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        paymentNumberLineEdit_->clear();
        customerComboBox_->clear();
        invoiceComboBox_->clear();
        amountLineEdit_->clear();
        paymentDateEdit_->clear();
        methodComboBox_->setCurrentIndex(0);
        statusComboBox_->setCurrentIndex(0);
        transactionIdLineEdit_->clear();
        notesLineEdit_->clear();
        currencyLineEdit_->clear();
    }
}


} // namespace Sales
} // namespace UI
} // namespace ERP