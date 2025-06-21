// UI/Integration/ExternalSystemManagementWidget.cpp
#include "ExternalSystemManagementWidget.h" // Standard includes
#include "IntegrationConfig.h"              // IntegrationConfig DTO
#include "APIEndpoint.h"                    // APIEndpoint DTO
#include "ExternalSystemService.h"          // ExternalSystem Service
#include "ISecurityManager.h"               // Security Manager
#include "Logger.h"                         // Logging
#include "ErrorHandler.h"                   // Error Handling
#include "Common.h"                         // Common Enums/Constants
#include "DateUtils.h"                      // Date Utilities
#include "StringUtils.h"                    // String Utilities
#include "CustomMessageBox.h"               // Custom Message Box
#include "UserService.h"                    // For getting user names
#include "DTOUtils.h"                       // For JSON conversions

#include <QInputDialog>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QTextEdit>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDoubleValidator>

namespace ERP {
namespace UI {
namespace Integration {

ExternalSystemManagementWidget::ExternalSystemManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IExternalSystemService> externalSystemService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      externalSystemService_(externalSystemService),
      securityManager_(securityManager) {

    if (!externalSystemService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ hệ thống bên ngoài hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("ExternalSystemManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("ExternalSystemManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("ExternalSystemManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadIntegrationConfigs();
    updateButtonsState();
}

ExternalSystemManagementWidget::~ExternalSystemManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void ExternalSystemManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo mã hệ thống hoặc tên...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &ExternalSystemManagementWidget::onSearchConfigClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    configTable_ = new QTableWidget(this);
    configTable_->setColumnCount(4); // ID, Tên hệ thống, Mã hệ thống, Loại, Trạng thái
    configTable_->setHorizontalHeaderLabels({"ID", "Tên Hệ thống", "Mã Hệ thống", "Loại", "Trạng thái"});
    configTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    configTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    configTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    configTable_->horizontalHeader()->setStretchLastSection(true);
    connect(configTable_, &QTableWidget::itemClicked, this, &ExternalSystemManagementWidget::onConfigTableItemClicked);
    mainLayout->addWidget(configTable_);

    // Form elements for editing/adding configs
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    systemNameLineEdit_ = new QLineEdit(this);
    systemCodeLineEdit_ = new QLineEdit(this);
    integrationTypeComboBox_ = new QComboBox(this); populateIntegrationTypeComboBox();
    baseUrlLineEdit_ = new QLineEdit(this);
    usernameLineEdit_ = new QLineEdit(this);
    passwordLineEdit_ = new QLineEdit(this); passwordLineEdit_->setEchoMode(QLineEdit::Password);
    isEncryptedCheckBox_ = new QCheckBox("Mã hóa thông tin xác thực", this);
    metadataTextEdit_ = new QTextEdit(this); metadataTextEdit_->setPlaceholderText("Metadata (JSON, tùy chọn)");
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();

    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("Tên Hệ thống:*", &systemNameLineEdit_);
    formLayout->addRow("Mã Hệ thống:*", &systemCodeLineEdit_);
    formLayout->addRow("Loại Tích hợp:*", &integrationTypeComboBox_);
    formLayout->addRow("Base URL:", &baseUrlLineEdit_);
    formLayout->addRow("Tên người dùng API:", &usernameLineEdit_);
    formLayout->addRow("Mật khẩu API:", &passwordLineEdit_);
    formLayout->addRow("", &isEncryptedCheckBox_);
    formLayout->addRow("Metadata (JSON):", &metadataTextEdit_);
    formLayout->addRow("Trạng thái:", &statusComboBox_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    createConfigButton_ = new QPushButton("Tạo Cấu hình", this); connect(createConfigButton_, &QPushButton::clicked, this, &ExternalSystemManagementWidget::onCreateConfigClicked);
    editConfigButton_ = new QPushButton("Sửa Cấu hình", this); connect(editConfigButton_, &QPushButton::clicked, this, &ExternalSystemManagementWidget::onEditConfigClicked);
    deleteConfigButton_ = new QPushButton("Xóa Cấu hình", this); connect(deleteConfigButton_, &QPushButton::clicked, this, &ExternalSystemManagementWidget::onDeleteConfigClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &ExternalSystemManagementWidget::onUpdateConfigStatusClicked);
    manageAPIEndpointsButton_ = new QPushButton("Quản lý Điểm cuối API", this); connect(manageAPIEndpointsButton_, &QPushButton::clicked, this, &ExternalSystemManagementWidget::onManageAPIEndpointsClicked);
    sendDataButton_ = new QPushButton("Gửi dữ liệu Test", this); connect(sendDataButton_, &QPushButton::clicked, this, &ExternalSystemManagementWidget::onSendDataClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &ExternalSystemManagementWidget::onSearchConfigClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &ExternalSystemManagementWidget::clearForm);
    
    buttonLayout->addWidget(createConfigButton_);
    buttonLayout->addWidget(editConfigButton_);
    buttonLayout->addWidget(deleteConfigButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(manageAPIEndpointsButton_);
    buttonLayout->addWidget(sendDataButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void ExternalSystemManagementWidget::loadIntegrationConfigs() {
    ERP::Logger::Logger::getInstance().info("ExternalSystemManagementWidget: Loading integration configs...");
    configTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Integration::DTO::IntegrationConfigDTO> configs = externalSystemService_->getAllIntegrationConfigs({}, currentUserId_, currentUserRoleIds_);

    configTable_->setRowCount(configs.size());
    for (int i = 0; i < configs.size(); ++i) {
        const auto& config = configs[i];
        configTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(config.id)));
        configTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(config.systemName)));
        configTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(config.systemCode)));
        configTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(config.getTypeString())));
        configTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(config.status))));
    }
    configTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ExternalSystemManagementWidget: Integration configs loaded successfully.");
}

