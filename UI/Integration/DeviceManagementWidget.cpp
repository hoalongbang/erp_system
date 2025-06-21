// UI/Integration/DeviceManagementWidget.cpp
#include "DeviceManagementWidget.h" // Standard includes
#include "DeviceConfig.h"           // DeviceConfig DTO
#include "DeviceEventLog.h"         // DeviceEventLog DTO
#include "DeviceManagerService.h"   // DeviceManager Service
#include "ISecurityManager.h"       // Security Manager
#include "Logger.h"                 // Logging
#include "ErrorHandler.h"           // Error Handling
#include "Common.h"                 // Common Enums/Constants
#include "DateUtils.h"              // Date Utilities
#include "StringUtils.h"            // String Utilities
#include "CustomMessageBox.h"       // Custom Message Box
#include "UserService.h"            // For getting user names
#include "Location.h"               // Location DTO (for display)
#include "WarehouseService.h"       // Warehouse Service (for locations)
#include "DTOUtils.h"               // For JSON conversions

#include <QInputDialog>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QTextEdit>
#include <QJsonDocument>
#include <QJsonObject>

namespace ERP {
namespace UI {
namespace Integration {

DeviceManagementWidget::DeviceManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IDeviceManagerService> deviceManagerService,
    std::shared_ptr<ISecurityManager> securityManager)
    : QWidget(parent),
      deviceManagerService_(deviceManagerService),
      securityManager_(securityManager) {

    if (!deviceManagerService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ quản lý thiết bị hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("DeviceManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("DeviceManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("DeviceManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadDeviceConfigs();
    updateButtonsState();
}

DeviceManagementWidget::~DeviceManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void DeviceManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên hoặc mã định danh thiết bị...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &DeviceManagementWidget::onSearchDeviceConfigClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    deviceConfigTable_ = new QTableWidget(this);
    deviceConfigTable_->setColumnCount(6); // ID, Tên, Mã định danh, Loại, Trạng thái kết nối, Địa điểm
    deviceConfigTable_->setHorizontalHeaderLabels({"ID", "Tên Thiết bị", "Mã định danh", "Loại", "Trạng thái kết nối", "Địa điểm"});
    deviceConfigTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    deviceConfigTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    deviceConfigTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    deviceConfigTable_->horizontalHeader()->setStretchLastSection(true);
    connect(deviceConfigTable_, &QTableWidget::itemClicked, this, &DeviceManagementWidget::onDeviceConfigTableItemClicked);
    mainLayout->addWidget(deviceConfigTable_);

    // Form elements for editing/adding configs
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    deviceNameLineEdit_ = new QLineEdit(this);
    deviceIdentifierLineEdit_ = new QLineEdit(this);
    deviceTypeComboBox_ = new QComboBox(this); populateDeviceTypeComboBox();
    connectionStringLineEdit_ = new QLineEdit(this);
    ipAddressLineEdit_ = new QLineEdit(this);
    connectionStatusComboBox_ = new QComboBox(this); populateConnectionStatusComboBox();
    locationIdLineEdit_ = new QLineEdit(this); // Manual ID, should be combo with populateLocationComboBox
    notesLineEdit_ = new QLineEdit(this);
    isCriticalCheckBox_ = new QCheckBox("Thiết bị quan trọng", this);

    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("Tên Thiết bị:*", &deviceNameLineEdit_);
    formLayout->addRow("Mã định danh:*", &deviceIdentifierLineEdit_);
    formLayout->addRow("Loại:*", &deviceTypeComboBox_);
    formLayout->addRow("Chuỗi kết nối:", &connectionStringLineEdit_);
    formLayout->addRow("Địa chỉ IP:", &ipAddressLineEdit_);
    formLayout->addRow("Trạng thái kết nối:", &connectionStatusComboBox_); // Read-only via UI, updated by action
    formLayout->addRow("ID Địa điểm:", &locationIdLineEdit_); // Consider replacing with combo
    formLayout->addRow("Ghi chú:", &notesLineEdit_);
    formLayout->addRow("", &isCriticalCheckBox_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    registerDeviceButton_ = new QPushButton("Đăng ký Thiết bị", this); connect(registerDeviceButton_, &QPushButton::clicked, this, &DeviceManagementWidget::onRegisterDeviceClicked);
    editDeviceConfigButton_ = new QPushButton("Sửa Cấu hình", this); connect(editDeviceConfigButton_, &QPushButton::clicked, this, &DeviceManagementWidget::onEditDeviceConfigClicked);
    deleteDeviceConfigButton_ = new QPushButton("Xóa Cấu hình", this); connect(deleteDeviceConfigButton_, &QPushButton::clicked, this, &DeviceManagementWidget::onDeleteDeviceConfigClicked);
    updateConnectionStatusButton_ = new QPushButton("Cập nhật TT Kết nối", this); connect(updateConnectionStatusButton_, &QPushButton::clicked, this, &DeviceManagementWidget::onUpdateConnectionStatusClicked);
    viewEventLogsButton_ = new QPushButton("Xem Nhật ký Sự kiện", this); connect(viewEventLogsButton_, &QPushButton::clicked, this, &DeviceManagementWidget::onViewEventLogsClicked);
    recordDeviceEventButton_ = new QPushButton("Ghi nhận Sự kiện", this); connect(recordDeviceEventButton_, &QPushButton::clicked, this, &DeviceManagementWidget::onRecordDeviceEventClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &DeviceManagementWidget::onSearchDeviceConfigClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &DeviceManagementWidget::clearForm);
    
    buttonLayout->addWidget(registerDeviceButton_);
    buttonLayout->addWidget(editDeviceConfigButton_);
    buttonLayout->addWidget(deleteDeviceConfigButton_);
    buttonLayout->addWidget(updateConnectionStatusButton_);
    buttonLayout->addWidget(viewEventLogsButton_);
    buttonLayout->addWidget(recordDeviceEventButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void DeviceManagementWidget::loadDeviceConfigs() {
    ERP::Logger::Logger::getInstance().info("DeviceManagementWidget: Loading device configs...");
    deviceConfigTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Integration::DTO::DeviceConfigDTO> configs = deviceManagerService_->getAllDeviceConfigs({}, currentUserId_, currentUserRoleIds_);

    deviceConfigTable_->setRowCount(configs.size());
    for (int i = 0; i < configs.size(); ++i) {
        const auto& config = configs[i];
        deviceConfigTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(config.id)));
        deviceConfigTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(config.deviceName)));
        deviceConfigTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(config.deviceIdentifier)));
        deviceConfigTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(config.getTypeString())));
        deviceConfigTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(config.getConnectionStatusString())));
        
        QString locationName = "N/A";
        if (config.locationId) {
            std::optional<ERP::Catalog::DTO::LocationDTO> location = securityManager_->getWarehouseService()->getLocationById(*config.locationId, currentUserId_, currentUserRoleIds_);
            if (location) locationName = QString::fromStdString(location->name);
        }
        deviceConfigTable_->setItem(i, 5, new QTableWidgetItem(locationName));
    }
    deviceConfigTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("DeviceManagementWidget: Device configs loaded successfully.");
}

