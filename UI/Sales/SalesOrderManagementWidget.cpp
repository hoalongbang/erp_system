// UI/Sales/SalesOrderManagementWidget.cpp
#include "SalesOrderManagementWidget.h" // Standard includes
#include "SalesOrder.h"                 // SalesOrder DTO
#include "SalesOrderDetail.h"           // SalesOrderDetail DTO
#include "Customer.h"                   // Customer DTO
#include "Warehouse.h"                  // Warehouse DTO
#include "Product.h"                    // Product DTO
#include "UnitOfMeasure.h"              // UnitOfMeasure DTO (for detail display)
#include "SalesOrderService.h"          // SalesOrder Service
#include "CustomerService.h"            // Customer Service
#include "WarehouseService.h"           // Warehouse Service
#include "ProductService.h"             // Product Service
#include "ISecurityManager.h"           // Security Manager
#include "Logger.h"                     // Logging
#include "ErrorHandler.h"               // Error Handling
#include "Common.h"                     // Common Enums/Constants
#include "DateUtils.h"                  // Date Utilities
#include "StringUtils.h"                // String Utilities
#include "CustomMessageBox.h"           // Custom Message Box
#include "UserService.h"                // For getting user names

#include <QInputDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDoubleValidator>
#include <QDialogButtonBox>

