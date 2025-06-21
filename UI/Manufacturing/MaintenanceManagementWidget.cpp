// UI/Manufacturing/MaintenanceManagementWidget.cpp
#include "MaintenanceManagementWidget.h" // Đã rút gọn include
#include "MaintenanceManagement.h" // Đã rút gọn include
#include "Asset.h"                     // Đã rút gọn include
#include "MaintenanceManagementService.h" // Đã rút gọn include
#include "AssetManagementService.h"       // Đã rút gọn include
#include "ISecurityManager.h"           // Đã rút gọn include
#include "Logger.h"                     // Đã rút gọn include
#include "ErrorHandler.h"               // Đã rút gọn include
#include "Common.h"                     // Đã rút gọn include
#include "DateUtils.h"                  // Đã rút gọn include
#include "StringUtils.h"                // Đã rút gọn include
#include "CustomMessageBox.h"           // Đã rút gọn include
#include "UserService.h"                // For getting user names

#include <QInputDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDoubleValidator>

namespace ERP {
namespace UI {
namespace Manufacturing {

MaintenanceManagementWidget::MaintenanceManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IMaintenanceManagementService> maintenanceService,
    std::shared_ptr<Asset::Services::IAssetManagementService> assetService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      maintenanceService_(maintenanceService),
      assetService_(assetService),
      securityManager_(securityManager) {

    if (!maintenanceService_ || !assetService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ bảo trì, tài sản hoặc bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("MaintenanceManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("MaintenanceManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("MaintenanceManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadMaintenanceRequests();
    updateButtonsState();
}

MaintenanceManagementWidget::~MaintenanceManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void MaintenanceManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo ID tài sản hoặc mô tả...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &MaintenanceManagementWidget::onSearchRequestClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    requestTable_ = new QTableWidget(this);
    requestTable_->setColumnCount(8); // ID, Tài sản, Loại YC, Ưu tiên, Trạng thái, YC bởi, Ngày YC, Được giao cho
    requestTable_->setHorizontalHeaderLabels({"ID", "Tài sản", "Loại YC", "Ưu tiên", "Trạng thái", "YC bởi", "Ngày YC", "Được giao cho"});
    requestTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    requestTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    requestTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    requestTable_->horizontalHeader()->setStretchLastSection(true);
    connect(requestTable_, &QTableWidget::itemClicked, this, &MaintenanceManagementWidget::onRequestTableItemClicked);
    mainLayout->addWidget(requestTable_);

    // Form elements for editing/adding requests
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    assetComboBox_ = new QComboBox(this);
    requestTypeComboBox_ = new QComboBox(this); populateRequestTypeComboBox();
    priorityComboBox_ = new QComboBox(this); populatePriorityComboBox();
    statusComboBox_ = new QComboBox(this); populateRequestStatusComboBox();
    descriptionLineEdit_ = new QLineEdit(this);
    requestedByLineEdit_ = new QLineEdit(this); requestedByLineEdit_->setReadOnly(true);
    requestedDateEdit_ = new QDateTimeEdit(this); requestedDateEdit_->setReadOnly(true); requestedDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    scheduledDateEdit_ = new QDateTimeEdit(this); scheduledDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    assignedToLineEdit_ = new QLineEdit(this); // Can be a combobox or search for users
    failureReasonLineEdit_ = new QLineEdit(this);


    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Tài sản:*", this), 1, 0); formLayout->addWidget(assetComboBox_, 1, 1);
    formLayout->addWidget(new QLabel("Loại YC:*", this), 2, 0); formLayout->addWidget(requestTypeComboBox_, 2, 1);
    formLayout->addWidget(new QLabel("Ưu tiên:*", this), 3, 0); formLayout->addWidget(priorityComboBox_, 3, 1);
    formLayout->addWidget(new QLabel("Trạng thái:*", this), 4, 0); formLayout->addWidget(statusComboBox_, 4, 1);
    formLayout->addWidget(new QLabel("Mô tả:", this), 5, 0); formLayout->addWidget(descriptionLineEdit_, 5, 1);
    formLayout->addWidget(new QLabel("YC bởi:", this), 6, 0); formLayout->addWidget(requestedByLineEdit_, 6, 1);
    formLayout->addWidget(new QLabel("Ngày YC:", this), 7, 0); formLayout->addWidget(requestedDateEdit_, 7, 1);
    formLayout->addWidget(new QLabel("Ngày lên lịch:", this), 8, 0); formLayout->addWidget(scheduledDateEdit_, 8, 1);
    formLayout->addWidget(new QLabel("Được giao cho:", this), 9, 0); formLayout->addWidget(assignedToLineEdit_, 9, 1);
    formLayout->addWidget(new QLabel("Lý do hỏng hóc:", this), 10, 0); formLayout->addWidget(failureReasonLineEdit_, 10, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addRequestButton_ = new QPushButton("Thêm mới", this); connect(addRequestButton_, &QPushButton::clicked, this, &MaintenanceManagementWidget::onAddRequestClicked);
    editRequestButton_ = new QPushButton("Sửa", this); connect(editRequestButton_, &QPushButton::clicked, this, &MaintenanceManagementWidget::onEditRequestClicked);
    deleteRequestButton_ = new QPushButton("Xóa", this); connect(deleteRequestButton_, &QPushButton::clicked, this, &MaintenanceManagementWidget::onDeleteRequestClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &MaintenanceManagementWidget::onUpdateRequestStatusClicked);
    recordActivityButton_ = new QPushButton("Ghi nhận Hoạt động", this); connect(recordActivityButton_, &QPushButton::clicked, this, &MaintenanceManagementWidget::onRecordActivityClicked);
    viewActivitiesButton_ = new QPushButton("Xem Hoạt động", this); connect(viewActivitiesButton_, &QPushButton::clicked, this, &MaintenanceManagementWidget::onViewActivitiesClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &MaintenanceManagementWidget::onSearchRequestClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &MaintenanceManagementWidget::clearForm);
    
    buttonLayout->addWidget(addRequestButton_);
    buttonLayout->addWidget(editRequestButton_);
    buttonLayout->addWidget(deleteRequestButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(recordActivityButton_);
    buttonLayout->addWidget(viewActivitiesButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void MaintenanceManagementWidget::loadMaintenanceRequests() {
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementWidget: Loading maintenance requests...");
    requestTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Manufacturing::DTO::MaintenanceRequestDTO> requests = maintenanceService_->getAllMaintenanceRequests({}, currentUserId_, currentUserRoleIds_);

    requestTable_->setRowCount(requests.size());
    for (int i = 0; i < requests.size(); ++i) {
        const auto& request = requests[i];
        requestTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(request.id)));
        
        QString assetName = "N/A";
        std::optional<ERP::Asset::DTO::AssetDTO> asset = assetService_->getAssetById(request.assetId, currentUserId_, currentUserRoleIds_);
        if (asset) assetName = QString::fromStdString(asset->assetName);
        requestTable_->setItem(i, 1, new QTableWidgetItem(assetName));

        requestTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(request.getTypeString())));
        requestTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(request.getPriorityString())));
        requestTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(request.getStatusString())));
        