void DeviceManagementWidget::populateDeviceTypeComboBox() {
    deviceTypeComboBox_->clear();
    deviceTypeComboBox_->addItem("Scanner", static_cast<int>(ERP::Integration::DTO::DeviceType::BARCODE_SCANNER));
    deviceTypeComboBox_->addItem("Scale", static_cast<int>(ERP::Integration::DTO::DeviceType::WEIGHING_SCALE));
    deviceTypeComboBox_->addItem("RFID Reader", static_cast<int>(ERP::Integration::DTO::DeviceType::RFID_READER));
    deviceTypeComboBox_->addItem("Printer", static_cast<int>(ERP::Integration::DTO::DeviceType::PRINTER));
    deviceTypeComboBox_->addItem("Sensor", static_cast<int>(ERP::Integration::DTO::DeviceType::SENSOR));
    deviceTypeComboBox_->addItem("Other", static_cast<int>(ERP::Integration::DTO::DeviceType::OTHER));
    deviceTypeComboBox_->addItem("Unknown", static_cast<int>(ERP::Integration::DTO::DeviceType::UNKNOWN));
}

void DeviceManagementWidget::populateConnectionStatusComboBox() {
    connectionStatusComboBox_->clear();
    connectionStatusComboBox_->addItem("Connected", static_cast<int>(ERP::Integration::DTO::ConnectionStatus::CONNECTED));
    connectionStatusComboBox_->addItem("Disconnected", static_cast<int>(ERP::Integration::DTO::ConnectionStatus::DISCONNECTED));
    connectionStatusComboBox_->addItem("Error", static_cast<int>(ERP::Integration::DTO::ConnectionStatus::ERROR));
}

