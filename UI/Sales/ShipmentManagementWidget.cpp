// UI/Sales/ShipmentManagementWidget.cpp
#include "ShipmentManagementWidget.h" // Standard includes
#include "Shipment.h"                 // Shipment DTO
#include "ShipmentDetail.h"           // ShipmentDetail DTO
#include "SalesOrder.h"               // SalesOrder DTO
#include "Customer.h"                 // Customer DTO
#include "Product.h"                  // Product DTO
#include "Warehouse.h"                // Warehouse DTO
#include "Location.h"                 // Location DTO
#include "ShipmentService.h"          // Shipment Service
#include "SalesOrderService.h"        // SalesOrder Service
#include "CustomerService.h"          // Customer Service
#include "WarehouseService.h"         // Warehouse Service
#include "ProductService.h"           // Product Service
#include "ISecurityManager.h"         // Security Manager
#include "Logger.h"                   // Logging
#include "ErrorHandler.h"             // Error Handling
#include "Common.h"                   // Common Enums/Constants
#include "DateUtils.h"                // Date Utilities
#include "StringUtils.h"              // String Utilities
#include "CustomMessageBox.h"         // Custom Message Box
#include "UserService.h"              // For getting user names

#include <QInputDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDoubleValidator>
#include <QDialogButtonBox>

namespace ERP {
namespace UI {
namespace Sales {

ShipmentManagementWidget::ShipmentManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IShipmentService> shipmentService,
    std::shared_ptr<Services::ISalesOrderService> salesOrderService,
    std::shared_ptr<Customer::Services::ICustomerService> customerService,
    std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<Product::Services::IProductService> productService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      shipmentService_(shipmentService),
      salesOrderService_(salesOrderService),
      customerService_(customerService),
      warehouseService_(warehouseService),
      productService_(productService),
      securityManager_(securityManager) {

    if (!shipmentService_ || !salesOrderService_ || !customerService_ || !warehouseService_ || !productService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ vận chuyển, đơn hàng bán, khách hàng, kho hàng, sản phẩm hoặc bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("ShipmentManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("ShipmentManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("ShipmentManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadShipments();
    updateButtonsState();
}

ShipmentManagementWidget::~ShipmentManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void ShipmentManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo số vận đơn...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &ShipmentManagementWidget::onSearchShipmentClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    shipmentTable_ = new QTableWidget(this);
    shipmentTable_->setColumnCount(9); // ID, Số vận đơn, Đơn hàng bán, Khách hàng, Ngày vận chuyển, Ngày giao hàng, Loại, Trạng thái, Người gửi
    shipmentTable_->setHorizontalHeaderLabels({"ID", "Số Vận đơn", "Đơn hàng bán", "Khách hàng", "Ngày VC", "Ngày GH", "Loại", "Trạng thái", "Người gửi"});
    shipmentTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    shipmentTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    shipmentTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    shipmentTable_->horizontalHeader()->setStretchLastSection(true);
    connect(shipmentTable_, &QTableWidget::itemClicked, this, &ShipmentManagementWidget::onShipmentTableItemClicked);
    mainLayout->addWidget(shipmentTable_);

    // Form elements for editing/adding shipments
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    shipmentNumberLineEdit_ = new QLineEdit(this);
    salesOrderComboBox_ = new QComboBox(this); populateSalesOrderComboBox();
    customerComboBox_ = new QComboBox(this); populateCustomerComboBox();
    shippedByLineEdit_ = new QLineEdit(this); // User ID, display name
    shipmentDateEdit_ = new QDateTimeEdit(this); shipmentDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    deliveryDateEdit_ = new QDateTimeEdit(this); deliveryDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    typeComboBox_ = new QComboBox(this); populateTypeComboBox();
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();
    carrierNameLineEdit_ = new QLineEdit(this);
    trackingNumberLineEdit_ = new QLineEdit(this);
    deliveryAddressLineEdit_ = new QLineEdit(this);
    notesLineEdit_ = new QLineEdit(this);


    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("Số Vận đơn:*", &shipmentNumberLineEdit_);
    formLayout->addRow("Đơn hàng bán:*", &salesOrderComboBox_);
    formLayout->addRow("Khách hàng:*", &customerComboBox_);
    formLayout->addRow("Người gửi hàng:", &shippedByLineEdit_);
    formLayout->addRow("Ngày Vận chuyển:*", &shipmentDateEdit_);
    formLayout->addRow("Ngày Giao hàng:", &deliveryDateEdit_);
    formLayout->addRow("Loại:*", &typeComboBox_);
    formLayout->addRow("Trạng thái:*", &statusComboBox_);
    formLayout->addRow("Tên hãng vận chuyển:", &carrierNameLineEdit_);
    formLayout->addRow("Số theo dõi:", &trackingNumberLineEdit_);
    formLayout->addRow("Địa chỉ giao hàng:", &deliveryAddressLineEdit_);
    formLayout->addRow("Ghi chú:", &notesLineEdit_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addShipmentButton_ = new QPushButton("Thêm mới", this); connect(addShipmentButton_, &QPushButton::clicked, this, &ShipmentManagementWidget::onAddShipmentClicked);
    editShipmentButton_ = new QPushButton("Sửa", this); connect(editShipmentButton_, &QPushButton::clicked, this, &ShipmentManagementWidget::onEditShipmentClicked);
    deleteShipmentButton_ = new QPushButton("Xóa", this); connect(deleteShipmentButton_, &QPushButton::clicked, this, &ShipmentManagementWidget::onDeleteShipmentClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &ShipmentManagementWidget::onUpdateShipmentStatusClicked);
    manageDetailsButton_ = new QPushButton("Quản lý Chi tiết", this); connect(manageDetailsButton_, &QPushButton::clicked, this, &ShipmentManagementWidget::onManageDetailsClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &ShipmentManagementWidget::onSearchShipmentClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &ShipmentManagementWidget::clearForm);
    
    buttonLayout->addWidget(addShipmentButton_);
    buttonLayout->addWidget(editShipmentButton_);
    buttonLayout->addWidget(deleteShipmentButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(manageDetailsButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void ShipmentManagementWidget::loadShipments() {
    ERP::Logger::Logger::getInstance().info("ShipmentManagementWidget: Loading shipments...");
    shipmentTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Sales::DTO::ShipmentDTO> shipments = shipmentService_->getAllShipments({}, currentUserId_, currentUserRoleIds_);

    shipmentTable_->setRowCount(shipments.size());
    for (int i = 0; i < shipments.size(); ++i) {
        const auto& shipment = shipments[i];
        shipmentTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(shipment.id)));
        shipmentTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(shipment.shipmentNumber)));
        
        QString salesOrderNumber = "N/A";
        std::optional<ERP::Sales::DTO::SalesOrderDTO> so = salesOrderService_->getSalesOrderById(shipment.salesOrderId, currentUserId_, currentUserRoleIds_);
        if (so) salesOrderNumber = QString::fromStdString(so->orderNumber);
        shipmentTable_->setItem(i, 2, new QTableWidgetItem(salesOrderNumber));

        QString customerName = "N/A";
        std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(shipment.customerId, currentUserId_, currentUserRoleIds_);
        if (customer) customerName = QString::fromStdString(customer->name);
        shipmentTable_->setItem(i, 3, new QTableWidgetItem(customerName));

        shipmentTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(shipment.shipmentDate, ERP::Common::DATETIME_FORMAT))));
        shipmentTable_->setItem(i, 5, new QTableWidgetItem(shipment.deliveryDate ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*shipment.deliveryDate, ERP::Common::DATETIME_FORMAT)) : "N/A"));
        shipmentTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(shipment.getTypeString())));
        shipmentTable_->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(shipment.getStatusString())));
        
        QString shippedByName = "N/A";
        std::optional<ERP::User::DTO::UserDTO> shippedByUser = securityManager_->getUserService()->getUserById(shipment.shippedByUserId, currentUserId_, currentUserRoleIds_);
        if (shippedByUser) shippedByName = QString::fromStdString(shippedByUser->username);
        shipmentTable_->setItem(i, 8, new QTableWidgetItem(shippedByName));
    }
    shipmentTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ShipmentManagementWidget: Shipments loaded successfully.");
}

