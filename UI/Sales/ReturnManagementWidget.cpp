// UI/Sales/ReturnManagementWidget.cpp
#include "ReturnManagementWidget.h"
#include "CustomMessageBox.h" // For Common::CustomMessageBox
#include "UserService.h"      // For getting user roles from SecurityManager
#include "UnitOfMeasureService.h" // For UnitOfMeasure DTO access

#include <QInputDialog> // For getting input from user for details
#include <QDoubleValidator> // For quantity/price inputs
#include <QMessageBox> // For standard message boxes if CustomMessageBox is not desired everywhere

namespace ERP {
namespace UI {
namespace Sales {

ReturnManagementWidget::ReturnManagementWidget(
    QWidget* parent,
    std::shared_ptr<Services::IReturnService> returnService,
    std::shared_ptr<Services::ISalesOrderService> salesOrderService,
    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService,
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<ERP::Product::Services::IProductService> productService,
    std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      returnService_(returnService),
      salesOrderService_(salesOrderService),
      customerService_(customerService),
      warehouseService_(warehouseService),
      productService_(productService),
      inventoryManagementService_(inventoryManagementService),
      securityManager_(securityManager) {

    if (!returnService_ || !salesOrderService_ || !customerService_ || !warehouseService_ || !productService_ || !inventoryManagementService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Một hoặc nhiều dịch vụ trả hàng/đơn hàng bán/khách hàng/kho hàng/sản phẩm/tồn kho/bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("ReturnManagementWidget: Initialized with null dependencies.");
        return;
    }

    // Attempt to get current user ID and roles from securityManager
    auto authService = securityManager_->getAuthenticationService();
    if (authService) {
        std::string dummySessionId = "current_session_id"; // Placeholder
        std::optional<ERP::Security::DTO::SessionDTO> currentSession = authService->validateSession(dummySessionId);
        if (currentSession) {
            currentUserId_ = currentSession->userId;
            currentUserRoleIds_ = securityManager_->getUserService()->getUserRoles(currentUserId_, {});
        } else {
            currentUserId_ = "system_user"; // Fallback for non-logged-in operations or system actions
            currentUserRoleIds_ = {"anonymous"}; // Fallback
            ERP::Logger::Logger::getInstance().warning("ReturnManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("ReturnManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadReturns();
    updateButtonsState(); // Set initial button states
}

ReturnManagementWidget::~ReturnManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void ReturnManagementWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Search and filter
    QHBoxLayout* searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo số phiếu trả hàng...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &ReturnManagementWidget::onSearchReturnClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    // Table of Returns
    returnTable_ = new QTableWidget(this);
    returnTable_->setColumnCount(8); // ID, Số phiếu, Đơn hàng bán, Khách hàng, Ngày trả, Tổng tiền, Trạng thái, Kho hàng
    returnTable_->setHorizontalHeaderLabels({"ID", "Số phiếu", "Đơn hàng bán", "Khách hàng", "Ngày trả", "Tổng tiền", "Trạng thái", "Kho hàng"});
    returnTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    returnTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    returnTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    returnTable_->horizontalHeader()->setStretchLastSection(true);
    connect(returnTable_, &QTableWidget::itemClicked, this, &ReturnManagementWidget::onReturnTableItemClicked);
    mainLayout->addWidget(returnTable_);

    // Form for Add/Edit
    QGridLayout* formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this);
    idLineEdit_->setReadOnly(true);
    returnNumberLineEdit_ = new QLineEdit(this);
    salesOrderComboBox_ = new QComboBox(this);
    customerComboBox_ = new QComboBox(this);
    returnDateEdit_ = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    returnDateEdit_->setCalendarPopup(true);
    returnDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    reasonLineEdit_ = new QLineEdit(this);
    totalAmountLineEdit_ = new QLineEdit(this);
    totalAmountLineEdit_->setReadOnly(true); // Calculated
    statusComboBox_ = new QComboBox(this);
    warehouseComboBox_ = new QComboBox(this);
    notesLineEdit_ = new QLineEdit(this);

    formLayout->addWidget(new QLabel("ID:", this), 0, 0);
    formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Số phiếu:", this), 1, 0);
    formLayout->addWidget(returnNumberLineEdit_, 1, 1);
    formLayout->addWidget(new QLabel("Đơn hàng bán:", this), 2, 0);
    formLayout->addWidget(salesOrderComboBox_, 2, 1);
    formLayout->addWidget(new QLabel("Khách hàng:", this), 3, 0);
    formLayout->addWidget(customerComboBox_, 3, 1);
    formLayout->addWidget(new QLabel("Ngày trả:", this), 4, 0);
    formLayout->addWidget(returnDateEdit_, 4, 1);
    formLayout->addWidget(new QLabel("Lý do:", this), 5, 0);
    formLayout->addWidget(reasonLineEdit_, 5, 1);
    formLayout->addWidget(new QLabel("Tổng tiền:", this), 6, 0);
    formLayout->addWidget(totalAmountLineEdit_, 6, 1);
    formLayout->addWidget(new QLabel("Trạng thái:", this), 7, 0);
    formLayout->addWidget(statusComboBox_, 7, 1);
    formLayout->addWidget(new QLabel("Kho hàng trả về:", this), 8, 0);
    formLayout->addWidget(warehouseComboBox_, 8, 1);
    formLayout->addWidget(new QLabel("Ghi chú:", this), 9, 0);
    formLayout->addWidget(notesLineEdit_, 9, 1);

    mainLayout->addLayout(formLayout);

    // Nested table for Return Details
    mainLayout->addWidget(new QLabel("<h3>Chi tiết trả hàng</h3>", this));
    detailsTable_ = new QTableWidget(this);
    detailsTable_->setColumnCount(6); // Product, Quantity, UoM, Unit Price, Refunded Amount, Notes
    detailsTable_->setHorizontalHeaderLabels({"Sản phẩm", "Số lượng YC", "Đã lấy", "Đơn vị", "Đơn giá", "Ghi chú"});
    detailsTable_->horizontalHeader()->setStretchLastSection(true);
    detailsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    detailsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(detailsTable_);

    QHBoxLayout* detailButtonsLayout = new QHBoxLayout();
    addDetailButton_ = new QPushButton("Thêm chi tiết", this);
    removeDetailButton_ = new QPushButton("Xóa chi tiết", this);
    detailButtonsLayout->addWidget(addDetailButton_);
    detailButtonsLayout->addWidget(removeDetailButton_);
    detailButtonsLayout->addStretch();
    mainLayout->addLayout(detailButtonsLayout);

    // Main control buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    addReturnButton_ = new QPushButton("Thêm mới", this);
    editReturnButton_ = new QPushButton("Sửa", this);
    deleteReturnButton_ = new QPushButton("Xóa", this);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this);
    clearFormButton_ = new QPushButton("Xóa Form", this);

    connect(addReturnButton_, &QPushButton::clicked, this, &ReturnManagementWidget::onAddReturnClicked);
    connect(editReturnButton_, &QPushButton::clicked, this, &ReturnManagementWidget::onEditReturnClicked);
    connect(deleteReturnButton_, &QPushButton::clicked, this, &ReturnManagementWidget::onDeleteReturnClicked);
    connect(updateStatusButton_, &QPushButton::clicked, this, &ReturnManagementWidget::onUpdateReturnStatusClicked);
    connect(clearFormButton_, &QPushButton::clicked, this, &ReturnManagementWidget::clearForm);
    connect(addDetailButton_, &QPushButton::clicked, this, &ReturnManagementWidget::onAddReturnDetailClicked);
    connect(removeDetailButton_, &QPushButton::clicked, this, &ReturnManagementWidget::onRemoveReturnDetailClicked);


    buttonLayout->addWidget(addReturnButton_);
    buttonLayout->addWidget(editReturnButton_);
    buttonLayout->addWidget(deleteReturnButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void ReturnManagementWidget::loadReturns() {
    if (!hasPermission("Sales.ViewReturns")) {
        showMessageBox("Lỗi quyền", "Bạn không có quyền xem phiếu trả hàng.", QMessageBox::Warning);
        returnTable_->setRowCount(0);
        return;
    }
    ERP::Logger::Logger::getInstance().info("ReturnManagementWidget: Loading returns...");
    returnTable_->setRowCount(0); // Clear existing rows

    std::map<std::string, std::any> filter;
    QString searchText = searchLineEdit_->text().trimmed();
    if (!searchText.isEmpty()) {
        filter["return_number_contains"] = searchText.toStdString();
    }

    std::vector<ERP::Sales::DTO::ReturnDTO> returns = returnService_->getAllReturns(filter, currentUserRoleIds_);

    returnTable_->setRowCount(returns.size());
    for (int i = 0; i < returns.size(); ++i) {
        const auto& returnObj = returns[i];
        returnTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(returnObj.id)));
        returnTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(returnObj.returnNumber)));
        
        QString salesOrderNumber = "N/A";
        std::optional<ERP::Sales::DTO::SalesOrderDTO> salesOrder = salesOrderService_->getSalesOrderById(returnObj.salesOrderId, currentUserRoleIds_);
        if (salesOrder) {
            salesOrderNumber = QString::fromStdString(salesOrder->orderNumber);
        }
        returnTable_->setItem(i, 2, new QTableWidgetItem(salesOrderNumber));

        QString customerName = "N/A";
        std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(returnObj.customerId, currentUserRoleIds_);
        if (customer) {
            customerName = QString::fromStdString(customer->name);
        }
        returnTable_->setItem(i, 3, new QTableWidgetItem(customerName));
        
        returnTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(returnObj.returnDate, ERP::Common::DATETIME_FORMAT))));
        returnTable_->setItem(i, 5, new QTableWidgetItem(QString::number(returnObj.totalAmount, 'f', 2)));
        returnTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(returnObj.getStatusString())));

        QString warehouseName = "N/A";
        if (returnObj.warehouseId) {
            std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(*returnObj.warehouseId, currentUserRoleIds_);
            if (warehouse) {
                warehouseName = QString::fromStdString(warehouse->name);
            }
        }
        returnTable_->setItem(i, 7, new QTableWidgetItem(warehouseName));
    }
    returnTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ReturnManagementWidget: Returns loaded successfully.");
}