void ExternalSystemManagementWidget::populateIntegrationTypeComboBox() {
    integrationTypeComboBox_->clear();
    integrationTypeComboBox_->addItem("ERP", static_cast<int>(ERP::Integration::DTO::IntegrationType::ERP));
    integrationTypeComboBox_->addItem("CRM", static_cast<int>(ERP::Integration::DTO::IntegrationType::CRM));
    integrationTypeComboBox_->addItem("WMS", static_cast<int>(ERP::Integration::DTO::IntegrationType::WMS));
    integrationTypeComboBox_->addItem("E-commerce", static_cast<int>(ERP::Integration::DTO::IntegrationType::E_COMMERCE));
    integrationTypeComboBox_->addItem("Payment Gateway", static_cast<int>(ERP::Integration::DTO::IntegrationType::PAYMENT_GATEWAY));
    integrationTypeComboBox_->addItem("Shipping Carrier", static_cast<int>(ERP::Integration::DTO::IntegrationType::SHIPPING_CARRIER));
    integrationTypeComboBox_->addItem("Manufacturing", static_cast<int>(ERP::Integration::DTO::IntegrationType::MANUFACTURING));
    integrationTypeComboBox_->addItem("Other", static_cast<int>(ERP::Integration::DTO::IntegrationType::OTHER));
    integrationTypeComboBox_->addItem("Unknown", static_cast<int>(ERP::Integration::DTO::IntegrationType::UNKNOWN));
}

void ExternalSystemManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Active", static_cast<int>(ERP::Common::EntityStatus::ACTIVE));
    statusComboBox_->addItem("Inactive", static_cast<int>(ERP::Common::EntityStatus::INACTIVE));
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Common::EntityStatus::PENDING));
    statusComboBox_->addItem("Deleted", static_cast<int>(ERP::Common::EntityStatus::DELETED));
}


void ExternalSystemManagementWidget::onCreateConfigClicked() {
    if (!hasPermission("Integration.CreateIntegrationConfig")) {
        showMessageBox("Lỗi", "Bạn không có quyền tạo cấu hình tích hợp.", QMessageBox::Warning);
        return;
    }
    clearForm();
    showConfigInputDialog();
}

void ExternalSystemManagementWidget::onEditConfigClicked() {
    if (!hasPermission("Integration.UpdateIntegrationConfig")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa cấu hình tích hợp.", QMessageBox::Warning);
        return;
    }

    int selectedRow = configTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Cấu hình Tích hợp", "Vui lòng chọn một cấu hình tích hợp để sửa.", QMessageBox::Information);
        return;
    }

    QString configId = configTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Integration::DTO::IntegrationConfigDTO> configOpt = externalSystemService_->getIntegrationConfigById(configId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (configOpt) {
        showConfigInputDialog(&(*configOpt));
    } else {
        showMessageBox("Sửa Cấu hình Tích hợp", "Không tìm thấy cấu hình tích hợp để sửa.", QMessageBox::Critical);
    }
}