void ShipmentManagementWidget::populateSalesOrderComboBox() {
    salesOrderComboBox_->clear();
    std::vector<ERP::Sales::DTO::SalesOrderDTO> allSalesOrders = salesOrderService_->getAllSalesOrders({}, currentUserId_, currentUserRoleIds_);
    for (const auto& so : allSalesOrders) {
        salesOrderComboBox_->addItem(QString::fromStdString(so.orderNumber), QString::fromStdString(so.id));
    }
}

void ShipmentManagementWidget::populateCustomerComboBox() {
    customerComboBox_->clear();
    std::vector<ERP::Customer::DTO::CustomerDTO> allCustomers = customerService_->getAllCustomers({}, currentUserId_, currentUserRoleIds_);
    for (const auto& customer : allCustomers) {
        customerComboBox_->addItem(QString::fromStdString(customer.name), QString::fromStdString(customer.id));
    }
}

void ShipmentManagementWidget::populateTypeComboBox() {
    typeComboBox_->clear();
    typeComboBox_->addItem("Sales Delivery", static_cast<int>(ERP::Sales::DTO::ShipmentType::SALES_DELIVERY));
    typeComboBox_->addItem("Sample Delivery", static_cast<int>(ERP::Sales::DTO::ShipmentType::SAMPLE_DELIVERY));
    typeComboBox_->addItem("Return Shipment", static_cast<int>(ERP::Sales::DTO::ShipmentType::RETURN_SHIPMENT));
    typeComboBox_->addItem("Other", static_cast<int>(ERP::Sales::DTO::ShipmentType::OTHER));
}

void ShipmentManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Sales::DTO::ShipmentStatus::PENDING));
    statusComboBox_->addItem("Packed", static_cast<int>(ERP::Sales::DTO::ShipmentStatus::PACKED));
    statusComboBox_->addItem("Shipped", static_cast<int>(ERP::Sales::DTO::ShipmentStatus::SHIPPED));
    statusComboBox_->addItem("Delivered", static_cast<int>(ERP::Sales::DTO::ShipmentStatus::DELIVERED));
    statusComboBox_->addItem("Cancelled", static_cast<int>(ERP::Sales::DTO::ShipmentStatus::CANCELLED));
    statusComboBox_->addItem("Returned", static_cast<int>(ERP::Sales::DTO::ShipmentStatus::RETURNED));
}

