// Modules/UI/Sales/QuotationManagementWidget.cpp
#include "QuotationManagementWidget.h"
#include "Quotation.h"
#include "QuotationDetail.h"
#include "Customer.h"
#include "Product.h"
#include "UnitOfMeasure.h"
#include "SalesOrder.h"
#include "SalesOrderDetail.h" // For converting to Sales Order
#include "QuotationService.h"
#include "CustomerService.h"
#include "ProductService.h"
#include "UnitOfMeasureService.h"
#include "SalesOrderService.h"
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

QuotationManagementWidget::QuotationManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IQuotationService> quotationService,
    std::shared_ptr<Customer::Services::ICustomerService> customerService,
    std::shared_ptr<Product::Services::IProductService> productService,
    std::shared_ptr<Catalog::Services::IUnitOfMeasureService> unitOfMeasureService,
    std::shared_ptr<Services::ISalesOrderService> salesOrderService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      quotationService_(quotationService),
      customerService_(customerService),
      productService_(productService),
      unitOfMeasureService_(unitOfMeasureService),
      salesOrderService_(salesOrderService),
      securityManager_(securityManager) {

    if (!quotationService_ || !customerService_ || !productService_ || !unitOfMeasureService_ || !salesOrderService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ báo giá, khách hàng, sản phẩm, đơn vị đo, đơn hàng bán hoặc bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("QuotationManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("QuotationManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("QuotationManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadQuotations();
    updateButtonsState();
}

QuotationManagementWidget::~QuotationManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void QuotationManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo số báo giá...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &QuotationManagementWidget::onSearchQuotationClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    quotationTable_ = new QTableWidget(this);
    quotationTable_->setColumnCount(8); // ID, Số báo giá, Khách hàng, Ngày báo giá, Ngày hiệu lực, Tổng tiền, Trạng thái
    quotationTable_->setHorizontalHeaderLabels({"ID", "Số Báo giá", "Khách hàng", "Ngày Báo giá", "Ngày Hiệu lực", "Tổng tiền", "Còn nợ", "Trạng thái"});
    quotationTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    quotationTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    quotationTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    quotationTable_->horizontalHeader()->setStretchLastSection(true);
    connect(quotationTable_, &QTableWidget::itemClicked, this, &QuotationManagementWidget::onQuotationTableItemClicked);
    mainLayout->addWidget(quotationTable_);

    // Form elements for editing/adding quotations
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    quotationNumberLineEdit_ = new QLineEdit(this);
    customerComboBox_ = new QComboBox(this); populateCustomerComboBox();
    requestedByLineEdit_ = new QLineEdit(this); requestedByLineEdit_->setReadOnly(true); // Should be current user
    quotationDateEdit_ = new QDateTimeEdit(this); quotationDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    validUntilDateEdit_ = new QDateTimeEdit(this); validUntilDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();
    totalAmountLineEdit_ = new QLineEdit(this); totalAmountLineEdit_->setReadOnly(true); // Calculated
    totalAmountLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    totalDiscountLineEdit_ = new QLineEdit(this); totalDiscountLineEdit_->setReadOnly(true); // Calculated
    totalDiscountLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    totalTaxLineEdit_ = new QLineEdit(this); totalTaxLineEdit_->setReadOnly(true); // Calculated
    totalTaxLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    netAmountLineEdit_ = new QLineEdit(this); netAmountLineEdit_->setReadOnly(true); // Calculated
    netAmountLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    currencyLineEdit_ = new QLineEdit(this);
    paymentTermsLineEdit_ = new QLineEdit(this);
    deliveryTermsLineEdit_ = new QLineEdit(this);
    notesLineEdit_ = new QLineEdit(this);


    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("Số Báo giá:*", &quotationNumberLineEdit_);
    formLayout->addRow("Khách hàng:*", &customerComboBox_);
    formLayout->addRow("Người yêu cầu:", &requestedByLineEdit_);
    formLayout->addRow("Ngày Báo giá:*", &quotationDateEdit_);
    formLayout->addRow("Ngày Hiệu lực:*", &validUntilDateEdit_);
    formLayout->addRow("Trạng thái:*", &statusComboBox_);
    formLayout->addRow("Tổng tiền:", &totalAmountLineEdit_);
    formLayout->addRow("Tổng chiết khấu:", &totalDiscountLineEdit_);
    formLayout->addRow("Tổng thuế:", &totalTaxLineEdit_);
    formLayout->addRow("Số tiền ròng:", &netAmountLineEdit_);
    formLayout->addRow("Tiền tệ:", &currencyLineEdit_);
    formLayout->addRow("Điều khoản TT:", &paymentTermsLineEdit_);
    formLayout->addRow("Điều khoản GH:", &deliveryTermsLineEdit_);
    formLayout->addRow("Ghi chú:", &notesLineEdit_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addQuotationButton_ = new QPushButton("Thêm mới", this); connect(addQuotationButton_, &QPushButton::clicked, this, &QuotationManagementWidget::onAddQuotationClicked);
    editQuotationButton_ = new QPushButton("Sửa", this); connect(editQuotationButton_, &QPushButton::clicked, this, &QuotationManagementWidget::onEditQuotationClicked);
    deleteQuotationButton_ = new QPushButton("Xóa", this); connect(deleteQuotationButton_, &QPushButton::clicked, this, &QuotationManagementWidget::onDeleteQuotationClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &QuotationManagementWidget::onUpdateQuotationStatusClicked);
    manageDetailsButton_ = new QPushButton("Quản lý Chi tiết", this); connect(manageDetailsButton_, &QPushButton::clicked, this, &QuotationManagementWidget::onManageDetailsClicked);
    convertToSalesOrderButton_ = new QPushButton("Chuyển thành Đơn hàng bán", this); connect(convertToSalesOrderButton_, &QPushButton::clicked, this, &QuotationManagementWidget::onConvertToSalesOrderClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &QuotationManagementWidget::onSearchQuotationClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &QuotationManagementWidget::clearForm);
    
    buttonLayout->addWidget(addQuotationButton_);
    buttonLayout->addWidget(editQuotationButton_);
    buttonLayout->addWidget(deleteQuotationButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(manageDetailsButton_);
    buttonLayout->addWidget(convertToSalesOrderButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void QuotationManagementWidget::loadQuotations() {
    ERP::Logger::Logger::getInstance().info("QuotationManagementWidget: Loading quotations...");
    quotationTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Sales::DTO::QuotationDTO> quotations = quotationService_->getAllQuotations({}, currentUserId_, currentUserRoleIds_);

    quotationTable_->setRowCount(quotations.size());
    for (int i = 0; i < quotations.size(); ++i) {
        const auto& quotation = quotations[i];
        quotationTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(quotation.id)));
        quotationTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(quotation.quotationNumber)));
        
        QString customerName = "N/A";
        std::optional<ERP::Customer::DTO::CustomerDTO> customer = securityManager_->getCustomerService()->getCustomerById(quotation.customerId, currentUserId_, currentUserRoleIds_);
        if (customer) customerName = QString::fromStdString(customer->name);
        quotationTable_->setItem(i, 2, new QTableWidgetItem(customerName));

        quotationTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(quotation.quotationDate, ERP::Common::DATETIME_FORMAT))));
        quotationTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(quotation.validUntilDate, ERP::Common::DATETIME_FORMAT))));
        quotationTable_->setItem(i, 5, new QTableWidgetItem(QString::number(quotation.totalAmount, 'f', 2) + " " + QString::fromStdString(quotation.currency)));
        quotationTable_->setItem(i, 6, new QTableWidgetItem(QString::number(quotation.netAmount, 'f', 2) + " " + QString::fromStdString(quotation.currency)));
        quotationTable_->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(quotation.getStatusString())));
    }
    quotationTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("QuotationManagementWidget: Quotations loaded successfully.");
}

