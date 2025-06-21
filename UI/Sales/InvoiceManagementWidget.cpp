// Modules/UI/Sales/InvoiceManagementWidget.cpp
#include "InvoiceManagementWidget.h"
#include "Invoice.h"
#include "InvoiceDetail.h"
#include "SalesOrder.h"
#include "Customer.h"
#include "Product.h"
#include "InvoiceService.h"
#include "SalesOrderService.h"
#include "CustomerService.h"
#include "ProductService.h"
#include "ISecurityManager.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "StringUtils.h"
#include "CustomMessageBox.h"
#include "UserService.h"

#include <QInputDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDoubleValidator>
#include <QDialogButtonBox>

namespace ERP {
namespace UI {
namespace Sales {

InvoiceManagementWidget::InvoiceManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IInvoiceService> invoiceService,
    std::shared_ptr<Services::ISalesOrderService> salesOrderService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      invoiceService_(invoiceService),
      salesOrderService_(salesOrderService),
      securityManager_(securityManager) {

    if (!invoiceService_ || !salesOrderService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ hóa đơn, đơn hàng bán hoặc bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("InvoiceManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("InvoiceManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("InvoiceManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadInvoices();
    updateButtonsState();
}

InvoiceManagementWidget::~InvoiceManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void InvoiceManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo số hóa đơn...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &InvoiceManagementWidget::onSearchInvoiceClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    invoiceTable_ = new QTableWidget(this);
    invoiceTable_->setColumnCount(10); // ID, Số hóa đơn, Khách hàng, Đơn hàng bán, Loại, Ngày hóa đơn, Ngày đáo hạn, Tổng tiền, Còn nợ, Trạng thái
    invoiceTable_->setHorizontalHeaderLabels({"ID", "Số HĐ", "Khách hàng", "Đơn hàng bán", "Loại", "Ngày HĐ", "Ngày Đáo hạn", "Tổng tiền", "Còn nợ", "Trạng thái"});
    invoiceTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    invoiceTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    invoiceTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    invoiceTable_->horizontalHeader()->setStretchLastSection(true);
    connect(invoiceTable_, &QTableWidget::itemClicked, this, &InvoiceManagementWidget::onInvoiceTableItemClicked);
    mainLayout->addWidget(invoiceTable_);

    // Form elements for editing/adding invoices
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    invoiceNumberLineEdit_ = new QLineEdit(this);
    customerComboBox_ = new QComboBox(this); populateCustomerComboBox();
    salesOrderComboBox_ = new QComboBox(this); populateSalesOrderComboBox();
    typeComboBox_ = new QComboBox(this); populateTypeComboBox();
    invoiceDateEdit_ = new QDateTimeEdit(this); invoiceDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    dueDateEdit_ = new QDateTimeEdit(this); dueDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();
    totalAmountLineEdit_ = new QLineEdit(this); totalAmountLineEdit_->setReadOnly(true); // Calculated, not directly editable
    totalAmountLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    totalDiscountLineEdit_ = new QLineEdit(this); totalDiscountLineEdit_->setReadOnly(true); // Calculated
    totalDiscountLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    totalTaxLineEdit_ = new QLineEdit(this); totalTaxLineEdit_->setReadOnly(true); // Calculated
    totalTaxLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    netAmountLineEdit_ = new QLineEdit(this); netAmountLineEdit_->setReadOnly(true); // Calculated
    netAmountLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    amountPaidLineEdit_ = new QLineEdit(this); amountPaidLineEdit_->setReadOnly(true); // Updated by payments
    amountPaidLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    amountDueLineEdit_ = new QLineEdit(this); amountDueLineEdit_->setReadOnly(true); // Calculated
    amountDueLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    currencyLineEdit_ = new QLineEdit(this);
    notesLineEdit_ = new QLineEdit(this);


    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("Số Hóa đơn:*", &invoiceNumberLineEdit_);
    formLayout->addRow("Khách hàng:*", &customerComboBox_);
    formLayout->addRow("Đơn hàng bán:*", &salesOrderComboBox_);
    formLayout->addRow("Loại:*", &typeComboBox_);
    formLayout->addRow("Ngày Hóa đơn:*", &invoiceDateEdit_);
    formLayout->addRow("Ngày Đáo hạn:*", &dueDateEdit_);
    formLayout->addRow("Trạng thái:*", &statusComboBox_);
    formLayout->addRow("Tổng tiền:", &totalAmountLineEdit_);
    formLayout->addRow("Tổng chiết khấu:", &totalDiscountLineEdit_);
    formLayout->addRow("Tổng thuế:", &totalTaxLineEdit_);
    formLayout->addRow("Số tiền ròng:", &netAmountLineEdit_);
    formLayout->addRow("Đã thanh toán:", &amountPaidLineEdit_);
    formLayout->addRow("Còn nợ:", &amountDueLineEdit_);
    formLayout->addRow("Tiền tệ:", &currencyLineEdit_);
    formLayout->addRow("Ghi chú:", &notesLineEdit_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addInvoiceButton_ = new QPushButton("Thêm mới", this); connect(addInvoiceButton_, &QPushButton::clicked, this, &InvoiceManagementWidget::onAddInvoiceClicked);
    editInvoiceButton_ = new QPushButton("Sửa", this); connect(editInvoiceButton_, &QPushButton::clicked, this, &InvoiceManagementWidget::onEditInvoiceClicked);
    deleteInvoiceButton_ = new QPushButton("Xóa", this); connect(deleteInvoiceButton_, &QPushButton::clicked, this, &InvoiceManagementWidget::onDeleteInvoiceClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &InvoiceManagementWidget::onUpdateInvoiceStatusClicked);
    manageDetailsButton_ = new QPushButton("Quản lý Chi tiết", this); connect(manageDetailsButton_, &QPushButton::clicked, this, &InvoiceManagementWidget::onManageDetailsClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &InvoiceManagementWidget::onSearchInvoiceClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &InvoiceManagementWidget::clearForm);
    
    buttonLayout->addWidget(addInvoiceButton_);
    buttonLayout->addWidget(editInvoiceButton_);
    buttonLayout->addWidget(deleteInvoiceButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(manageDetailsButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void InvoiceManagementWidget::loadInvoices() {
    ERP::Logger::Logger::getInstance().info("InvoiceManagementWidget: Loading invoices...");
    invoiceTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Sales::DTO::InvoiceDTO> invoices = invoiceService_->getAllInvoices({}, currentUserId_, currentUserRoleIds_);

    invoiceTable_->setRowCount(invoices.size());
    for (int i = 0; i < invoices.size(); ++i) {
        const auto& invoice = invoices[i];
        invoiceTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(invoice.id)));
        invoiceTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(invoice.invoiceNumber)));
        
        QString customerName = "N/A";
        std::optional<ERP::Customer::DTO::CustomerDTO> customer = securityManager_->getCustomerService()->getCustomerById(invoice.customerId, currentUserId_, currentUserRoleIds_);
        if (customer) customerName = QString::fromStdString(customer->name);
        invoiceTable_->setItem(i, 2, new QTableWidgetItem(customerName));

        QString salesOrderNumber = "N/A";
        std::optional<ERP::Sales::DTO::SalesOrderDTO> so = salesOrderService_->getSalesOrderById(invoice.salesOrderId, currentUserId_, currentUserRoleIds_);
        if (so) salesOrderNumber = QString::fromStdString(so->orderNumber);
        invoiceTable_->setItem(i, 3, new QTableWidgetItem(salesOrderNumber));

        invoiceTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(invoice.getTypeString())));
        invoiceTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(invoice.invoiceDate, ERP::Common::DATETIME_FORMAT))));
        invoiceTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(invoice.dueDate, ERP::Common::DATETIME_FORMAT))));
        invoiceTable_->setItem(i, 7, new QTableWidgetItem(QString::number(invoice.totalAmount, 'f', 2)));
        invoiceTable_->setItem(i, 8, new QTableWidgetItem(QString::number(invoice.amountDue, 'f', 2)));
        invoiceTable_->setItem(i, 9, new QTableWidgetItem(QString::fromStdString(invoice.getStatusString())));
    }
    invoiceTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("InvoiceManagementWidget: Invoices loaded successfully.");
}