void ShipmentManagementWidget::populateWarehouseComboBox() {
    // Used in details dialog
}

void ShipmentManagementWidget::populateLocationComboBox(QComboBox* comboBox, const std::string& warehouseId) {
    // Used in details dialog
}


void ShipmentManagementWidget::onAddShipmentClicked() {
    if (!hasPermission("Sales.CreateShipment")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm vận chuyển.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateSalesOrderComboBox();
    populateCustomerComboBox();
    populateTypeComboBox();
    showShipmentInputDialog();
}

void ShipmentManagementWidget::onEditShipmentClicked() {
    if (!hasPermission("Sales.UpdateShipment")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa vận chuyển.", QMessageBox::Warning);
        return;
    }

    int selectedRow = shipmentTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Vận chuyển", "Vui lòng chọn một vận chuyển để sửa.", QMessageBox::Information);
        return;
    }

    QString shipmentId = shipmentTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::ShipmentDTO> shipmentOpt = shipmentService_->getShipmentById(shipmentId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (shipmentOpt) {
        populateSalesOrderComboBox();
        populateCustomerComboBox();
        populateTypeComboBox();
        showShipmentInputDialog(&(*shipmentOpt));
    } else {
        showMessageBox("Sửa Vận chuyển", "Không tìm thấy vận chuyển để sửa.", QMessageBox::Critical);
    }
}

void ShipmentManagementWidget::onDeleteShipmentClicked() {
    if (!hasPermission("Sales.DeleteShipment")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa vận chuyển.", QMessageBox::Warning);
        return;
    }

    int selectedRow = shipmentTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Vận chuyển", "Vui lòng chọn một vận chuyển để xóa.", QMessageBox::Information);
        return;
    }

    QString shipmentId = shipmentTable_->item(selectedRow, 0)->text();
    QString shipmentNumber = shipmentTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Vận chuyển");
    confirmBox.setText("Bạn có chắc chắn muốn xóa vận chuyển '" + shipmentNumber + "' (ID: " + shipmentId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (shipmentService_->deleteShipment(shipmentId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Vận chuyển", "Vận chuyển đã được xóa thành công.", QMessageBox::Information);
            loadShipments();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa vận chuyển. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void ShipmentManagementWidget::onUpdateShipmentStatusClicked() {
    if (!hasPermission("Sales.UpdateShipmentStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái vận chuyển.", QMessageBox::Warning);
        return;
    }

    int selectedRow = shipmentTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một vận chuyển để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString shipmentId = shipmentTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::ShipmentDTO> shipmentOpt = shipmentService_->getShipmentById(shipmentId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!shipmentOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy vận chuyển để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Sales::DTO::ShipmentDTO currentShipment = *shipmentOpt;
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
    int currentStatusIndex = newStatusCombo.findData(static_cast<int>(currentShipment.status));
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
        ERP::Sales::DTO::ShipmentStatus newStatus = static_cast<ERP::Sales::DTO::ShipmentStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái vận chuyển");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái vận chuyển '" + QString::fromStdString(currentShipment.shipmentNumber) + "' thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (shipmentService_->updateShipmentStatus(shipmentId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái vận chuyển đã được cập nhật thành công.", QMessageBox::Information);
                loadShipments();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái vận chuyển. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}


void ShipmentManagementWidget::onSearchShipmentClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["shipment_number_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    shipmentTable_->setRowCount(0);
    std::vector<ERP::Sales::DTO::ShipmentDTO> shipments = shipmentService_->getAllShipments(filter, currentUserId_, currentUserRoleIds_);

    shipmentTable_->setRowCount(shipments.size());
    for (int i = 0; i < shipments.size(); ++i) {
        const auto& shipment = shipments[i];
        shipmentTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(shipment.id)));
        shipmentTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(shipment.shipmentNumber)));
        
        QString salesOrderNumber = "N/A";
        std::optional<ERP::Sales::DTO::SalesOrderDTO> so = salesOrderService_->getSalesOrderById(shipment.salesOrderId, currentUserId_, currentUserRoleIds_);
        if (so) salesOrderNumber = QString::fromStdString(so->orderNumber);
        shipmentTable_->setItem(i, 2, new QTableWidgetItem(salesOrderNumber));

        QString customerName = "N/A";
        std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(shipment.customerId, currentUserId_, currentUserRoleIds_);
        if (customer) customerName = QString::fromStdString(customer->name);
        shipmentTable_->setItem(i, 3, new QTableWidgetItem(customerName));

        shipmentTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(shipment.shipmentDate, ERP::Common::DATETIME_FORMAT))));
        shipmentTable_->setItem(i, 5, new QTableWidgetItem(shipment.deliveryDate ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*shipment.deliveryDate, ERP::Common::DATETIME_FORMAT)) : "N/A"));
        shipmentTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(shipment.getTypeString())));
        shipmentTable_->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(shipment.getStatusString())));
        
        QString shippedByName = "N/A";
        std::optional<ERP::User::DTO::UserDTO> shippedByUser = securityManager_->getUserService()->getUserById(shipment.shippedByUserId, currentUserId_, currentUserRoleIds_);
        if (shippedByUser) shippedByName = QString::fromStdString(shippedByUser->username);
        shipmentTable_->setItem(i, 8, new QTableWidgetItem(shippedByName));
    }
    shipmentTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ShipmentManagementWidget: Search completed.");
}

