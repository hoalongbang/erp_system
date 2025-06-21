// Modules/UI/Warehouse/PickingRequestManagementWidget.cpp
#include "PickingRequestManagementWidget.h" // Standard includes
#include "PickingRequest.h"                 // PickingRequest DTO
#include "PickingDetail.h"                  // PickingDetail DTO
#include "SalesOrder.h"                     // SalesOrder DTO
#include "Product.h"                        // Product DTO
#include "Warehouse.h"                      // Warehouse DTO
#include "Location.h"                       // Location DTO
#include "PickingService.h"                 // Picking Service
#include "SalesOrderService.h"              // SalesOrder Service
#include "InventoryManagementService.h"     // InventoryManagement Service
#include "ISecurityManager.h"               // Security Manager
#include "Logger.h"                         // Logging
#include "ErrorHandler.h"                   // Error Handling
#include "Common.h"                         // Common Enums/Constants
#include "DateUtils.h"                      // Date Utilities
#include "StringUtils.h"                    // String Utilities
#include "CustomMessageBox.h"               // Custom Message Box
#include "UserService.h"                    // For getting user names

#include <QInputDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDoubleValidator>
#include <QDialogButtonBox>

namespace ERP {
namespace UI {
namespace Warehouse {

PickingRequestManagementWidget::PickingRequestManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IPickingService> pickingService,
    std::shared_ptr<Sales::Services::ISalesOrderService> salesOrderService,
    std::shared_ptr<Warehouse::Services::IInventoryManagementService> inventoryManagementService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      pickingService_(pickingService),
      salesOrderService_(salesOrderService),
      inventoryManagementService_(inventoryManagementService),
      securityManager_(securityManager) {

    if (!pickingService_ || !salesOrderService_ || !inventoryManagementService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ lấy hàng, đơn hàng bán, tồn kho hoặc bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("PickingRequestManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("PickingRequestManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("PickingRequestManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadPickingRequests();
    updateButtonsState();
}

PickingRequestManagementWidget::~PickingRequestManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void PickingRequestManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo ID đơn hàng bán...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &PickingRequestManagementWidget::onSearchRequestClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    requestTable_ = new QTableWidget(this);
    requestTable_->setColumnCount(7); // ID, Đơn hàng bán, Người YC, Ngày YC, Trạng thái, Người lấy, Ngày bắt đầu lấy
    requestTable_->setHorizontalHeaderLabels({"ID YC", "Đơn hàng bán", "Người YC", "Ngày YC", "Trạng thái", "Người lấy", "Ngày BĐ Lấy"});
    requestTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    requestTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    requestTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    requestTable_->horizontalHeader()->setStretchLastSection(true);
    connect(requestTable_, &QTableWidget::itemClicked, this, &PickingRequestManagementWidget::onRequestTableItemClicked);
    mainLayout->addWidget(requestTable_);

    // Form elements for editing/adding requests
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    salesOrderComboBox_ = new QComboBox(this); populateSalesOrderComboBox();
    requestedByLineEdit_ = new QLineEdit(this); requestedByLineEdit_->setReadOnly(true); // Should be current user
    pickedByLineEdit_ = new QLineEdit(this); // User ID, display name
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();
    pickStartTimeEdit_ = new QDateTimeEdit(this); pickStartTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    pickEndTimeEdit_ = new QDateTimeEdit(this); pickEndTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    notesLineEdit_ = new QLineEdit(this);


    formLayout->addWidget(new QLabel("ID YC:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("Đơn hàng bán:*", &salesOrderComboBox_);
    formLayout->addRow("Người yêu cầu:", &requestedByLineEdit_);
    formLayout->addRow("Người lấy:", &pickedByLineEdit_);
    formLayout->addRow("Trạng thái:*", &statusComboBox_);
    formLayout->addRow("Thời gian bắt đầu lấy:", &pickStartTimeEdit_);
    formLayout->addRow("Thời gian kết thúc lấy:", &pickEndTimeEdit_);
    formLayout->addRow("Ghi chú:", &notesLineEdit_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addRequestButton_ = new QPushButton("Thêm mới", this); connect(addRequestButton_, &QPushButton::clicked, this, &PickingRequestManagementWidget::onAddRequestClicked);
    editRequestButton_ = new QPushButton("Sửa", this); connect(editRequestButton_, &QPushButton::clicked, this, &PickingRequestManagementWidget::onEditRequestClicked);
    deleteRequestButton_ = new QPushButton("Xóa", this); connect(deleteRequestButton_, &QPushButton::clicked, this, &PickingRequestManagementWidget::onDeleteRequestClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &PickingRequestManagementWidget::onUpdateRequestStatusClicked);
    manageDetailsButton_ = new QPushButton("Quản lý Chi tiết", this); connect(manageDetailsButton_, &QPushButton::clicked, this, &PickingRequestManagementWidget::onManageDetailsClicked);
    recordPickedQuantityButton_ = new QPushButton("Ghi nhận SL đã lấy", this); connect(recordPickedQuantityButton_, &QPushButton::clicked, this, &PickingRequestManagementWidget::onRecordPickedQuantityClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &PickingRequestManagementWidget::onSearchRequestClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &PickingRequestManagementWidget::clearForm);
    
    buttonLayout->addWidget(addRequestButton_);
    buttonLayout->addWidget(editRequestButton_);
    buttonLayout->addWidget(deleteRequestButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(manageDetailsButton_);
    buttonLayout->addWidget(recordPickedQuantityButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void PickingRequestManagementWidget::loadPickingRequests() {
    ERP::Logger::Logger::getInstance().info("PickingRequestManagementWidget: Loading picking requests...");
    requestTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Warehouse::DTO::PickingRequestDTO> requests = pickingService_->getAllPickingRequests({}, currentUserId_, currentUserRoleIds_);

    requestTable_->setRowCount(requests.size());
    for (int i = 0; i < requests.size(); ++i) {
        const auto& request = requests[i];
        requestTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(request.id)));
        
        QString salesOrderNumber = "N/A";
        std::optional<ERP::Sales::DTO::SalesOrderDTO> so = salesOrderService_->getSalesOrderById(request.salesOrderId, currentUserId_, currentUserRoleIds_);
        if (so) salesOrderNumber = QString::fromStdString(so->orderNumber);
        requestTable_->setItem(i, 1, new QTableWidgetItem(salesOrderNumber));

        QString requestedByName = "N/A";
        std::optional<ERP::User::DTO::UserDTO> requestedByUser = securityManager_->getUserService()->getUserById(request.requestedByUserId, currentUserId_, currentUserRoleIds_);
        if (requestedByUser) requestedByName = QString::fromStdString(requestedByUser->username);
        requestTable_->setItem(i, 2, new QTableWidgetItem(requestedByName));

        requestTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(request.createdAt, ERP::Common::DATETIME_FORMAT)))); // Assuming createdAt as requested date
        requestTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(request.getStatusString())));
        
        QString pickedByName = "N/A";
        if (request.pickedByUserId) {
            std::optional<ERP::User::DTO::UserDTO> pickedByUser = securityManager_->getUserService()->getUserById(*request.pickedByUserId, currentUserId_, currentUserRoleIds_);
            if (pickedByUser) pickedByName = QString::fromStdString(pickedByUser->username);
        }
        requestTable_->setItem(i, 5, new QTableWidgetItem(pickedByName));
        requestTable_->setItem(i, 6, new QTableWidgetItem(request.pickStartTime ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*request.pickStartTime, ERP::Common::DATETIME_FORMAT)) : "N/A"));
    }
    requestTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("PickingRequestManagementWidget: Picking requests loaded successfully.");
}

void PickingRequestManagementWidget::populateSalesOrderComboBox() {
    salesOrderComboBox_->clear();
    std::vector<ERP::Sales::DTO::SalesOrderDTO> allSalesOrders = salesOrderService_->getAllSalesOrders({}, currentUserId_, currentUserRoleIds_);
    for (const auto& so : allSalesOrders) {
        salesOrderComboBox_->addItem(QString::fromStdString(so.orderNumber), QString::fromStdString(so.id));
    }
}

void PickingRequestManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Warehouse::DTO::PickingRequestStatus::PENDING));
    statusComboBox_->addItem("In Progress", static_cast<int>(ERP::Warehouse::DTO::PickingRequestStatus::IN_PROGRESS));
    statusComboBox_->addItem("Completed", static_cast<int>(ERP::Warehouse::DTO::PickingRequestStatus::COMPLETED));
    statusComboBox_->addItem("Cancelled", static_cast<int>(ERP::Warehouse::DTO::PickingRequestStatus::CANCELLED));
    statusComboBox_->addItem("Partially Picked", static_cast<int>(ERP::Warehouse::DTO::PickingRequestStatus::PARTIALLY_PICKED));
}