void InvoiceManagementWidget::populateCustomerComboBox() {
    customerComboBox_->clear();
    std::vector<ERP::Customer::DTO::CustomerDTO> allCustomers = securityManager_->getCustomerService()->getAllCustomers({}, currentUserId_, currentUserRoleIds_);
    for (const auto& customer : allCustomers) {
        customerComboBox_->addItem(QString::fromStdString(customer.name), QString::fromStdString(customer.id));
    }
}

void InvoiceManagementWidget::populateSalesOrderComboBox() {
    salesOrderComboBox_->clear();
    salesOrderComboBox_->addItem("None", "");
    std::vector<ERP::Sales::DTO::SalesOrderDTO> allSalesOrders = salesOrderService_->getAllSalesOrders({}, currentUserId_, currentUserRoleIds_);
    for (const auto& so : allSalesOrders) {
        salesOrderComboBox_->addItem(QString::fromStdString(so.orderNumber), QString::fromStdString(so.id));
    }
}

void InvoiceManagementWidget::populateTypeComboBox() {
    typeComboBox_->clear();
    typeComboBox_->addItem("Sales Invoice", static_cast<int>(ERP::Sales::DTO::InvoiceType::SALES_INVOICE));
    typeComboBox_->addItem("Proforma Invoice", static_cast<int>(ERP::Sales::DTO::InvoiceType::PROFORMA_INVOICE));
    typeComboBox_->addItem("Credit Note", static_cast<int>(ERP::Sales::DTO::InvoiceType::CREDIT_NOTE));
    typeComboBox_->addItem("Debit Note", static_cast<int>(ERP::Sales::DTO::InvoiceType::DEBIT_NOTE));
}

void InvoiceManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Draft", static_cast<int>(ERP::Sales::DTO::InvoiceStatus::DRAFT));
    statusComboBox_->addItem("Issued", static_cast<int>(ERP::Sales::DTO::InvoiceStatus::ISSUED));
    statusComboBox_->addItem("Paid", static_cast<int>(ERP::Sales::DTO::InvoiceStatus::PAID));
    statusComboBox_->addItem("Partially Paid", static_cast<int>(ERP::Sales::DTO::InvoiceStatus::PARTIALLY_PAID));
    statusComboBox_->addItem("Cancelled", static_cast<int>(ERP::Sales::DTO::InvoiceStatus::CANCELLED));
    statusComboBox_->addItem("Overdue", static_cast<int>(ERP::Sales::DTO::InvoiceStatus::OVERDUE));
}