        QString requestedByName = "N/A";
        std::optional<ERP::User::DTO::UserDTO> requestedByUser = securityManager_->getUserService()->getUserById(request.requestedByUserId, currentUserId_, currentUserRoleIds_);
        if (requestedByUser) requestedByName = QString::fromStdString(requestedByUser->username);
        requestTable_->setItem(i, 5, new QTableWidgetItem(requestedByName));

        requestTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(request.requestedDate, ERP::Common::DATETIME_FORMAT))));
        
        QString assignedToName = "N/A";
        if (request.assignedToUserId) {
            std::optional<ERP::User::DTO::UserDTO> assignedToUser = securityManager_->getUserService()->getUserById(*request.assignedToUserId, currentUserId_, currentUserRoleIds_);
            if (assignedToUser) assignedToName = QString::fromStdString(assignedToUser->username);
        }
        requestTable_->setItem(i, 7, new QTableWidgetItem(assignedToName));
    }
    requestTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementWidget: Maintenance requests loaded successfully.");
}

void MaintenanceManagementWidget::populateAssetComboBox() {
    assetComboBox_->clear();
    std::vector<ERP::Asset::DTO::AssetDTO> allAssets = assetService_->getAllAssets({}, currentUserId_, currentUserRoleIds_);
    for (const auto& asset : allAssets) {
        assetComboBox_->addItem(QString::fromStdString(asset.assetName + " (" + asset.assetCode + ")"), QString::fromStdString(asset.id));
    }
}