void PickingRequestManagementWidget::populateUserComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("None", "");
    std::vector<ERP::User::DTO::UserDTO> allUsers = securityManager_->getUserService()->getAllUsers({}, currentUserId_, currentUserRoleIds_);
    for (const auto& user : allUsers) {
        comboBox->addItem(QString::fromStdString(user.username), QString::fromStdString(user.id));
    }
}

void PickingRequestManagementWidget::populateProductComboBox(QComboBox* comboBox) {
    comboBox->clear();
    std::vector<ERP::Product::DTO::ProductDTO> allProducts = securityManager_->getProductService()->getAllProducts({}, currentUserId_, currentUserRoleIds_);
    for (const auto& product : allProducts) {
        comboBox->addItem(QString::fromStdString(product.name + " (" + product.productCode + ")"), QString::fromStdString(product.id));
    }
}

void PickingRequestManagementWidget::populateWarehouseComboBox(QComboBox* comboBox) {
    comboBox->clear();
    std::vector<ERP::Catalog::DTO::WarehouseDTO> allWarehouses = securityManager_->getWarehouseService()->getAllWarehouses({}, currentUserId_, currentUserRoleIds_);
    for (const auto& warehouse : allWarehouses) {
        comboBox->addItem(QString::fromStdString(warehouse.name), QString::fromStdString(warehouse.id));
    }
}