void ExternalSystemManagementWidget::onDeleteConfigClicked() {
    if (!hasPermission("Integration.DeleteIntegrationConfig")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa cấu hình tích hợp.", QMessageBox::Warning);
        return;
    }

    int selectedRow = configTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Cấu hình Tích hợp", "Vui lòng chọn một cấu hình tích hợp để xóa.", QMessageBox::Information);
        return;
    }

    QString configId = configTable_->item(selectedRow, 0)->text();
    QString systemName = configTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Cấu hình Tích hợp");
    confirmBox.setText("Bạn có chắc chắn muốn xóa cấu hình tích hợp '" + systemName + "' (ID: " + configId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (externalSystemService_->deleteIntegrationConfig(configId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Cấu hình Tích hợp", "Cấu hình tích hợp đã được xóa thành công.", QMessageBox::Information);
            loadIntegrationConfigs();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa cấu hình tích hợp. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void ExternalSystemManagementWidget::onUpdateConfigStatusClicked() {
    if (!hasPermission("Integration.UpdateIntegrationConfigStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái cấu hình tích hợp.", QMessageBox::Warning);
        return;
    }

    int selectedRow = configTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một cấu hình tích hợp để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString configId = configTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Integration::DTO::IntegrationConfigDTO> configOpt = externalSystemService_->getIntegrationConfigById(configId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!configOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy cấu hình tích hợp để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Integration::DTO::IntegrationConfigDTO currentConfig = *configOpt;
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
    int currentStatusIndex = newStatusCombo.findData(static_cast<int>(currentConfig.status));
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
        ERP::Common::EntityStatus newStatus = static_cast<ERP::Common::EntityStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái cấu hình tích hợp");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái cấu hình tích hợp '" + QString::fromStdString(currentConfig.systemName) + "' thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (externalSystemService_->updateIntegrationConfigStatus(configId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái cấu hình tích hợp đã được cập nhật thành công.", QMessageBox::Information);
                loadIntegrationConfigs();
                clearForm();
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật trạng thái cấu hình tích hợp. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
    }
}


void ExternalSystemManagementWidget::onSearchConfigClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["search_term"] = searchText.toStdString(); // Assuming generic search by code/name
    }
    loadIntegrationConfigs(); // Reloads with current filter if any
    ERP::Logger::Logger::getInstance().info("ExternalSystemManagementWidget: Integration Config Search completed.");
}