void MaintenanceManagementWidget::populateRequestTypeComboBox() {
    requestTypeComboBox_->clear();
    requestTypeComboBox_->addItem("Preventive", static_cast<int>(ERP::Manufacturing::DTO::MaintenanceRequestType::PREVENTIVE));
    requestTypeComboBox_->addItem("Corrective", static_cast<int>(ERP::Manufacturing::DTO::MaintenanceRequestType::CORRECTIVE));
    requestTypeComboBox_->addItem("Predictive", static_cast<int>(ERP::Manufacturing::DTO::MaintenanceRequestType::PREDICTIVE));
    requestTypeComboBox_->addItem("Inspection", static_cast<int>(ERP::Manufacturing::DTO::MaintenanceRequestType::INSPECTION));
}

void MaintenanceManagementWidget::populatePriorityComboBox() {
    priorityComboBox_->clear();
    priorityComboBox_->addItem("Low", static_cast<int>(ERP::Manufacturing::DTO::MaintenancePriority::LOW));
    priorityComboBox_->addItem("Normal", static_cast<int>(ERP::Manufacturing::DTO::MaintenancePriority::NORMAL));
    priorityComboBox_->addItem("High", static_cast<int>(ERP::Manufacturing::DTO::MaintenancePriority::HIGH));
    priorityComboBox_->addItem("Urgent", static_cast<int>(ERP::Manufacturing::DTO::MaintenancePriority::URGENT));
}

void MaintenanceManagementWidget::populateRequestStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Manufacturing::DTO::MaintenanceRequestStatus::PENDING));
    statusComboBox_->addItem("Scheduled", static_cast<int>(ERP::Manufacturing::DTO::MaintenanceRequestStatus::SCHEDULED));
    statusComboBox_->addItem("In Progress", static_cast<int>(ERP::Manufacturing::DTO::MaintenanceRequestStatus::IN_PROGRESS));
    statusComboBox_->addItem("Completed", static_cast<int>(ERP::Manufacturing::DTO::MaintenanceRequestStatus::COMPLETED));
    statusComboBox_->addItem("Cancelled", static_cast<int>(ERP::Manufacturing::DTO::MaintenanceRequestStatus::CANCELLED));
    statusComboBox_->addItem("Rejected", static_cast<int>(ERP::Manufacturing::DTO::MaintenanceRequestStatus::REJECTED));
}

void MaintenanceManagementWidget::populateUserComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("None", "");
    std::vector<ERP::User::DTO::UserDTO> allUsers = securityManager_->getUserService()->getAllUsers({}, currentUserId_, currentUserRoleIds_);
    for (const auto& user : allUsers) {
        comboBox->addItem(QString::fromStdString(user.username), QString::fromStdString(user.id));
    }
}