namespace ERP {
namespace UI {
namespace Sales {

SalesOrderManagementWidget::SalesOrderManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::ISalesOrderService> salesOrderService,
    std::shared_ptr<Customer::Services::ICustomerService> customerService,
    std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<Product::Services::IProductService> productService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      salesOrderService_(salesOrderService),
      customerService_(customerService),
      warehouseService_(warehouseService),
      productService_(productService),
      securityManager_(securityManager) {

    if (!salesOrderService_ || !customerService_ || !warehouseService_ || !productService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ đơn hàng bán, khách hàng, kho hàng, sản phẩm hoặc bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("SalesOrderManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("SalesOrderManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("SalesOrderManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadSalesOrders();
    updateButtonsState();
}

SalesOrderManagementWidget::~SalesOrderManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void SalesOrderManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo số đơn hàng...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &SalesOrderManagementWidget::onSearchOrderClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    orderTable_ = new QTableWidget(this);
    orderTable_->setColumnCount(10); // ID, Số đơn hàng, Khách hàng, Ngày đặt, Ngày giao, Tổng tiền, Còn nợ, Trạng thái
    orderTable_->setHorizontalHeaderLabels({"ID", "Số Đơn hàng", "Khách hàng", "Ngày Đặt", "Ngày Giao", "Tổng tiền", "Còn nợ", "Trạng thái", "Người YC", "Kho hàng"});
    orderTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    orderTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    orderTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    orderTable_->horizontalHeader()->setStretchLastSection(true);
    connect(orderTable_, &QTableWidget::itemClicked, this, &SalesOrderManagementWidget::onOrderTableItemClicked);
    mainLayout->addWidget(orderTable_);

    // Form elements for editing/adding orders
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    orderNumberLineEdit_ = new QLineEdit(this);
    customerComboBox_ = new QComboBox(this); populateCustomerComboBox();
    requestedByLineEdit_ = new QLineEdit(this); requestedByLineEdit_->setReadOnly(true); // Should be current user
    approvedByLineEdit_ = new QLineEdit(this); // Can be a combobox for users, read-only here
    orderDateEdit_ = new QDateTimeEdit(this); orderDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    requiredDeliveryDateEdit_ = new QDateTimeEdit(this); requiredDeliveryDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();
    totalAmountLineEdit_ = new QLineEdit(this); totalAmountLineEdit_->setReadOnly(true); // Calculated
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
    paymentTermsLineEdit_ = new QLineEdit(this);
    deliveryAddressLineEdit_ = new QLineEdit(this);
    notesLineEdit_ = new QLineEdit(this);
    warehouseComboBox_ = new QComboBox(this); populateWarehouseComboBox();
    quotationIdLineEdit_ = new QLineEdit(this); quotationIdLineEdit_->setReadOnly(true); // Linked, not editable directly


    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("Số Đơn hàng:*", &orderNumberLineEdit_);
    formLayout->addRow("Khách hàng:*", &customerComboBox_);
    formLayout->addRow("Người yêu cầu:", &requestedByLineEdit_);
    formLayout->addRow("Người phê duyệt:", &approvedByLineEdit_);
    formLayout->addRow("Ngày Đặt hàng:*", &orderDateEdit_);
    formLayout->addRow("Ngày Giao hàng YC:", &requiredDeliveryDateEdit_);
    formLayout->addRow("Trạng thái:*", &statusComboBox_);
    formLayout->addRow("Tổng tiền:", &totalAmountLineEdit_);
    formLayout->addRow("Tổng chiết khấu:", &totalDiscountLineEdit_);
    formLayout->addRow("Tổng thuế:", &totalTaxLineEdit_);
    formLayout->addRow("Số tiền ròng:", &netAmountLineEdit_);
    formLayout->addRow("Đã thanh toán:", &amountPaidLineEdit_);
    formLayout->addRow("Còn nợ:", &amountDueLineEdit_);
    formLayout->addRow("Tiền tệ:", &currencyLineEdit_);
    formLayout->addRow("Điều khoản TT:", &paymentTermsLineEdit_);
    formLayout->addRow("Địa chỉ Giao hàng:", &deliveryAddressLineEdit_);
    formLayout->addRow("Ghi chú:", &notesLineEdit_);
    formLayout->addRow("Kho hàng mặc định:", &warehouseComboBox_);
    formLayout->addRow("ID Báo giá:", &quotationIdLineEdit_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addOrderButton_ = new QPushButton("Thêm mới", this); connect(addOrderButton_, &QPushButton::clicked, this, &SalesOrderManagementWidget::onAddOrderClicked);
    editOrderButton_ = new QPushButton("Sửa", this); connect(editOrderButton_, &QPushButton::clicked, this, &SalesOrderManagementWidget::onEditOrderClicked);
    deleteOrderButton_ = new QPushButton("Xóa", this); connect(deleteOrderButton_, &QPushButton::clicked, this, &SalesOrderManagementWidget::onDeleteOrderClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &SalesOrderManagementWidget::onUpdateOrderStatusClicked);
    manageDetailsButton_ = new QPushButton("Quản lý Chi tiết", this); connect(manageDetailsButton_, &QPushButton::clicked, this, &SalesOrderManagementWidget::onManageDetailsClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &SalesOrderManagementWidget::onSearchOrderClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &SalesOrderManagementWidget::clearForm);
    
    buttonLayout->addWidget(addOrderButton_);
    buttonLayout->addWidget(editOrderButton_);
    buttonLayout->addWidget(deleteOrderButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(manageDetailsButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void SalesOrderManagementWidget::loadSalesOrders() {
    ERP::Logger::Logger::getInstance().info("SalesOrderManagementWidget: Loading sales orders...");
    orderTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Sales::DTO::SalesOrderDTO> orders = salesOrderService_->getAllSalesOrders({}, currentUserId_, currentUserRoleIds_);

    orderTable_->setRowCount(orders.size());
    for (int i = 0; i < orders.size(); ++i) {
        const auto& order = orders[i];
        orderTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(order.id)));
        orderTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(order.orderNumber)));
        
        QString customerName = "N/A";
        std::optional<ERP::Customer::DTO::CustomerDTO> customer = securityManager_->getCustomerService()->getCustomerById(order.customerId, currentUserId_, currentUserRoleIds_);
        if (customer) customerName = QString::fromStdString(customer->name);
        orderTable_->setItem(i, 2, new QTableWidgetItem(customerName));

        orderTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(order.orderDate, ERP::Common::DATETIME_FORMAT))));
        orderTable_->setItem(i, 4, new QTableWidgetItem(order.requiredDeliveryDate ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*order.requiredDeliveryDate, ERP::Common::DATETIME_FORMAT)) : "N/A"));
        orderTable_->setItem(i, 5, new QTableWidgetItem(QString::number(order.totalAmount, 'f', 2) + " " + QString::fromStdString(order.currency)));
        orderTable_->setItem(i, 6, new QTableWidgetItem(QString::number(order.amountDue, 'f', 2) + " " + QString::fromStdString(order.currency)));
        orderTable_->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(order.getStatusString())));
        
        QString requestedByName = "N/A";
        std::optional<ERP::User::DTO::UserDTO> requestedByUser = securityManager_->getUserService()->getUserById(order.requestedByUserId, currentUserId_, currentUserRoleIds_);
        if (requestedByUser) requestedByName = QString::fromStdString(requestedByUser->username);
        orderTable_->setItem(i, 8, new QTableWidgetItem(requestedByName));

        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = securityManager_->getWarehouseService()->getWarehouseById(order.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) warehouseName = QString::fromStdString(warehouse->name);
        orderTable_->setItem(i, 9, new QTableWidgetItem(warehouseName));
    }
    orderTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("SalesOrderManagementWidget: Sales orders loaded successfully.");
}