void DeviceManagementWidget::populateLocationComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("None", "");
    std::vector<ERP::Catalog::DTO::LocationDTO> allLocations = securityManager_->getWarehouseService()->getAllLocations({}, currentUserId_, currentUserRoleIds_);
    for (const auto& location : allLocations) {
        comboBox->addItem(QString::fromStdString(location.name), QString::fromStdString(location.id));
    }
}


void DeviceManagementWidget::onRegisterDeviceClicked() {
    if (!hasPermission("Integration.RegisterDevice")) {
        showMessageBox("Lỗi", "Bạn không có quyền đăng ký thiết bị.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateLocationComboBox(locationIdLineEdit_); // Assuming it's a QLineEdit in the form, needs to be combo
    showDeviceConfigInputDialog();
}

void DeviceManagementWidget::onEditDeviceConfigClicked() {
    if (!hasPermission("Integration.UpdateDeviceConfig")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa cấu hình thiết bị.", QMessageBox::Warning);
        return;
    }

    int selectedRow = deviceConfigTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Cấu hình Thiết bị", "Vui lòng chọn một cấu hình thiết bị để sửa.", QMessageBox::Information);
        return;
    }

    QString deviceId = deviceConfigTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Integration::DTO::DeviceConfigDTO> configOpt = deviceManagerService_->getDeviceConfigById(deviceId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (configOpt) {
        populateLocationComboBox(locationIdLineEdit_); // Assuming it's a QLineEdit in the form, needs to be combo
        showDeviceConfigInputDialog(&(*configOpt));
    } else {
        showMessageBox("Sửa Cấu hình Thiết bị", "Không tìm thấy cấu hình thiết bị để sửa.", QMessageBox::Critical);
    }
}

void DeviceManagementWidget::onDeleteDeviceConfigClicked() {
    if (!hasPermission("Integration.DeleteDeviceConfig")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa cấu hình thiết bị.", QMessageBox::Warning);
        return;
    }

    int selectedRow = deviceConfigTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Cấu hình Thiết bị", "Vui lòng chọn một cấu hình thiết bị để xóa.", QMessageBox::Information);
        return;
    }

    QString deviceId = deviceConfigTable_->item(selectedRow, 0)->text();
    QString deviceName = deviceConfigTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Cấu hình Thiết bị");
    confirmBox.setText("Bạn có chắc chắn muốn xóa cấu hình thiết bị '" + deviceName + "' (ID: " + deviceId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (deviceManagerService_->deleteDeviceConfig(deviceId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Cấu hình Thiết bị", "Cấu hình thiết bị đã được xóa thành công.", QMessageBox::Information);
            loadDeviceConfigs();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa cấu hình thiết bị. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void DeviceManagementWidget::onUpdateConnectionStatusClicked() {
    if (!hasPermission("Integration.UpdateDeviceConnectionStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái kết nối thiết bị.", QMessageBox::Warning);
        return;
    }

    int selectedRow = deviceConfigTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật TT Kết nối", "Vui lòng chọn một cấu hình thiết bị để cập nhật trạng thái kết nối.", QMessageBox::Information);
        return;
    }

    QString deviceId = deviceConfigTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Integration::DTO::DeviceConfigDTO> configOpt = deviceManagerService_->getDeviceConfigById(deviceId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!configOpt) {
        showMessageBox("Cập nhật TT Kết nối", "Không tìm thấy cấu hình thiết bị để cập nhật trạng thái kết nối.", QMessageBox::Critical);
        return;
    }

    ERP::Integration::DTO::DeviceConfigDTO currentConfig = *configOpt;
    // Use a QInputDialog or custom dialog to get the new status and optional message
    QDialog statusDialog(this);
    statusDialog.setWindowTitle("Cập nhật Trạng Thái Kết nối");
    QVBoxLayout *layout = new QVBoxLayout(&statusDialog);
    QComboBox newStatusCombo;
    populateConnectionStatusComboBox(); // Populate it
    for(int i = 0; i < connectionStatusComboBox_->count(); ++i) { // Copy items
        newStatusCombo.addItem(connectionStatusComboBox_->itemText(i), connectionStatusComboBox_->itemData(i));
    }
    // Set current status as default selected
    int currentStatusIndex = newStatusCombo.findData(static_cast<int>(currentConfig.connectionStatus));
    if (currentStatusIndex != -1) newStatusCombo.setCurrentIndex(currentStatusIndex);

    QLineEdit messageEdit; messageEdit.setPlaceholderText("Tin nhắn (tùy chọn)");

    layout->addWidget(new QLabel("Chọn trạng thái mới:", &statusDialog));
    layout->addWidget(&newStatusCombo);
    layout->addWidget(new QLabel("Tin nhắn:"));
    layout->addWidget(&messageEdit);

    QPushButton *okButton = new QPushButton("Cập nhật", &statusDialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &statusDialog);
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(okButton);
    btnLayout->addWidget(cancelButton);
    layout->addLayout(btnLayout);

    connect(okButton, &QPushButton::clicked, &statusDialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (statusDialog.exec() == QDialog::Accepted) {
        ERP::Integration::DTO::ConnectionStatus newStatus = static_cast<ERP::Integration::DTO::ConnectionStatus>(newStatusCombo.currentData().toInt());
        std::optional<std::string> message = messageEdit.text().isEmpty() ? std::nullopt : std::make_optional(messageEdit.text().toStdString());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái kết nối");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái kết nối thiết bị '" + QString::fromStdString(currentConfig.deviceName) + "' thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (deviceManagerService_->updateDeviceConnectionStatus(deviceId.toStdString(), newStatus, message, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật TT Kết nối", "Trạng thái kết nối đã được cập nhật thành công.", QMessageBox::Information);
                loadDeviceConfigs();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái kết nối. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}

void DeviceManagementWidget::onRecordDeviceEventClicked() {
    if (!hasPermission("Integration.RecordDeviceEvent")) {
        showMessageBox("Lỗi", "Bạn không có quyền ghi nhận sự kiện thiết bị.", QMessageBox::Warning);
        return;
    }

    int selectedRow = deviceConfigTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Ghi nhận Sự kiện", "Vui lòng chọn một thiết bị để ghi nhận sự kiện.", QMessageBox::Information);
        return;
    }

    QString deviceId = deviceConfigTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Integration::DTO::DeviceConfigDTO> configOpt = deviceManagerService_->getDeviceConfigById(deviceId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (configOpt) {
        showRecordDeviceEventDialog(&(*configOpt));
    } else {
        showMessageBox("Ghi nhận Sự kiện", "Không tìm thấy thiết bị để ghi nhận sự kiện.", QMessageBox::Critical);
    }
}

void DeviceManagementWidget::onViewEventLogsClicked() {
    if (!hasPermission("Integration.ViewDeviceEventLogs")) {
        showMessageBox("Lỗi", "Bạn không có quyền xem nhật ký sự kiện thiết bị.", QMessageBox::Warning);
        return;
    }

    int selectedRow = deviceConfigTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xem Nhật ký Sự kiện", "Vui lòng chọn một thiết bị để xem nhật ký sự kiện.", QMessageBox::Information);
        return;
    }

    QString deviceId = deviceConfigTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Integration::DTO::DeviceConfigDTO> configOpt = deviceManagerService_->getDeviceConfigById(deviceId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (configOpt) {
        showViewEventLogsDialog(&(*configOpt));
    } else {
        showMessageBox("Xem Nhật ký Sự kiện", "Không tìm thấy thiết bị để xem nhật ký sự kiện.", QMessageBox::Critical);
    }
}


void DeviceManagementWidget::onSearchDeviceConfigClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["search_term"] = searchText.toStdString(); // Assuming generic search
    }
    loadDeviceConfigs(); // Reloads with current filter if any
    ERP::Logger::Logger::getInstance().info("DeviceManagementWidget: Device Config Search completed.");
}

void DeviceManagementWidget::onDeviceConfigTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString deviceId = deviceConfigTable_->item(row, 0)->text();
    std::optional<ERP::Integration::DTO::DeviceConfigDTO> configOpt = deviceManagerService_->getDeviceConfigById(deviceId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (configOpt) {
        idLineEdit_->setText(QString::fromStdString(configOpt->id));
        deviceNameLineEdit_->setText(QString::fromStdString(configOpt->deviceName));
        deviceIdentifierLineEdit_->setText(QString::fromStdString(configOpt->deviceIdentifier));
        
        populateDeviceTypeComboBox();
        int typeIndex = deviceTypeComboBox_->findData(static_cast<int>(configOpt->type));
        if (typeIndex != -1) deviceTypeComboBox_->setCurrentIndex(typeIndex);

        connectionStringLineEdit_->setText(QString::fromStdString(configOpt->connectionString.value_or("")));
        ipAddressLineEdit_->setText(QString::fromStdString(configOpt->ipAddress.value_or("")));
        
        populateConnectionStatusComboBox();
        int statusIndex = connectionStatusComboBox_->findData(static_cast<int>(configOpt->connectionStatus));
        if (statusIndex != -1) connectionStatusComboBox_->setCurrentIndex(statusIndex);

        locationIdLineEdit_->setText(QString::fromStdString(configOpt->locationId.value_or(""))); // Display ID
        notesLineEdit_->setText(QString::fromStdString(configOpt->notes.value_or("")));
        isCriticalCheckBox_->setChecked(configOpt->isCritical);

    } else {
        showMessageBox("Thông tin Cấu hình Thiết bị", "Không thể tải chi tiết cấu hình thiết bị đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void DeviceManagementWidget::clearForm() {
    idLineEdit_->clear();
    deviceNameLineEdit_->clear();
    deviceIdentifierLineEdit_->clear();
    deviceTypeComboBox_->setCurrentIndex(0);
    connectionStringLineEdit_->clear();
    ipAddressLineEdit_->clear();
    connectionStatusComboBox_->setCurrentIndex(0);
    locationIdLineEdit_->clear();
    notesLineEdit_->clear();
    isCriticalCheckBox_->setChecked(false);
    deviceConfigTable_->clearSelection();
    updateButtonsState();
}


void DeviceManagementWidget::showDeviceConfigInputDialog(ERP::Integration::DTO::DeviceConfigDTO* config) {
    QDialog dialog(this);
    dialog.setWindowTitle(config ? "Sửa Cấu hình Thiết bị" : "Đăng ký Thiết bị Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit deviceNameEdit(this);
    QLineEdit deviceIdentifierEdit(this);
    QComboBox deviceTypeCombo(this); populateDeviceTypeComboBox();
    for(int i = 0; i < deviceTypeComboBox_->count(); ++i) deviceTypeCombo.addItem(deviceTypeComboBox_->itemText(i), deviceTypeComboBox_->itemData(i));
    QLineEdit connectionStringEdit(this);
    QLineEdit ipAddressEdit(this);
    QComboBox locationCombo(this); populateLocationComboBox(&locationCombo); // Re-populate for dialog
    QLineEdit notesEdit(this);
    QCheckBox isCriticalCheck("Thiết bị quan trọng", this);

    if (config) {
        deviceNameEdit.setText(QString::fromStdString(config->deviceName));
        deviceIdentifierEdit.setText(QString::fromStdString(config->deviceIdentifier));
        int typeIndex = deviceTypeCombo.findData(static_cast<int>(config->type));
        if (typeIndex != -1) deviceTypeCombo.setCurrentIndex(typeIndex);
        connectionStringEdit.setText(QString::fromStdString(config->connectionString.value_or("")));
        ipAddressEdit.setText(QString::fromStdString(config->ipAddress.value_or("")));
        if (config->locationId) {
            int locIndex = locationCombo.findData(QString::fromStdString(*config->locationId));
            if (locIndex != -1) locationCombo.setCurrentIndex(locIndex);
            else locationCombo.setCurrentIndex(0); // "None"
        } else {
            locationCombo.setCurrentIndex(0); // "None"
        }
        notesEdit.setText(QString::fromStdString(config->notes.value_or("")));
        isCriticalCheck.setChecked(config->isCritical);

        deviceIdentifierEdit.setReadOnly(true); // Don't allow changing identifier for existing
    } else {
        // Defaults for new
    }

    formLayout->addRow("Tên Thiết bị:*", &deviceNameEdit);
    formLayout->addRow("Mã định danh:*", &deviceIdentifierEdit);
    formLayout->addRow("Loại:*", &deviceTypeCombo);
    formLayout->addRow("Chuỗi kết nối:", &connectionStringEdit);
    formLayout->addRow("Địa chỉ IP:", &ipAddressEdit);
    formLayout->addRow("Địa điểm:", &locationCombo);
    formLayout->addRow("Ghi chú:", &notesEdit);
    formLayout->addRow("", &isCriticalCheck);
    
    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(config ? "Lưu" : "Đăng ký", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Integration::DTO::DeviceConfigDTO newConfigData;
        if (config) {
            newConfigData = *config;
        }

        newConfigData.deviceName = deviceNameEdit.text().toStdString();
        newConfigData.deviceIdentifier = deviceIdentifierEdit.text().toStdString();
        newConfigData.type = static_cast<ERP::Integration::DTO::DeviceType>(deviceTypeCombo.currentData().toInt());
        newConfigData.connectionString = connectionStringEdit.text().isEmpty() ? std::nullopt : std::make_optional(connectionStringEdit.text().toStdString());
        newConfigData.ipAddress = ipAddressEdit.text().isEmpty() ? std::nullopt : std::make_optional(ipAddressEdit.text().toStdString());
        
        QString selectedLocationId = locationCombo.currentData().toString();
        newConfigData.locationId = selectedLocationId.isEmpty() ? std::nullopt : std::make_optional(selectedLocationId.toStdString());

        newConfigData.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());
        newConfigData.isCritical = isCriticalCheck.isChecked();
        newConfigData.status = ERP::Common::EntityStatus::ACTIVE; // Always active from form
        newConfigData.connectionStatus = config ? config->connectionStatus : ERP::Integration::DTO::ConnectionStatus::DISCONNECTED; // Preserve or default

        bool success = false;
        if (config) {
            success = deviceManagerService_->updateDeviceConfig(newConfigData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Cấu hình Thiết bị", "Cấu hình thiết bị đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật cấu hình thiết bị. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Integration::DTO::DeviceConfigDTO> createdConfig = deviceManagerService_->registerDevice(newConfigData, currentUserId_, currentUserRoleIds_);
            if (createdConfig) {
                showMessageBox("Đăng ký Thiết bị", "Thiết bị mới đã được đăng ký thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể đăng ký thiết bị mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadDeviceConfigs();
            clearForm();
        }
    }
}

void DeviceManagementWidget::showUpdateConnectionStatusDialog(ERP::Integration::DTO::DeviceConfigDTO* config) {
    if (!config) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Cập nhật Trạng Thái Kết nối cho: " + QString::fromStdString(config->deviceName));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox connectionStatusCombo(this); populateConnectionStatusComboBox();
    for(int i = 0; i < connectionStatusComboBox_->count(); ++i) connectionStatusCombo.addItem(connectionStatusComboBox_->itemText(i), connectionStatusComboBox_->itemData(i));
    int currentStatusIndex = connectionStatusCombo.findData(static_cast<int>(config->connectionStatus));
    if (currentStatusIndex != -1) connectionStatusCombo.setCurrentIndex(currentStatusIndex);

    QLineEdit messageEdit(this); messageEdit.setPlaceholderText("Thông báo trạng thái (tùy chọn)");

    formLayout->addRow("Trạng thái mới:*", &connectionStatusCombo);
    formLayout->addRow("Thông báo:", &messageEdit);

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton("Cập nhật", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Integration::DTO::ConnectionStatus newStatus = static_cast<ERP::Integration::DTO::ConnectionStatus>(connectionStatusCombo.currentData().toInt());
        std::optional<std::string> message = messageEdit.text().isEmpty() ? std::nullopt : std::make_optional(messageEdit.text().toStdString());

        if (deviceManagerService_->updateDeviceConnectionStatus(config->id, newStatus, message, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Cập nhật Trạng Thái Kết nối", "Trạng thái kết nối đã được cập nhật thành công.", QMessageBox::Information);
            loadDeviceConfigs(); // Refresh table
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật trạng thái kết nối. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}

void DeviceManagementWidget::showRecordDeviceEventDialog(ERP::Integration::DTO::DeviceConfigDTO* config) {
    if (!config) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Ghi nhận Sự kiện Thiết bị cho: " + QString::fromStdString(config->deviceName));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox eventTypeCombo(this);
    eventTypeCombo.addItem("Connection Established", static_cast<int>(ERP::Integration::DTO::DeviceEventType::CONNECTION_ESTABLISHED));
    eventTypeCombo.addItem("Connection Lost", static_cast<int>(ERP::Integration::DTO::DeviceEventType::CONNECTION_LOST));
    eventTypeCombo.addItem("Connection Failed", static_cast<int>(ERP::Integration::DTO::DeviceEventType::CONNECTION_FAILED));
    eventTypeCombo.addItem("Data Received", static_cast<int>(ERP::Integration::DTO::DeviceEventType::DATA_RECEIVED));
    eventTypeCombo.addItem("Command Sent", static_cast<int>(ERP::Integration::DTO::DeviceEventType::COMMAND_SENT));
    eventTypeCombo.addItem("Error", static_cast<int>(ERP::Integration::DTO::DeviceEventType::ERROR));
    eventTypeCombo.addItem("Warning", static_cast<int>(ERP::Integration::DTO::DeviceEventType::WARNING));
    eventTypeCombo.addItem("Other", static_cast<int>(ERP::Integration::DTO::DeviceEventType::OTHER));
    
    QLineEdit eventDescriptionEdit(this);
    QLineEdit eventDataJsonEdit(this); eventDataJsonEdit.setPlaceholderText("Dữ liệu sự kiện (JSON, tùy chọn)");

    formLayout->addRow("Loại Sự kiện:*", &eventTypeCombo);
    formLayout->addRow("Mô tả Sự kiện:*", &eventDescriptionEdit);
    formLayout->addRow("Dữ liệu Sự kiện (JSON):", &eventDataJsonEdit);

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
        if (eventDescriptionEdit.text().isEmpty()) {
            showMessageBox("Lỗi", "Vui lòng nhập mô tả sự kiện.", QMessageBox::Warning);
            return;
        }

        ERP::Integration::DTO::DeviceEventLogDTO eventLog;
        eventLog.deviceId = config->id;
        eventLog.eventType = static_cast<ERP::Integration::DTO::DeviceEventType>(eventTypeCombo.currentData().toInt());
        eventLog.eventDescription = eventDescriptionEdit.text().toStdString();
        eventLog.eventData = ERP::Utils::DTOUtils::jsonStringToMap(eventDataJsonEdit.text().toStdString()); // Convert JSON string to map
        eventLog.eventTime = ERP::Utils::DateUtils::now(); // Current time

        if (deviceManagerService_->recordDeviceEvent(eventLog, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Ghi nhận Sự kiện", "Sự kiện thiết bị đã được ghi nhận thành công.", QMessageBox::Information);
            // Optionally refresh event log viewer if it was open
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể ghi nhận sự kiện thiết bị. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}

void DeviceManagementWidget::showViewEventLogsDialog(ERP::Integration::DTO::DeviceConfigDTO* config) {
    if (!config) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Nhật ký Sự kiện cho Thiết bị: " + QString::fromStdString(config->deviceName));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *logsTable = new QTableWidget(&dialog);
    logsTable->setColumnCount(5); // Event Time, Event Type, Description, Event Data, Notes
    logsTable->setHorizontalHeaderLabels({"Thời gian", "Loại", "Mô tả", "Dữ liệu (JSON)", "Ghi chú"});
    logsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    logsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    logsTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(logsTable);

    // Load event logs for this device
    std::vector<ERP::Integration::DTO::DeviceEventLogDTO> logs = deviceManagerService_->getDeviceEventLogsByDevice(config->id, {}, currentUserId_, currentUserRoleIds_);
    logsTable->setRowCount(logs.size());
    for (int i = 0; i < logs.size(); ++i) {
        const auto& log = logs[i];
        logsTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(log.eventTime, ERP::Common::DATETIME_FORMAT))));
        logsTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(log.getEventTypeString())));
        logsTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(log.eventDescription)));
        logsTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DTOUtils::mapToJsonString(log.eventData)))); // Convert map to JSON string
        logsTable->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(log.notes.value_or(""))));
    }
    logsTable->resizeColumnsToContents();

    QPushButton *closeButton = new QPushButton("Đóng", &dialog);
    dialogLayout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}


void DeviceManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool DeviceManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void DeviceManagementWidget::updateButtonsState() {
    bool canRegister = hasPermission("Integration.RegisterDevice");
    bool canUpdate = hasPermission("Integration.UpdateDeviceConfig");
    bool canDelete = hasPermission("Integration.DeleteDeviceConfig");
    bool canUpdateConnectionStatus = hasPermission("Integration.UpdateDeviceConnectionStatus");
    bool canViewEvents = hasPermission("Integration.ViewDeviceEventLogs");
    bool canRecordEvent = hasPermission("Integration.RecordDeviceEvent");

    registerDeviceButton_->setEnabled(canRegister);
    searchButton_->setEnabled(hasPermission("Integration.ViewDeviceConfigs"));

    bool isRowSelected = deviceConfigTable_->currentRow() >= 0;
    editDeviceConfigButton_->setEnabled(isRowSelected && canUpdate);
    deleteDeviceConfigButton_->setEnabled(isRowSelected && canDelete);
    updateConnectionStatusButton_->setEnabled(isRowSelected && canUpdateConnectionStatus);
    viewEventLogsButton_->setEnabled(isRowSelected && canViewEvents);
    recordDeviceEventButton_->setEnabled(isRowSelected && canRecordEvent);


    bool enableForm = isRowSelected && canUpdate;
    deviceNameLineEdit_->setEnabled(enableForm);
    deviceIdentifierLineEdit_->setEnabled(enableForm); // Will be read-only for existing anyway
    deviceTypeComboBox_->setEnabled(enableForm);
    connectionStringLineEdit_->setEnabled(enableForm);
    ipAddressLineEdit_->setEnabled(enableForm);
    // connectionStatusComboBox_ is read-only
    locationIdLineEdit_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);
    isCriticalCheckBox_->setEnabled(enableForm);

    // Read-only fields
    idLineEdit_->setEnabled(false);

    if (!isRowSelected) {
        idLineEdit_->clear();
        deviceNameLineEdit_->clear();
        deviceIdentifierLineEdit_->clear();
        deviceTypeComboBox_->setCurrentIndex(0);
        connectionStringLineEdit_->clear();
        ipAddressLineEdit_->clear();
        connectionStatusComboBox_->setCurrentIndex(0);
        locationIdLineEdit_->clear();
        notesLineEdit_->clear();
        isCriticalCheckBox_->setChecked(false);
    }
}

} // namespace Integration
} // namespace UI
} // namespace ERP