void MaintenanceManagementWidget::onAddRequestClicked() {
    if (!hasPermission("Manufacturing.CreateMaintenanceRequest")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm yêu cầu bảo trì.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateAssetComboBox();
    // populateRequestTypeComboBox(); // Already done in setupUI
    // populatePriorityComboBox();    // Already done in setupUI
    // populateRequestStatusComboBox(); // Already done in setupUI
    showRequestInputDialog();
}

void MaintenanceManagementWidget::onEditRequestClicked() {
    if (!hasPermission("Manufacturing.UpdateMaintenanceRequest")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa yêu cầu bảo trì.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Yêu Cầu Bảo Trì", "Vui lòng chọn một yêu cầu bảo trì để sửa.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> requestOpt = maintenanceService_->getMaintenanceRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (requestOpt) {
        populateAssetComboBox();
        showRequestInputDialog(&(*requestOpt));
    } else {
        showMessageBox("Sửa Yêu Cầu Bảo Trì", "Không tìm thấy yêu cầu bảo trì để sửa.", QMessageBox::Critical);
    }
}

void MaintenanceManagementWidget::onDeleteRequestClicked() {
    if (!hasPermission("Manufacturing.DeleteMaintenanceRequest")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa yêu cầu bảo trì.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Yêu Cầu Bảo Trì", "Vui lòng chọn một yêu cầu bảo trì để xóa.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    QString assetName = requestTable_->item(selectedRow, 1)->text(); // Display asset name

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Yêu Cầu Bảo Trì");
    confirmBox.setText("Bạn có chắc chắn muốn xóa yêu cầu bảo trì cho tài sản '" + assetName + "' (ID: " + requestId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (maintenanceService_->deleteMaintenanceRequest(requestId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Yêu Cầu Bảo Trì", "Yêu cầu bảo trì đã được xóa thành công.", QMessageBox::Information);
            loadMaintenanceRequests();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa yêu cầu bảo trì. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void MaintenanceManagementWidget::onUpdateRequestStatusClicked() {
    if (!hasPermission("Manufacturing.UpdateMaintenanceRequestStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái yêu cầu bảo trì.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một yêu cầu bảo trì để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> requestOpt = maintenanceService_->getMaintenanceRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!requestOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy yêu cầu bảo trì để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    // Use a QInputDialog or custom dialog to get the new status
    QDialog statusDialog(this);
    statusDialog.setWindowTitle("Chọn Trạng Thái Mới");
    QVBoxLayout *layout = new QVBoxLayout(&statusDialog);
    QComboBox newStatusCombo;
    populateRequestStatusComboBox(); // Populate it
    for(int i = 0; i < statusComboBox_->count(); ++i) { // Copy items
        newStatusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));
    }
    // Set current status as default selected
    int currentStatusIndex = newStatusCombo.findData(static_cast<int>(requestOpt->status));
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
        ERP::Manufacturing::DTO::MaintenanceRequestStatus newStatus = static_cast<ERP::Manufacturing::DTO::MaintenanceRequestStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái yêu cầu bảo trì");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái yêu cầu bảo trì này thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (maintenanceService_->updateMaintenanceRequestStatus(requestId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái yêu cầu bảo trì đã được cập nhật thành công.", QMessageBox::Information);
                loadMaintenanceRequests();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái yêu cầu bảo trì. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}


void MaintenanceManagementWidget::onSearchRequestClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["asset_id_or_description_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    requestTable_->setRowCount(0);
    std::vector<ERP::Manufacturing::DTO::MaintenanceRequestDTO> requests = maintenanceService_->getAllMaintenanceRequests(filter, currentUserId_, currentUserRoleIds_);

    requestTable_->setRowCount(requests.size());
    for (int i = 0; i < requests.size(); ++i) {
        const auto& request = requests[i];
        requestTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(request.id)));
        
        QString assetName = "N/A";
        std::optional<ERP::Asset::DTO::AssetDTO> asset = assetService_->getAssetById(request.assetId, currentUserId_, currentUserRoleIds_);
        if (asset) assetName = QString::fromStdString(asset->assetName);
        requestTable_->setItem(i, 1, new QTableWidgetItem(assetName));

        requestTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(request.getTypeString())));
        requestTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(request.getPriorityString())));
        requestTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(request.getStatusString())));
        
        QString requestedByName = "N/A";
        std::optional<ERP::User::DTO::UserDTO> requestedByUser = securityManager_->getUserService()->getUserById(request.requestedByUserId, currentUserId_, currentUserRoleIds_);
        if (requestedByUser) requestedByName = QString::fromStdString(requestedByUser->username);
        requestTable_->setItem(i, 5, new QTableWidgetItem(requestedByName));

        requestTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(request.requestedDate, ERP::Common::DATETIME_FORMAT))));
        
        QString assignedToName = "N/A";
        if (request.assignedToUserId) {
            std::optional<ERP::User::DTO::UserDTO> assignedToUser = securityManager_->getUserService()->getUserById(*request.assignedToUserId, currentUserId_, currentUserRoleIds_);
            if (assignedToUser) assignedToName = QString::fromStdString(assignedToUser->username);
        }
        requestTable_->setItem(i, 7, new QTableWidgetItem(assignedToName));
    }
    requestTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("MaintenanceManagementWidget: Search completed.");
}