void SalesOrderManagementWidget::populateCustomerComboBox() {
    customerComboBox_->clear();
    std::vector<ERP::Customer::DTO::CustomerDTO> allCustomers = securityManager_->getCustomerService()->getAllCustomers({}, currentUserId_, currentUserRoleIds_);
    for (const auto& customer : allCustomers) {
        customerComboBox_->addItem(QString::fromStdString(customer.name), QString::fromStdString(customer.id));
    }
}

void SalesOrderManagementWidget::populateWarehouseComboBox() {
    warehouseComboBox_->clear();
    std::vector<ERP::Catalog::DTO::WarehouseDTO> allWarehouses = securityManager_->getWarehouseService()->getAllWarehouses({}, currentUserId_, currentUserRoleIds_);
    for (const auto& warehouse : allWarehouses) {
        warehouseComboBox_->addItem(QString::fromStdString(warehouse.name), QString::fromStdString(warehouse.id));
    }
}

void SalesOrderManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Draft", static_cast<int>(ERP::Sales::DTO::SalesOrderStatus::DRAFT));
    statusComboBox_->addItem("Pending Approval", static_cast<int>(ERP::Sales::DTO::SalesOrderStatus::PENDING_APPROVAL));
    statusComboBox_->addItem("Approved", static_cast<int>(ERP::Sales::DTO::SalesOrderStatus::APPROVED));
    statusComboBox_->addItem("In Progress", static_cast<int>(ERP::Sales::DTO::SalesOrderStatus::IN_PROGRESS));
    statusComboBox_->addItem("Completed", static_cast<int>(ERP::Sales::DTO::SalesOrderStatus::COMPLETED));
    statusComboBox_->addItem("Cancelled", static_cast<int>(ERP::Sales::DTO::SalesOrderStatus::CANCELLED));
    statusComboBox_->addItem("Rejected", static_cast<int>(ERP::Sales::DTO::SalesOrderStatus::REJECTED));
    statusComboBox_->addItem("Partially Delivered", static_cast<int>(ERP::Sales::DTO::SalesOrderStatus::PARTIALLY_DELIVERED));
}

void SalesOrderManagementWidget::populateUserComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("None", "");
    std::vector<ERP::User::DTO::UserDTO> allUsers = securityManager_->getUserService()->getAllUsers({}, currentUserId_, currentUserRoleIds_);
    for (const auto& user : allUsers) {
        comboBox->addItem(QString::fromStdString(user.username), QString::fromStdString(user.id));
    }
}