void ExternalSystemManagementWidget::onConfigTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString configId = configTable_->item(row, 0)->text();
    std::optional<ERP::Integration::DTO::IntegrationConfigDTO> configOpt = externalSystemService_->getIntegrationConfigById(configId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (configOpt) {
        idLineEdit_->setText(QString::fromStdString(configOpt->id));
        systemNameLineEdit_->setText(QString::fromStdString(configOpt->systemName));
        systemCodeLineEdit_->setText(QString::fromStdString(configOpt->systemCode));
        
        populateIntegrationTypeComboBox();
        int typeIndex = integrationTypeComboBox_->findData(static_cast<int>(configOpt->type));
        if (typeIndex != -1) integrationTypeComboBox_->setCurrentIndex(typeIndex);

        baseUrlLineEdit_->setText(QString::fromStdString(configOpt->baseUrl.value_or("")));
        usernameLineEdit_->setText(QString::fromStdString(configOpt->username.value_or("")));
        // Password is not displayed for security
        isEncryptedCheckBox_->setChecked(configOpt->isEncrypted);
        metadataTextEdit_->setText(QString::fromStdString(ERP::Utils::DTOUtils::mapToJsonString(configOpt->metadata)));
        
        populateStatusComboBox();
        int statusIndex = statusComboBox_->findData(static_cast<int>(configOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);

    } else {
        showMessageBox("Thông tin Cấu hình Tích hợp", "Không thể tải chi tiết cấu hình tích hợp đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void ExternalSystemManagementWidget::clearForm() {
    idLineEdit_->clear();
    systemNameLineEdit_->clear();
    systemCodeLineEdit_->clear();
    integrationTypeComboBox_->setCurrentIndex(0);
    baseUrlLineEdit_->clear();
    usernameLineEdit_->clear();
    passwordLineEdit_->clear();
    isEncryptedCheckBox_->setChecked(false);
    metadataTextEdit_->clear();
    statusComboBox_->setCurrentIndex(0);
    configTable_->clearSelection();
    updateButtonsState();
}

void ExternalSystemManagementWidget::onManageAPIEndpointsClicked() {
    if (!hasPermission("Integration.ManageAPIEndpoints")) { // Assuming this permission
        showMessageBox("Lỗi", "Bạn không có quyền quản lý điểm cuối API.", QMessageBox::Warning);
        return;
    }

    int selectedRow = configTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Quản lý Điểm cuối API", "Vui lòng chọn một cấu hình tích hợp để quản lý điểm cuối API.", QMessageBox::Information);
        return;
    }

    QString configId = configTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Integration::DTO::IntegrationConfigDTO> configOpt = externalSystemService_->getIntegrationConfigById(configId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (configOpt) {
        showManageAPIEndpointsDialog(&(*configOpt));
    } else {
        showMessageBox("Quản lý Điểm cuối API", "Không tìm thấy cấu hình tích hợp để quản lý điểm cuối API.", QMessageBox::Critical);
    }
}

void ExternalSystemManagementWidget::onSendDataClicked() {
    if (!hasPermission("Integration.SendData")) {
        showMessageBox("Lỗi", "Bạn không có quyền gửi dữ liệu test.", QMessageBox::Warning);
        return;
    }

    int selectedRow = configTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Gửi dữ liệu Test", "Vui lòng chọn một cấu hình tích hợp để gửi dữ liệu.", QMessageBox::Information);
        return;
    }

    QString configId = configTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Integration::DTO::IntegrationConfigDTO> configOpt = externalSystemService_->getIntegrationConfigById(configId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!configOpt) {
        showMessageBox("Gửi dữ liệu Test", "Không tìm thấy cấu hình tích hợp để gửi dữ liệu.", QMessageBox::Critical);
        return;
    }
    
    showSendDataDialog(&(*configOpt));
}


void ExternalSystemManagementWidget::showConfigInputDialog(ERP::Integration::DTO::IntegrationConfigDTO* config) {
    QDialog dialog(this);
    dialog.setWindowTitle(config ? "Sửa Cấu hình Tích hợp" : "Tạo Cấu hình Tích hợp Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit systemNameEdit(this);
    QLineEdit systemCodeEdit(this);
    QComboBox integrationTypeCombo(this); populateIntegrationTypeComboBox();
    for(int i = 0; i < integrationTypeComboBox_->count(); ++i) integrationTypeCombo.addItem(integrationTypeComboBox_->itemText(i), integrationTypeComboBox_->itemData(i));
    QLineEdit baseUrlEdit(this);
    QLineEdit usernameEdit(this);
    QLineEdit passwordEdit(this); passwordEdit.setEchoMode(QLineEdit::Password);
    QCheckBox isEncryptedCheck("Mã hóa thông tin xác thực", this);
    QTextEdit metadataEdit(this); metadataEdit.setPlaceholderText("Metadata (JSON, tùy chọn)");

    if (config) {
        systemNameEdit.setText(QString::fromStdString(config->systemName));
        systemCodeEdit.setText(QString::fromStdString(config->systemCode));
        int typeIndex = integrationTypeCombo.findData(static_cast<int>(config->type));
        if (typeIndex != -1) integrationTypeCombo.setCurrentIndex(typeIndex);
        baseUrlEdit.setText(QString::fromStdString(config->baseUrl.value_or("")));
        usernameEdit.setText(QString::fromStdString(config->username.value_or("")));
        // Password is not pre-filled for security
        isEncryptedCheck.setChecked(config->isEncrypted);
        metadataEdit.setText(QString::fromStdString(ERP::Utils::DTOUtils::mapToJsonString(config->metadata)));
        
        systemCodeEdit.setReadOnly(true); // Don't allow changing code for existing
    } else {
        // Defaults for new
        isEncryptedCheck.setChecked(false);
        metadataEdit.setText("{}");
    }

    formLayout->addRow("Tên Hệ thống:*", &systemNameEdit);
    formLayout->addRow("Mã Hệ thống:*", &systemCodeEdit);
    formLayout->addRow("Loại Tích hợp:*", &integrationTypeCombo);
    formLayout->addRow("Base URL:", &baseUrlEdit);
    formLayout->addRow("Tên người dùng API:", &usernameEdit);
    formLayout->addRow("Mật khẩu API:", &passwordEdit);
    formLayout->addRow("", &isEncryptedCheck);
    formLayout->addRow("Metadata (JSON):", &metadataEdit);
    
    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(config ? "Lưu" : "Tạo", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Integration::DTO::IntegrationConfigDTO newConfigData;
        if (config) {
            newConfigData = *config;
        }

        newConfigData.systemName = systemNameEdit.text().toStdString();
        newConfigData.systemCode = systemCodeEdit.text().toStdString();
        newConfigData.type = static_cast<ERP::Integration::DTO::IntegrationType>(integrationTypeCombo.currentData().toInt());
        newConfigData.baseUrl = baseUrlEdit.text().isEmpty() ? std::nullopt : std::make_optional(baseUrlEdit.text().toStdString());
        newConfigData.username = usernameEdit.text().isEmpty() ? std::nullopt : std::make_optional(usernameEdit.text().toStdString());
        
        // Handle password update: only update if password field is not empty
        if (!passwordEdit.text().isEmpty()) {
            newConfigData.password = passwordEdit.text().toStdString(); // Service will handle encryption
        } else if (config) { // If editing and password field is empty, retain old password
            newConfigData.password = config->password;
        } else { // New config and password is empty
            newConfigData.password = std::nullopt;
        }

        newConfigData.isEncrypted = isEncryptedCheck.isChecked();
        newConfigData.metadata = ERP::Utils::DTOUtils::jsonStringToMap(metadataEdit.toPlainText().toStdString()); // Convert JSON string to map
        newConfigData.status = ERP::Common::EntityStatus::ACTIVE; // Always active from form

        // API Endpoints are handled in separate dialog
        std::vector<ERP::Integration::DTO::APIEndpointDTO> currentEndpoints;
        if (config) {
             currentEndpoints = externalSystemService_->getAPIEndpointsByIntegrationConfig(config->id, currentUserId_, currentUserRoleIds_);
        }

        bool success = false;
        if (config) {
            success = externalSystemService_->updateIntegrationConfig(newConfigData, currentEndpoints, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Cấu hình Tích hợp", "Cấu hình tích hợp đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật cấu hình tích hợp. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Integration::DTO::IntegrationConfigDTO> createdConfig = externalSystemService_->createIntegrationConfig(newConfigData, {}, currentUserId_, currentUserRoleIds_); // Create with empty endpoints
            if (createdConfig) {
                showMessageBox("Tạo Cấu hình Tích hợp", "Cấu hình tích hợp mới đã được tạo thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandler::getLastUserMessage().value_or("Không thể tạo cấu hình tích hợp mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadIntegrationConfigs();
            clearForm();
        }
    }
}

void ExternalSystemManagementWidget::showManageAPIEndpointsDialog(ERP::Integration::DTO::IntegrationConfigDTO* config) {
    if (!config) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Quản lý Điểm cuối API cho: " + QString::fromStdString(config->systemName));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *endpointsTable = new QTableWidget(&dialog);
    endpointsTable->setColumnCount(5); // Code, Method, URL, Description, IsActive
    endpointsTable->setHorizontalHeaderLabels({"Mã Điểm cuối", "Phương thức", "URL", "Mô tả", "Hoạt động"});
    endpointsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    endpointsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    endpointsTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(endpointsTable);

    // Load existing endpoints
    std::vector<ERP::Integration::DTO::APIEndpointDTO> currentEndpoints = externalSystemService_->getAPIEndpointsByIntegrationConfig(config->id, currentUserId_, currentUserRoleIds_);
    endpointsTable->setRowCount(currentEndpoints.size());
    for (int i = 0; i < currentEndpoints.size(); ++i) {
        const auto& endpoint = currentEndpoints[i];
        endpointsTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(endpoint.endpointCode)));
        endpointsTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(endpoint.getMethodString())));
        endpointsTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(endpoint.url)));
        endpointsTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(endpoint.description.value_or(""))));
        endpointsTable->setItem(i, 4, new QTableWidgetItem(endpoint.status == ERP::Common::EntityStatus::ACTIVE ? "Yes" : "No"));
        endpointsTable->item(i, 0)->setData(Qt::UserRole, QString::fromStdString(endpoint.id)); // Store endpoint ID
        endpointsTable->item(i, 1)->setData(Qt::UserRole, static_cast<int>(endpoint.method)); // Store method int
    }


    QHBoxLayout *itemButtonsLayout = new QHBoxLayout();
    QPushButton *addItemButton = new QPushButton("Thêm Điểm cuối", &dialog);
    QPushButton *editItemButton = new QPushButton("Sửa Điểm cuối", &dialog);
    QPushButton *deleteItemButton = new QPushButton("Xóa Điểm cuối", &dialog);
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
        itemDialog.setWindowTitle("Thêm Điểm cuối API");
        QFormLayout itemFormLayout;
        QLineEdit codeEdit;
        QComboBox methodCombo;
        methodCombo.addItem("GET", static_cast<int>(ERP::Integration::DTO::HTTPMethod::GET));
        methodCombo.addItem("POST", static_cast<int>(ERP::Integration::DTO::HTTPMethod::POST));
        methodCombo.addItem("PUT", static_cast<int>(ERP::Integration::DTO::HTTPMethod::PUT));
        methodCombo.addItem("DELETE", static_cast<int>(ERP::Integration::DTO::HTTPMethod::DELETE));
        QLineEdit urlEdit;
        QLineEdit descriptionEdit;
        QCheckBox isActiveCheck("Hoạt động", &itemDialog); isActiveCheck.setChecked(true);

        itemFormLayout.addRow("Mã Điểm cuối:*", &codeEdit);
        itemFormLayout.addRow("Phương thức:*", &methodCombo);
        itemFormLayout.addRow("URL:*", &urlEdit);
        itemFormLayout.addRow("Mô tả:", &descriptionEdit);
        itemFormLayout.addRow("", &isActiveCheck);

        QPushButton *okItemButton = new QPushButton("Thêm", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (codeEdit.text().isEmpty() || urlEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin điểm cuối (Mã, URL).", QMessageBox::Warning);
                return;
            }
            // Add to table
            int newRow = endpointsTable->rowCount();
            endpointsTable->insertRow(newRow);
            endpointsTable->setItem(newRow, 0, new QTableWidgetItem(codeEdit.text()));
            endpointsTable->setItem(newRow, 1, new QTableWidgetItem(methodCombo.currentText()));
            endpointsTable->setItem(newRow, 2, new QTableWidgetItem(urlEdit.text()));
            endpointsTable->setItem(newRow, 3, new QTableWidgetItem(descriptionEdit.text()));
            endpointsTable->setItem(newRow, 4, new QTableWidgetItem(isActiveCheck.isChecked() ? "Yes" : "No"));
            endpointsTable->item(newRow, 0)->setData(Qt::UserRole, QVariant()); // No ID until saved
            endpointsTable->item(newRow, 1)->setData(Qt::UserRole, methodCombo.currentData()); // Store method int
        }
    });

    connect(editItemButton, &QPushButton::clicked, [&]() {
        int selectedItemRow = endpointsTable->currentRow();
        if (selectedItemRow < 0) {
            showMessageBox("Sửa Điểm cuối", "Vui lòng chọn một điểm cuối để sửa.", QMessageBox::Information);
            return;
        }

        QDialog itemDialog(&dialog);
        itemDialog.setWindowTitle("Sửa Điểm cuối API");
        QFormLayout itemFormLayout;
        QLineEdit codeEdit; codeEdit.setReadOnly(true); // Code usually immutable
        QComboBox methodCombo;
        methodCombo.addItem("GET", static_cast<int>(ERP::Integration::DTO::HTTPMethod::GET));
        methodCombo.addItem("POST", static_cast<int>(ERP::Integration::DTO::HTTPMethod::POST));
        methodCombo.addItem("PUT", static_cast<int>(ERP::Integration::DTO::HTTPMethod::PUT));
        methodCombo.addItem("DELETE", static_cast<int>(ERP::Integration::DTO::HTTPMethod::DELETE));
        QLineEdit urlEdit;
        QLineEdit descriptionEdit;
        QCheckBox isActiveCheck("Hoạt động", &itemDialog);

        // Populate with current item data
        codeEdit.setText(endpointsTable->item(selectedItemRow, 0)->text());
        int methodIndex = methodCombo.findData(endpointsTable->item(selectedItemRow, 1)->data(Qt::UserRole)); // Use UserRole if method int is stored
        if (methodIndex != -1) methodCombo.setCurrentIndex(methodIndex);
        else { // Fallback if only text stored
            int textMethodIndex = methodCombo.findText(endpointsTable->item(selectedItemRow, 1)->text());
            if (textMethodIndex != -1) methodCombo.setCurrentIndex(textMethodIndex);
        }
        urlEdit.setText(endpointsTable->item(selectedItemRow, 2)->text());
        descriptionEdit.setText(endpointsTable->item(selectedItemRow, 3)->text());
        isActiveCheck.setChecked(endpointsTable->item(selectedItemRow, 4)->text() == "Yes");

        itemFormLayout.addRow("Mã Điểm cuối:*", &codeEdit);
        itemFormLayout.addRow("Phương thức:*", &methodCombo);
        itemFormLayout.addRow("URL:*", &urlEdit);
        itemFormLayout.addRow("Mô tả:", &descriptionEdit);
        itemFormLayout.addRow("", &isActiveCheck);

        QPushButton *okItemButton = new QPushButton("Lưu", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (codeEdit.text().isEmpty() || urlEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin điểm cuối (Mã, URL).", QMessageBox::Warning);
                return;
            }
            // Update table row
            endpointsTable->setItem(selectedItemRow, 0, new QTableWidgetItem(codeEdit.text()));
            endpointsTable->setItem(selectedItemRow, 1, new QTableWidgetItem(methodCombo.currentText()));
            endpointsTable->setItem(selectedItemRow, 2, new QTableWidgetItem(urlEdit.text()));
            endpointsTable->setItem(selectedItemRow, 3, new QTableWidgetItem(descriptionEdit.text()));
            endpointsTable->setItem(selectedItemRow, 4, new QTableWidgetItem(isActiveCheck.isChecked() ? "Yes" : "No"));
            endpointsTable->item(selectedItemRow, 1)->setData(Qt::UserRole, methodCombo.currentData()); // Store method int
        }
    });

    connect(deleteItemButton, &QPushButton::clicked, [&]() {
        int selectedItemRow = endpointsTable->currentRow();
        if (selectedItemRow < 0) {
            showMessageBox("Xóa Điểm cuối", "Vui lòng chọn một điểm cuối để xóa.", QMessageBox::Information);
            return;
        }
        Common::CustomMessageBox confirmDelBox(&dialog);
        confirmDelBox.setWindowTitle("Xóa Điểm cuối API");
        confirmDelBox.setText("Bạn có chắc chắn muốn xóa điểm cuối API này?");
        confirmDelBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmDelBox.exec() == QMessageBox::Yes) {
            endpointsTable->removeRow(selectedItemRow);
        }
    });

    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Collect all items from the table
        std::vector<ERP::Integration::DTO::APIEndpointDTO> updatedEndpoints;
        for (int i = 0; i < endpointsTable->rowCount(); ++i) {
            ERP::Integration::DTO::APIEndpointDTO endpoint;
            // If item had an ID (from existing data), reuse it, otherwise generate new
            QString existingId = endpointsTable->item(i, 0)->data(Qt::UserRole).toString();
            endpoint.id = existingId.isEmpty() ? ERP::Utils::generateUUID() : existingId.toStdString();

            endpoint.integrationConfigId = config->id;
            endpoint.endpointCode = endpointsTable->item(i, 0)->text().toStdString();
            endpoint.method = static_cast<ERP::Integration::DTO::HTTPMethod>(endpointsTable->item(i, 1)->data(Qt::UserRole).toInt());
            endpoint.url = endpointsTable->item(i, 2)->text().toStdString();
            endpoint.description = endpointsTable->item(i, 3)->text().isEmpty() ? std::nullopt : std::make_optional(endpointsTable->item(i, 3)->text().toStdString());
            endpoint.status = endpointsTable->item(i, 4)->text() == "Yes" ? ERP::Common::EntityStatus::ACTIVE : ERP::Common::EntityStatus::INACTIVE;
            
            updatedEndpoints.push_back(endpoint);
        }

        // Call service to update Integration Config with new endpoint list
        // This assumes externalSystemService_->updateIntegrationConfig takes the full list of endpoints for replacement
        if (externalSystemService_->updateIntegrationConfig(*config, updatedEndpoints, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Quản lý Điểm cuối API", "Điểm cuối API đã được cập nhật thành công.", QMessageBox::Information);
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật điểm cuối API. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}

void ExternalSystemManagementWidget::showSendDataDialog(ERP::Integration::DTO::IntegrationConfigDTO* config) {
    if (!config) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Gửi dữ liệu Test qua: " + QString::fromStdString(config->systemName));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox endpointCombo(this);
    std::vector<ERP::Integration::DTO::APIEndpointDTO> endpoints = externalSystemService_->getAPIEndpointsByIntegrationConfig(config->id, currentUserId_, currentUserRoleIds_);
    for (const auto& ep : endpoints) {
        endpointCombo.addItem(QString::fromStdString(ep.endpointCode + " (" + ep.getMethodString() + " " + ep.url + ")"), QString::fromStdString(ep.endpointCode));
    }
    
    QTextEdit dataToSendEdit(this); dataToSendEdit.setPlaceholderText("Dữ liệu gửi (JSON)");

    formLayout->addRow("Chọn Điểm cuối:*", &endpointCombo);
    formLayout->addRow("Dữ liệu gửi (JSON):*", &dataToSendEdit);

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton("Gửi", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        if (endpointCombo.currentData().isNull() || dataToSendEdit.toPlainText().isEmpty()) {
            showMessageBox("Lỗi", "Vui lòng chọn một điểm cuối và nhập dữ liệu JSON để gửi.", QMessageBox::Warning);
            return;
        }
        
        QString selectedEndpointCode = endpointCombo.currentData().toString();
        std::map<std::string, std::any> dataMapToSend;
        try {
            dataMapToSend = ERP::Utils::DTOUtils::jsonStringToMap(dataToSendEdit.toPlainText().toStdString());
        } catch (const std::exception& e) {
            showMessageBox("Lỗi JSON", "Dữ liệu JSON không hợp lệ: " + QString::fromStdString(e.what()), QMessageBox::Warning);
            return;
        }

        if (externalSystemService_->sendDataToExternalSystem(selectedEndpointCode.toStdString(), dataMapToSend, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Gửi dữ liệu Test", "Dữ liệu đã được gửi thành công. Vui lòng kiểm tra log hệ thống bên ngoài.", QMessageBox::Information);
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandler::getLastUserMessage().value_or("Không thể gửi dữ liệu test. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void ExternalSystemManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool ExternalSystemManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void ExternalSystemManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Integration.CreateIntegrationConfig");
    bool canUpdate = hasPermission("Integration.UpdateIntegrationConfig");
    bool canDelete = hasPermission("Integration.DeleteIntegrationConfig");
    bool canUpdateStatus = hasPermission("Integration.UpdateIntegrationConfigStatus");
    bool canManageEndpoints = hasPermission("Integration.ManageAPIEndpoints"); // Assuming this permission
    bool canSendData = hasPermission("Integration.SendData");

    createConfigButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Integration.ViewIntegrationConfigs"));

    bool isRowSelected = configTable_->currentRow() >= 0;
    editConfigButton_->setEnabled(isRowSelected && canUpdate);
    deleteConfigButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canUpdateStatus);
    manageAPIEndpointsButton_->setEnabled(isRowSelected && canManageEndpoints);
    sendDataButton_->setEnabled(isRowSelected && canSendData);


    bool enableForm = isRowSelected && canUpdate;
    systemNameLineEdit_->setEnabled(enableForm);
    systemCodeLineEdit_->setEnabled(enableForm); // Will be read-only for existing anyway
    integrationTypeComboBox_->setEnabled(enableForm);
    baseUrlLineEdit_->setEnabled(enableForm);
    usernameLineEdit_->setEnabled(enableForm);
    passwordLineEdit_->setEnabled(enableForm);
    isEncryptedCheckBox_->setEnabled(enableForm);
    metadataTextEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);

    // Read-only fields
    idLineEdit_->setEnabled(false);

    if (!isRowSelected) {
        idLineEdit_->clear();
        systemNameLineEdit_->clear();
        systemCodeLineEdit_->clear();
        integrationTypeComboBox_->setCurrentIndex(0);
        baseUrlLineEdit_->clear();
        usernameLineEdit_->clear();
        passwordLineEdit_->clear();
        isEncryptedCheckBox_->setChecked(false);
        metadataTextEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
    }
}


} // namespace Integration
} // namespace UI
} // namespace ERP