void MaintenanceManagementWidget::onRequestTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString requestId = requestTable_->item(row, 0)->text();
    std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> requestOpt = maintenanceService_->getMaintenanceRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (requestOpt) {
        idLineEdit_->setText(QString::fromStdString(requestOpt->id));
        
        populateAssetComboBox();
        int assetIndex = assetComboBox_->findData(QString::fromStdString(requestOpt->assetId));
        if (assetIndex != -1) assetComboBox_->setCurrentIndex(assetIndex);

        int requestTypeIndex = requestTypeComboBox_->findData(static_cast<int>(requestOpt->requestType));
        if (requestTypeIndex != -1) requestTypeComboBox_->setCurrentIndex(requestTypeIndex);

        int priorityIndex = priorityComboBox_->findData(static_cast<int>(requestOpt->priority));
        if (priorityIndex != -1) priorityComboBox_->setCurrentIndex(priorityIndex);

        int statusIndex = statusComboBox_->findData(static_cast<int>(requestOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);
        
        descriptionLineEdit_->setText(QString::fromStdString(requestOpt->description.value_or("")));
        requestedByLineEdit_->setText(QString::fromStdString(requestOpt->requestedByUserId)); // Display ID, can fetch name later
        requestedDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(requestOpt->requestedDate.time_since_epoch()).count()));
        if (requestOpt->scheduledDate) scheduledDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(requestOpt->scheduledDate->time_since_epoch()).count()));
        else scheduledDateEdit_->clear();
        assignedToLineEdit_->setText(QString::fromStdString(requestOpt->assignedToUserId.value_or("")));
        failureReasonLineEdit_->setText(QString::fromStdString(requestOpt->failureReason.value_or("")));

    } else {
        showMessageBox("Thông tin Yêu Cầu Bảo Trì", "Không thể tải chi tiết yêu cầu bảo trì đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void MaintenanceManagementWidget::clearForm() {
    idLineEdit_->clear();
    assetComboBox_->clear(); // Will be repopulated
    requestTypeComboBox_->setCurrentIndex(0);
    priorityComboBox_->setCurrentIndex(0);
    statusComboBox_->setCurrentIndex(0);
    descriptionLineEdit_->clear();
    requestedByLineEdit_->clear();
    requestedDateEdit_->clear();
    scheduledDateEdit_->clear();
    assignedToLineEdit_->clear();
    failureReasonLineEdit_->clear();
    requestTable_->clearSelection();
    updateButtonsState();
}

void MaintenanceManagementWidget::onRecordActivityClicked() {
    if (!hasPermission("Manufacturing.RecordMaintenanceActivity")) {
        showMessageBox("Lỗi", "Bạn không có quyền ghi nhận hoạt động bảo trì.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Ghi nhận Hoạt động", "Vui lòng chọn một yêu cầu bảo trì để ghi nhận hoạt động.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> requestOpt = maintenanceService_->getMaintenanceRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (requestOpt) {
        showRecordActivityDialog(&(*requestOpt));
    } else {
        showMessageBox("Ghi nhận Hoạt động", "Không tìm thấy yêu cầu bảo trì để ghi nhận hoạt động.", QMessageBox::Critical);
    }
}

void MaintenanceManagementWidget::onViewActivitiesClicked() {
    if (!hasPermission("Manufacturing.ViewMaintenanceActivities")) { // Assuming this permission
        showMessageBox("Lỗi", "Bạn không có quyền xem hoạt động bảo trì.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xem Hoạt động", "Vui lòng chọn một yêu cầu bảo trì để xem hoạt động.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> requestOpt = maintenanceService_->getMaintenanceRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (requestOpt) {
        showViewActivitiesDialog(&(*requestOpt));
    } else {
        showMessageBox("Xem Hoạt động", "Không tìm thấy yêu cầu bảo trì để xem hoạt động.", QMessageBox::Critical);
    }
}


void MaintenanceManagementWidget::showRequestInputDialog(ERP::Manufacturing::DTO::MaintenanceRequestDTO* request) {
    QDialog dialog(this);
    dialog.setWindowTitle(request ? "Sửa Yêu Cầu Bảo Trì" : "Thêm Yêu Cầu Bảo Trì Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox assetCombo(this); populateAssetComboBox();
    for (int i = 0; i < assetComboBox_->count(); ++i) assetCombo.addItem(assetComboBox_->itemText(i), assetComboBox_->itemData(i));
    
    QComboBox requestTypeCombo(this); populateRequestTypeComboBox();
    for (int i = 0; i < requestTypeComboBox_->count(); ++i) requestTypeCombo.addItem(requestTypeComboBox_->itemText(i), requestTypeComboBox_->itemData(i));

    QComboBox priorityCombo(this); populatePriorityComboBox();
    for (int i = 0; i < priorityComboBox_->count(); ++i) priorityCombo.addItem(priorityComboBox_->itemText(i), priorityComboBox_->itemData(i));

    QComboBox statusCombo(this); populateRequestStatusComboBox();
    for (int i = 0; i < statusComboBox_->count(); ++i) statusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));

    QLineEdit descriptionEdit(this);
    QDateTimeEdit scheduledDateEdit(this); scheduledDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QComboBox assignedToCombo(this); populateUserComboBox(&assignedToCombo); // Re-populate for dialog
    QLineEdit failureReasonEdit(this);

    if (request) {
        int assetIndex = assetCombo.findData(QString::fromStdString(request->assetId));
        if (assetIndex != -1) assetCombo.setCurrentIndex(assetIndex);

        int requestTypeIndex = requestTypeCombo.findData(static_cast<int>(request->requestType));
        if (requestTypeIndex != -1) requestTypeCombo.setCurrentIndex(requestTypeIndex);

        int priorityIndex = priorityCombo.findData(static_cast<int>(request->priority));
        if (priorityIndex != -1) priorityCombo.setCurrentIndex(priorityIndex);

        int statusIndex = statusCombo.findData(static_cast<int>(request->status));
        if (statusIndex != -1) statusCombo.setCurrentIndex(statusIndex);
        
        descriptionEdit.setText(QString::fromStdString(request->description.value_or("")));
        if (request->scheduledDate) scheduledDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(request->scheduledDate->time_since_epoch()).count()));
        else scheduledDateEdit.clear();
        if (request->assignedToUserId) {
            int userIndex = assignedToCombo.findData(QString::fromStdString(*request->assignedToUserId));
            if (userIndex != -1) assignedToCombo.setCurrentIndex(userIndex);
            else assignedToCombo.setCurrentIndex(0); // "None"
        } else {
            assignedToCombo.setCurrentIndex(0); // "None"
        }
        failureReasonEdit.setText(QString::fromStdString(request->failureReason.value_or("")));
    } else {
        scheduledDateEdit.setDateTime(QDateTime::currentDateTime());
        // Defaults for new
    }

    formLayout->addRow("Tài sản:*", &assetCombo);
    formLayout->addRow("Loại YC:*", &requestTypeCombo);
    formLayout->addRow("Ưu tiên:*", &priorityCombo);
    formLayout->addRow("Trạng thái:*", &statusCombo);
    formLayout->addRow("Mô tả:", &descriptionEdit);
    formLayout->addRow("Ngày lên lịch:", &scheduledDateEdit);
    formLayout->addRow("Được giao cho:", &assignedToCombo);
    formLayout->addRow("Lý do hỏng hóc:", &failureReasonEdit);
    
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
        ERP::Manufacturing::DTO::MaintenanceRequestDTO newRequestData;
        if (request) {
            newRequestData = *request;
        }

        newRequestData.assetId = assetCombo.currentData().toString().toStdString();
        newRequestData.requestType = static_cast<ERP::Manufacturing::DTO::MaintenanceRequestType>(requestTypeCombo.currentData().toInt());
        newRequestData.priority = static_cast<ERP::Manufacturing::DTO::MaintenancePriority>(priorityCombo.currentData().toInt());
        newRequestData.status = static_cast<ERP::Manufacturing::DTO::MaintenanceRequestStatus>(statusCombo.currentData().toInt());
        newRequestData.description = descriptionEdit.text().isEmpty() ? std::nullopt : std::make_optional(descriptionEdit.text().toStdString());
        newRequestData.scheduledDate = scheduledDateEdit.dateTime().isNull() ? std::nullopt : std::make_optional(ERP::Utils::DateUtils::qDateTimeToTimePoint(scheduledDateEdit.dateTime()));
        
        QString assignedToId = assignedToCombo.currentData().toString();
        newRequestData.assignedToUserId = assignedToId.isEmpty() ? std::nullopt : std::make_optional(assignedToId.toStdString());

        newRequestData.failureReason = failureReasonEdit.text().isEmpty() ? std::nullopt : std::make_optional(failureReasonEdit.text().toStdString());

        bool success = false;
        if (request) {
            success = maintenanceService_->updateMaintenanceRequest(newRequestData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Yêu Cầu Bảo Trì", "Yêu cầu bảo trì đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật yêu cầu bảo trì. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            newRequestData.requestedByUserId = currentUserId_; // Set requested by current user for new requests
            newRequestData.requestedDate = ERP::Utils::DateUtils::now(); // Set requested date
            std::optional<ERP::Manufacturing::DTO::MaintenanceRequestDTO> createdRequest = maintenanceService_->createMaintenanceRequest(newRequestData, currentUserId_, currentUserRoleIds_);
            if (createdRequest) {
                showMessageBox("Thêm Yêu Cầu Bảo Trì", "Yêu cầu bảo trì mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm yêu cầu bảo trì mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadMaintenanceRequests();
            clearForm();
        }
    }
}

void MaintenanceManagementWidget::showRecordActivityDialog(ERP::Manufacturing::DTO::MaintenanceRequestDTO* request) {
    if (!request) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Ghi nhận Hoạt động Bảo trì cho: " + QString::fromStdString(request->assetId));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit descriptionEdit(this);
    QDateTimeEdit activityDateEdit(this); activityDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    activityDateEdit.setDateTime(QDateTime::currentDateTime());
    QComboBox performedByCombo(this); populateUserComboBox(&performedByCombo);
    QLineEdit durationEdit(this); durationEdit.setValidator(new QDoubleValidator(0.0, 99999.0, 2, this));
    QLineEdit costEdit(this); costEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    QLineEdit costCurrencyEdit(this);
    QLineEdit partsUsedEdit(this);

    // Populate default values if needed
    // performedByCombo.setCurrentIndex(performedByCombo.findData(QString::fromStdString(currentUserId_))); // Auto-select current user

    formLayout->addRow("Mô tả hoạt động:*", &descriptionEdit);
    formLayout->addRow("Ngày hoạt động:*", &activityDateEdit);
    formLayout->addRow("Thực hiện bởi:*", &performedByCombo);
    formLayout->addRow("Thời lượng (giờ):*", &durationEdit);
    formLayout->addRow("Chi phí:", &costEdit);
    formLayout->addRow("Tiền tệ chi phí:", &costCurrencyEdit);
    formLayout->addRow("Linh kiện đã dùng:", &partsUsedEdit);

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton("Ghi nhận", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Manufacturing::DTO::MaintenanceActivityDTO newActivityData;
        newActivityData.maintenanceRequestId = request->id;
        newActivityData.activityDescription = descriptionEdit.text().toStdString();
        newActivityData.activityDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(activityDateEdit.dateTime());
        newActivityData.performedByUserId = performedByCombo.currentData().toString().toStdString();
        newActivityData.durationHours = durationEdit.text().toDouble();
        newActivityData.cost = costEdit.text().isEmpty() ? std::nullopt : std::make_optional(costEdit.text().toDouble());
        newActivityData.costCurrency = costCurrencyEdit.text().isEmpty() ? std::nullopt : std::make_optional(costCurrencyEdit.text().toStdString());
        newActivityData.partsUsed = partsUsedEdit.text().isEmpty() ? std::nullopt : std::make_optional(partsUsedEdit.text().toStdString());
        newActivityData.status = ERP::Common::EntityStatus::ACTIVE; // Default active

        if (maintenanceService_->recordMaintenanceActivity(newActivityData, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Ghi nhận Hoạt động", "Hoạt động bảo trì đã được ghi nhận thành công.", QMessageBox::Information);
            // Optionally update request status if activity completes it
            if (request->status != ERP::Manufacturing::DTO::MaintenanceRequestStatus::COMPLETED) {
                // Check if all activities are done, or if this is the final one
                // For simplicity, auto-set to IN_PROGRESS if not already, then suggest manual COMPLETED
                if (request->status != ERP::Manufacturing::DTO::MaintenanceRequestStatus::IN_PROGRESS) {
                    maintenanceService_->updateMaintenanceRequestStatus(request->id, ERP::Manufacturing::DTO::MaintenanceRequestStatus::IN_PROGRESS, currentUserId_, currentUserRoleIds_);
                }
            }
            loadMaintenanceRequests(); // Refresh main table
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể ghi nhận hoạt động bảo trì. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}

void MaintenanceManagementWidget::showViewActivitiesDialog(ERP::Manufacturing::DTO::MaintenanceRequestDTO* request) {
    if (!request) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Hoạt động Bảo trì cho: " + QString::fromStdString(request->assetId));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *activitiesTable = new QTableWidget(&dialog);
    activitiesTable->setColumnCount(7); // Description, Date, Performed By, Duration, Cost, Currency, Parts Used
    activitiesTable->setHorizontalHeaderLabels({"Mô tả", "Ngày", "Thực hiện bởi", "Thời lượng (giờ)", "Chi phí", "Tiền tệ", "Linh kiện"});
    activitiesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    activitiesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    activitiesTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(activitiesTable);

    // Load activities
    std::vector<ERP::Manufacturing::DTO::MaintenanceActivityDTO> activities = maintenanceService_->getMaintenanceActivitiesByRequest(request->id, currentUserId_, currentUserRoleIds_);
    activitiesTable->setRowCount(activities.size());
    for (int i = 0; i < activities.size(); ++i) {
        const auto& activity = activities[i];
        activitiesTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(activity.activityDescription)));
        activitiesTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(activity.activityDate, ERP::Common::DATETIME_FORMAT))));
        
        QString performedByName = "N/A";
        std::optional<ERP::User::DTO::UserDTO> performedByUser = securityManager_->getUserService()->getUserById(activity.performedByUserId, currentUserId_, currentUserRoleIds_);
        if (performedByUser) performedByName = QString::fromStdString(performedByUser->username);
        activitiesTable->setItem(i, 2, new QTableWidgetItem(performedByName));

        activitiesTable->setItem(i, 3, new QTableWidgetItem(QString::number(activity.durationHours)));
        activitiesTable->setItem(i, 4, new QTableWidgetItem(QString::number(activity.cost.value_or(0.0), 'f', 2)));
        activitiesTable->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(activity.costCurrency.value_or(""))));
        activitiesTable->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(activity.partsUsed.value_or(""))));
    }
    activitiesTable->resizeColumnsToContents();

    QPushButton *closeButton = new QPushButton("Đóng", &dialog);
    dialogLayout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}


void MaintenanceManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool MaintenanceManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void MaintenanceManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Manufacturing.CreateMaintenanceRequest");
    bool canUpdate = hasPermission("Manufacturing.UpdateMaintenanceRequest");
    bool canDelete = hasPermission("Manufacturing.DeleteMaintenanceRequest");
    bool canChangeStatus = hasPermission("Manufacturing.UpdateMaintenanceRequestStatus");
    bool canRecordActivity = hasPermission("Manufacturing.RecordMaintenanceActivity");
    bool canViewActivities = hasPermission("Manufacturing.ViewMaintenanceActivities");

    addRequestButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Manufacturing.ViewMaintenanceManagement"));

    bool isRowSelected = requestTable_->currentRow() >= 0;
    editRequestButton_->setEnabled(isRowSelected && canUpdate);
    deleteRequestButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    recordActivityButton_->setEnabled(isRowSelected && canRecordActivity);
    viewActivitiesButton_->setEnabled(isRowSelected && canViewActivities);

    bool enableForm = isRowSelected && canUpdate;
    assetComboBox_->setEnabled(enableForm);
    requestTypeComboBox_->setEnabled(enableForm);
    priorityComboBox_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    descriptionLineEdit_->setEnabled(enableForm);
    // requestedByLineEdit_ always read-only
    // requestedDateEdit_ always read-only
    scheduledDateEdit_->setEnabled(enableForm);
    assignedToLineEdit_->setEnabled(enableForm);
    failureReasonLineEdit_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        assetComboBox_->clear();
        requestTypeComboBox_->setCurrentIndex(0);
        priorityComboBox_->setCurrentIndex(0);
        statusComboBox_->setCurrentIndex(0);
        descriptionLineEdit_->clear();
        requestedByLineEdit_->clear();
        requestedDateEdit_->clear();
        scheduledDateEdit_->clear();
        assignedToLineEdit_->clear();
        failureReasonLineEdit_->clear();
    }
}


} // namespace Manufacturing
} // namespace UI
} // namespace ERP