void SalesOrderManagementWidget::onAddOrderClicked() {
    if (!hasPermission("Sales.CreateSalesOrder")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm đơn hàng bán.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateCustomerComboBox();
    populateWarehouseComboBox();
    showOrderInputDialog();
}

void SalesOrderManagementWidget::onEditOrderClicked() {
    if (!hasPermission("Sales.UpdateSalesOrder")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa đơn hàng bán.", QMessageBox::Warning);
        return;
    }

    int selectedRow = orderTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Đơn hàng bán", "Vui lòng chọn một đơn hàng bán để sửa.", QMessageBox::Information);
        return;
    }

    QString orderId = orderTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::SalesOrderDTO> orderOpt = salesOrderService_->getSalesOrderById(orderId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (orderOpt) {
        populateCustomerComboBox();
        populateWarehouseComboBox();
        showOrderInputDialog(&(*orderOpt));
    } else {
        showMessageBox("Sửa Đơn hàng bán", "Không tìm thấy đơn hàng bán để sửa.", QMessageBox::Critical);
    }
}

void SalesOrderManagementWidget::onDeleteOrderClicked() {
    if (!hasPermission("Sales.DeleteSalesOrder")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa đơn hàng bán.", QMessageBox::Warning);
        return;
    }

    int selectedRow = orderTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Đơn hàng bán", "Vui lòng chọn một đơn hàng bán để xóa.", QMessageBox::Information);
        return;
    }

    QString orderId = orderTable_->item(selectedRow, 0)->text();
    QString orderNumber = orderTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Đơn hàng bán");
    confirmBox.setText("Bạn có chắc chắn muốn xóa đơn hàng bán '" + orderNumber + "' (ID: " + orderId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (salesOrderService_->deleteSalesOrder(orderId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Đơn hàng bán", "Đơn hàng bán đã được xóa thành công.", QMessageBox::Information);
            loadSalesOrders();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa đơn hàng bán. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void SalesOrderManagementWidget::onUpdateOrderStatusClicked() {
    if (!hasPermission("Sales.UpdateSalesOrderStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái đơn hàng bán.", QMessageBox::Warning);
        return;
    }

    int selectedRow = orderTable_->currentRow();
    if (selectedRow < 0) {
    showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một đơn hàng bán để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString orderId = orderTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::SalesOrderDTO> orderOpt = salesOrderService_->getSalesOrderById(orderId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!orderOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy đơn hàng bán để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Sales::DTO::SalesOrderDTO currentOrder = *orderOpt;
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
    int currentStatusIndex = newStatusCombo.findData(static_cast<int>(currentOrder.status));
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
        ERP::Sales::DTO::SalesOrderStatus newStatus = static_cast<ERP::Sales::DTO::SalesOrderStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái đơn hàng bán");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái đơn hàng bán '" + QString::fromStdString(currentOrder.orderNumber) + "' thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (salesOrderService_->updateSalesOrderStatus(orderId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái đơn hàng bán đã được cập nhật thành công.", QMessageBox::Information);
                loadSalesOrders();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái đơn hàng bán. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}


void SalesOrderManagementWidget::onSearchOrderClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["order_number_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    orderTable_->setRowCount(0);
    std::vector<ERP::Sales::DTO::SalesOrderDTO> orders = salesOrderService_->getAllSalesOrders(filter, currentUserId_, currentUserRoleIds_);

    orderTable_->setRowCount(orders.size());
    for (int i = 0; i < orders.size(); ++i) {
        const auto& order = orders[i];
        orderTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(order.id)));
        orderTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(order.orderNumber)));
        
        QString customerName = "N/A";
        std::optional<ERP::Customer::DTO::CustomerDTO> customer = securityManager_->getCustomerService()->getCustomerById(order.customerId, currentUserId_, currentUserRoleIds_);
        if (customer) customerName = QString::fromStdString(customer->name);
        orderTable_->setItem(i, 2, new QTableWidgetItem(customerName));

        orderTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(order.orderDate, ERP::Common::DATETIME_FORMAT))));
        orderTable_->setItem(i, 4, new QTableWidgetItem(order.requiredDeliveryDate ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*order.requiredDeliveryDate, ERP::Common::DATETIME_FORMAT)) : "N/A"));
        orderTable_->setItem(i, 5, new QTableWidgetItem(QString::number(order.totalAmount, 'f', 2) + " " + QString::fromStdString(order.currency)));
        orderTable_->setItem(i, 6, new QTableWidgetItem(QString::number(order.amountDue, 'f', 2) + " " + QString::fromStdString(order.currency)));
        orderTable_->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(order.getStatusString())));
        
        QString requestedByName = "N/A";
        std::optional<ERP::User::DTO::UserDTO> requestedByUser = securityManager_->getUserService()->getUserById(order.requestedByUserId, currentUserId_, currentUserRoleIds_);
        if (requestedByUser) requestedByName = QString::fromStdString(requestedByUser->username);
        orderTable_->setItem(i, 8, new QTableWidgetItem(requestedByName));

        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = securityManager_->getWarehouseService()->getWarehouseById(order.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) warehouseName = QString::fromStdString(warehouse->name);
        orderTable_->setItem(i, 9, new QTableWidgetItem(warehouseName));
    }
    orderTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("SalesOrderManagementWidget: Search completed.");
}