void QuotationManagementWidget::populateCustomerComboBox() {
    customerComboBox_->clear();
    std::vector<ERP::Customer::DTO::CustomerDTO> allCustomers = securityManager_->getCustomerService()->getAllCustomers({}, currentUserId_, currentUserRoleIds_);
    for (const auto& customer : allCustomers) {
        customerComboBox_->addItem(QString::fromStdString(customer.name), QString::fromStdString(customer.id));
    }
}

void QuotationManagementWidget::populateProductComboBox() {
    // Used in details dialog
}

void QuotationManagementWidget::populateUnitOfMeasureComboBox() {
    // Used in details dialog
}

void QuotationManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Draft", static_cast<int>(ERP::Sales::DTO::QuotationStatus::DRAFT));
    statusComboBox_->addItem("Sent", static_cast<int>(ERP::Sales::DTO::QuotationStatus::SENT));
    statusComboBox_->addItem("Accepted", static_cast<int>(ERP::Sales::DTO::QuotationStatus::ACCEPTED));
    statusComboBox_->addItem("Rejected", static_cast<int>(ERP::Sales::DTO::QuotationStatus::REJECTED));
    statusComboBox_->addItem("Expired", static_cast<int>(ERP::Sales::DTO::QuotationStatus::EXPIRED));
    statusComboBox_->addItem("Cancelled", static_cast<int>(ERP::Sales::DTO::QuotationStatus::CANCELLED));
}