void InvoiceManagementWidget::onAddInvoiceClicked() {
    if (!hasPermission("Sales.CreateInvoice")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm hóa đơn.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateCustomerComboBox();
    populateSalesOrderComboBox();
    populateTypeComboBox();
    showInvoiceInputDialog();
}

void InvoiceManagementWidget::onEditInvoiceClicked() {
    if (!hasPermission("Sales.UpdateInvoice")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa hóa đơn.", QMessageBox::Warning);
        return;
    }

    int selectedRow = invoiceTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Hóa đơn", "Vui lòng chọn một hóa đơn để sửa.", QMessageBox::Information);
        return;
    }

    QString invoiceId = invoiceTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::InvoiceDTO> invoiceOpt = invoiceService_->getInvoiceById(invoiceId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (invoiceOpt) {
        populateCustomerComboBox();
        populateSalesOrderComboBox();
        populateTypeComboBox();
        showInvoiceInputDialog(&(*invoiceOpt));
    } else {
        showMessageBox("Sửa Hóa đơn", "Không tìm thấy hóa đơn để sửa.", QMessageBox::Critical);
    }
}

void InvoiceManagementWidget::onDeleteInvoiceClicked() {
    if (!hasPermission("Sales.DeleteInvoice")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa hóa đơn.", QMessageBox::Warning);
        return;
    }

    int selectedRow = invoiceTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Hóa đơn", "Vui lòng chọn một hóa đơn để xóa.", QMessageBox::Information);
        return;
    }

    QString invoiceId = invoiceTable_->item(selectedRow, 0)->text();
    QString invoiceNumber = invoiceTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Hóa đơn");
    confirmBox.setText("Bạn có chắc chắn muốn xóa hóa đơn '" + invoiceNumber + "' (ID: " + invoiceId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (invoiceService_->deleteInvoice(invoiceId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Hóa đơn", "Hóa đơn đã được xóa thành công.", QMessageBox::Information);
            loadInvoices();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa hóa đơn. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void InvoiceManagementWidget::onUpdateInvoiceStatusClicked() {
    if (!hasPermission("Sales.UpdateInvoiceStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái hóa đơn.", QMessageBox::Warning);
        return;
    }

    int selectedRow = invoiceTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một hóa đơn để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString invoiceId = invoiceTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::InvoiceDTO> invoiceOpt = invoiceService_->getInvoiceById(invoiceId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!invoiceOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy hóa đơn để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Sales::DTO::InvoiceDTO currentInvoice = *invoiceOpt;
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
    int currentStatusIndex = newStatusCombo.findData(static_cast<int>(currentInvoice.status));
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
        ERP::Sales::DTO::InvoiceStatus newStatus = static_cast<ERP::Sales::DTO::InvoiceStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái hóa đơn");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái hóa đơn '" + QString::fromStdString(currentInvoice.invoiceNumber) + "' thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (invoiceService_->updateInvoiceStatus(invoiceId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái hóa đơn đã được cập nhật thành công.", QMessageBox::Information);
                loadInvoices();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái hóa đơn. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}


void InvoiceManagementWidget::onSearchInvoiceClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["invoice_number_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    invoiceTable_->setRowCount(0);
    std::vector<ERP::Sales::DTO::InvoiceDTO> invoices = invoiceService_->getAllInvoices(filter, currentUserId_, currentUserRoleIds_);

    invoiceTable_->setRowCount(invoices.size());
    for (int i = 0; i < invoices.size(); ++i) {
        const auto& invoice = invoices[i];
        invoiceTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(invoice.id)));
        invoiceTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(invoice.invoiceNumber)));
        
        QString customerName = "N/A";
        std::optional<ERP::Customer::DTO::CustomerDTO> customer = securityManager_->getCustomerService()->getCustomerById(invoice.customerId, currentUserId_, currentUserRoleIds_);
        if (customer) customerName = QString::fromStdString(customer->name);
        invoiceTable_->setItem(i, 2, new QTableWidgetItem(customerName));

        QString salesOrderNumber = "N/A";
        std::optional<ERP::Sales::DTO::SalesOrderDTO> so = salesOrderService_->getSalesOrderById(invoice.salesOrderId, currentUserId_, currentUserRoleIds_);
        if (so) salesOrderNumber = QString::fromStdString(so->orderNumber);
        invoiceTable_->setItem(i, 3, new QTableWidgetItem(salesOrderNumber));

        invoiceTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(invoice.getTypeString())));
        invoiceTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(invoice.invoiceDate, ERP::Common::DATETIME_FORMAT))));
        invoiceTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(invoice.dueDate, ERP::Common::DATETIME_FORMAT))));
        invoiceTable_->setItem(i, 7, new QTableWidgetItem(QString::number(invoice.totalAmount, 'f', 2)));
        invoiceTable_->setItem(i, 8, new QTableWidgetItem(QString::number(invoice.amountDue, 'f', 2)));
        invoiceTable_->setItem(i, 9, new QTableWidgetItem(QString::fromStdString(invoice.getStatusString())));
    }
    invoiceTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("InvoiceManagementWidget: Search completed.");
}