void ShipmentManagementWidget::onShipmentTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString shipmentId = shipmentTable_->item(row, 0)->text();
    std::optional<ERP::Sales::DTO::ShipmentDTO> shipmentOpt = shipmentService_->getShipmentById(shipmentId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (shipmentOpt) {
        idLineEdit_->setText(QString::fromStdString(shipmentOpt->id));
        shipmentNumberLineEdit_->setText(QString::fromStdString(shipmentOpt->shipmentNumber));
        
        populateSalesOrderComboBox();
        int salesOrderIndex = salesOrderComboBox_->findData(QString::fromStdString(shipmentOpt->salesOrderId));
        if (salesOrderIndex != -1) salesOrderComboBox_->setCurrentIndex(salesOrderIndex);

        populateCustomerComboBox();
        int customerIndex = customerComboBox_->findData(QString::fromStdString(shipmentOpt->customerId));
        if (customerIndex != -1) customerComboBox_->setCurrentIndex(customerIndex);

        shippedByLineEdit_->setText(QString::fromStdString(shipmentOpt->shippedByUserId)); // Display ID
        shipmentDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(shipmentOpt->shipmentDate.time_since_epoch()).count()));
        if (shipmentOpt->deliveryDate) deliveryDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(shipmentOpt->deliveryDate->time_since_epoch()).count()));
        else deliveryDateEdit_->clear();
        
        populateTypeComboBox();
        int typeIndex = typeComboBox_->findData(static_cast<int>(shipmentOpt->type));
        if (typeIndex != -1) typeComboBox_->setCurrentIndex(typeIndex);

        populateStatusComboBox();
        int statusIndex = statusComboBox_->findData(static_cast<int>(shipmentOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);

        carrierNameLineEdit_->setText(QString::fromStdString(shipmentOpt->carrierName.value_or("")));
        trackingNumberLineEdit_->setText(QString::fromStdString(shipmentOpt->trackingNumber.value_or("")));
        deliveryAddressLineEdit_->setText(QString::fromStdString(shipmentOpt->deliveryAddress.value_or("")));
        notesLineEdit_->setText(QString::fromStdString(shipmentOpt->notes.value_or("")));

    } else {
        showMessageBox("Thông tin Vận chuyển", "Không tìm thấy vận chuyển đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void ShipmentManagementWidget::clearForm() {
    idLineEdit_->clear();
    shipmentNumberLineEdit_->clear();
    salesOrderComboBox_->clear();
    customerComboBox_->clear();
    shippedByLineEdit_->clear();
    shipmentDateEdit_->clear();
    deliveryDateEdit_->clear();
    typeComboBox_->setCurrentIndex(0);
    statusComboBox_->setCurrentIndex(0);
    carrierNameLineEdit_->clear();
    trackingNumberLineEdit_->clear();
    deliveryAddressLineEdit_->clear();
    notesLineEdit_->clear();
    shipmentTable_->clearSelection();
    updateButtonsState();
}

void ShipmentManagementWidget::onManageDetailsClicked() {
    if (!hasPermission("Sales.ManageShipmentDetails")) { // Assuming this permission
        showMessageBox("Lỗi", "Bạn không có quyền quản lý chi tiết vận chuyển.", QMessageBox::Warning);
        return;
    }

    int selectedRow = shipmentTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Quản lý Chi tiết", "Vui lòng chọn một vận chuyển để quản lý chi tiết.", QMessageBox::Information);
        return;
    }

    QString shipmentId = shipmentTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Sales::DTO::ShipmentDTO> shipmentOpt = shipmentService_->getShipmentById(shipmentId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (shipmentOpt) {
        showManageDetailsDialog(&(*shipmentOpt));
    } else {
        showMessageBox("Quản lý Chi tiết", "Không tìm thấy vận chuyển để quản lý chi tiết.", QMessageBox::Critical);
    }
}


void ShipmentManagementWidget::showShipmentInputDialog(ERP::Sales::DTO::ShipmentDTO* shipment) {
    QDialog dialog(this);
    dialog.setWindowTitle(shipment ? "Sửa Vận chuyển" : "Thêm Vận chuyển Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit shipmentNumberEdit(this);
    QComboBox salesOrderCombo(this); populateSalesOrderComboBox();
    for (int i = 0; i < salesOrderComboBox_->count(); ++i) salesOrderCombo.addItem(salesOrderComboBox_->itemText(i), salesOrderComboBox_->itemData(i));
    
    QComboBox customerCombo(this); populateCustomerComboBox();
    for (int i = 0; i < customerComboBox_->count(); ++i) customerCombo.addItem(customerComboBox_->itemText(i), customerComboBox_->itemData(i));
    
    QLineEdit shippedByEdit(this); // User ID, display name
    QDateTimeEdit shipmentDateEdit(this); shipmentDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QDateTimeEdit deliveryDateEdit(this); deliveryDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QComboBox typeCombo(this); populateTypeComboBox();
    for (int i = 0; i < typeComboBox_->count(); ++i) typeCombo.addItem(typeComboBox_->itemText(i), typeComboBox_->itemData(i));
    
    QComboBox statusCombo(this); populateStatusComboBox();
    for (int i = 0; i < statusComboBox_->count(); ++i) statusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));
    
    QLineEdit carrierNameEdit(this);
    QLineEdit trackingNumberEdit(this);
    QLineEdit deliveryAddressEdit(this);
    QLineEdit notesEdit(this);

    if (shipment) {
        shipmentNumberEdit.setText(QString::fromStdString(shipment->shipmentNumber));
        int salesOrderIndex = salesOrderCombo.findData(QString::fromStdString(shipment->salesOrderId));
        if (salesOrderIndex != -1) salesOrderCombo.setCurrentIndex(salesOrderIndex);
        int customerIndex = customerCombo.findData(QString::fromStdString(shipment->customerId));
        if (customerIndex != -1) customerCombo.setCurrentIndex(customerIndex);
        shippedByEdit.setText(QString::fromStdString(shipment->shippedByUserId));
        shipmentDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(shipment->shipmentDate.time_since_epoch()).count()));
        if (shipment->deliveryDate) deliveryDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(shipment->deliveryDate->time_since_epoch()).count()));
        else deliveryDateEdit.clear();
        int typeIndex = typeCombo.findData(static_cast<int>(shipment->type));
        if (typeIndex != -1) typeCombo.setCurrentIndex(typeIndex);
        int statusIndex = statusCombo.findData(static_cast<int>(shipment->status));
        if (statusIndex != -1) statusCombo.setCurrentIndex(statusIndex);
        carrierNameEdit.setText(QString::fromStdString(shipment->carrierName.value_or("")));
        trackingNumberEdit.setText(QString::fromStdString(shipment->trackingNumber.value_or("")));
        deliveryAddressEdit.setText(QString::fromStdString(shipment->deliveryAddress.value_or("")));
        notesEdit.setText(QString::fromStdString(shipment->notes.value_or("")));

        shipmentNumberEdit.setReadOnly(true); // Don't allow changing shipment number for existing
    } else {
        shipmentNumberEdit.setText("SHP-" + QString::fromStdString(ERP::Utils::generateUUID().substr(0, 8))); // Auto-generate
        shippedByEdit.setText(QString::fromStdString(currentUserId_)); // Auto-fill current user
        shipmentDateEdit.setDateTime(QDateTime::currentDateTime());
        // Defaults for new
    }

    formLayout->addRow("Số Vận đơn:*", &shipmentNumberEdit);
    formLayout->addRow("Đơn hàng bán:*", &salesOrderCombo);
    formLayout->addRow("Khách hàng:*", &customerCombo);
    formLayout->addRow("Người gửi hàng:", &shippedByEdit);
    formLayout->addRow("Ngày Vận chuyển:*", &shipmentDateEdit);
    formLayout->addRow("Ngày Giao hàng:", &deliveryDateEdit);
    formLayout->addRow("Loại:*", &typeCombo);
    formLayout->addRow("Trạng thái:*", &statusCombo);
    formLayout->addRow("Tên hãng vận chuyển:", &carrierNameEdit);
    formLayout->addRow("Số theo dõi:", &trackingNumberEdit);
    formLayout->addRow("Địa chỉ giao hàng:", &deliveryAddressEdit);
    formLayout->addRow("Ghi chú:", &notesEdit);
    
    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(shipment ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Sales::DTO::ShipmentDTO newShipmentData;
        if (shipment) {
            newShipmentData = *shipment;
        }

        newShipmentData.shipmentNumber = shipmentNumberEdit.text().toStdString();
        newShipmentData.salesOrderId = salesOrderCombo.currentData().toString().toStdString();
        newShipmentData.customerId = customerCombo.currentData().toString().toStdString();
        newShipmentData.shippedByUserId = shippedByEdit.text().toStdString();
        newShipmentData.shipmentDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(shipmentDateEdit.dateTime());
        newShipmentData.deliveryDate = deliveryDateEdit.dateTime().isNull() ? std::nullopt : std::make_optional(ERP::Utils::DateUtils::qDateTimeToTimePoint(deliveryDateEdit.dateTime()));
        newShipmentData.type = static_cast<ERP::Sales::DTO::ShipmentType>(typeCombo.currentData().toInt());
        newShipmentData.status = static_cast<ERP::Sales::DTO::ShipmentStatus>(statusCombo.currentData().toInt());
        newShipmentData.carrierName = carrierNameEdit.text().isEmpty() ? std::nullopt : std::make_optional(carrierNameEdit.text().toStdString());
        newShipmentData.trackingNumber = trackingNumberEdit.text().isEmpty() ? std::nullopt : std::make_optional(trackingNumberEdit.text().toStdString());
        newShipmentData.deliveryAddress = deliveryAddressEdit.text().isEmpty() ? std::nullopt : std::make_optional(deliveryAddressEdit.text().toStdString());
        newShipmentData.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());

        // For new shipments, details are added in a separate dialog after creation
        std::vector<ERP::Sales::DTO::ShipmentDetailDTO> currentDetails;
        if (shipment) { // When editing, load existing details first
             currentDetails = shipmentService_->getShipmentDetails(shipment->id, currentUserId_, currentUserRoleIds_);
        }

        bool success = false;
        if (shipment) {
            success = shipmentService_->updateShipment(newShipmentData, currentDetails, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Vận chuyển", "Vận chuyển đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật vận chuyển. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Sales::DTO::ShipmentDTO> createdShipment = shipmentService_->createShipment(newShipmentData, {}, currentUserId_, currentUserRoleIds_); // Create with empty details
            if (createdShipment) {
                showMessageBox("Thêm Vận chuyển", "Vận chuyển mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm vận chuyển mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadShipments();
            clearForm();
        }
    }
}

void ShipmentManagementWidget::showManageDetailsDialog(ERP::Sales::DTO::ShipmentDTO* shipment) {
    if (!shipment) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Quản lý Chi tiết Vận chuyển: " + QString::fromStdString(shipment->shipmentNumber));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *detailsTable = new QTableWidget(&dialog);
    detailsTable->setColumnCount(7); // Product, Warehouse, Location, Quantity, Lot/Serial, Notes, Sales Order Item Link (Optional)
    detailsTable->setHorizontalHeaderLabels({"Sản phẩm", "Kho hàng", "Vị trí", "Số lượng", "Số lô/Serial", "Ghi chú", "Liên kết SP Đơn hàng"});
    detailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    detailsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    detailsTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(detailsTable);

    // Load existing details
    std::vector<ERP::Sales::DTO::ShipmentDetailDTO> currentDetails = shipmentService_->getShipmentDetails(shipment->id, currentUserId_, currentUserRoleIds_);
    detailsTable->setRowCount(currentDetails.size());
    for (int i = 0; i < currentDetails.size(); ++i) {
        const auto& detail = currentDetails[i];
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = securityManager_->getProductService()->getProductById(detail.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);

        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = securityManager_->getWarehouseService()->getWarehouseById(detail.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) warehouseName = QString::fromStdString(warehouse->name);

        QString locationName = "N/A";
        std::optional<ERP::Catalog::DTO::LocationDTO> location = securityManager_->getWarehouseService()->getLocationById(detail.locationId, currentUserId_, currentUserRoleIds_);
        if (location) locationName = QString::fromStdString(location->name);

        detailsTable->setItem(i, 0, new QTableWidgetItem(productName));
        detailsTable->setItem(i, 1, new QTableWidgetItem(warehouseName));
        detailsTable->setItem(i, 2, new QTableWidgetItem(locationName));
        detailsTable->setItem(i, 3, new QTableWidgetItem(QString::number(detail.quantity)));
        detailsTable->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(detail.lotNumber.value_or("") + "/" + detail.serialNumber.value_or(""))));
        detailsTable->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(detail.notes.value_or(""))));
        detailsTable->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(detail.salesOrderId.value_or("")))); // Link to Sales Order Item
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

    // Connect item management buttons
    connect(addItemButton, &QPushButton::clicked, [&]() {
        QDialog itemDialog(&dialog);
        itemDialog.setWindowTitle("Thêm Chi tiết Vận chuyển");
        QFormLayout itemFormLayout;
        QComboBox productCombo; populateProductComboBox();
        for (int i = 0; i < securityManager_->getProductService()->getAllProducts({}).size(); ++i) productCombo.addItem(productComboBox_->itemText(i), productComboBox_->itemData(i));
        
        QComboBox warehouseCombo; populateWarehouseComboBox();
        for (int i = 0; i < securityManager_->getWarehouseService()->getAllWarehouses({}).size(); ++i) warehouseCombo.addItem(warehouseComboBox_->itemText(i), warehouseComboBox_->itemData(i));
        
        QComboBox locationCombo; // Populated dynamically based on warehouse selection
        connect(&warehouseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index){
            QString selectedWarehouseId = warehouseCombo.currentData().toString();
            populateLocationComboBox(&locationCombo, selectedWarehouseId.toStdString());
        });
        if (warehouseCombo.count() > 0) populateLocationComboBox(&locationCombo, warehouseCombo.itemData(0).toString().toStdString());

        QLineEdit quantityEdit; quantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit lotNumberEdit;
        QLineEdit serialNumberEdit;
        QLineEdit notesEdit;
        QLineEdit salesOrderItemIdEdit; // Optional link to sales order detail

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Kho hàng:*", &warehouseCombo);
        itemFormLayout.addRow("Vị trí:*", &locationCombo);
        itemFormLayout.addRow("Số lượng:*", &quantityEdit);
        itemFormLayout.addRow("Số lô:", &lotNumberEdit);
        itemFormLayout.addRow("Số Serial:", &serialNumberEdit);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);
        itemFormLayout.addRow("ID SP Đơn hàng (liên kết):", &salesOrderItemIdEdit);

        QPushButton *okItemButton = new QPushButton("Thêm", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (productCombo.currentData().isNull() || warehouseCombo.currentData().isNull() || locationCombo.currentData().isNull() || quantityEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Add to table
            int newRow = detailsTable->rowCount();
            detailsTable->insertRow(newRow);
            detailsTable->setItem(newRow, 0, new QTableWidgetItem(productCombo.currentText()));
            detailsTable->setItem(newRow, 1, new QTableWidgetItem(warehouseCombo.currentText()));
            detailsTable->setItem(newRow, 2, new QTableWidgetItem(locationCombo.currentText()));
            detailsTable->setItem(newRow, 3, new QTableWidgetItem(quantityEdit.text()));
            detailsTable->setItem(newRow, 4, new QTableWidgetItem(lotNumberEdit.text() + "/" + serialNumberEdit.text()));
            detailsTable->setItem(newRow, 5, new QTableWidgetItem(notesEdit.text()));
            detailsTable->setItem(newRow, 6, new QTableWidgetItem(salesOrderItemIdEdit.text()));
            detailsTable->item(newRow, 0)->setData(Qt::UserRole, productCombo.currentData()); // Store product ID
            detailsTable->item(newRow, 1)->setData(Qt::UserRole, warehouseCombo.currentData()); // Store warehouse ID
            detailsTable->item(newRow, 2)->setData(Qt::UserRole, locationCombo.currentData()); // Store location ID
            // No detail ID assigned yet until saved
        }
    });

    connect(editItemButton, &QPushButton::clicked, [&]() {
        int selectedItemRow = detailsTable->currentRow();
        if (selectedItemRow < 0) {
            showMessageBox("Sửa Chi tiết", "Vui lòng chọn một chi tiết để sửa.", QMessageBox::Information);
            return;
        }

        QDialog itemDialog(&dialog);
        itemDialog.setWindowTitle("Sửa Chi tiết Vận chuyển");
        QFormLayout itemFormLayout;
        QComboBox productCombo; populateProductComboBox();
        for (int i = 0; i < productService_->getAllProducts({}).size(); ++i) productCombo.addItem(productComboBox_->itemText(i), productComboBox_->itemData(i));
        
        QComboBox warehouseCombo; populateWarehouseComboBox();
        for (int i = 0; i < securityManager_->getWarehouseService()->getAllWarehouses({}).size(); ++i) warehouseCombo.addItem(warehouseComboBox_->itemText(i), warehouseComboBox_->itemData(i));
        
        QComboBox locationCombo;
        connect(&warehouseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index){
            QString selectedWarehouseId = warehouseCombo.currentData().toString();
            populateLocationComboBox(&locationCombo, selectedWarehouseId.toStdString());
        });

        QLineEdit quantityEdit; quantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit lotNumberEdit;
        QLineEdit serialNumberEdit;
        QLineEdit notesEdit;
        QLineEdit salesOrderItemIdEdit;

        // Populate with current item data
        QString currentProductId = detailsTable->item(selectedItemRow, 0)->data(Qt::UserRole).toString();
        int productIndex = productCombo.findData(currentProductId);
        if (productIndex != -1) productCombo.setCurrentIndex(productIndex); productCombo.setEnabled(false); // Fixed
        
        QString currentWarehouseId = detailsTable->item(selectedItemRow, 1)->data(Qt::UserRole).toString();
        int warehouseIndex = warehouseCombo.findData(currentWarehouseId);
        if (warehouseIndex != -1) warehouseCombo.setCurrentIndex(warehouseIndex);
        populateLocationComboBox(&locationCombo, currentWarehouseId.toStdString()); // Populate locations for current warehouse
        QString currentLocationId = detailsTable->item(selectedItemRow, 2)->data(Qt::UserRole).toString();
        int locationIndex = locationCombo.findData(currentLocationId);
        if (locationIndex != -1) locationCombo.setCurrentIndex(locationIndex);
        
        quantityEdit.setText(detailsTable->item(selectedItemRow, 3)->text());
        lotNumberEdit.setText(detailsTable->item(selectedItemRow, 4)->text().split("/")[0]);
        serialNumberEdit.setText(detailsTable->item(selectedItemRow, 4)->text().split("/").size() > 1 ? detailsTable->item(selectedItemRow, 4)->text().split("/")[1] : "");
        notesEdit.setText(detailsTable->item(selectedItemRow, 5)->text());
        salesOrderItemIdEdit.setText(detailsTable->item(selectedItemRow, 6)->text());

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Kho hàng:*", &warehouseCombo);
        itemFormLayout.addRow("Vị trí:*", &locationCombo);
        itemFormLayout.addRow("Số lượng:*", &quantityEdit);
        itemFormLayout.addRow("Số lô:", &lotNumberEdit);
        itemFormLayout.addRow("Số Serial:", &serialNumberEdit);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);
        itemFormLayout.addRow("ID SP Đơn hàng (liên kết):", &salesOrderItemIdEdit);

        QPushButton *okItemButton = new QPushButton("Lưu", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (productCombo.currentData().isNull() || warehouseCombo.currentData().isNull() || locationCombo.currentData().isNull() || quantityEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Update table row
            detailsTable->setItem(selectedItemRow, 0, new QTableWidgetItem(productCombo.currentText()));
            detailsTable->setItem(selectedItemRow, 1, new QTableWidgetItem(warehouseCombo.currentText()));
            detailsTable->setItem(selectedItemRow, 2, new QTableWidgetItem(locationCombo.currentText()));
            detailsTable->setItem(selectedItemRow, 3, new QTableWidgetItem(quantityEdit.text()));
            detailsTable->setItem(selectedItemRow, 4, new QTableWidgetItem(lotNumberEdit.text() + "/" + serialNumberEdit.text()));
            detailsTable->setItem(selectedItemRow, 5, new QTableWidgetItem(notesEdit.text()));
            detailsTable->setItem(selectedItemRow, 6, new QTableWidgetItem(salesOrderItemIdEdit.text()));
            detailsTable->item(selectedItemRow, 0)->setData(Qt::UserRole, productCombo.currentData());
            detailsTable->item(selectedItemRow, 1)->setData(Qt::UserRole, warehouseCombo.currentData());
            detailsTable->item(selectedItemRow, 2)->setData(Qt::UserRole, locationCombo.currentData());
        }
    });

    connect(deleteItemButton, &QPushButton::clicked, [&]() {
        int selectedItemRow = detailsTable->currentRow();
        if (selectedItemRow < 0) {
            showMessageBox("Xóa Chi tiết", "Vui lòng chọn một chi tiết để xóa.", QMessageBox::Information);
            return;
        }
        Common::CustomMessageBox confirmDelBox(&dialog);
        confirmDelBox.setWindowTitle("Xóa Chi tiết Vận chuyển");
        confirmDelBox.setText("Bạn có chắc chắn muốn xóa chi tiết vận chuyển này?");
        confirmDelBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmDelBox.exec() == QMessageBox::Yes) {
            detailsTable->removeRow(selectedItemRow);
        }
    });

    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Collect all items from the table
        std::vector<ERP::Sales::DTO::ShipmentDetailDTO> updatedDetails;
        for (int i = 0; i < detailsTable->rowCount(); ++i) {
            ERP::Sales::DTO::ShipmentDetailDTO detail;
            // Item ID is stored in product item's UserRole data
            QString existingId = detailsTable->item(i, 0)->data(Qt::UserRole).toString();
            detail.id = existingId.isEmpty() ? ERP::Utils::generateUUID() : existingId.toStdString();

            detail.shipmentId = shipment->id;
            detail.productId = detailsTable->item(i, 0)->data(Qt::UserRole).toString().toStdString();
            detail.warehouseId = detailsTable->item(i, 1)->data(Qt::UserRole).toString().toStdString();
            detail.locationId = detailsTable->item(i, 2)->data(Qt::UserRole).toString().toStdString();
            detail.quantity = detailsTable->item(i, 3)->text().toDouble();
            
            QString lotSerialText = detailsTable->item(i, 4)->text();
            QStringList lotSerialParts = lotSerialText.split("/");
            if (lotSerialParts.size() > 0) detail.lotNumber = lotSerialParts[0].toStdString();
            if (lotSerialParts.size() > 1) detail.serialNumber = lotSerialParts[1].toStdString();

            detail.notes = detailsTable->item(i, 5)->text().isEmpty() ? std::nullopt : std::make_optional(detailsTable->item(i, 5)->text().toStdString());
            detail.salesOrderId = detailsTable->item(i, 6)->text().isEmpty() ? std::nullopt : std::make_optional(detailsTable->item(i, 6)->text().toStdString());
            
            updatedDetails.push_back(detail);
        }

        // Call service to update Shipment with new detail list
        // This assumes shipmentService_->updateShipment takes the full list of details for replacement
        if (shipmentService_->updateShipment(*shipment, updatedDetails, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Quản lý Chi tiết", "Chi tiết vận chuyển đã được cập nhật thành công.", QMessageBox::Information);
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật chi tiết vận chuyển. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void ShipmentManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool ShipmentManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void ShipmentManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Sales.CreateShipment");
    bool canUpdate = hasPermission("Sales.UpdateShipment");
    bool canDelete = hasPermission("Sales.DeleteShipment");
    bool canChangeStatus = hasPermission("Sales.UpdateShipmentStatus");
    bool canManageDetails = hasPermission("Sales.ManageShipmentDetails"); // Assuming this permission

    addShipmentButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Sales.ViewShipments"));

    bool isRowSelected = shipmentTable_->currentRow() >= 0;
    editShipmentButton_->setEnabled(isRowSelected && canUpdate);
    deleteShipmentButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    manageDetailsButton_->setEnabled(isRowSelected && canManageDetails);


    bool enableForm = isRowSelected && canUpdate;
    shipmentNumberLineEdit_->setEnabled(enableForm); // Will be read-only for existing anyway
    salesOrderComboBox_->setEnabled(enableForm);
    customerComboBox_->setEnabled(enableForm);
    shippedByLineEdit_->setEnabled(enableForm);
    shipmentDateEdit_->setEnabled(enableForm);
    deliveryDateEdit_->setEnabled(enableForm);
    typeComboBox_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    carrierNameLineEdit_->setEnabled(enableForm);
    trackingNumberLineEdit_->setEnabled(enableForm);
    deliveryAddressLineEdit_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);

    // Read-only fields
    idLineEdit_->setEnabled(false);

    if (!isRowSelected) {
        idLineEdit_->clear();
        shipmentNumberLineEdit_->clear();
        salesOrderComboBox_->clear();
        customerComboBox_->clear();
        shippedByLineEdit_->clear();
        shipmentDateEdit_->clear();
        deliveryDateEdit_->clear();
        typeComboBox_->setCurrentIndex(0);
        statusComboBox_->setCurrentIndex(0);
        carrierNameLineEdit_->clear();
        trackingNumberLineEdit_->clear();
        deliveryAddressLineEdit_->clear();
        notesLineEdit_->clear();
    }
}


} // namespace Sales
} // namespace UI
} // namespace ERP