void PickingRequestManagementWidget::populateLocationComboBox(QComboBox* comboBox, const std::string& warehouseId) {
    comboBox->clear();
    std::vector<ERP::Catalog::DTO::LocationDTO> locations;
    if (!warehouseId.empty()) {
        locations = securityManager_->getWarehouseService()->getLocationsByWarehouse(warehouseId, currentUserId_, currentUserRoleIds_);
    }
    for (const auto& location : locations) {
        comboBox->addItem(QString::fromStdString(location.name), QString::fromStdString(location.id));
    }
}


void PickingRequestManagementWidget::onAddRequestClicked() {
    if (!hasPermission("Warehouse.CreatePickingRequest")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm yêu cầu lấy hàng.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateSalesOrderComboBox();
    showRequestInputDialog();
}

void PickingRequestManagementWidget::onEditRequestClicked() {
    if (!hasPermission("Warehouse.UpdatePickingRequest")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa yêu cầu lấy hàng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Yêu Cầu Lấy Hàng", "Vui lòng chọn một yêu cầu lấy hàng để sửa.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Warehouse::DTO::PickingRequestDTO> requestOpt = pickingService_->getPickingRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (requestOpt) {
        populateSalesOrderComboBox();
        showRequestInputDialog(&(*requestOpt));
    } else {
        showMessageBox("Sửa Yêu Cầu Lấy Hàng", "Không tìm thấy yêu cầu lấy hàng để sửa.", QMessageBox::Critical);
    }
}

void PickingRequestManagementWidget::onDeleteRequestClicked() {
    if (!hasPermission("Warehouse.DeletePickingRequest")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa yêu cầu lấy hàng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Yêu Cầu Lấy Hàng", "Vui lòng chọn một yêu cầu lấy hàng để xóa.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    QString salesOrderNum = requestTable_->item(selectedRow, 1)->text(); // Display sales order number

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Yêu Cầu Lấy Hàng");
    confirmBox.setText("Bạn có chắc chắn muốn xóa yêu cầu lấy hàng cho đơn hàng '" + salesOrderNum + "' (ID: " + requestId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (pickingService_->deletePickingRequest(requestId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Yêu Cầu Lấy Hàng", "Yêu cầu lấy hàng đã được xóa thành công.", QMessageBox::Information);
            loadPickingRequests();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa yêu cầu lấy hàng. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void PickingRequestManagementWidget::onUpdateRequestStatusClicked() {
    if (!hasPermission("Warehouse.UpdatePickingRequestStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái yêu cầu lấy hàng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
    showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một yêu cầu lấy hàng để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Warehouse::DTO::PickingRequestDTO> requestOpt = pickingService_->getPickingRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!requestOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy yêu cầu lấy hàng để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Warehouse::DTO::PickingRequestDTO currentRequest = *requestOpt;
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
    int currentStatusIndex = newStatusCombo.findData(static_cast<int>(currentRequest.status));
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
        ERP::Warehouse::DTO::PickingRequestStatus newStatus = static_cast<ERP::Warehouse::DTO::PickingRequestStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái yêu cầu lấy hàng");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái yêu cầu lấy hàng này thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (pickingService_->updatePickingRequestStatus(requestId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái yêu cầu lấy hàng đã được cập nhật thành công.", QMessageBox::Information);
                loadPickingRequests();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái yêu cầu lấy hàng. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}


void PickingRequestManagementWidget::onSearchRequestClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["sales_order_id_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    requestTable_->setRowCount(0);
    std::vector<ERP::Warehouse::DTO::PickingRequestDTO> requests = pickingService_->getAllPickingRequests(filter, currentUserId_, currentUserRoleIds_);

    requestTable_->setRowCount(requests.size());
    for (int i = 0; i < requests.size(); ++i) {
        const auto& request = requests[i];
        requestTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(request.id)));
        
        QString salesOrderNumber = "N/A";
        std::optional<ERP::Sales::DTO::SalesOrderDTO> so = salesOrderService_->getSalesOrderById(request.salesOrderId, currentUserId_, currentUserRoleIds_);
        if (so) salesOrderNumber = QString::fromStdString(so->orderNumber);
        requestTable_->setItem(i, 1, new QTableWidgetItem(salesOrderNumber));

        QString requestedByName = "N/A";
        std::optional<ERP::User::DTO::UserDTO> requestedByUser = securityManager_->getUserService()->getUserById(request.requestedByUserId, currentUserId_, currentUserRoleIds_);
        if (requestedByUser) requestedByName = QString::fromStdString(requestedByUser->username);
        requestTable_->setItem(i, 2, new QTableWidgetItem(requestedByName));

        requestTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(request.createdAt, ERP::Common::DATETIME_FORMAT))));
        requestTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(request.getStatusString())));
        
        QString pickedByName = "N/A";
        if (request.pickedByUserId) {
            std::optional<ERP::User::DTO::UserDTO> pickedByUser = securityManager_->getUserService()->getUserById(*request.pickedByUserId, currentUserId_, currentUserRoleIds_);
            if (pickedByUser) pickedByName = QString::fromStdString(pickedByUser->username);
        }
        requestTable_->setItem(i, 5, new QTableWidgetItem(pickedByName));
        requestTable_->setItem(i, 6, new QTableWidgetItem(request.pickStartTime ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*request.pickStartTime, ERP::Common::DATETIME_FORMAT)) : "N/A"));
    }
    requestTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("PickingRequestManagementWidget: Search completed.");
}

