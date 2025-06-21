// UI/Warehouse/StocktakeRequestManagementWidget.cpp
#include "StocktakeManagementWidget.h" // Standard includes
#include "StocktakeRequest.h"          // StocktakeRequest DTO
#include "StocktakeDetail.h"           // StocktakeDetail DTO
#include "InventoryTransaction.h"      // InventoryTransaction DTO
#include "Product.h"                   // Product DTO
#include "Warehouse.h"                 // Warehouse DTO
#include "Location.h"                  // Location DTO
#include "StocktakeService.h"          // Stocktake Service
#include "InventoryManagementService.h"// InventoryManagement Service
#include "WarehouseService.h"          // Warehouse Service
#include "ProductService.h"            // Product Service
#include "ISecurityManager.h"          // Security Manager
#include "Logger.h"                    // Logging
#include "ErrorHandler.h"              // Error Handling
#include "Common.h"                    // Common Enums/Constants
#include "DateUtils.h"                 // Date Utilities
#include "StringUtils.h"               // String Utilities
#include "CustomMessageBox.h"          // Custom Message Box
#include "UserService.h"               // For getting user names

#include <QInputDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDoubleValidator>
#include <QDialogButtonBox>

namespace ERP {
namespace UI {
namespace Warehouse {

StocktakeRequestManagementWidget::StocktakeRequestManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IStocktakeService> stocktakeService,
    std::shared_ptr<Warehouse::Services::IInventoryManagementService> inventoryManagementService,
    std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      stocktakeService_(stocktakeService),
      inventoryManagementService_(inventoryManagementService),
      warehouseService_(warehouseService),
      securityManager_(securityManager) {

    if (!stocktakeService_ || !inventoryManagementService_ || !warehouseService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ kiểm kê, tồn kho, kho hàng hoặc bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("StocktakeRequestManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("StocktakeRequestManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("StocktakeRequestManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadStocktakeRequests();
    updateButtonsState();
}

StocktakeRequestManagementWidget::~StocktakeRequestManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void StocktakeRequestManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo ID kho hàng...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &StocktakeRequestManagementWidget::onSearchRequestClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    requestTable_ = new QTableWidget(this);
    requestTable_->setColumnCount(6); // ID, Kho hàng, Địa điểm, Người YC, Ngày đếm, Trạng thái
    requestTable_->setHorizontalHeaderLabels({"ID YC", "Kho hàng", "Địa điểm", "Người YC", "Ngày đếm", "Trạng thái"});
    requestTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    requestTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    requestTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    requestTable_->horizontalHeader()->setStretchLastSection(true);
    connect(requestTable_, &QTableWidget::itemClicked, this, &StocktakeRequestManagementWidget::onRequestTableItemClicked);
    mainLayout->addWidget(requestTable_);

    // Form elements for editing/adding requests
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    warehouseComboBox_ = new QComboBox(this); populateWarehouseComboBox();
    locationComboBox_ = new QComboBox(this); // Populated dynamically based on warehouse selection
    connect(warehouseComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index){
        QString selectedWarehouseId = warehouseComboBox_->currentData().toString();
        populateLocationComboBox(selectedWarehouseId.toStdString());
    });
    // Initial populate for location based on default/first warehouse
    if (warehouseComboBox_->count() > 0) populateLocationComboBox(warehouseComboBox_->itemData(0).toString().toStdString());
    
    requestedByLineEdit_ = new QLineEdit(this); requestedByLineEdit_->setReadOnly(true); // Should be current user
    countedByLineEdit_ = new QLineEdit(this); // User ID, display name
    countDateEdit_ = new QDateTimeEdit(this); countDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();
    notesLineEdit_ = new QLineEdit(this);


    formLayout->addWidget(new QLabel("ID YC:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("Kho hàng:*", &warehouseComboBox_);
    formLayout->addRow("Vị trí (tùy chọn):", &locationComboBox_);
    formLayout->addRow("Người yêu cầu:", &requestedByLineEdit_);
    formLayout->addRow("Người đếm:", &countedByLineEdit_);
    formLayout->addRow("Ngày đếm:*", &countDateEdit_);
    formLayout->addRow("Trạng thái:*", &statusComboBox_);
    formLayout->addRow("Ghi chú:", &notesLineEdit_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addRequestButton_ = new QPushButton("Thêm mới", this); connect(addRequestButton_, &QPushButton::clicked, this, &StocktakeRequestManagementWidget::onAddRequestClicked);
    editRequestButton_ = new QPushButton("Sửa", this); connect(editRequestButton_, &QPushButton::clicked, this, &StocktakeRequestManagementWidget::onEditRequestClicked);
    deleteRequestButton_ = new QPushButton("Xóa", this); connect(deleteRequestButton_, &QPushButton::clicked, this, &StocktakeRequestManagementWidget::onDeleteRequestClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &StocktakeRequestManagementWidget::onUpdateStatusClicked);
    manageDetailsButton_ = new QPushButton("Quản lý Chi tiết", this); connect(manageDetailsButton_, &QPushButton::clicked, this, &StocktakeRequestManagementWidget::onManageDetailsClicked);
    recordCountedQuantityButton_ = new QPushButton("Ghi nhận SL đã đếm", this); connect(recordCountedQuantityButton_, &QPushButton::clicked, this, &StocktakeRequestManagementWidget::onRecordCountedQuantityClicked);
    reconcileStocktakeButton_ = new QPushButton("Đối chiếu Kiểm kê", this); connect(reconcileStocktakeButton_, &QPushButton::clicked, this, &StocktakeRequestManagementWidget::onReconcileStocktakeClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &StocktakeRequestManagementWidget::onSearchRequestClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &StocktakeRequestManagementWidget::clearForm);
    
    buttonLayout->addWidget(addRequestButton_);
    buttonLayout->addWidget(editRequestButton_);
    buttonLayout->addWidget(deleteRequestButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(manageDetailsButton_);
    buttonLayout->addWidget(recordCountedQuantityButton_);
    buttonLayout->addWidget(reconcileStocktakeButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void StocktakeRequestManagementWidget::loadStocktakeRequests() {
    ERP::Logger::Logger::getInstance().info("StocktakeRequestManagementWidget: Loading stocktake requests...");
    requestTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Warehouse::DTO::StocktakeRequestDTO> requests = stocktakeService_->getAllStocktakeRequests({}, currentUserId_, currentUserRoleIds_);

    requestTable_->setRowCount(requests.size());
    for (int i = 0; i < requests.size(); ++i) {
        const auto& request = requests[i];
        requestTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(request.id)));
        
        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(request.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) warehouseName = QString::fromStdString(warehouse->name);
        requestTable_->setItem(i, 1, new QTableWidgetItem(warehouseName));

        QString locationName = "Toàn bộ kho";
        if (request.locationId) {
            std::optional<ERP::Catalog::DTO::LocationDTO> location = warehouseService_->getLocationById(*request.locationId, currentUserId_, currentUserRoleIds_);
            if (location) locationName = QString::fromStdString(location->name);
        }
        requestTable_->setItem(i, 2, new QTableWidgetItem(locationName));

        QString requestedByName = "N/A";
        std::optional<ERP::User::DTO::UserDTO> requestedByUser = securityManager_->getUserService()->getUserById(request.requestedByUserId, currentUserId_, currentUserRoleIds_);
        if (requestedByUser) requestedByName = QString::fromStdString(requestedByUser->username);
        requestTable_->setItem(i, 3, new QTableWidgetItem(requestedByName));

        requestTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(request.countDate, ERP::Common::DATETIME_FORMAT))));
        requestTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(request.getStatusString())));
    }
    requestTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("StocktakeRequestManagementWidget: Stocktake requests loaded successfully.");
}

void StocktakeRequestManagementWidget::populateWarehouseComboBox() {
    warehouseComboBox_->clear();
    std::vector<ERP::Catalog::DTO::WarehouseDTO> allWarehouses = warehouseService_->getAllWarehouses({}, currentUserId_, currentUserRoleIds_);
    for (const auto& warehouse : allWarehouses) {
        warehouseComboBox_->addItem(QString::fromStdString(warehouse.name), QString::fromStdString(warehouse.id));
    }
}

void StocktakeRequestManagementWidget::populateLocationComboBox(const std::string& warehouseId) {
    locationComboBox_->clear();
    locationComboBox_->addItem("None (Toàn bộ kho)", ""); // Option for entire warehouse
    std::vector<ERP::Catalog::DTO::LocationDTO> locations;
    if (!warehouseId.empty()) {
        locations = warehouseService_->getLocationsByWarehouse(warehouseId, currentUserId_, currentUserRoleIds_);
    }
    for (const auto& location : locations) {
        locationComboBox_->addItem(QString::fromStdString(location.name), QString::fromStdString(location.id));
    }
}

void StocktakeRequestManagementWidget::populateUserComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("None", "");
    std::vector<ERP::User::DTO::UserDTO> allUsers = securityManager_->getUserService()->getAllUsers({}, currentUserId_, currentUserRoleIds_);
    for (const auto& user : allUsers) {
        comboBox->addItem(QString::fromStdString(user.username), QString::fromStdString(user.id));
    }
}

void StocktakeRequestManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Warehouse::DTO::StocktakeRequestStatus::PENDING));
    statusComboBox_->addItem("In Progress", static_cast<int>(ERP::Warehouse::DTO::StocktakeRequestStatus::IN_PROGRESS));
    statusComboBox_->addItem("Counted", static_cast<int>(ERP::Warehouse::DTO::StocktakeRequestStatus::COUNTED));
    statusComboBox_->addItem("Reconciled", static_cast<int>(ERP::Warehouse::DTO::StocktakeRequestStatus::RECONCILED));
    statusComboBox_->addItem("Completed", static_cast<int>(ERP::Warehouse::DTO::StocktakeRequestStatus::COMPLETED));
    statusComboBox_->addItem("Cancelled", static_cast<int>(ERP::Warehouse::DTO::StocktakeRequestStatus::CANCELLED));
}