void ReturnManagementWidget::populateSalesOrderComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("Chọn đơn hàng bán", ""); // Empty option
    std::vector<ERP::Sales::DTO::SalesOrderDTO> salesOrders = salesOrderService_->getAllSalesOrders({}, currentUserRoleIds_);
    for (const auto& order : salesOrders) {
        // Only show completed/in-progress orders that can be returned
        if (order.status == ERP::Sales::DTO::SalesOrderStatus::COMPLETED || order.status == ERP::Sales::DTO::SalesOrderStatus::IN_PROGRESS) {
            comboBox->addItem(QString::fromStdString(order.orderNumber), QString::fromStdString(order.id));
        }
    }
}

void ReturnManagementWidget::populateCustomerComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("Chọn khách hàng", ""); // Empty option
    std::vector<ERP::Customer::DTO::CustomerDTO> customers = customerService_->getAllCustomers({}, currentUserRoleIds_);
    for (const auto& customer : customers) {
        if (customer.status == ERP::Common::EntityStatus::ACTIVE) {
            comboBox->addItem(QString::fromStdString(customer.name), QString::fromStdString(customer.id));
        }
    }
}

void ReturnManagementWidget::populateWarehouseComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("Chọn kho hàng trả về", ""); // Empty option
    std::vector<ERP::Catalog::DTO::WarehouseDTO> warehouses = warehouseService_->getAllWarehouses({}, currentUserRoleIds_);
    for (const auto& warehouse : warehouses) {
        if (warehouse.status == ERP::Common::EntityStatus::ACTIVE) {
            comboBox->addItem(QString::fromStdString(warehouse.name), QVariant(QString::fromStdString(warehouse.id)));
        }
    }
}

void ReturnManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Đang chờ", static_cast<int>(ERP::Sales::DTO::ReturnStatus::PENDING));
    statusComboBox_->addItem("Đã nhận", static_cast<int>(ERP::Sales::DTO::ReturnStatus::RECEIVED));
    statusComboBox_->addItem("Đã xử lý", static_cast<int>(ERP::Sales::DTO::ReturnStatus::PROCESSED));
    statusComboBox_->addItem("Đã hủy", static_cast<int>(ERP::Sales::DTO::ReturnStatus::CANCELLED));
    statusComboBox_->addItem("Không xác định", static_cast<int>(ERP::Sales::DTO::ReturnStatus::UNKNOWN));
}

void ReturnManagementWidget::onAddReturnClicked() {
    if (!hasPermission("Sales.CreateReturn")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm phiếu trả hàng.", QMessageBox::Warning);
        return;
    }
    clearForm();
    showReturnInputDialog();
}

void ReturnManagementWidget::onEditReturnClicked() {
    if (!hasPermission("Sales.UpdateReturn")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa phiếu trả hàng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = returnTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Phiếu Trả Hàng", "Vui lòng chọn một phiếu trả hàng để sửa.", QMessageBox::Information);
        return;
    }

    QString returnId = returnTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::ReturnDTO> returnOpt = returnService_->getReturnById(returnId.toStdString(), currentUserRoleIds_);

    if (returnOpt) {
        idLineEdit_->setText(QString::fromStdString(returnOpt->id));
        returnNumberLineEdit_->setText(QString::fromStdString(returnOpt->returnNumber));
        returnDateEdit_->setDateTime(ERP::Utils::DateUtils::timePointToQDateTime(returnOpt->returnDate));
        reasonLineEdit_->setText(QString::fromStdString(returnOpt->reason.value_or("")));
        totalAmountLineEdit_->setText(QString::number(returnOpt->totalAmount, 'f', 2));
        notesLineEdit_->setText(QString::fromStdString(returnOpt->notes.value_or("")));

        populateSalesOrderComboBox(salesOrderComboBox_);
        int soIndex = salesOrderComboBox_->findData(QString::fromStdString(returnOpt->salesOrderId));
        if (soIndex != -1) salesOrderComboBox_->setCurrentIndex(soIndex);

        populateCustomerComboBox(customerComboBox_);
        int custIndex = customerComboBox_->findData(QString::fromStdString(returnOpt->customerId));
        if (custIndex != -1) customerComboBox_->setCurrentIndex(custIndex);

        populateWarehouseComboBox(warehouseComboBox_);
        if (returnOpt->warehouseId) {
            int whIndex = warehouseComboBox_->findData(QString::fromStdString(*returnOpt->warehouseId));
            if (whIndex != -1) warehouseComboBox_->setCurrentIndex(whIndex);
        }

        populateStatusComboBox(); // Ensure populated before setting
        int statusIndex = statusComboBox_->findData(static_cast<int>(returnOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);

        // Populate details table
        updateDetailTable(returnOpt->details);

        showReturnInputDialog(&(*returnOpt)); // Pass by address to modify
    } else {
        showMessageBox("Sửa Phiếu Trả Hàng", "Không tìm thấy phiếu trả hàng để sửa.", QMessageBox::Critical);
    }
}

void ReturnManagementWidget::onDeleteReturnClicked() {
    if (!hasPermission("Sales.DeleteReturn")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa phiếu trả hàng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = returnTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Phiếu Trả Hàng", "Vui lòng chọn một phiếu trả hàng để xóa.", QMessageBox::Information);
        return;
    }

    QString returnId = returnTable_->item(selectedRow, 0)->text();
    QString returnNumber = returnTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Phiếu Trả Hàng");
    confirmBox.setText("Bạn có chắc chắn muốn xóa phiếu trả hàng '" + returnNumber + "' (ID: " + returnId + ")? Thao tác này có thể không hoàn tác được.");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (returnService_->deleteReturn(returnId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Phiếu Trả Hàng", "Phiếu trả hàng đã được xóa thành công.", QMessageBox::Information);
            loadReturns();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể xóa phiếu trả hàng. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}

void ReturnManagementWidget::onUpdateReturnStatusClicked() {
    if (!hasPermission("Sales.UpdateReturn")) { // Specific permission for status update if needed
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái phiếu trả hàng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = returnTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một phiếu trả hàng để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString returnId = returnTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::ReturnDTO> returnOpt = returnService_->getReturnById(returnId.toStdString(), currentUserRoleIds_);

    if (!returnOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy phiếu trả hàng để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Sales::DTO::ReturnDTO currentReturn = *returnOpt;
    
    QDialog statusDialog(this);
    statusDialog.setWindowTitle("Chọn Trạng Thái Mới");
    QVBoxLayout *layout = new QVBoxLayout(&statusDialog);
    QComboBox newStatusCombo;
    populateStatusComboBox(); // Populate it with all possible statuses
    // Find and set current status as default selected
    int currentStatusIndex = newStatusCombo.findData(static_cast<int>(currentReturn.status));
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
    connect(cancelButton, &QPushButton::clicked, &statusDialog, &QDialog::reject);

    if (statusDialog.exec() == QDialog::Accepted) {
        ERP::Sales::DTO::ReturnStatus newStatus = static_cast<ERP::Sales::DTO::ReturnStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái phiếu trả hàng");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái phiếu trả hàng '" + QString::fromStdString(currentReturn.returnNumber) + "' thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (returnService_->updateReturnStatus(returnId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái phiếu trả hàng đã được cập nhật thành công.", QMessageBox::Information);
                loadReturns();
                clearForm();
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật trạng thái. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
    }
}

void ReturnManagementWidget::onSearchReturnClicked() {
    loadReturns(); // Reloads with search text filter
}

void ReturnManagementWidget::onReturnTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString returnId = returnTable_->item(row, 0)->text();
    std::optional<ERP::Sales::DTO::ReturnDTO> returnOpt = returnService_->getReturnById(returnId.toStdString(), currentUserRoleIds_);

    if (returnOpt) {
        idLineEdit_->setText(QString::fromStdString(returnOpt->id));
        returnNumberLineEdit_->setText(QString::fromStdString(returnOpt->returnNumber));
        returnDateEdit_->setDateTime(ERP::Utils::DateUtils::timePointToQDateTime(returnOpt->returnDate));
        reasonLineEdit_->setText(QString::fromStdString(returnOpt->reason.value_or("")));
        totalAmountLineEdit_->setText(QString::number(returnOpt->totalAmount, 'f', 2));
        notesLineEdit_->setText(QString::fromStdString(returnOpt->notes.value_or("")));

        // Set combo boxes
        populateSalesOrderComboBox(salesOrderComboBox_);
        int soIndex = salesOrderComboBox_->findData(QString::fromStdString(returnOpt->salesOrderId));
        if (soIndex != -1) salesOrderComboBox_->setCurrentIndex(soIndex);

        populateCustomerComboBox(customerComboBox_);
        int custIndex = customerComboBox_->findData(QString::fromStdString(returnOpt->customerId));
        if (custIndex != -1) customerComboBox_->setCurrentIndex(custIndex);

        populateWarehouseComboBox(warehouseComboBox_);
        if (returnOpt->warehouseId) {
            int whIndex = warehouseComboBox_->findData(QString::fromStdString(*returnOpt->warehouseId));
            if (whIndex != -1) warehouseComboBox_->setCurrentIndex(whIndex);
        }

        populateStatusComboBox();
        int statusIndex = statusComboBox_->findData(static_cast<int>(returnOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);

        // Populate details table
        updateDetailTable(returnOpt->details);

        showReturnInputDialog(&(*returnOpt)); // Pass by address to modify
    } else {
        showMessageBox("Thông tin Phiếu Trả Hàng", "Không thể tải chi tiết phiếu trả hàng đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void ReturnManagementWidget::clearForm() {
    idLineEdit_->clear();
    returnNumberLineEdit_->clear();
    salesOrderComboBox_->setCurrentIndex(0);
    customerComboBox_->setCurrentIndex(0);
    returnDateEdit_->setDateTime(QDateTime::currentDateTime());
    reasonLineEdit_->clear();
    totalAmountLineEdit_->setText("0.00");
    statusComboBox_->setCurrentIndex(0);
    warehouseComboBox_->setCurrentIndex(0);
    notesLineEdit_->clear();
    detailsTable_->setRowCount(0);
    returnTable_->clearSelection();
    updateButtonsState();
}

void ReturnManagementWidget::onAddReturnDetailClicked() {
    // Dialog to add a new return detail item
    QDialog detailDialog(this);
    detailDialog.setWindowTitle("Thêm Chi Tiết Trả Hàng");
    QFormLayout* formLayout = new QFormLayout(&detailDialog);

    QComboBox productCombo(&detailDialog);
    productCombo.addItem("Chọn sản phẩm", "");
    std::vector<ERP::Product::DTO::ProductDTO> products = productService_->getAllProducts({}, currentUserId_, currentUserRoleIds_);
    for(const auto& p : products) {
        if (p.status == ERP::Common::EntityStatus::ACTIVE) {
            productCombo.addItem(QString::fromStdString(p.name + " (" + p.productCode + ")"), QString::fromStdString(p.id));
        }
    }

    QLineEdit quantityEdit(&detailDialog);
    quantityEdit.setValidator(new QDoubleValidator(0.0, 9999999.0, 2, &quantityEdit)); // Allow decimal quantities
    
    // Unit of Measure combo box
    QComboBox uomCombo(&detailDialog);
    uomCombo.addItem("Chọn đơn vị", "");
    // Populate UoM from service
    std::vector<ERP::Catalog::DTO::UnitOfMeasureDTO> uoms = securityManager_->getUnitOfMeasureService()->getAllUnitOfMeasures({}, currentUserRoleIds_);
    for(const auto& u : uoms) {
        if (u.status == ERP::Common::EntityStatus::ACTIVE) {
            uomCombo.addItem(QString::fromStdString(u.symbol), QString::fromStdString(u.id));
        }
    }

    QLineEdit unitPriceEdit(&detailDialog);
    unitPriceEdit.setValidator(new QDoubleValidator(0.0, 9999999999.0, 2, &unitPriceEdit));

    QLineEdit conditionEdit(&detailDialog);
    QLineEdit notesEdit(&detailDialog);

    formLayout->addRow("Sản phẩm:", &productCombo);
    formLayout->addRow("Số lượng:", &quantityEdit);
    formLayout->addRow("Đơn vị:", &uomCombo);
    formLayout->addRow("Đơn giá:", &unitPriceEdit);
    formLayout->addRow("Tình trạng:", &conditionEdit);
    formLayout->addRow("Ghi chú:", &notesEdit);

    QPushButton* okButton = new QPushButton("Thêm", &detailDialog);
    QPushButton* cancelButton = new QPushButton("Hủy", &detailDialog);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    formLayout->addRow(buttonLayout);

    connect(okButton, &QPushButton::clicked, &detailDialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &detailDialog, &QDialog::reject);

    if (detailDialog.exec() == QDialog::Accepted) {
        // Collect data and add to detailsTable_
        ERP::Sales::DTO::ReturnDetailDTO newDetail;
        newDetail.id = ERP::Utils::generateUUID(); // Client-side ID for new detail
        newDetail.returnId = idLineEdit_->text().toStdString(); // Link to current return
        newDetail.productId = productCombo.currentData().toString().toStdString();
        newDetail.quantity = quantityEdit.text().toDouble();
        newDetail.unitOfMeasureId = uomCombo.currentData().toString().toStdString();
        newDetail.unitPrice = unitPriceEdit.text().toDouble();
        newDetail.condition = conditionEdit.text().isEmpty() ? std::nullopt : std::make_optional(conditionEdit.text().toStdString());
        newDetail.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());
        newDetail.status = ERP::Common::EntityStatus::ACTIVE; // Default active

        // Add to the table for display (will be saved when main return is saved/updated)
        int row = detailsTable_->rowCount();
        detailsTable_->insertRow(row);
        detailsTable_->setItem(row, 0, new QTableWidgetItem(productCombo.currentText())); // Product Name
        detailsTable_->setItem(row, 1, new QTableWidgetItem(QString::number(newDetail.quantity)));
        detailsTable_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(newDetail.unitOfMeasureId))); // UoM Symbol
        detailsTable_->setItem(row, 3, new QTableWidgetItem(QString::number(newDetail.unitPrice, 'f', 2)));
        detailsTable_->setItem(row, 4, new QTableWidgetItem(QString::number(newDetail.quantity * newDetail.unitPrice, 'f', 2))); // Calculated total
        detailsTable_->setItem(row, 5, new QTableWidgetItem(notesEdit.text()));
        // Store DTO ID in hidden column or as UserRole for later retrieval (for saving)
        detailsTable_->item(row,0)->setData(Qt::UserRole, QString::fromStdString(newDetail.id)); // Store ID for update/delete details
        detailsTable_->item(row,0)->setData(Qt::UserRole + 1, QString::fromStdString(newDetail.productId)); // Store Product ID
        detailsTable_->item(row,0)->setData(Qt::UserRole + 2, QString::fromStdString(newDetail.unitOfMeasureId)); // Store UoM ID
        detailsTable_->item(row,0)->setData(Qt::UserRole + 3, QString::number(newDetail.quantity)); // Store quantity
        detailsTable_->item(row,0)->setData(Qt::UserRole + 4, QString::number(newDetail.unitPrice)); // Store unitPrice

        // Recalculate total amount
        double currentTotal = totalAmountLineEdit_->text().toDouble();
        totalAmountLineEdit_->setText(QString::number(currentTotal + (newDetail.quantity * newDetail.unitPrice), 'f', 2));

        showMessageBox("Thêm Chi Tiết", "Chi tiết trả hàng đã được thêm vào form. Hãy lưu phiếu để xác nhận.", QMessageBox::Information);
    }
}

void ReturnManagementWidget::onRemoveReturnDetailClicked() {
    int selectedRow = detailsTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Chi Tiết", "Vui lòng chọn một chi tiết để xóa.", QMessageBox::Information);
        return;
    }

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Chi Tiết Trả Hàng");
    confirmBox.setText("Bạn có chắc chắn muốn xóa chi tiết trả hàng này?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        // Recalculate total amount before removing row
        double removedQuantity = detailsTable_->item(selectedRow, 1)->text().toDouble();
        double removedUnitPrice = detailsTable_->item(selectedRow, 3)->text().toDouble();
        double currentTotal = totalAmountLineEdit_->text().toDouble();
        totalAmountLineEdit_->setText(QString::number(currentTotal - (removedQuantity * removedUnitPrice), 'f', 2));

        detailsTable_->removeRow(selectedRow);
        showMessageBox("Xóa Chi Tiết", "Chi tiết đã được xóa khỏi form. Hãy lưu phiếu để xác nhận.", QMessageBox::Information);
    }
}

void ReturnManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool ReturnManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void ReturnManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Sales.CreateReturn");
    bool canUpdate = hasPermission("Sales.UpdateReturn");
    bool canDelete = hasPermission("Sales.DeleteReturn");
    bool canChangeStatus = hasPermission("Sales.UpdateReturn"); // Assuming update covers status changes

    addReturnButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Sales.ViewReturns"));

    bool isRowSelected = returnTable_->currentRow() >= 0;
    editReturnButton_->setEnabled(isRowSelected && canUpdate);
    deleteReturnButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);

    bool enableForm = isRowSelected && canUpdate;
    returnNumberLineEdit_->setEnabled(enableForm);
    salesOrderComboBox_->setEnabled(enableForm);
    customerComboBox_->setEnabled(enableForm);
    returnDateEdit_->setEnabled(enableForm);
    reasonLineEdit_->setEnabled(enableForm);
    // totalAmountLineEdit_ is read-only
    statusComboBox_->setEnabled(enableForm);
    warehouseComboBox_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);

    addDetailButton_->setEnabled(enableForm);
    removeDetailButton_->setEnabled(enableForm && detailsTable_->currentRow() >= 0);

    if (!isRowSelected) {
        idLineEdit_->clear();
        returnNumberLineEdit_->clear();
        salesOrderComboBox_->setCurrentIndex(0);
        customerComboBox_->setCurrentIndex(0);
        returnDateEdit_->setDateTime(QDateTime::currentDateTime());
        reasonLineEdit_->clear();
        totalAmountLineEdit_->setText("0.00");
        statusComboBox_->setCurrentIndex(0);
        warehouseComboBox_->setCurrentIndex(0);
        notesLineEdit_->clear();
        detailsTable_->setRowCount(0); // Clear details table
    }
}