void PickingRequestManagementWidget::onRequestTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString requestId = requestTable_->item(row, 0)->text();
    std::optional<ERP::Warehouse::DTO::PickingRequestDTO> requestOpt = pickingService_->getPickingRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (requestOpt) {
        idLineEdit_->setText(QString::fromStdString(requestOpt->id));
        
        populateSalesOrderComboBox();
        int salesOrderIndex = salesOrderComboBox_->findData(QString::fromStdString(requestOpt->salesOrderId));
        if (salesOrderIndex != -1) salesOrderComboBox_->setCurrentIndex(salesOrderIndex);

        requestedByLineEdit_->setText(QString::fromStdString(requestOpt->requestedByUserId)); // Display ID
        pickedByLineEdit_->setText(QString::fromStdString(requestOpt->pickedByUserId.value_or(""))); // Display ID
        
        populateStatusComboBox();
        int statusIndex = statusComboBox_->findData(static_cast<int>(requestOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);

        if (requestOpt->pickStartTime) pickStartTimeEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(requestOpt->pickStartTime->time_since_epoch()).count()));
        else pickStartTimeEdit_->clear();
        if (requestOpt->pickEndTime) pickEndTimeEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(requestOpt->pickEndTime->time_since_epoch()).count()));
        else pickEndTimeEdit_->clear();
        notesLineEdit_->setText(QString::fromStdString(requestOpt->notes.value_or("")));

    } else {
        showMessageBox("Thông tin Yêu Cầu Lấy Hàng", "Không tìm thấy yêu cầu lấy hàng đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void PickingRequestManagementWidget::clearForm() {
    idLineEdit_->clear();
    salesOrderComboBox_->clear();
    requestedByLineEdit_->clear();
    pickedByLineEdit_->clear();
    statusComboBox_->setCurrentIndex(0);
    pickStartTimeEdit_->clear();
    pickEndTimeEdit_->clear();
    notesLineEdit_->clear();
    requestTable_->clearSelection();
    updateButtonsState();
}

void PickingRequestManagementWidget::onManageDetailsClicked() {
    if (!hasPermission("Warehouse.ManagePickingDetails")) { // Assuming this permission
        showMessageBox("Lỗi", "Bạn không có quyền quản lý chi tiết lấy hàng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Quản lý Chi tiết", "Vui lòng chọn một yêu cầu lấy hàng để quản lý chi tiết.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Warehouse::DTO::PickingRequestDTO> requestOpt = pickingService_->getPickingRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (requestOpt) {
        showManageDetailsDialog(&(*requestOpt));
    } else {
        showMessageBox("Quản lý Chi tiết", "Không tìm thấy yêu cầu lấy hàng để quản lý chi tiết.", QMessageBox::Critical);
    }
}

void PickingRequestManagementWidget::onRecordPickedQuantityClicked() {
    if (!hasPermission("Warehouse.RecordPickedQuantity")) {
        showMessageBox("Lỗi", "Bạn không có quyền ghi nhận số lượng đã lấy.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Ghi nhận SL đã lấy", "Vui lòng chọn một yêu cầu lấy hàng trước.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Warehouse::DTO::PickingRequestDTO> parentRequestOpt = pickingService_->getPickingRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);
    if (!parentRequestOpt) {
        showMessageBox("Ghi nhận SL đã lấy", "Không tìm thấy yêu cầu lấy hàng.", QMessageBox::Critical);
        return;
    }

    // Show a dialog to select a detail and input quantity
    QDialog dialog(this);
    dialog.setWindowTitle("Ghi nhận Số lượng Đã lấy Thực tế");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox detailComboBox;
    // Populate with details of the selected request
    std::vector<ERP::Warehouse::DTO::PickingDetailDTO> details = pickingService_->getPickingDetails(requestId.toStdString(), currentUserId_, currentUserRoleIds_);
    for (const auto& detail : details) {
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = securityManager_->getProductService()->getProductById(detail.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);
        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = securityManager_->getWarehouseService()->getWarehouseById(detail.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) warehouseName = QString::fromStdString(warehouse->name);
        QString locationName = "N/A";
        std::optional<ERP::Catalog::DTO::LocationDTO> location = securityManager_->getWarehouseService()->getLocationById(detail.locationId, currentUserId_, currentUserRoleIds_);
        if (location) locationName = QString::fromStdString(location->name);

        detailComboBox.addItem(productName + " (" + warehouseName + "/" + locationName + ") (YC: " + QString::number(detail.requestedQuantity) + ", Đã lấy: " + QString::number(detail.pickedQuantity) + ")", QString::fromStdString(detail.id));
    }

    QLineEdit quantityLineEdit;
    quantityLineEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));

    formLayout->addRow("Chọn Chi tiết:", &detailComboBox);
    formLayout->addRow("Số lượng Đã lấy Thực tế:*", &quantityLineEdit);
    layout->addLayout(formLayout);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString selectedDetailId = detailComboBox.currentData().toString();
        double quantity = quantityLineEdit.text().toDouble();

        std::optional<ERP::Warehouse::DTO::PickingDetailDTO> selectedDetailOpt = pickingService_->getPickingDetailById(selectedDetailId.toStdString()); // Needs to be fixed to take currentUser, roles
        if (!selectedDetailOpt) {
            showMessageBox("Lỗi", "Không tìm thấy chi tiết lấy hàng đã chọn.", QMessageBox::Critical);
            return;
        }

        if (pickingService_->recordPickedQuantity(selectedDetailId.toStdString(), quantity, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Ghi nhận SL đã lấy", "Số lượng đã lấy được ghi nhận thành công.", QMessageBox::Information);
            loadPickingRequests(); // Refresh parent table
            clearForm();
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể ghi nhận số lượng đã lấy. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void PickingRequestManagementWidget::showRequestInputDialog(ERP::Warehouse::DTO::PickingRequestDTO* request) {
    QDialog dialog(this);
    dialog.setWindowTitle(request ? "Sửa Yêu Cầu Lấy Hàng" : "Thêm Yêu Cầu Lấy Hàng Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox salesOrderCombo(this); populateSalesOrderComboBox();
    for (int i = 0; i < salesOrderComboBox_->count(); ++i) salesOrderCombo.addItem(salesOrderComboBox_->itemText(i), salesOrderComboBox_->itemData(i));
    
    QLineEdit requestedByEdit(this); requestedByEdit.setReadOnly(true); // Auto-filled
    QComboBox pickedByCombo(this); populateUserComboBox(&pickedByCombo); // User who performs picking
    QDateTimeEdit pickStartTimeEdit(this); pickStartTimeEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QDateTimeEdit pickEndTimeEdit(this); pickEndTimeEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QLineEdit notesEdit(this);
    QComboBox statusCombo(this); populateStatusComboBox();
    for(int i = 0; i < statusComboBox_->count(); ++i) statusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));


    if (request) {
        int salesOrderIndex = salesOrderCombo.findData(QString::fromStdString(request->salesOrderId));
        if (salesOrderIndex != -1) salesOrderCombo.setCurrentIndex(salesOrderIndex);
        requestedByEdit.setText(QString::fromStdString(request->requestedByUserId));
        if (request->pickedByUserId) {
            int pickedByIndex = pickedByCombo.findData(QString::fromStdString(*request->pickedByUserId));
            if (pickedByIndex != -1) pickedByCombo.setCurrentIndex(pickedByIndex);
            else pickedByCombo.setCurrentIndex(0);
        } else {
            pickedByCombo.setCurrentIndex(0);
        }
        int statusIndex = statusCombo.findData(static_cast<int>(request->status));
        if (statusIndex != -1) statusCombo.setCurrentIndex(statusIndex);
        if (request->pickStartTime) pickStartTimeEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(request->pickStartTime->time_since_epoch()).count()));
        else pickStartTimeEdit.clear();
        if (request->pickEndTime) pickEndTimeEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(request->pickEndTime->time_since_epoch()).count()));
        else pickEndTimeEdit.clear();
        notesEdit.setText(QString::fromStdString(request->notes.value_or("")));

        // Sales order is usually immutable for existing requests
        salesOrderCombo.setEnabled(false);
    } else {
        requestedByEdit.setText(QString::fromStdString(currentUserId_));
        pickStartTimeEdit.setDateTime(QDateTime::currentDateTime());
        // Defaults for new
    }

    formLayout->addRow("Đơn hàng bán:*", &salesOrderCombo);
    formLayout->addRow("Người yêu cầu:", &requestedByEdit);
    formLayout->addRow("Người lấy:", &pickedByCombo);
    formLayout->addRow("Trạng thái:*", &statusCombo);
    formLayout->addRow("Thời gian bắt đầu lấy:", &pickStartTimeEdit);
    formLayout->addRow("Thời gian kết thúc lấy:", &pickEndTimeEdit);
    formLayout->addRow("Ghi chú:", &notesEdit);
    
    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(request ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Warehouse::DTO::PickingRequestDTO newRequestData;
        if (request) {
            newRequestData = *request;
        } else {
            newRequestData.id = ERP::Utils::generateUUID(); // New ID for new request
        }

        newRequestData.salesOrderId = salesOrderCombo.currentData().toString().toStdString();
        newRequestData.requestedByUserId = requestedByEdit.text().toStdString();
        
        QString selectedPickedById = pickedByCombo.currentData().toString();
        newRequestData.pickedByUserId = selectedPickedById.isEmpty() ? std::nullopt : std::make_optional(selectedPickedById.toStdString());

        newRequestData.status = static_cast<ERP::Warehouse::DTO::PickingRequestStatus>(statusCombo.currentData().toInt());
        newRequestData.pickStartTime = pickStartTimeEdit.dateTime().isNull() ? std::nullopt : std::make_optional(ERP::Utils::DateUtils::qDateTimeToTimePoint(pickStartTimeEdit.dateTime()));
        newRequestData.pickEndTime = pickEndTimeEdit.dateTime().isNull() ? std::nullopt : std::make_optional(ERP::Utils::DateUtils::qDateTimeToTimePoint(pickEndTimeEdit.dateTime()));
        newRequestData.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());

        // For new requests, details are added in a separate dialog after creation
        std::vector<ERP::Warehouse::DTO::PickingDetailDTO> currentDetails;
        if (request) { // When editing, load existing details first
             currentDetails = pickingService_->getPickingDetails(request->id, currentUserId_, currentUserRoleIds_); // Ensure this method exists and takes current user/roles
        }

        bool success = false;
        if (request) {
            success = pickingService_->updatePickingRequest(newRequestData, currentDetails, currentUserId_, currentUserRoleIds_); // Pass existing details if not modified
            if (success) showMessageBox("Sửa Yêu Cầu Lấy Hàng", "Yêu cầu lấy hàng đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật yêu cầu lấy hàng. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Warehouse::DTO::PickingRequestDTO> createdRequest = pickingService_->createPickingRequest(newRequestData, {}, currentUserId_, currentUserRoleIds_); // Create with empty details
            if (createdRequest) {
                showMessageBox("Thêm Yêu Cầu Lấy Hàng", "Yêu cầu lấy hàng mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm yêu cầu lấy hàng mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadPickingRequests();
            clearForm();
        }
    }
}

void PickingRequestManagementWidget::showManageDetailsDialog(ERP::Warehouse::DTO::PickingRequestDTO* request) {
    if (!request) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Quản lý Chi tiết Yêu Cầu Lấy Hàng: " + QString::fromStdString(request->salesOrderId));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *detailsTable = new QTableWidget(&dialog);
    detailsTable->setColumnCount(7); // Product, Warehouse, Location, Requested Qty, Picked Qty, Is Picked, Notes
    detailsTable->setHorizontalHeaderLabels({"Sản phẩm", "Kho hàng", "Vị trí", "SL YC", "SL Đã lấy", "Đã lấy đủ", "Ghi chú"});
    detailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    detailsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    detailsTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(detailsTable);

    // Load existing details
    std::vector<ERP::Warehouse::DTO::PickingDetailDTO> currentDetails = pickingService_->getPickingDetails(request->id, currentUserId_, currentUserRoleIds_);
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
        detailsTable->setItem(i, 3, new QTableWidgetItem(QString::number(detail.requestedQuantity)));
        detailsTable->setItem(i, 4, new QTableWidgetItem(QString::number(detail.pickedQuantity)));
        detailsTable->setItem(i, 5, new QTableWidgetItem(detail.isPicked ? "Yes" : "No"));
        detailsTable->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(detail.notes.value_or(""))));
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
        itemDialog.setWindowTitle("Thêm Chi tiết Yêu Cầu Lấy Hàng");
        QFormLayout itemFormLayout;
        QComboBox productCombo; populateProductComboBox(&productCombo);
        QComboBox warehouseCombo; populateWarehouseComboBox(&warehouseCombo);
        QComboBox locationCombo;
        connect(&warehouseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index){
            QString selectedWarehouseId = warehouseCombo.currentData().toString();
            populateLocationComboBox(&locationCombo, selectedWarehouseId.toStdString());
        });
        if (warehouseCombo.count() > 0) populateLocationComboBox(&locationCombo, warehouseCombo.itemData(0).toString().toStdString());

        QLineEdit requestedQuantityEdit; requestedQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit lotNumberEdit;
        QLineEdit serialNumberEdit;
        QLineEdit notesEdit;

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Kho hàng:*", &warehouseCombo);
        itemFormLayout.addRow("Vị trí:*", &locationCombo);
        itemFormLayout.addRow("Số lượng YC:*", &requestedQuantityEdit);
        itemFormLayout.addRow("Số lô:", &lotNumberEdit);
        itemFormLayout.addRow("Số Serial:", &serialNumberEdit);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);

        QPushButton *okItemButton = new QPushButton("Thêm", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (productCombo.currentData().isNull() || warehouseCombo.currentData().isNull() || locationCombo.currentData().isNull() || requestedQuantityEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Add to table
            int newRow = detailsTable->rowCount();
            detailsTable->insertRow(newRow);
            detailsTable->setItem(newRow, 0, new QTableWidgetItem(productCombo.currentText()));
            detailsTable->setItem(newRow, 1, new QTableWidgetItem(warehouseCombo.currentText()));
            detailsTable->setItem(newRow, 2, new QTableWidgetItem(locationCombo.currentText()));
            detailsTable->setItem(newRow, 3, new QTableWidgetItem(requestedQuantityEdit.text()));
            detailsTable->setItem(newRow, 4, new QTableWidgetItem("0.0")); // Picked Qty
            detailsTable->setItem(newRow, 5, new QTableWidgetItem("No")); // Is Picked
            detailsTable->setItem(newRow, 6, new QTableWidgetItem(notesEdit.text()));
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
        itemDialog.setWindowTitle("Sửa Chi tiết Yêu Cầu Lấy Hàng");
        QFormLayout itemFormLayout;
        QComboBox productCombo; populateProductComboBox(&productCombo);
        QComboBox warehouseCombo; populateWarehouseComboBox(&warehouseCombo);
        QComboBox locationCombo;
        connect(&warehouseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index){
            QString selectedWarehouseId = warehouseCombo.currentData().toString();
            populateLocationComboBox(&locationCombo, selectedWarehouseId.toStdString());
        });

        QLineEdit requestedQuantityEdit; requestedQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit lotNumberEdit;
        QLineEdit serialNumberEdit;
        QLineEdit notesEdit;

        // Populate with current item data
        QString currentProductId = detailsTable->item(selectedItemRow, 0)->data(Qt::UserRole).toString();
        int productIndex = productCombo.findData(currentProductId);
        if (productIndex != -1) productCombo.setCurrentIndex(productIndex);

        QString currentWarehouseId = detailsTable->item(selectedItemRow, 1)->data(Qt::UserRole).toString();
        int warehouseIndex = warehouseCombo.findData(currentWarehouseId);
        if (warehouseIndex != -1) warehouseCombo.setCurrentIndex(warehouseIndex);
        populateLocationComboBox(&locationCombo, currentWarehouseId.toStdString()); // Populate locations for current warehouse
        QString currentLocationId = detailsTable->item(selectedItemRow, 2)->data(Qt::UserRole).toString();
        int locationIndex = locationCombo.findData(currentLocationId);
        if (locationIndex != -1) locationCombo.setCurrentIndex(locationIndex);

        requestedQuantityEdit.setText(detailsTable->item(selectedItemRow, 3)->text());
        QString lotSerialText = detailsTable->item(selectedItemRow, 4)->text().split("/")[0]; // Assuming 4 holds lot/serial
        QStringList lotSerialParts = detailsTable->item(selectedItemRow, 4)->text().split("/");
        if (lotSerialParts.size() > 0) lotNumberEdit.setText(lotSerialParts[0]);
        if (lotSerialParts.size() > 1) serialNumberEdit.setText(lotSerialParts[1]);
        notesEdit.setText(detailsTable->item(selectedItemRow, 6)->text());

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Kho hàng:*", &warehouseCombo);
        itemFormLayout.addRow("Vị trí:*", &locationCombo);
        itemFormLayout.addRow("Số lượng YC:*", &requestedQuantityEdit);
        itemFormLayout.addRow("Số lô:", &lotNumberEdit);
        itemFormLayout.addRow("Số Serial:", &serialNumberEdit);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);

        QPushButton *okItemButton = new QPushButton("Lưu", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (productCombo.currentData().isNull() || warehouseCombo.currentData().isNull() || locationCombo.currentData().isNull() || requestedQuantityEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Update table row
            detailsTable->item(selectedItemRow, 0)->setText(productCombo.currentText());
            detailsTable->item(selectedItemRow, 1)->setText(warehouseCombo.currentText());
            detailsTable->item(selectedItemRow, 2)->setText(locationCombo.currentText());
            detailsTable->item(selectedItemRow, 3)->setText(requestedQuantityEdit.text());
            detailsTable->item(selectedItemRow, 4)->setText(lotNumberEdit.text() + "/" + serialNumberEdit.text());
            detailsTable->item(selectedItemRow, 6)->setText(notesEdit.text());
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
        confirmDelBox.setWindowTitle("Xóa Chi tiết Yêu Cầu Lấy Hàng");
        confirmDelBox.setText("Bạn có chắc chắn muốn xóa chi tiết yêu cầu lấy hàng này?");
        confirmDelBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmDelBox.exec() == QMessageBox::Yes) {
            detailsTable->removeRow(selectedItemRow);
        }
    });

    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Collect all items from the table
        std::vector<ERP::Warehouse::DTO::PickingDetailDTO> updatedDetails;
        for (int i = 0; i < detailsTable->rowCount(); ++i) {
            ERP::Warehouse::DTO::PickingDetailDTO detail;
            // If item had an ID (from existing data), reuse it, otherwise generate new
            QString existingId = detailsTable->item(i, 0)->data(Qt::UserRole).toString();
            detail.id = existingId.isEmpty() ? ERP::Utils::generateUUID() : existingId.toStdString();

            detail.pickingRequestId = request->id;
            detail.productId = detailsTable->item(i, 0)->data(Qt::UserRole).toString().toStdString();
            detail.warehouseId = detailsTable->item(i, 1)->data(Qt::UserRole).toString().toStdString();
            detail.locationId = detailsTable->item(i, 2)->data(Qt::UserRole).toString().toStdString();
            detail.requestedQuantity = detailsTable->item(i, 3)->text().toDouble();
            detail.pickedQuantity = detailsTable->item(i, 4)->text().toDouble(); // Keep current picked qty
            
            QString lotSerialText = detailsTable->item(i, 4)->text(); // Assuming lot/serial is in column 4
            QStringList lotSerialParts = lotSerialText.split("/");
            if (lotSerialParts.size() > 0) detail.lotNumber = lotSerialParts[0].toStdString();
            if (lotSerialParts.size() > 1) detail.serialNumber = lotSerialParts[1].toStdString();

            detail.isPicked = detailsTable->item(i, 5)->text() == "Yes"; // Keep current status
            detail.notes = detailsTable->item(i, 6)->text().isEmpty() ? std::nullopt : std::make_optional(detailsTable->item(i, 6)->text().toStdString());
            
            updatedDetails.push_back(detail);
        }

        // Call service to update Picking Request with new detail list
        if (pickingService_->updatePickingRequest(*request, updatedDetails, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Quản lý Chi tiết", "Chi tiết yêu cầu lấy hàng đã được cập nhật thành công.", QMessageBox::Information);
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật chi tiết yêu cầu lấy hàng. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void PickingRequestManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool PickingRequestManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void PickingRequestManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Warehouse.CreatePickingRequest");
    bool canUpdate = hasPermission("Warehouse.UpdatePickingRequest");
    bool canDelete = hasPermission("Warehouse.DeletePickingRequest");
    bool canChangeStatus = hasPermission("Warehouse.UpdatePickingRequestStatus");
    bool canManageDetails = hasPermission("Warehouse.ManagePickingDetails"); // Assuming this permission
    bool canRecordQuantity = hasPermission("Warehouse.RecordPickedQuantity");

    addRequestButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Warehouse.ViewPickingRequests"));

    bool isRowSelected = requestTable_->currentRow() >= 0;
    editRequestButton_->setEnabled(isRowSelected && canUpdate);
    deleteRequestButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    manageDetailsButton_->setEnabled(isRowSelected && canManageDetails);
    recordPickedQuantityButton_->setEnabled(isRowSelected && canRecordQuantity);


    bool enableForm = isRowSelected && canUpdate;
    salesOrderComboBox_->setEnabled(enableForm);
    // requestedByLineEdit_ is read-only
    pickedByLineEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    pickStartTimeEdit_->setEnabled(enableForm);
    pickEndTimeEdit_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);

    // Read-only fields
    idLineEdit_->setEnabled(false);

    if (!isRowSelected) {
        idLineEdit_->clear();
        salesOrderComboBox_->clear();
        requestedByLineEdit_->clear();
        pickedByLineEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
        pickStartTimeEdit_->clear();
        pickEndTimeEdit_->clear();
        notesLineEdit_->clear();
    }
}


} // namespace Warehouse
} // namespace UI
} // namespace ERP