void SalesOrderManagementWidget::onOrderTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString orderId = orderTable_->item(row, 0)->text();
    std::optional<ERP::Sales::DTO::SalesOrderDTO> orderOpt = salesOrderService_->getSalesOrderById(orderId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (orderOpt) {
        idLineEdit_->setText(QString::fromStdString(orderOpt->id));
        orderNumberLineEdit_->setText(QString::fromStdString(orderOpt->orderNumber));
        
        populateCustomerComboBox();
        int customerIndex = customerComboBox_->findData(QString::fromStdString(orderOpt->customerId));
        if (customerIndex != -1) customerComboBox_->setCurrentIndex(customerIndex);

        requestedByLineEdit_->setText(QString::fromStdString(orderOpt->requestedByUserId)); // Display ID
        approvedByLineEdit_->setText(QString::fromStdString(orderOpt->approvedByUserId.value_or(""))); // Display ID
        orderDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(orderOpt->orderDate.time_since_epoch()).count()));
        if (orderOpt->requiredDeliveryDate) requiredDeliveryDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(orderOpt->requiredDeliveryDate->time_since_epoch()).count()));
        else requiredDeliveryDateEdit_->clear();
        
        populateStatusComboBox();
        int statusIndex = statusComboBox_->findData(static_cast<int>(orderOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);

        totalAmountLineEdit_->setText(QString::number(orderOpt->totalAmount, 'f', 2));
        totalDiscountLineEdit_->setText(QString::number(orderOpt->totalDiscount, 'f', 2));
        totalTaxLineEdit_->setText(QString::number(orderOpt->totalTax, 'f', 2));
        netAmountLineEdit_->setText(QString::number(orderOpt->netAmount, 'f', 2));
        amountPaidLineEdit_->setText(QString::number(orderOpt->amountPaid, 'f', 2));
        amountDueLineEdit_->setText(QString::number(orderOpt->amountDue, 'f', 2));
        currencyLineEdit_->setText(QString::fromStdString(orderOpt->currency));
        paymentTermsLineEdit_->setText(QString::fromStdString(orderOpt->paymentTerms.value_or("")));
        deliveryAddressLineEdit_->setText(QString::fromStdString(orderOpt->deliveryAddress.value_or("")));
        notesLineEdit_->setText(QString::fromStdString(orderOpt->notes.value_or("")));
        
        populateWarehouseComboBox();
        int warehouseIndex = warehouseComboBox_->findData(QString::fromStdString(orderOpt->warehouseId));
        if (warehouseIndex != -1) warehouseComboBox_->setCurrentIndex(warehouseIndex);

        quotationIdLineEdit_->setText(QString::fromStdString(orderOpt->quotationId.value_or("")));

    } else {
        showMessageBox("Thông tin Đơn hàng bán", "Không tìm thấy đơn hàng bán đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void SalesOrderManagementWidget::clearForm() {
    idLineEdit_->clear();
    orderNumberLineEdit_->clear();
    customerComboBox_->clear();
    requestedByLineEdit_->clear();
    approvedByLineEdit_->clear();
    orderDateEdit_->clear();
    requiredDeliveryDateEdit_->clear();
    statusComboBox_->setCurrentIndex(0);
    totalAmountLineEdit_->clear();
    totalDiscountLineEdit_->clear();
    totalTaxLineEdit_->clear();
    netAmountLineEdit_->clear();
    amountPaidLineEdit_->clear();
    amountDueLineEdit_->clear();
    currencyLineEdit_->clear();
    paymentTermsLineEdit_->clear();
    deliveryAddressLineEdit_->clear();
    notesLineEdit_->clear();
    warehouseComboBox_->clear();
    quotationIdLineEdit_->clear();
    orderTable_->clearSelection();
    updateButtonsState();
}

void SalesOrderManagementWidget::onManageDetailsClicked() {
    if (!hasPermission("Sales.ManageSalesOrderDetails")) { // Assuming this permission
        showMessageBox("Lỗi", "Bạn không có quyền quản lý chi tiết đơn hàng bán.", QMessageBox::Warning);
        return;
    }

    int selectedRow = orderTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Quản lý Chi tiết", "Vui lòng chọn một đơn hàng bán để quản lý chi tiết.", QMessageBox::Information);
        return;
    }

    QString orderId = orderTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::SalesOrderDTO> orderOpt = salesOrderService_->getSalesOrderById(orderId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (orderOpt) {
        showManageDetailsDialog(&(*orderOpt));
    } else {
        showMessageBox("Quản lý Chi tiết", "Không tìm thấy đơn hàng bán để quản lý chi tiết.", QMessageBox::Critical);
    }
}


void SalesOrderManagementWidget::showOrderInputDialog(ERP::Sales::DTO::SalesOrderDTO* order) {
    QDialog dialog(this);
    dialog.setWindowTitle(order ? "Sửa Đơn hàng bán" : "Thêm Đơn hàng bán Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit orderNumberEdit(this);
    QComboBox customerCombo(this); populateCustomerComboBox();
    for(int i = 0; i < customerComboBox_->count(); ++i) customerCombo.addItem(customerComboBox_->itemText(i), customerComboBox_->itemData(i));
    
    QComboBox requestedByCombo(this); populateUserComboBox(&requestedByCombo);
    QComboBox approvedByCombo(this); populateUserComboBox(&approvedByCombo);
    QDateTimeEdit orderDateEdit(this); orderDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QDateTimeEdit requiredDeliveryDateEdit(this); requiredDeliveryDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QComboBox statusCombo(this); populateStatusComboBox();
    for(int i = 0; i < statusComboBox_->count(); ++i) statusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));
    
    QLineEdit currencyEdit(this);
    QLineEdit paymentTermsEdit(this);
    QLineEdit deliveryAddressEdit(this);
    QLineEdit notesEdit(this);
    QComboBox warehouseCombo(this); populateWarehouseComboBox();
    for(int i = 0; i < warehouseComboBox_->count(); ++i) warehouseCombo.addItem(warehouseComboBox_->itemText(i), warehouseComboBox_->itemData(i));
    
    QLineEdit quotationIdEdit(this); quotationIdEdit.setReadOnly(true); // Linked, not editable directly

    if (order) {
        orderNumberEdit.setText(QString::fromStdString(order->orderNumber));
        int customerIndex = customerCombo.findData(QString::fromStdString(order->customerId));
        if (customerIndex != -1) customerCombo.setCurrentIndex(customerIndex);
        int requestedByIndex = requestedByCombo.findData(QString::fromStdString(order->requestedByUserId));
        if (requestedByIndex != -1) requestedByCombo.setCurrentIndex(requestedByIndex);
        if (order->approvedByUserId) {
            int approvedByIndex = approvedByCombo.findData(QString::fromStdString(*order->approvedByUserId));
            if (approvedByIndex != -1) approvedByCombo.setCurrentIndex(approvedByIndex);
            else approvedByCombo.setCurrentIndex(0);
        } else {
            approvedByCombo.setCurrentIndex(0);
        }
        orderDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(order->orderDate.time_since_epoch()).count()));
        if (order->requiredDeliveryDate) requiredDeliveryDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(order->requiredDeliveryDate->time_since_epoch()).count()));
        else requiredDeliveryDateEdit.clear();
        int statusIndex = statusCombo.findData(static_cast<int>(order->status));
        if (statusIndex != -1) statusCombo.setCurrentIndex(statusIndex);
        currencyEdit.setText(QString::fromStdString(order->currency));
        paymentTermsEdit.setText(QString::fromStdString(order->paymentTerms.value_or("")));
        deliveryAddressEdit.setText(QString::fromStdString(order->deliveryAddress.value_or("")));
        notesEdit.setText(QString::fromStdString(order->notes.value_or("")));
        int warehouseIndex = warehouseCombo.findData(QString::fromStdString(order->warehouseId));
        if (warehouseIndex != -1) warehouseCombo.setCurrentIndex(warehouseIndex);
        quotationIdEdit.setText(QString::fromStdString(order->quotationId.value_or("")));

        orderNumberEdit.setReadOnly(true); // Don't allow changing order number for existing
    } else {
        orderNumberEdit.setText("SO-" + QString::fromStdString(ERP::Utils::generateUUID().substr(0, 8))); // Auto-generate
        orderDateEdit.setDateTime(QDateTime::currentDateTime());
        requiredDeliveryDateEdit.setDateTime(QDateTime::currentDateTime().addDays(7)); // Default 7 days
        requestedByCombo.setCurrentIndex(requestedByCombo.findData(QString::fromStdString(currentUserId_))); // Default to current user
        currencyEdit.setText("VND");
    }

    formLayout->addRow("Số Đơn hàng:*", &orderNumberEdit);
    formLayout->addRow("Khách hàng:*", &customerCombo);
    formLayout->addRow("Người yêu cầu:*", &requestedByCombo);
    formLayout->addRow("Người phê duyệt:", &approvedByCombo);
    formLayout->addRow("Ngày Đặt hàng:*", &orderDateEdit);
    formLayout->addRow("Ngày Giao hàng YC:", &requiredDeliveryDateEdit);
    formLayout->addRow("Trạng thái:*", &statusCombo);
    formLayout->addRow("Tiền tệ:", &currencyEdit);
    formLayout->addRow("Điều khoản TT:", &paymentTermsEdit);
    formLayout->addRow("Địa chỉ Giao hàng:", &deliveryAddressEdit);
    formLayout->addRow("Ghi chú:", &notesEdit);
    formLayout->addRow("Kho hàng mặc định:", &warehouseCombo);
    formLayout->addRow("ID Báo giá:", &quotationIdEdit);
    
    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(order ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Sales::DTO::SalesOrderDTO newOrderData;
        if (order) {
            newOrderData = *order;
        }

        newOrderData.orderNumber = orderNumberEdit.text().toStdString();
        newOrderData.customerId = customerCombo.currentData().toString().toStdString();
        newOrderData.requestedByUserId = requestedByCombo.currentData().toString().toStdString();
        
        QString selectedApprovedById = approvedByCombo.currentData().toString();
        newOrderData.approvedByUserId = selectedApprovedById.isEmpty() ? std::nullopt : std::make_optional(selectedApprovedById.toStdString());

        newOrderData.orderDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(orderDateEdit.dateTime());
        newOrderData.requiredDeliveryDate = requiredDeliveryDateEdit.dateTime().isNull() ? std::nullopt : std::make_optional(ERP::Utils::DateUtils::qDateTimeToTimePoint(requiredDeliveryDateEdit.dateTime()));
        newOrderData.status = static_cast<ERP::Sales::DTO::SalesOrderStatus>(statusCombo.currentData().toInt());
        newOrderData.currency = currencyEdit.text().toStdString();
        newOrderData.paymentTerms = paymentTermsEdit.text().isEmpty() ? std::nullopt : std::make_optional(paymentTermsEdit.text().toStdString());
        newOrderData.deliveryAddress = deliveryAddressEdit.text().isEmpty() ? std::nullopt : std::make_optional(deliveryAddressEdit.text().toStdString());
        newOrderData.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());
        newOrderData.warehouseId = warehouseCombo.currentData().toString().toStdString();
        newOrderData.quotationId = quotationIdEdit.text().isEmpty() ? std::nullopt : std::make_optional(quotationIdEdit.text().toStdString());

        // Calculated totals are not set directly here, they should be derived from details or updated by service logic.
        // For new order, they will be 0. For existing, they will be preserved and potentially re-calculated by service.
        newOrderData.totalAmount = order ? order->totalAmount : 0.0;
        newOrderData.totalDiscount = order ? order->totalDiscount : 0.0;
        newOrderData.totalTax = order ? order->totalTax : 0.0;
        newOrderData.netAmount = order ? order->netAmount : 0.0;
        newOrderData.amountPaid = order ? order->amountPaid : 0.0;
        newOrderData.amountDue = order ? order->amountDue : 0.0;

        // For new orders, details are added in a separate dialog after creation
        std::vector<ERP::Sales::DTO::SalesOrderDetailDTO> currentDetails;
        if (order) { // When editing, load existing details first
             currentDetails = salesOrderService_->getSalesOrderDetails(order->id, currentUserId_, currentUserRoleIds_);
        }

        bool success = false;
        if (order) {
            success = salesOrderService_->updateSalesOrder(newOrderData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Đơn hàng bán", "Đơn hàng bán đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật đơn hàng bán. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Sales::DTO::SalesOrderDTO> createdOrder = salesOrderService_->createSalesOrder(newOrderData, currentUserId_, currentUserRoleIds_); // Create with empty details
            if (createdOrder) {
                showMessageBox("Thêm Đơn hàng bán", "Đơn hàng bán mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm đơn hàng bán mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadSalesOrders();
            clearForm();
        }
    }
}

void SalesOrderManagementWidget::showManageDetailsDialog(ERP::Sales::DTO::SalesOrderDTO* order) {
    if (!order) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Quản lý Chi tiết Đơn hàng bán: " + QString::fromStdString(order->orderNumber));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *detailsTable = new QTableWidget(&dialog);
    detailsTable->setColumnCount(7); // Product, Quantity, Unit Price, Discount, Discount Type, Tax Rate, Line Total, Notes
    detailsTable->setHorizontalHeaderLabels({"Sản phẩm", "SL", "Đơn giá", "CK", "Loại CK", "Thuế suất", "Tổng dòng", "Ghi chú"});
    detailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    detailsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    detailsTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(detailsTable);

    // Load existing details
    std::vector<ERP::Sales::DTO::SalesOrderDetailDTO> currentDetails = salesOrderService_->getSalesOrderDetails(order->id, currentUserId_, currentUserRoleIds_);
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
        itemDialog.setWindowTitle("Thêm Chi tiết Đơn hàng bán");
        QFormLayout itemFormLayout;
        QComboBox productCombo; populateProductComboBox();
        for (int i = 0; i < productService_->getAllProducts({}).size(); ++i) productCombo.addItem(productComboBox_->itemText(i), productComboBox_->itemData(i));
        
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
        itemDialog.setWindowTitle("Sửa Chi tiết Đơn hàng bán");
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
        confirmDelBox.setWindowTitle("Xóa Chi tiết Đơn hàng bán");
        confirmDelBox.setText("Bạn có chắc chắn muốn xóa chi tiết đơn hàng bán này?");
        confirmDelBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmDelBox.exec() == QMessageBox::Yes) {
            detailsTable->removeRow(selectedItemRow);
        }
    });

    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Collect all items from the table
        std::vector<ERP::Sales::DTO::SalesOrderDetailDTO> updatedDetails;
        for (int i = 0; i < detailsTable->rowCount(); ++i) {
            ERP::Sales::DTO::SalesOrderDetailDTO detail;
            // Item ID is stored in product item's UserRole data
            QString existingId = detailsTable->item(i, 0)->data(Qt::UserRole).toString();
            detail.id = existingId.isEmpty() ? ERP::Utils::generateUUID() : existingId.toStdString();

            detail.salesOrderId = order->id;
            detail.productId = detailsTable->item(i, 0)->data(Qt::UserRole).toString().toStdString();
            detail.quantity = detailsTable->item(i, 1)->text().toDouble();
            detail.unitPrice = detailsTable->item(i, 2)->text().toDouble();
            detail.discount = detailsTable->item(i, 3)->text().toDouble();
            detail.discountType = static_cast<ERP::Sales::DTO::DiscountType>(detailsTable->item(i, 4)->data(Qt::UserRole).toInt());
            detail.taxRate = detailsTable->item(i, 5)->text().toDouble();
            detail.lineTotal = detailsTable->item(i, 6)->text().toDouble();
            detail.notes = detailsTable->item(i, 7)->text().isEmpty() ? std::nullopt : std::make_optional(detailsTable->item(i, 7)->text().toStdString());
            
            // Assuming deliveredQuantity, invoicedQuantity, isFullyDelivered, isFullyInvoiced are managed by other specific actions/services
            // For now, keep their original values if editing an existing detail
            // Or set to default if adding new
            detail.deliveredQuantity = 0.0;
            detail.invoicedQuantity = 0.0;
            detail.isFullyDelivered = false;
            detail.isFullyInvoiced = false;
            
            updatedDetails.push_back(detail);
        }

        // Call service to update Sales Order with new detail list
        // This line assumes updateSalesOrder also handles updating details.
        // If not, you need to add salesOrderService_->updateSalesOrderDetails(...)
        // For now, it will effectively only update parent SO data, not details from this dialog.
        // The service method SalesOrderService::updateSalesOrder already takes an empty details vector.
        // So, this UI logic for details will not be persisted by this service call.
        // This requires dedicated service method for updating sales order details, or updating sales order DTO to contain details.
        showMessageBox("Lỗi", "Chức năng quản lý chi tiết đơn hàng bán chưa được triển khai đầy đủ. Vui lòng kiểm tra log.", QMessageBox::Critical);
        return;
    }
}


void SalesOrderManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool SalesOrderManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void SalesOrderManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Sales.CreateSalesOrder");
    bool canUpdate = hasPermission("Sales.UpdateSalesOrder");
    bool canDelete = hasPermission("Sales.DeleteSalesOrder");
    bool canChangeStatus = hasPermission("Sales.UpdateSalesOrderStatus");
    bool canManageDetails = hasPermission("Sales.ManageSalesOrderDetails"); // Assuming this permission

    addOrderButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Sales.ViewSalesOrders"));

    bool isRowSelected = orderTable_->currentRow() >= 0;
    editOrderButton_->setEnabled(isRowSelected && canUpdate);
    deleteOrderButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    manageDetailsButton_->setEnabled(isRowSelected && canManageDetails);


    bool enableForm = isRowSelected && canUpdate;
    orderNumberLineEdit_->setEnabled(enableForm); // Will be read-only for existing anyway
    customerComboBox_->setEnabled(enableForm);
    requestedByLineEdit_->setEnabled(enableForm);
    approvedByLineEdit_->setEnabled(enableForm);
    orderDateEdit_->setEnabled(enableForm);
    requiredDeliveryDateEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    currencyLineEdit_->setEnabled(enableForm);
    paymentTermsLineEdit_->setEnabled(enableForm);
    deliveryAddressLineEdit_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);
    warehouseComboBox_->setEnabled(enableForm);
    // quotationIdLineEdit_ is read-only

    // Calculated read-only fields
    idLineEdit_->setEnabled(false);
    totalAmountLineEdit_->setEnabled(false);
    totalDiscountLineEdit_->setEnabled(false);
    totalTaxLineEdit_->setEnabled(false);
    netAmountLineEdit_->setEnabled(false);
    amountPaidLineEdit_->setEnabled(false);
    amountDueLineEdit_->setEnabled(false);


    if (!isRowSelected) {
        idLineEdit_->clear();
        orderNumberLineEdit_->clear();
        customerComboBox_->clear();
        requestedByLineEdit_->clear();
        approvedByLineEdit_->clear();
        orderDateEdit_->clear();
        requiredDeliveryDateEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
        totalAmountLineEdit_->clear();
        totalDiscountLineEdit_->clear();
        totalTaxLineEdit_->clear();
        netAmountLineEdit_->clear();
        amountPaidLineEdit_->clear();
        amountDueLineEdit_->clear();
        currencyLineEdit_->clear();
        paymentTermsLineEdit_->clear();
        deliveryAddressLineEdit_->clear();
        notesLineEdit_->clear();
        warehouseComboBox_->clear();
        quotationIdLineEdit_->clear();
    }
}


} // namespace Sales
} // namespace UI
} // namespace ERP