void ReturnManagementWidget::updateDetailTable(const std::vector<ERP::Sales::DTO::ReturnDetailDTO>& details) {
    detailsTable_->setRowCount(0); // Clear existing details
    detailsTable_->setRowCount(details.size());

    for (int i = 0; i < details.size(); ++i) {
        const auto& detail = details[i];
        std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(detail.productId, currentUserId_, currentUserRoleIds_);
        QString productName = product ? QString::fromStdString(product->name + " (" + product->productCode + ")") : "Không rõ";

        detailsTable_->setItem(i, 0, new QTableWidgetItem(productName));
        detailsTable_->setItem(i, 1, new QTableWidgetItem(QString::number(detail.quantity)));
        detailsTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(detail.unitOfMeasureId))); // UoM Symbol
        detailsTable_->setItem(i, 3, new QTableWidgetItem(QString::number(detail.unitPrice, 'f', 2)));
        detailsTable_->setItem(i, 4, new QTableWidgetItem(QString::number(detail.quantity * detail.unitPrice, 'f', 2))); // Calculated total
        detailsTable_->setItem(i, 5, new QTableWidgetItem(detail.notes ? QString::fromStdString(*detail.notes) : ""));
        
        detailsTable_->item(i,0)->setData(Qt::UserRole, QString::fromStdString(detail.id)); // Store actual detail ID
        detailsTable_->item(i,0)->setData(Qt::UserRole + 1, QString::fromStdString(detail.productId)); // Store Product ID
        detailsTable_->item(i,0)->setData(Qt::UserRole + 2, QString::fromStdString(detail.unitOfMeasureId)); // Store UoM ID
        detailsTable_->item(i,0)->setData(Qt::UserRole + 3, QString::number(detail.quantity)); // Store quantity
        detailsTable_->item(i,0)->setData(Qt::UserRole + 4, QString::number(detail.unitPrice)); // Store unitPrice
    }
    detailsTable_->resizeColumnsToContents();
}


} // namespace Sales
} // namespace UI
} // namespace ERP