void QuotationManagementWidget::onAddQuotationClicked() {
    if (!hasPermission("Sales.CreateQuotation")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm báo giá.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateCustomerComboBox();
    showQuotationInputDialog();
}

void QuotationManagementWidget::onEditQuotationClicked() {
    if (!hasPermission("Sales.UpdateQuotation")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa báo giá.", QMessageBox::Warning);
        return;
    }

    int selectedRow = quotationTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Báo giá", "Vui lòng chọn một báo giá để sửa.", QMessageBox::Information);
        return;
    }

    QString quotationId = quotationTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::QuotationDTO> quotationOpt = quotationService_->getQuotationById(quotationId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (quotationOpt) {
        populateCustomerComboBox();
        showQuotationInputDialog(&(*quotationOpt));
    } else {
        showMessageBox("Sửa Báo giá", "Không tìm thấy báo giá để sửa.", QMessageBox::Critical);
    }
}

void QuotationManagementWidget::onDeleteQuotationClicked() {
    if (!hasPermission("Sales.DeleteQuotation")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa báo giá.", QMessageBox::Warning);
        return;
    }

    int selectedRow = quotationTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Báo giá", "Vui lòng chọn một báo giá để xóa.", QMessageBox::Information);
        return;
    }

    QString quotationId = quotationTable_->item(selectedRow, 0)->text();
    QString quotationNumber = quotationTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Báo giá");
    confirmBox.setText("Bạn có chắc chắn muốn xóa báo giá '" + quotationNumber + "' (ID: " + quotationId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (quotationService_->deleteQuotation(quotationId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Báo giá", "Báo giá đã được xóa thành công.", QMessageBox::Information);
            loadQuotations();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa báo giá. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void QuotationManagementWidget::onUpdateQuotationStatusClicked() {
    if (!hasPermission("Sales.UpdateQuotationStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái báo giá.", QMessageBox::Warning);
        return;
    }

    int selectedRow = quotationTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một báo giá để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString quotationId = quotationTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::QuotationDTO> quotationOpt = quotationService_->getQuotationById(quotationId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!quotationOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy báo giá để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Sales::DTO::QuotationDTO currentQuotation = *quotationOpt;
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
    int currentStatusIndex = newStatusCombo.findData(static_cast<int>(currentQuotation.status));
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
        ERP::Sales::DTO::QuotationStatus newStatus = static_cast<ERP::Sales::DTO::QuotationStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái báo giá");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái báo giá '" + QString::fromStdString(currentQuotation.quotationNumber) + "' thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (quotationService_->updateQuotationStatus(quotationId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái báo giá đã được cập nhật thành công.", QMessageBox::Information);
                loadQuotations();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái báo giá. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}

void QuotationManagementWidget::onManageDetailsClicked() {
    if (!hasPermission("Sales.ManageQuotationDetails")) { // Assuming this permission
        showMessageBox("Lỗi", "Bạn không có quyền quản lý chi tiết báo giá.", QMessageBox::Warning);
        return;
    }

    int selectedRow = quotationTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Quản lý Chi tiết", "Vui lòng chọn một báo giá để quản lý chi tiết.", QMessageBox::Information);
        return;
    }

    QString quotationId = quotationTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::QuotationDTO> quotationOpt = quotationService_->getQuotationById(quotationId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (quotationOpt) {
        showManageDetailsDialog(&(*quotationOpt));
    } else {
        showMessageBox("Quản lý Chi tiết", "Không tìm thấy báo giá để quản lý chi tiết.", QMessageBox::Critical);
    }
}

void QuotationManagementWidget::onConvertToSalesOrderClicked() {
    if (!hasPermission("Sales.ConvertQuotationToSalesOrder")) {
        showMessageBox("Lỗi", "Bạn không có quyền chuyển đổi báo giá thành đơn hàng bán.", QMessageBox::Warning);
        return;
    }

    int selectedRow = quotationTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Chuyển đổi thành Đơn hàng bán", "Vui lòng chọn một báo giá để chuyển đổi.", QMessageBox::Information);
        return;
    }

    QString quotationId = quotationTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::QuotationDTO> quotationOpt = quotationService_->getQuotationById(quotationId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!quotationOpt) {
        showMessageBox("Chuyển đổi thành Đơn hàng bán", "Không tìm thấy báo giá để chuyển đổi.", QMessageBox::Critical);
        return;
    }

    ERP::Sales::DTO::QuotationDTO currentQuotation = *quotationOpt;
    if (currentQuotation.status != ERP::Sales::DTO::QuotationStatus::ACCEPTED) {
        showMessageBox("Chuyển đổi thành Đơn hàng bán", "Chỉ có thể chuyển đổi báo giá ở trạng thái 'Accepted' thành đơn hàng bán. Trạng thái hiện tại là: " + QString::fromStdString(currentQuotation.getStatusString()), QMessageBox::Warning);
        return;
    }

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Chuyển đổi Báo giá");
    confirmBox.setText("Bạn có chắc chắn muốn chuyển đổi báo giá '" + QString::fromStdString(currentQuotation.quotationNumber) + "' thành Đơn hàng bán không?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        std::optional<ERP::Sales::DTO::SalesOrderDTO> newSalesOrder = quotationService_->convertQuotationToSalesOrder(quotationId.toStdString(), currentUserId_, currentUserRoleIds_);
        if (newSalesOrder) {
            showMessageBox("Chuyển đổi thành Đơn hàng bán", "Báo giá đã được chuyển đổi thành Đơn hàng bán thành công: " + QString::fromStdString(newSalesOrder->orderNumber), QMessageBox::Information);
            loadQuotations(); // Refresh quotation table
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể chuyển đổi báo giá thành đơn hàng bán. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void QuotationManagementWidget::onSearchQuotationClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["quotation_number_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    quotationTable_->setRowCount(0);
    std::vector<ERP::Sales::DTO::QuotationDTO> quotations = quotationService_->getAllQuotations(filter, currentUserId_, currentUserRoleIds_);

    quotationTable_->setRowCount(quotations.size());
    for (int i = 0; i < quotations.size(); ++i) {
        const auto& quotation = quotations[i];
        quotationTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(quotation.id)));
        quotationTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(quotation.quotationNumber)));
        
        QString customerName = "N/A";
        std::optional<ERP::Customer::DTO::CustomerDTO> customer = securityManager_->getCustomerService()->getCustomerById(quotation.customerId, currentUserId_, currentUserRoleIds_);
        if (customer) customerName = QString::fromStdString(customer->name);
        quotationTable_->setItem(i, 2, new QTableWidgetItem(customerName));

        quotationTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(quotation.quotationDate, ERP::Common::DATETIME_FORMAT))));
        quotationTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(quotation.validUntilDate, ERP::Common::DATETIME_FORMAT))));
        quotationTable_->setItem(i, 5, new QTableWidgetItem(QString::number(quotation.totalAmount, 'f', 2) + " " + QString::fromStdString(quotation.currency)));
        quotationTable_->setItem(i, 6, new QTableWidgetItem(QString::number(quotation.netAmount, 'f', 2) + " " + QString::fromStdString(quotation.currency)));
        quotationTable_->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(quotation.getStatusString())));
    }
    quotationTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("QuotationManagementWidget: Search completed.");
}