void StocktakeRequestManagementWidget::onAddRequestClicked() {
    if (!hasPermission("Warehouse.CreateStocktake")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm yêu cầu kiểm kê.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateWarehouseComboBox();
    populateUserComboBox(countedByLineEdit_); // For counted by
    showRequestInputDialog();
}

void StocktakeRequestManagementWidget::onEditRequestClicked() {
    if (!hasPermission("Warehouse.UpdateStocktake")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa yêu cầu kiểm kê.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Yêu Cầu Kiểm Kê", "Vui lòng chọn một yêu cầu kiểm kê để sửa.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> requestOpt = stocktakeService_->getStocktakeRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (requestOpt) {
        populateWarehouseComboBox();
        populateUserComboBox(countedByLineEdit_);
        showRequestInputDialog(&(*requestOpt));
    } else {
        showMessageBox("Sửa Yêu Cầu Kiểm Kê", "Không tìm thấy yêu cầu kiểm kê để sửa.", QMessageBox::Critical);
    }
}

void StocktakeRequestManagementWidget::onDeleteRequestClicked() {
    if (!hasPermission("Warehouse.DeleteStocktake")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa yêu cầu kiểm kê.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Yêu Cầu Kiểm Kê", "Vui lòng chọn một yêu cầu kiểm kê để xóa.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    QString warehouseName = requestTable_->item(selectedRow, 1)->text(); // Display warehouse name

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Yêu Cầu Kiểm Kê");
    confirmBox.setText("Bạn có chắc chắn muốn xóa yêu cầu kiểm kê cho kho '" + warehouseName + "' (ID: " + requestId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (stocktakeService_->deleteStocktakeRequest(requestId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Yêu Cầu Kiểm Kê", "Yêu cầu kiểm kê đã được xóa thành công.", QMessageBox::Information);
            loadStocktakeRequests();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa yêu cầu kiểm kê. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void StocktakeRequestManagementWidget::onUpdateRequestStatusClicked() {
    if (!hasPermission("Warehouse.UpdateStocktakeStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái yêu cầu kiểm kê.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một yêu cầu kiểm kê để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> requestOpt = stocktakeService_->getStocktakeRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!requestOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy yêu cầu kiểm kê để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Warehouse::DTO::StocktakeRequestDTO currentRequest = *requestOpt;
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
        ERP::Warehouse::DTO::StocktakeRequestStatus newStatus = static_cast<ERP::Warehouse::DTO::StocktakeRequestStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái yêu cầu kiểm kê");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái yêu cầu kiểm kê này thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (stocktakeService_->updateStocktakeRequestStatus(requestId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái yêu cầu kiểm kê đã được cập nhật thành công.", QMessageBox::Information);
                loadStocktakeRequests();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái yêu cầu kiểm kê. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}


void StocktakeRequestManagementWidget::onSearchRequestClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["warehouse_id_or_location_id_contains"] = searchText.toStdString(); // Assuming generic search
    }
    loadStocktakeRequests(); // Reloads with current filter if any
    ERP::Logger::Logger::getInstance().info("StocktakeRequestManagementWidget: Search completed.");
}

void StocktakeRequestManagementWidget::onRequestTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString requestId = requestTable_->item(row, 0)->text();
    std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> requestOpt = stocktakeService_->getStocktakeRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (requestOpt) {
        idLineEdit_->setText(QString::fromStdString(requestOpt->id));
        
        populateWarehouseComboBox();
        int warehouseIndex = warehouseComboBox_->findData(QString::fromStdString(requestOpt->warehouseId));
        if (warehouseIndex != -1) warehouseComboBox_->setCurrentIndex(warehouseIndex);

        populateLocationComboBox(requestOpt->warehouseId); // Populate locations for this warehouse
        if (requestOpt->locationId) {
            int locationIndex = locationComboBox_->findData(QString::fromStdString(*requestOpt->locationId));
            if (locationIndex != -1) locationComboBox_->setCurrentIndex(locationIndex);
            else locationComboBox_->setCurrentIndex(0); // "None"
        } else {
            locationComboBox_->setCurrentIndex(0); // "None"
        }

        requestedByLineEdit_->setText(QString::fromStdString(requestOpt->requestedByUserId)); // Display ID
        countedByLineEdit_->setText(QString::fromStdString(requestOpt->countedByUserId.value_or(""))); // Display ID
        countDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(requestOpt->countDate.time_since_epoch()).count()));
        
        populateStatusComboBox();
        int statusIndex = statusComboBox_->findData(static_cast<int>(requestOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);
        notesLineEdit_->setText(QString::fromStdString(requestOpt->notes.value_or("")));

    } else {
        showMessageBox("Thông tin Yêu Cầu Kiểm Kê", "Không tìm thấy yêu cầu kiểm kê đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void StocktakeRequestManagementWidget::clearForm() {
    idLineEdit_->clear();
    warehouseComboBox_->clear(); // Repopulated on demand
    locationComboBox_->clear(); // Repopulated on demand
    requestedByLineEdit_->clear();
    countedByLineEdit_->clear();
    countDateEdit_->clear();
    statusComboBox_->setCurrentIndex(0);
    notesLineEdit_->clear();
    requestTable_->clearSelection();
    updateButtonsState();
}

void StocktakeRequestManagementWidget::onManageDetailsClicked() {
    if (!hasPermission("Warehouse.ManageStocktakeDetails")) { // Assuming this permission
        showMessageBox("Lỗi", "Bạn không có quyền quản lý chi tiết kiểm kê.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Quản lý Chi tiết", "Vui lòng chọn một yêu cầu kiểm kê để quản lý chi tiết.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> requestOpt = stocktakeService_->getStocktakeRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (requestOpt) {
        showManageDetailsDialog(&(*requestOpt));
    } else {
        showMessageBox("Quản lý Chi tiết", "Không tìm thấy yêu cầu kiểm kê để quản lý chi tiết.", QMessageBox::Critical);
    }
}

void StocktakeRequestManagementWidget::onRecordCountedQuantityClicked() {
    if (!hasPermission("Warehouse.RecordCountedQuantity")) {
        showMessageBox("Lỗi", "Bạn không có quyền ghi nhận số lượng đã đếm.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Ghi nhận SL đã đếm", "Vui lòng chọn một yêu cầu kiểm kê trước.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> parentRequestOpt = stocktakeService_->getStocktakeRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);
    if (!parentRequestOpt) {
        showMessageBox("Ghi nhận SL đã đếm", "Không tìm thấy yêu cầu kiểm kê.", QMessageBox::Critical);
        return;
    }

    // Show a dialog to select a detail and input quantity
    QDialog dialog(this);
    dialog.setWindowTitle("Ghi nhận Số lượng Đã đếm Thực tế");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox detailComboBox;
    // Populate with details of the selected request
    std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> details = stocktakeService_->getStocktakeDetails(requestId.toStdString(), currentUserId_, currentUserRoleIds_);
    for (const auto& detail : details) {
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = securityManager_->getProductService()->getProductById(detail.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);
        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(detail.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) warehouseName = QString::fromStdString(warehouse->name);
        QString locationName = "N/A";
        std::optional<ERP::Catalog::DTO::LocationDTO> location = warehouseService_->getLocationById(detail.locationId, currentUserId_, currentUserRoleIds_);
        if (location) locationName = QString::fromStdString(location->name);

        detailComboBox.addItem(productName + " (" + warehouseName + "/" + locationName + ") (Hệ thống: " + QString::number(detail.systemQuantity) + ", Đã đếm: " + QString::number(detail.countedQuantity) + ")", QString::fromStdString(detail.id));
    }

    QLineEdit quantityLineEdit;
    quantityLineEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));

    formLayout->addRow("Chọn Chi tiết:", &detailComboBox);
    formLayout->addRow("Số lượng Đã đếm Thực tế:*", &quantityLineEdit);
    layout->addLayout(formLayout);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString selectedDetailId = detailComboBox.currentData().toString();
        double quantity = quantityLineEdit.text().toDouble();

        std::optional<ERP::Warehouse::DTO::StocktakeDetailDTO> selectedDetailOpt = stocktakeService_->getStocktakeDetailById(selectedDetailId.toStdString()); // Needs to be fixed to take current user, roles
        if (!selectedDetailOpt) {
            showMessageBox("Lỗi", "Không tìm thấy chi tiết kiểm kê đã chọn.", QMessageBox::Critical);
            return;
        }

        if (stocktakeService_->recordCountedQuantity(selectedDetailId.toStdString(), quantity, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Ghi nhận SL đã đếm", "Số lượng đã đếm được ghi nhận thành công.", QMessageBox::Information);
            loadStocktakeRequests(); // Refresh parent table
            clearForm();
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandler::getLastUserMessage().value_or("Không thể ghi nhận số lượng đã đếm. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}

void StocktakeRequestManagementWidget::onReconcileStocktakeClicked() {
    if (!hasPermission("Warehouse.ReconcileStocktake")) {
        showMessageBox("Lỗi", "Bạn không có quyền đối chiếu kiểm kê.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Đối chiếu Kiểm kê", "Vui lòng chọn một yêu cầu kiểm kê để đối chiếu.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> requestOpt = stocktakeService_->getStocktakeRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!requestOpt) {
        showMessageBox("Đối chiếu Kiểm kê", "Không tìm thấy yêu cầu kiểm kê để đối chiếu.", QMessageBox::Critical);
        return;
    }

    ERP::Warehouse::DTO::StocktakeRequestDTO currentRequest = *requestOpt;
    if (currentRequest.status != ERP::Warehouse::DTO::StocktakeRequestStatus::COUNTED) {
        showMessageBox("Đối chiếu Kiểm kê", "Chỉ có thể đối chiếu yêu cầu kiểm kê ở trạng thái 'Đã đếm'. Trạng thái hiện tại là: " + QString::fromStdString(currentRequest.getStatusString()), QMessageBox::Warning);
        return;
    }

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Đối chiếu Kiểm kê");
    confirmBox.setText("Bạn có chắc chắn muốn đối chiếu kiểm kê này? Thao tác này sẽ tạo các điều chỉnh tồn kho.");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (stocktakeService_->reconcileStocktake(requestId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Đối chiếu Kiểm kê", "Kiểm kê đã được đối chiếu và điều chỉnh tồn kho thành công.", QMessageBox::Information);
            loadStocktakeRequests();
            clearForm();
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandler::getLastUserMessage().value_or("Không thể đối chiếu kiểm kê. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void StocktakeRequestManagementWidget::showRequestInputDialog(ERP::Warehouse::DTO::StocktakeRequestDTO* request) {
    QDialog dialog(this);
    dialog.setWindowTitle(request ? "Sửa Yêu Cầu Kiểm Kê" : "Thêm Yêu Cầu Kiểm Kê Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox warehouseCombo(this); populateWarehouseComboBox();
    for (int i = 0; i < warehouseComboBox_->count(); ++i) warehouseCombo.addItem(warehouseComboBox_->itemText(i), warehouseComboBox_->itemData(i));
    
    QComboBox locationCombo(this); // Populated dynamically, will be cleared/populated on warehouse change
    connect(&warehouseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index){
        QString selectedWarehouseId = warehouseCombo.currentData().toString();
        populateLocationComboBox(selectedWarehouseId.toStdString());
    });
    // Initial populate for location based on default/first warehouse
    if (warehouseCombo.count() > 0) populateLocationComboBox(warehouseCombo.itemData(0).toString().toStdString());
    
    QLineEdit requestedByEdit(this); requestedByEdit.setReadOnly(true); // Auto-filled
    QComboBox countedByCombo(this); populateUserComboBox(&countedByCombo);
    QDateTimeEdit countDateEdit(this); countDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QComboBox statusCombo(this); populateStatusComboBox();
    for (int i = 0; i < statusComboBox_->count(); ++i) statusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));
    QLineEdit notesEdit(this);

    if (request) {
        int warehouseIndex = warehouseCombo.findData(QString::fromStdString(request->warehouseId));
        if (warehouseIndex != -1) warehouseCombo.setCurrentIndex(warehouseIndex);

        populateLocationComboBox(request->warehouseId); // Populate for existing warehouse
        if (request->locationId) {
            int locationIndex = locationCombo.findData(QString::fromStdString(*request->locationId));
            if (locationIndex != -1) locationCombo.setCurrentIndex(locationIndex);
            else locationCombo.setCurrentIndex(0); // "None"
        } else {
            locationCombo.setCurrentIndex(0); // "None"
        }

        requestedByEdit.setText(QString::fromStdString(request->requestedByUserId));
        if (request->countedByUserId) {
            int countedByIndex = countedByCombo.findData(QString::fromStdString(*request->countedByUserId));
            if (countedByIndex != -1) countedByCombo.setCurrentIndex(countedByIndex);
            else countedByCombo.setCurrentIndex(0);
        } else {
            countedByCombo.setCurrentIndex(0);
        }
        countDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(request->countDate.time_since_epoch()).count()));
        int statusIndex = statusCombo.findData(static_cast<int>(request->status));
        if (statusIndex != -1) statusCombo.setCurrentIndex(statusIndex);
        notesEdit.setText(QString::fromStdString(request->notes.value_or("")));

        warehouseCombo.setEnabled(false); // Warehouse/location usually immutable for existing stocktake
        locationCombo.setEnabled(false);
    } else {
        countDateEdit.setDateTime(QDateTime::currentDateTime());
        requestedByEdit.setText(QString::fromStdString(currentUserId_)); // Default to current user
        // Defaults for new
    }

    formLayout->addRow("Kho hàng:*", &warehouseCombo);
    formLayout->addRow("Vị trí (tùy chọn):", &locationCombo);
    formLayout->addRow("Người yêu cầu:", &requestedByEdit);
    formLayout->addRow("Người đếm:", &countedByCombo);
    formLayout->addRow("Ngày đếm:*", &countDateEdit);
    formLayout->addRow("Trạng thái:*", &statusCombo);
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
        ERP::Warehouse::DTO::StocktakeRequestDTO newRequestData;
        if (request) {
            newRequestData = *request;
        } else {
            newRequestData.id = ERP::Utils::generateUUID(); // New ID for new request
        }

        newRequestData.warehouseId = warehouseCombo.currentData().toString().toStdString();
        
        QString selectedLocationId = locationCombo.currentData().toString();
        newRequestData.locationId = selectedLocationId.isEmpty() ? std::nullopt : std::make_optional(selectedLocationId.toStdString());

        newRequestData.requestedByUserId = requestedByEdit.text().toStdString();
        
        QString selectedCountedById = countedByCombo.currentData().toString();
        newRequestData.countedByUserId = selectedCountedById.isEmpty() ? std::nullopt : std::make_optional(selectedCountedById.toStdString());

        newRequestData.countDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(countDateEdit.dateTime());
        newRequestData.status = static_cast<ERP::Warehouse::DTO::StocktakeRequestStatus>(statusCombo.currentData().toInt());
        newRequestData.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());

        // For new requests, details are often pre-populated from current inventory or added in separate dialog
        std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> currentDetails;
        if (request) { // When editing, load existing details first
             currentDetails = stocktakeService_->getStocktakeDetails(request->id, currentUserId_, currentUserRoleIds_);
        }

        bool success = false;
        if (request) {
            success = stocktakeService_->updateStocktakeRequest(newRequestData, currentDetails, currentUserId_, currentUserRoleIds_); // Pass existing details if not modified
            if (success) showMessageBox("Sửa Yêu Cầu Kiểm Kê", "Yêu cầu kiểm kê đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật yêu cầu kiểm kê. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> createdRequest = stocktakeService_->createStocktakeRequest(newRequestData, {}, currentUserId_, currentUserRoleIds_); // Create with empty details
            if (createdRequest) {
                showMessageBox("Thêm Yêu Cầu Kiểm Kê", "Yêu cầu kiểm kê mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandler::getLastUserMessage().value_or("Không thể thêm yêu cầu kiểm kê mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadStocktakeRequests();
            clearForm();
        }
    }
}

void StocktakeRequestManagementWidget::showManageDetailsDialog(ERP::Warehouse::DTO::StocktakeRequestDTO* request) {
    if (!request) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Quản lý Chi tiết Yêu Cầu Kiểm Kê: " + QString::fromStdString(request->warehouseId));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *detailsTable = new QTableWidget(&dialog);
    detailsTable->setColumnCount(8); // Product, Warehouse, Location, System Qty, Counted Qty, Difference, Lot/Serial, Notes
    detailsTable->setHorizontalHeaderLabels({"Sản phẩm", "Kho hàng", "Vị trí", "SL Hệ thống", "SL Đã đếm", "Chênh lệch", "Số lô/Serial", "Ghi chú"});
    detailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    detailsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    detailsTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(detailsTable);

    // Load existing details
    std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> currentDetails = stocktakeService_->getStocktakeDetails(request->id, currentUserId_, currentUserRoleIds_);
    detailsTable->setRowCount(currentDetails.size());
    for (int i = 0; i < currentDetails.size(); ++i) {
        const auto& detail = currentDetails[i];
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = securityManager_->getProductService()->getProductById(detail.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);

        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(detail.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) warehouseName = QString::fromStdString(warehouse->name);

        QString locationName = "N/A";
        std::optional<ERP::Catalog::DTO::LocationDTO> location = warehouseService_->getLocationById(detail.locationId, currentUserId_, currentUserRoleIds_);
        if (location) locationName = QString::fromStdString(location->name);

        detailsTable->setItem(i, 0, new QTableWidgetItem(productName));
        detailsTable->setItem(i, 1, new QTableWidgetItem(warehouseName));
        detailsTable->setItem(i, 2, new QTableWidgetItem(locationName));
        detailsTable->setItem(i, 3, new QTableWidgetItem(QString::number(detail.systemQuantity)));
        detailsTable->setItem(i, 4, new QTableWidgetItem(QString::number(detail.countedQuantity)));
        detailsTable->setItem(i, 5, new QTableWidgetItem(QString::number(detail.difference)));
        detailsTable->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(detail.lotNumber.value_or("") + "/" + detail.serialNumber.value_or(""))));
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

    // Connect item management buttons
    connect(addItemButton, &QPushButton::clicked, [&]() {
        QDialog itemDialog(&dialog);
        itemDialog.setWindowTitle("Thêm Chi tiết Kiểm kê");
        QFormLayout itemFormLayout;
        QComboBox productCombo;
        std::vector<ERP::Product::DTO::ProductDTO> allProducts = securityManager_->getProductService()->getAllProducts({}, currentUserId_, currentUserRoleIds_);
        for (const auto& prod : allProducts) productCombo.addItem(QString::fromStdString(prod.name), QString::fromStdString(prod.id));
        
        QComboBox warehouseCombo; populateWarehouseComboBox();
        for (int i = 0; i < warehouseComboBox_->count(); ++i) warehouseCombo.addItem(warehouseComboBox_->itemText(i), warehouseComboBox_->itemData(i));
        warehouseCombo.setCurrentData(QString::fromStdString(request->warehouseId)); // Default to parent warehouse
        warehouseCombo.setEnabled(false); // Warehouse is fixed for this stocktake request

        QComboBox locationCombo;
        populateLocationComboBox(&locationCombo, request->warehouseId); // Populate locations for this warehouse
        if (request->locationId) { // If stocktake is for specific location, pre-select and disable
            int locIndex = locationCombo.findData(QString::fromStdString(*request->locationId));
            if (locIndex != -1) locationCombo.setCurrentIndex(locIndex);
            locationCombo.setEnabled(false);
        }

        QLineEdit systemQuantityEdit; systemQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog)); systemQuantityEdit.setReadOnly(true); // Auto-filled from actual inventory
        QLineEdit countedQuantityEdit; countedQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit lotNumberEdit;
        QLineEdit serialNumberEdit;
        QLineEdit notesEdit;

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Kho hàng:*", &warehouseCombo);
        itemFormLayout.addRow("Vị trí:*", &locationCombo);
        itemFormLayout.addRow("SL Hệ thống:", &systemQuantityEdit);
        itemFormLayout.addRow("SL Đã đếm:*", &countedQuantityEdit);
        itemFormLayout.addRow("Số lô:", &lotNumberEdit);
        itemFormLayout.addRow("Số Serial:", &serialNumberEdit);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);

        // Auto-fill system quantity based on product/warehouse/location selection
        connect(&productCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index){
            QString prodId = productCombo.currentData().toString();
            QString whId = warehouseCombo.currentData().toString();
            QString locId = locationCombo.currentData().toString();
            if (!prodId.isEmpty() && !whId.isEmpty() && !locId.isEmpty()) {
                std::optional<ERP::Warehouse::DTO::InventoryDTO> inv = inventoryManagementService_->getInventoryByProductLocation(prodId.toStdString(), whId.toStdString(), locId.toStdString(), currentUserId_, currentUserRoleIds_);
                systemQuantityEdit.setText(QString::number(inv ? inv->quantity : 0.0));
            }
        });
        connect(&locationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index){
            QString prodId = productCombo.currentData().toString();
            QString whId = warehouseCombo.currentData().toString();
            QString locId = locationCombo.currentData().toString();
            if (!prodId.isEmpty() && !whId.isEmpty() && !locId.isEmpty()) {
                std::optional<ERP::Warehouse::DTO::InventoryDTO> inv = inventoryManagementService_->getInventoryByProductLocation(prodId.toStdString(), whId.toStdString(), locId.toStdString(), currentUserId_, currentUserRoleIds_);
                systemQuantityEdit.setText(QString::number(inv ? inv->quantity : 0.0));
            }
        });


        QPushButton *okItemButton = new QPushButton("Thêm", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (productCombo.currentData().isNull() || warehouseCombo.currentData().isNull() || locationCombo.currentData().isNull() || countedQuantityEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Add to table
            int newRow = detailsTable->rowCount();
            detailsTable->insertRow(newRow);
            detailsTable->setItem(newRow, 0, new QTableWidgetItem(productCombo.currentText()));
            detailsTable->setItem(newRow, 1, new QTableWidgetItem(warehouseCombo.currentText()));
            detailsTable->setItem(newRow, 2, new QTableWidgetItem(locationCombo.currentText()));
            detailsTable->setItem(newRow, 3, new QTableWidgetItem(systemQuantityEdit.text()));
            detailsTable->setItem(newRow, 4, new QTableWidgetItem(countedQuantityEdit.text()));
            detailsTable->setItem(newRow, 5, new QTableWidgetItem(QString::number(systemQuantityEdit.text().toDouble() - countedQuantityEdit.text().toDouble()))); // Difference
            detailsTable->setItem(newRow, 6, new QTableWidgetItem(lotNumberEdit.text() + "/" + serialNumberEdit.text()));
            detailsTable->setItem(newRow, 7, new QTableWidgetItem(notesEdit.text()));
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
        itemDialog.setWindowTitle("Sửa Chi tiết Kiểm kê");
        QFormLayout itemFormLayout;
        QComboBox productCombo; populateProductComboBox();
        QComboBox warehouseCombo; populateWarehouseComboBox(); warehouseCombo.setEnabled(false); // Fixed
        QComboBox locationCombo; populateLocationComboBox(&locationCombo, request->warehouseId); locationCombo.setEnabled(false); // Fixed

        QLineEdit systemQuantityEdit; systemQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog)); systemQuantityEdit.setReadOnly(true);
        QLineEdit countedQuantityEdit; countedQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit lotNumberEdit;
        QLineEdit serialNumberEdit;
        QLineEdit notesEdit;

        // Populate with current item data
        QString currentProductId = detailsTable->item(selectedItemRow, 0)->data(Qt::UserRole).toString();
        int productIndex = productCombo.findData(currentProductId);
        if (productIndex != -1) productCombo.setCurrentIndex(productIndex); productCombo.setEnabled(false); // Fixed
        
        warehouseCombo.setCurrentData(detailsTable->item(selectedItemRow, 1)->data(Qt::UserRole));
        locationCombo.setCurrentData(detailsTable->item(selectedItemRow, 2)->data(Qt::UserRole));

        systemQuantityEdit.setText(detailsTable->item(selectedItemRow, 3)->text());
        countedQuantityEdit.setText(detailsTable->item(selectedItemRow, 4)->text());
        lotNumberEdit.setText(detailsTable->item(selectedItemRow, 6)->text().split("/")[0]);
        serialNumberEdit.setText(detailsTable->item(selectedItemRow, 6)->text().split("/").size() > 1 ? detailsTable->item(selectedItemRow, 6)->text().split("/")[1] : "");
        notesEdit.setText(detailsTable->item(selectedItemRow, 7)->text());

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Kho hàng:*", &warehouseCombo);
        itemFormLayout.addRow("Vị trí:*", &locationCombo);
        itemFormLayout.addRow("SL Hệ thống:", &systemQuantityEdit);
        itemFormLayout.addRow("SL Đã đếm:*", &countedQuantityEdit);
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
            if (productCombo.currentData().isNull() || warehouseCombo.currentData().isNull() || locationCombo.currentData().isNull() || countedQuantityEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Recalculate difference
            double systemQuantity = systemQuantityEdit.text().toDouble();
            double countedQuantity = countedQuantityEdit.text().toDouble();
            double difference = systemQuantity - countedQuantity;

            // Update table row
            detailsTable->setItem(selectedItemRow, 0, new QTableWidgetItem(productCombo.currentText()));
            detailsTable->setItem(selectedItemRow, 1, new QTableWidgetItem(warehouseCombo.currentText()));
            detailsTable->setItem(selectedItemRow, 2, new QTableWidgetItem(locationCombo.currentText()));
            detailsTable->setItem(selectedItemRow, 3, new QTableWidgetItem(systemQuantityEdit.text()));
            detailsTable->setItem(selectedItemRow, 4, new QTableWidgetItem(countedQuantityEdit.text()));
            detailsTable->setItem(selectedItemRow, 5, new QTableWidgetItem(QString::number(difference)));
            detailsTable->setItem(selectedItemRow, 6, new QTableWidgetItem(lotNumberEdit.text() + "/" + serialNumberEdit.text()));
            detailsTable->setItem(selectedItemRow, 7, new QTableWidgetItem(notesEdit.text()));
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
        confirmDelBox.setWindowTitle("Xóa Chi tiết Kiểm kê");
        confirmDelBox.setText("Bạn có chắc chắn muốn xóa chi tiết kiểm kê này?");
        confirmDelBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmDelBox.exec() == QMessageBox::Yes) {
            detailsTable->removeRow(selectedItemRow);
        }
    });

    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Collect all items from the table
        std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> updatedDetails;
        for (int i = 0; i < detailsTable->rowCount(); ++i) {
            ERP::Warehouse::DTO::StocktakeDetailDTO detail;
            // Item ID is stored in product item's UserRole data
            QString existingId = detailsTable->item(i, 0)->data(Qt::UserRole).toString();
            detail.id = existingId.isEmpty() ? ERP::Utils::generateUUID() : existingId.toStdString();

            detail.stocktakeRequestId = request->id;
            detail.productId = detailsTable->item(i, 0)->data(Qt::UserRole).toString().toStdString();
            detail.warehouseId = detailsTable->item(i, 1)->data(Qt::UserRole).toString().toStdString();
            detail.locationId = detailsTable->item(i, 2)->data(Qt::UserRole).toString().toStdString();
            detail.systemQuantity = detailsTable->item(i, 3)->text().toDouble();
            detail.countedQuantity = detailsTable->item(i, 4)->text().toDouble();
            detail.difference = detailsTable->item(i, 5)->text().toDouble(); // Directly use calculated difference from table
            
            QString lotSerialText = detailsTable->item(i, 6)->text();
            QStringList lotSerialParts = lotSerialText.split("/");
            if (lotSerialParts.size() > 0) detail.lotNumber = lotSerialParts[0].toStdString();
            if (lotSerialParts.size() > 1) detail.serialNumber = lotSerialParts[1].toStdString();

            detail.notes = detailsTable->item(i, 7)->text().isEmpty() ? std::nullopt : std::make_optional(detailsTable->item(i, 7)->text().toStdString());
            
            updatedDetails.push_back(detail);
        }

        // Call service to update Stocktake Request with new detail list
        if (stocktakeService_->updateStocktakeRequest(*request, updatedDetails, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Quản lý Chi tiết", "Chi tiết kiểm kê đã được cập nhật thành công.", QMessageBox::Information);
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật chi tiết kiểm kê. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void StocktakeManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool StocktakeManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void StocktakeManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Warehouse.CreateStocktake");
    bool canUpdate = hasPermission("Warehouse.UpdateStocktake");
    bool canDelete = hasPermission("Warehouse.DeleteStocktake");
    bool canChangeStatus = hasPermission("Warehouse.UpdateStocktakeStatus");
    bool canManageDetails = hasPermission("Warehouse.ManageStocktakeDetails"); // Assuming this permission
    bool canRecordQuantity = hasPermission("Warehouse.RecordCountedQuantity");
    bool canReconcile = hasPermission("Warehouse.ReconcileStocktake");

    addRequestButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Warehouse.ViewStocktakes"));

    bool isRowSelected = requestTable_->currentRow() >= 0;
    editRequestButton_->setEnabled(isRowSelected && canUpdate);
    deleteRequestButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    manageDetailsButton_->setEnabled(isRowSelected && canManageDetails);
    recordCountedQuantityButton_->setEnabled(isRowSelected && canRecordQuantity);
    reconcileStocktakeButton_->setEnabled(isRowSelected && canReconcile && requestTable_->item(requestTable_->currentRow(), 5)->text() == "Counted"); // Only if status is 'Counted'


    bool enableForm = isRowSelected && canUpdate;
    warehouseComboBox_->setEnabled(enableForm); // Will be read-only for existing anyway
    locationComboBox_->setEnabled(enableForm); // Will be read-only for existing anyway
    requestedByLineEdit_->setEnabled(enableForm);
    countedByLineEdit_->setEnabled(enableForm);
    countDateEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);

    // Read-only fields
    idLineEdit_->setEnabled(false);

    if (!isRowSelected) {
        idLineEdit_->clear();
        warehouseComboBox_->clear();
        locationComboBox_->clear();
        requestedByLineEdit_->clear();
        countedByLineEdit_->clear();
        countDateEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
        notesLineEdit_->clear();
    }
}


} // namespace Warehouse
} // namespace UI
} // namespace ERP