void InvoiceManagementWidget::onInvoiceTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString invoiceId = invoiceTable_->item(row, 0)->text();
    std::optional<ERP::Sales::DTO::InvoiceDTO> invoiceOpt = invoiceService_->getInvoiceById(invoiceId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (invoiceOpt) {
        idLineEdit_->setText(QString::fromStdString(invoiceOpt->id));
        invoiceNumberLineEdit_->setText(QString::fromStdString(invoiceOpt->invoiceNumber));
        
        populateCustomerComboBox();
        int customerIndex = customerComboBox_->findData(QString::fromStdString(invoiceOpt->customerId));
        if (customerIndex != -1) customerComboBox_->setCurrentIndex(customerIndex);

        populateSalesOrderComboBox();
        int salesOrderIndex = salesOrderComboBox_->findData(QString::fromStdString(invoiceOpt->salesOrderId));
        if (salesOrderIndex != -1) salesOrderComboBox_->setCurrentIndex(salesOrderIndex);

        populateTypeComboBox();
        int typeIndex = typeComboBox_->findData(static_cast<int>(invoiceOpt->type));
        if (typeIndex != -1) typeComboBox_->setCurrentIndex(typeIndex);
        
        invoiceDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(invoiceOpt->invoiceDate.time_since_epoch()).count()));
        dueDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(invoiceOpt->dueDate.time_since_epoch()).count()));
        
        populateStatusComboBox();
        int statusIndex = statusComboBox_->findData(static_cast<int>(invoiceOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);

        totalAmountLineEdit_->setText(QString::number(invoiceOpt->totalAmount, 'f', 2));
        totalDiscountLineEdit_->setText(QString::number(invoiceOpt->totalDiscount, 'f', 2));
        totalTaxLineEdit_->setText(QString::number(invoiceOpt->totalTax, 'f', 2));
        netAmountLineEdit_->setText(QString::number(invoiceOpt->netAmount, 'f', 2));
        amountPaidLineEdit_->setText(QString::number(invoiceOpt->amountPaid, 'f', 2));
        amountDueLineEdit_->setText(QString::number(invoiceOpt->amountDue, 'f', 2));
        currencyLineEdit_->setText(QString::fromStdString(invoiceOpt->currency));
        notesLineEdit_->setText(QString::fromStdString(invoiceOpt->notes.value_or("")));

    } else {
        showMessageBox("Thông tin Hóa đơn", "Không thể tải chi tiết hóa đơn đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void InvoiceManagementWidget::clearForm() {
    idLineEdit_->clear();
    invoiceNumberLineEdit_->clear();
    customerComboBox_->clear();
    salesOrderComboBox_->clear();
    typeComboBox_->setCurrentIndex(0);
    invoiceDateEdit_->clear();
    dueDateEdit_->clear();
    statusComboBox_->setCurrentIndex(0);
    totalAmountLineEdit_->clear();
    totalDiscountLineEdit_->clear();
    totalTaxLineEdit_->clear();
    netAmountLineEdit_->clear();
    amountPaidLineEdit_->clear();
    amountDueLineEdit_->clear();
    currencyLineEdit_->clear();
    notesLineEdit_->clear();
    invoiceTable_->clearSelection();
    updateButtonsState();
}

void InvoiceManagementWidget::onManageDetailsClicked() {
    if (!hasPermission("Sales.ManageInvoiceDetails")) { // Assuming this permission
        showMessageBox("Lỗi", "Bạn không có quyền quản lý chi tiết hóa đơn.", QMessageBox::Warning);
        return;
    }

    int selectedRow = invoiceTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Quản lý Chi tiết", "Vui lòng chọn một hóa đơn để quản lý chi tiết.", QMessageBox::Information);
        return;
    }

    QString invoiceId = invoiceTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::InvoiceDTO> invoiceOpt = invoiceService_->getInvoiceById(invoiceId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (invoiceOpt) {
        showManageDetailsDialog(&(*invoiceOpt));
    } else {
        showMessageBox("Quản lý Chi tiết", "Không tìm thấy hóa đơn để quản lý chi tiết.", QMessageBox::Critical);
    }
}


void InvoiceManagementWidget::showInvoiceInputDialog(ERP::Sales::DTO::InvoiceDTO* invoice) {
    QDialog dialog(this);
    dialog.setWindowTitle(invoice ? "Sửa Hóa đơn" : "Thêm Hóa đơn Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit invoiceNumberEdit(this);
    QComboBox customerCombo(this); populateCustomerComboBox();
    for(int i = 0; i < customerComboBox_->count(); ++i) customerCombo.addItem(customerComboBox_->itemText(i), customerComboBox_->itemData(i));
    
    QComboBox salesOrderCombo(this); populateSalesOrderComboBox();
    for(int i = 0; i < salesOrderComboBox_->count(); ++i) salesOrderCombo.addItem(salesOrderComboBox_->itemText(i), salesOrderComboBox_->itemData(i));
    
    QComboBox typeCombo(this); populateTypeComboBox();
    for(int i = 0; i < typeComboBox_->count(); ++i) typeCombo.addItem(typeComboBox_->itemText(i), typeComboBox_->itemData(i));
    
    QDateTimeEdit invoiceDateEdit(this); invoiceDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QDateTimeEdit dueDateEdit(this); dueDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QComboBox statusCombo(this); populateStatusComboBox();
    for(int i = 0; i < statusComboBox_->count(); ++i) statusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));

    QLineEdit currencyEdit(this);
    QLineEdit notesEdit(this);

    if (invoice) {
        invoiceNumberEdit.setText(QString::fromStdString(invoice->invoiceNumber));
        int customerIndex = customerCombo.findData(QString::fromStdString(invoice->customerId));
        if (customerIndex != -1) customerCombo.setCurrentIndex(customerIndex);
        int salesOrderIndex = salesOrderCombo.findData(QString::fromStdString(invoice->salesOrderId));
        if (salesOrderIndex != -1) salesOrderCombo.setCurrentIndex(salesOrderIndex);
        int typeIndex = typeCombo.findData(static_cast<int>(invoice->type));
        if (typeIndex != -1) typeCombo.setCurrentIndex(typeIndex);
        invoiceDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(invoice->invoiceDate.time_since_epoch()).count()));
        dueDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(invoice->dueDate.time_since_epoch()).count()));
        int statusIndex = statusCombo.findData(static_cast<int>(invoice->status));
        if (statusIndex != -1) statusCombo.setCurrentIndex(statusIndex);
        currencyEdit.setText(QString::fromStdString(invoice->currency));
        notesEdit.setText(QString::fromStdString(invoice->notes.value_or("")));

        invoiceNumberEdit.setReadOnly(true); // Don't allow changing invoice number for existing
    } else {
        invoiceNumberEdit.setText("INV-" + QString::fromStdString(ERP::Utils::generateUUID().substr(0, 8))); // Auto-generate
        invoiceDateEdit.setDateTime(QDateTime::currentDateTime());
        dueDateEdit.setDateTime(QDateTime::currentDateTime().addMonths(1)); // Default 1 month due
        currencyEdit.setText("VND");
    }

    formLayout->addRow("Số Hóa đơn:*", &invoiceNumberEdit);
    formLayout->addRow("Khách hàng:*", &customerCombo);
    formLayout->addRow("Đơn hàng bán:*", &salesOrderCombo);
    formLayout->addRow("Loại:*", &typeCombo);
    formLayout->addRow("Ngày Hóa đơn:*", &invoiceDateEdit);
    formLayout->addRow("Ngày Đáo hạn:*", &dueDateEdit);
    formLayout->addRow("Trạng thái:*", &statusCombo);
    formLayout->addRow("Tiền tệ:", &currencyEdit);
    formLayout->addRow("Ghi chú:", &notesEdit);
    
    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(invoice ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Sales::DTO::InvoiceDTO newInvoiceData;
        if (invoice) {
            newInvoiceData = *invoice;
        }

        newInvoiceData.invoiceNumber = invoiceNumberEdit.text().toStdString();
        newInvoiceData.customerId = customerCombo.currentData().toString().toStdString();
        newInvoiceData.salesOrderId = salesOrderCombo.currentData().toString().toStdString();
        newInvoiceData.type = static_cast<ERP::Sales::DTO::InvoiceType>(typeCombo.currentData().toInt());
        newInvoiceData.invoiceDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(invoiceDateEdit.dateTime());
        newInvoiceData.dueDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(dueDateEdit.dateTime());
        newInvoiceData.status = static_cast<ERP::Sales::DTO::InvoiceStatus>(statusCombo.currentData().toInt());
        newInvoiceData.currency = currencyEdit.text().toStdString();
        newInvoiceData.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());

        // For new invoices, total amounts are often calculated from details after creation
        // For existing, these are updated from details/payments.
        // For simplicity in this input dialog, keep current values or set to 0.
        newInvoiceData.totalAmount = invoice ? invoice->totalAmount : 0.0;
        newInvoiceData.totalDiscount = invoice ? invoice->totalDiscount : 0.0;
        newInvoiceData.totalTax = invoice ? invoice->totalTax : 0.0;
        newInvoiceData.netAmount = invoice ? invoice->netAmount : 0.0;
        newInvoiceData.amountPaid = invoice ? invoice->amountPaid : 0.0;
        newInvoiceData.amountDue = invoice ? invoice->amountDue : 0.0;

        // For new slips, details are added in a separate dialog after creation
        std::vector<ERP::Sales::DTO::InvoiceDetailDTO> currentDetails;
        if (invoice) { // When editing, load existing details first
             currentDetails = invoiceService_->getInvoiceDetails(invoice->id, currentUserId_, currentUserRoleIds_);
        }

        bool success = false;
        if (invoice) {
            success = invoiceService_->updateInvoice(newInvoiceData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Hóa đơn", "Hóa đơn đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật hóa đơn. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Sales::DTO::InvoiceDTO> createdInvoice = invoiceService_->createInvoice(newInvoiceData, currentUserId_, currentUserRoleIds_); // Create with empty details
            if (createdInvoice) {
                showMessageBox("Thêm Hóa đơn", "Hóa đơn mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm hóa đơn mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadInvoices();
            clearForm();
        }
    }
}

void InvoiceManagementWidget::showManageDetailsDialog(ERP::Sales::DTO::InvoiceDTO* invoice) {
    if (!invoice) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Quản lý Chi tiết Hóa đơn: " + QString::fromStdString(invoice->invoiceNumber));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *detailsTable = new QTableWidget(&dialog);
    detailsTable->setColumnCount(7); // Product, Quantity, Unit Price, Discount, Tax Rate, Line Total, Notes
    detailsTable->setHorizontalHeaderLabels({"Sản phẩm", "SL", "Đơn giá", "CK", "Loại CK", "Thuế suất", "Tổng dòng", "Ghi chú"});
    detailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    detailsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    detailsTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(detailsTable);

    // Load existing details
    std::vector<ERP::Sales::DTO::InvoiceDetailDTO> currentDetails = invoiceService_->getInvoiceDetails(invoice->id, currentUserId_, currentUserRoleIds_);
    detailsTable->setRowCount(currentDetails.size());
    for (int i = 0; i < currentDetails.size(); ++i) {
        const auto& detail = currentDetails[i];
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = securityManager_->getProductService()->getProductById(detail.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);

        detailsTable->setItem(i, 0, new QTableWidgetItem(productName));
        detailsTable->setItem(i, 1, new QTableWidgetItem(QString::number(detail.quantity)));
        detailsTable->setItem(i, 2, new QTableWidgetItem(QString::number(detail.unitPrice, 'f', 2)));
        detailsTable->setItem(i, 3, new QTableWidgetItem(QString::number(detail.discount, 'f', 2)));
        detailsTable->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(detail.discountType == ERP::Sales::DTO::DiscountType::PERCENTAGE ? "Phần trăm" : "Số tiền cố định")));
        detailsTable->setItem(i, 5, new QTableWidgetItem(QString::number(detail.taxRate, 'f', 2)));
        detailsTable->setItem(i, 6, new QTableWidgetItem(QString::number(detail.lineTotal, 'f', 2)));
        detailsTable->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(detail.notes.value_or(""))));
        detailsTable->item(i, 0)->setData(Qt::UserRole, QString::fromStdString(detail.id)); // Store detail ID
    }


    QHBoxLayout *itemButtonsLayout = new QHBoxLayout();
    QPushButton *addItemButton = new QPushButton("Thêm Chi tiết", &dialog);
    QPushButton *editItemButton = new QPushButton("Sửa Chi tiết", &dialog);
    QPushButton *deleteItemButton = new QPushButton("Xóa Chi tiết", &dialog);
    itemButtonsLayout->addWidget(addItemButton);
    itemButtonsLayout->addWidget(editItemButton);
    itemButtonsLayout->addWidget(deleteItemButton);
    dialogLayout->addLayout(itemButtonsLayout);

    QPushButton *saveButton = new QPushButton("Lưu", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *actionButtonsLayout = new QHBoxLayout();
    actionButtonsLayout->addWidget(saveButton);
    actionButtonsLayout->addWidget(cancelButton);
    dialogLayout->addLayout(actionButtonsLayout);

    // Connect item management buttons (logic will be in lambdas for simplicity)
    connect(addItemButton, &QPushButton::clicked, [&]() {
        QDialog itemDialog(&dialog);
        itemDialog.setWindowTitle("Thêm Chi tiết Hóa đơn");
        QFormLayout itemFormLayout;
        QComboBox productCombo;
        std::vector<ERP::Product::DTO::ProductDTO> allProducts = securityManager_->getProductService()->getAllProducts({}, currentUserId_, currentUserRoleIds_);
        for (const auto& prod : allProducts) productCombo.addItem(QString::fromStdString(prod.name), QString::fromStdString(prod.id));

        QLineEdit quantityEdit; quantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit unitPriceEdit; unitPriceEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit discountEdit; discountEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QComboBox discountTypeCombo;
        discountTypeCombo.addItem("Fixed Amount", static_cast<int>(ERP::Sales::DTO::DiscountType::FIXED_AMOUNT));
        discountTypeCombo.addItem("Percentage", static_cast<int>(ERP::Sales::DTO::DiscountType::PERCENTAGE));
        QLineEdit taxRateEdit; taxRateEdit.setValidator(new QDoubleValidator(0.0, 100.0, 2, &itemDialog)); // 0-100%
        QLineEdit notesEdit;

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Số lượng:*", &quantityEdit);
        itemFormLayout.addRow("Đơn giá:*", &unitPriceEdit);
        itemFormLayout.addRow("Chiết khấu:", &discountEdit);
        itemFormLayout.addRow("Loại chiết khấu:", &discountTypeCombo);
        itemFormLayout.addRow("Thuế suất (%):*", &taxRateEdit);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);

        QPushButton *okItemButton = new QPushButton("Thêm", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (productCombo.currentData().isNull() || quantityEdit.text().isEmpty() || unitPriceEdit.text().isEmpty() || taxRateEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Calculate lineTotal here (simplified)
            double quantity = quantityEdit.text().toDouble();
            double unitPrice = unitPriceEdit.text().toDouble();
            double discount = discountEdit.text().toDouble();
            ERP::Sales::DTO::DiscountType discountType = static_cast<ERP::Sales::DTO::DiscountType>(discountTypeCombo.currentData().toInt());
            double taxRate = taxRateEdit.text().toDouble();

            double effectivePrice = unitPrice;
            if (discountType == ERP::Sales::DTO::DiscountType::PERCENTAGE) {
                effectivePrice *= (1 - discount / 100.0);
            } else {
                effectivePrice -= discount;
            }
            double lineTotal = (effectivePrice * quantity) * (1 + taxRate / 100.0);

            // Add to table
            int newRow = detailsTable->rowCount();
            detailsTable->insertRow(newRow);
            detailsTable->setItem(newRow, 0, new QTableWidgetItem(productCombo.currentText()));
            detailsTable->setItem(newRow, 1, new QTableWidgetItem(quantityEdit.text()));
            detailsTable->setItem(newRow, 2, new QTableWidgetItem(unitPriceEdit.text()));
            detailsTable->setItem(newRow, 3, new QTableWidgetItem(discountEdit.text()));
            detailsTable->setItem(newRow, 4, new QTableWidgetItem(discountTypeCombo.currentText()));
            detailsTable->setItem(newRow, 5, new QTableWidgetItem(taxRateEdit.text()));
            detailsTable->setItem(newRow, 6, new QTableWidgetItem(QString::number(lineTotal, 'f', 2)));
            detailsTable->setItem(newRow, 7, new QTableWidgetItem(notesEdit.text()));
            detailsTable->item(newRow, 0)->setData(Qt::UserRole, productCombo.currentData()); // Store product ID
            detailsTable->item(newRow, 4)->setData(Qt::UserRole, discountTypeCombo.currentData()); // Store discount type ID
        }
    });

    connect(editItemButton, &QPushButton::clicked, [&]() {
        int selectedItemRow = detailsTable->currentRow();
        if (selectedItemRow < 0) {
            showMessageBox("Sửa Chi tiết", "Vui lòng chọn một chi tiết để sửa.", QMessageBox::Information);
            return;
        }

        QDialog itemDialog(&dialog);
        itemDialog.setWindowTitle("Sửa Chi tiết Hóa đơn");
        QFormLayout itemFormLayout;
        QComboBox productCombo;
        std::vector<ERP::Product::DTO::ProductDTO> allProducts = securityManager_->getProductService()->getAllProducts({}, currentUserId_, currentUserRoleIds_);
        for (const auto& prod : allProducts) productCombo.addItem(QString::fromStdString(prod.name), QString::fromStdString(prod.id));

        QLineEdit quantityEdit; quantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit unitPriceEdit; unitPriceEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit discountEdit; discountEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QComboBox discountTypeCombo;
        discountTypeCombo.addItem("Fixed Amount", static_cast<int>(ERP::Sales::DTO::DiscountType::FIXED_AMOUNT));
        discountTypeCombo.addItem("Percentage", static_cast<int>(ERP::Sales::DTO::DiscountType::PERCENTAGE));
        QLineEdit taxRateEdit; taxRateEdit.setValidator(new QDoubleValidator(0.0, 100.0, 2, &itemDialog));
        QLineEdit notesEdit;

        // Populate with current item data
        QString currentProductId = detailsTable->item(selectedItemRow, 0)->data(Qt::UserRole).toString();
        int productIndex = productCombo.findData(currentProductId);
        if (productIndex != -1) productCombo.setCurrentIndex(productIndex);

        quantityEdit.setText(detailsTable->item(selectedItemRow, 1)->text());
        unitPriceEdit.setText(detailsTable->item(selectedItemRow, 2)->text());
        discountEdit.setText(detailsTable->item(selectedItemRow, 3)->text());
        
        QString currentDiscountTypeText = detailsTable->item(selectedItemRow, 4)->text();
        int discountTypeIndex = discountTypeCombo.findText(currentDiscountTypeText);
        if (discountTypeIndex != -1) discountTypeCombo.setCurrentIndex(discountTypeIndex);

        taxRateEdit.setText(detailsTable->item(selectedItemRow, 5)->text());
        notesEdit.setText(detailsTable->item(selectedItemRow, 7)->text());

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Số lượng:*", &quantityEdit);
        itemFormLayout.addRow("Đơn giá:*", &unitPriceEdit);
        itemFormLayout.addRow("Chiết khấu:", &discountEdit);
        itemFormLayout.addRow("Loại chiết khấu:", &discountTypeCombo);
        itemFormLayout.addRow("Thuế suất (%):*", &taxRateEdit);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);

        QPushButton *okItemButton = new QPushButton("Lưu", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (productCombo.currentData().isNull() || quantityEdit.text().isEmpty() || unitPriceEdit.text().isEmpty() || taxRateEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Recalculate lineTotal
            double quantity = quantityEdit.text().toDouble();
            double unitPrice = unitPriceEdit.text().toDouble();
            double discount = discountEdit.text().toDouble();
            ERP::Sales::DTO::DiscountType discountType = static_cast<ERP::Sales::DTO::DiscountType>(discountTypeCombo.currentData().toInt());
            double taxRate = taxRateEdit.text().toDouble();

            double effectivePrice = unitPrice;
            if (discountType == ERP::Sales::DTO::DiscountType::PERCENTAGE) {
                effectivePrice *= (1 - discount / 100.0);
            } else {
                effectivePrice -= discount;
            }
            double lineTotal = (effectivePrice * quantity) * (1 + taxRate / 100.0);

            // Update table row
            detailsTable->item(selectedItemRow, 0)->setText(productCombo.currentText());
            detailsTable->item(selectedItemRow, 1)->setText(quantityEdit.text());
            detailsTable->item(selectedItemRow, 2)->setText(unitPriceEdit.text());
            detailsTable->item(selectedItemRow, 3)->setText(discountEdit.text());
            detailsTable->item(selectedItemRow, 4)->setText(discountTypeCombo.currentText());
            detailsTable->item(selectedItemRow, 5)->setText(taxRateEdit.text());
            detailsTable->item(selectedItemRow, 6)->setText(QString::number(lineTotal, 'f', 2));
            detailsTable->item(selectedItemRow, 7)->setText(notesEdit.text());
            detailsTable->item(selectedItemRow, 0)->setData(Qt::UserRole, productCombo.currentData());
            detailsTable->item(selectedItemRow, 4)->setData(Qt::UserRole, discountTypeCombo.currentData());
        }
    });

    connect(deleteItemButton, &QPushButton::clicked, [&]() {
        int selectedItemRow = detailsTable->currentRow();
        if (selectedItemRow < 0) {
            showMessageBox("Xóa Chi tiết", "Vui lòng chọn một chi tiết để xóa.", QMessageBox::Information);
            return;
        }
        Common::CustomMessageBox confirmDelBox(&dialog);
        confirmDelBox.setWindowTitle("Xóa Chi tiết Hóa đơn");
        confirmDelBox.setText("Bạn có chắc chắn muốn xóa chi tiết hóa đơn này?");
        confirmDelBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmDelBox.exec() == QMessageBox::Yes) {
            detailsTable->removeRow(selectedItemRow);
        }
    });

    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Collect all items from the table
        std::vector<ERP::Sales::DTO::InvoiceDetailDTO> updatedDetails;
        for (int i = 0; i < detailsTable->rowCount(); ++i) {
            ERP::Sales::DTO::InvoiceDetailDTO detail;
            // Item ID is stored in product item's UserRole data
            QString existingId = detailsTable->item(i, 0)->data(Qt::UserRole).toString();
            detail.id = existingId.isEmpty() ? ERP::Utils::generateUUID() : existingId.toStdString();

            detail.invoiceId = invoice->id;
            detail.productId = detailsTable->item(i, 0)->data(Qt::UserRole).toString().toStdString();
            detail.quantity = detailsTable->item(i, 1)->text().toDouble();
            detail.unitPrice = detailsTable->item(i, 2)->text().toDouble();
            detail.discount = detailsTable->item(i, 3)->text().toDouble();
            detail.discountType = static_cast<ERP::Sales::DTO::DiscountType>(detailsTable->item(i, 4)->data(Qt::UserRole).toInt());
            detail.taxRate = detailsTable->item(i, 5)->text().toDouble();
            detail.lineTotal = detailsTable->item(i, 6)->text().toDouble();
            detail.notes = detailsTable->item(i, 7)->text().isEmpty() ? std::nullopt : std::make_optional(detailsTable->item(i, 7)->text().toStdString());
            
            updatedDetails.push_back(detail);
        }

        // Call service to update Invoice with new detail list
        if (invoiceService_->updateInvoice(*invoice, updatedDetails, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Quản lý Chi tiết", "Chi tiết hóa đơn đã được cập nhật thành công.", QMessageBox::Information);
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật chi tiết hóa đơn. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void InvoiceManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool InvoiceManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void InvoiceManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Sales.CreateInvoice");
    bool canUpdate = hasPermission("Sales.UpdateInvoice");
    bool canDelete = hasPermission("Sales.DeleteInvoice");
    bool canChangeStatus = hasPermission("Sales.UpdateInvoiceStatus");
    bool canManageDetails = hasPermission("Sales.ManageInvoiceDetails"); // Assuming this permission

    addInvoiceButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Sales.ViewInvoices"));

    bool isRowSelected = invoiceTable_->currentRow() >= 0;
    editInvoiceButton_->setEnabled(isRowSelected && canUpdate);
    deleteInvoiceButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    manageDetailsButton_->setEnabled(isRowSelected && canManageDetails);


    bool enableForm = isRowSelected && canUpdate;
    invoiceNumberLineEdit_->setEnabled(enableForm); // Will be read-only for existing anyway
    customerComboBox_->setEnabled(enableForm);
    salesOrderComboBox_->setEnabled(enableForm);
    typeComboBox_->setEnabled(enableForm);
    invoiceDateEdit_->setEnabled(enableForm);
    dueDateEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    currencyLineEdit_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);

    // Calculated read-only fields
    totalAmountLineEdit_->setEnabled(false);
    totalDiscountLineEdit_->setEnabled(false);
    totalTaxLineEdit_->setEnabled(false);
    netAmountLineEdit_->setEnabled(false);
    amountPaidLineEdit_->setEnabled(false);
    amountDueLineEdit_->setEnabled(false);

    if (!isRowSelected) {
        idLineEdit_->clear();
        invoiceNumberLineEdit_->clear();
        customerComboBox_->clear();
        salesOrderComboBox_->clear();
        typeComboBox_->setCurrentIndex(0);
        invoiceDateEdit_->clear();
        dueDateEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
        totalAmountLineEdit_->clear();
        totalDiscountLineEdit_->clear();
        totalTaxLineEdit_->clear();
        netAmountLineEdit_->clear();
        amountPaidLineEdit_->clear();
        amountDueLineEdit_->clear();
        currencyLineEdit_->clear();
        notesLineEdit_->clear();
    }
}


} // namespace Sales
} // namespace UI
} // namespace ERP