void QuotationManagementWidget::onQuotationTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString quotationId = quotationTable_->item(row, 0)->text();
    std::optional<ERP::Sales::DTO::QuotationDTO> quotationOpt = quotationService_->getQuotationById(quotationId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (quotationOpt) {
        idLineEdit_->setText(QString::fromStdString(quotationOpt->id));
        quotationNumberLineEdit_->setText(QString::fromStdString(quotationOpt->quotationNumber));
        
        populateCustomerComboBox();
        int customerIndex = customerComboBox_->findData(QString::fromStdString(quotationOpt->customerId));
        if (customerIndex != -1) customerComboBox_->setCurrentIndex(customerIndex);

        requestedByLineEdit_->setText(QString::fromStdString(quotationOpt->requestedByUserId)); // Display ID
        quotationDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(quotationOpt->quotationDate.time_since_epoch()).count()));
        validUntilDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(quotationOpt->validUntilDate.time_since_epoch()).count()));
        
        populateStatusComboBox();
        int statusIndex = statusComboBox_->findData(static_cast<int>(quotationOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);

        totalAmountLineEdit_->setText(QString::number(quotationOpt->totalAmount, 'f', 2));
        totalDiscountLineEdit_->setText(QString::number(quotationOpt->totalDiscount, 'f', 2));
        totalTaxLineEdit_->setText(QString::number(quotationOpt->totalTax, 'f', 2));
        netAmountLineEdit_->setText(QString::number(quotationOpt->netAmount, 'f', 2));
        currencyLineEdit_->setText(QString::fromStdString(quotationOpt->currency));
        paymentTermsLineEdit_->setText(QString::fromStdString(quotationOpt->paymentTerms.value_or("")));
        deliveryTermsLineEdit_->setText(QString::fromStdString(quotationOpt->deliveryTerms.value_or("")));
        notesLineEdit_->setText(QString::fromStdString(quotationOpt->notes.value_or("")));

    } else {
        showMessageBox("Thông tin Báo giá", "Không tìm thấy báo giá đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void QuotationManagementWidget::clearForm() {
    idLineEdit_->clear();
    quotationNumberLineEdit_->clear();
    customerComboBox_->clear();
    requestedByLineEdit_->clear();
    quotationDateEdit_->clear();
    validUntilDateEdit_->clear();
    statusComboBox_->setCurrentIndex(0);
    totalAmountLineEdit_->clear();
    totalDiscountLineEdit_->clear();
    totalTaxLineEdit_->clear();
    netAmountLineEdit_->clear();
    currencyLineEdit_->clear();
    paymentTermsLineEdit_->clear();
    deliveryTermsLineEdit_->clear();
    notesLineEdit_->clear();
    quotationTable_->clearSelection();
    updateButtonsState();
}

void QuotationManagementWidget::showManageDetailsDialog(ERP::Sales::DTO::QuotationDTO* quotation) {
    if (!quotation) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Quản lý Chi tiết Báo giá: " + QString::fromStdString(quotation->quotationNumber));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *detailsTable = new QTableWidget(&dialog);
    detailsTable->setColumnCount(7); // Product, Quantity, Unit Price, Discount, Discount Type, Tax Rate, Line Total, Notes
    detailsTable->setHorizontalHeaderLabels({"Sản phẩm", "SL", "Đơn giá", "CK", "Loại CK", "Thuế suất", "Tổng dòng", "Ghi chú"});
    detailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    detailsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    detailsTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(detailsTable);

    // Load existing details
    std::vector<ERP::Sales::DTO::QuotationDetailDTO> currentDetails = quotationService_->getQuotationDetails(quotation->id, currentUserId_, currentUserRoleIds_);
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
        itemDialog.setWindowTitle("Thêm Chi tiết Báo giá");
        QFormLayout itemFormLayout;
        QComboBox productCombo; populateProductComboBox();
        for (int i = 0; i < productComboBox_->count(); ++i) productCombo.addItem(productComboBox_->itemText(i), productComboBox_->itemData(i));
        
        QLineEdit quantityEdit; quantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit unitPriceEdit; unitPriceEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit discountEdit; discountEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QComboBox discountTypeCombo;
        discountTypeCombo.addItem("Fixed Amount", static_cast<int>(ERP::Sales::DTO::DiscountType::FIXED_AMOUNT));
        discountTypeCombo.addItem("Percentage", static_cast<int>(ERP::Sales::DTO::DiscountType::PERCENTAGE));
        QLineEdit taxRateEdit; taxRateEdit.setValidator(new QDoubleValidator(0.0, 100.0, 2, &itemDialog));
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
        itemDialog.setWindowTitle("Sửa Chi tiết Báo giá");
        QFormLayout itemFormLayout;
        QComboBox productCombo; populateProductComboBox();
        for (int i = 0; i < productService_->getAllProducts({}).size(); ++i) productCombo.addItem(productComboBox_->itemText(i), productComboBox_->itemData(i));
        
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
        confirmDelBox.setWindowTitle("Xóa Chi tiết Báo giá");
        confirmDelBox.setText("Bạn có chắc chắn muốn xóa chi tiết báo giá này?");
        confirmDelBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmDelBox.exec() == QMessageBox::Yes) {
            detailsTable->removeRow(selectedItemRow);
        }
    });

    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Collect all items from the table
        std::vector<ERP::Sales::DTO::QuotationDetailDTO> updatedDetails;
        for (int i = 0; i < detailsTable->rowCount(); ++i) {
            ERP::Sales::DTO::QuotationDetailDTO detail;
            // Item ID is stored in product item's UserRole data
            QString existingId = detailsTable->item(i, 0)->data(Qt::UserRole).toString();
            detail.id = existingId.isEmpty() ? ERP::Utils::generateUUID() : existingId.toStdString();

            detail.quotationId = quotation->id;
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

        // Call service to update Quotation with new detail list
        if (quotationService_->updateQuotation(*quotation, updatedDetails, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Quản lý Chi tiết", "Chi tiết báo giá đã được cập nhật thành công.", QMessageBox::Information);
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật chi tiết báo giá. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void QuotationManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool QuotationManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void QuotationManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Sales.CreateQuotation");
    bool canUpdate = hasPermission("Sales.UpdateQuotation");
    bool canDelete = hasPermission("Sales.DeleteQuotation");
    bool canChangeStatus = hasPermission("Sales.UpdateQuotationStatus");
    bool canManageDetails = hasPermission("Sales.ManageQuotationDetails"); // Assuming this permission
    bool canConvert = hasPermission("Sales.ConvertQuotationToSalesOrder"); // Assuming this permission

    addQuotationButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Sales.ViewQuotations"));

    bool isRowSelected = quotationTable_->currentRow() >= 0;
    editQuotationButton_->setEnabled(isRowSelected && canUpdate);
    deleteQuotationButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    manageDetailsButton_->setEnabled(isRowSelected && canManageDetails);
    convertToSalesOrderButton_->setEnabled(isRowSelected && canConvert && quotationTable_->item(quotationTable_->currentRow(), 7)->text() == "Accepted");


    bool enableForm = isRowSelected && canUpdate;
    quotationNumberLineEdit_->setEnabled(enableForm); // Will be read-only for existing anyway
    customerComboBox_->setEnabled(enableForm);
    // requestedByLineEdit_ is read-only
    quotationDateEdit_->setEnabled(enableForm);
    validUntilDateEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    currencyLineEdit_->setEnabled(enableForm);
    paymentTermsLineEdit_->setEnabled(enableForm);
    deliveryTermsLineEdit_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);

    // Calculated read-only fields
    totalAmountLineEdit_->setEnabled(false);
    totalDiscountLineEdit_->setEnabled(false);
    totalTaxLineEdit_->setEnabled(false);
    netAmountLineEdit_->setEnabled(false);

    if (!isRowSelected) {
        idLineEdit_->clear();
        quotationNumberLineEdit_->clear();
        customerComboBox_->clear();
        requestedByLineEdit_->clear();
        quotationDateEdit_->clear();
        validUntilDateEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
        totalAmountLineEdit_->clear();
        totalDiscountLineEdit_->clear();
        totalTaxLineEdit_->clear();
        netAmountLineEdit_->clear();
        currencyLineEdit_->clear();
        paymentTermsLineEdit_->clear();
        deliveryTermsLineEdit_->clear();
        notesLineEdit_->clear();
    }
}


} // namespace Sales
} // namespace UI
} // namespace ERP