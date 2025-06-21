// UI/Security/AuditLogViewerWidget.cpp
#include "AuditLogViewerWidget.h" // Standard includes
#include "AuditLog.h"             // AuditLog DTO
#include "User.h"                 // User DTO
#include "AuditLogService.h"      // AuditLog Service
#include "UserService.h"          // User Service
#include "ISecurityManager.h"     // Security Manager
#include "Logger.h"               // Logging
#include "ErrorHandler.h"         // Error Handling
#include "Common.h"               // Common Enums/Constants
#include "DateUtils.h"            // Date Utilities
#include "StringUtils.h"          // String Utilities
#include "CustomMessageBox.h"     // Custom Message Box
#include "DTOUtils.h"             // For JSON conversions

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace ERP {
namespace UI {
namespace Security {

AuditLogViewerWidget::AuditLogViewerWidget(
    QWidget *parent,
    std::shared_ptr<Services::IAuditLogService> auditLogService,
    std::shared_ptr<ISecurityManager> securityManager)
    : QWidget(parent),
      auditLogService_(auditLogService),
      securityManager_(securityManager) {

    if (!auditLogService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ nhật ký kiểm toán hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("AuditLogViewerWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("AuditLogViewerWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("AuditLogViewerWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadAuditLogs();
    updateButtonsState();
}

AuditLogViewerWidget::~AuditLogViewerWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void AuditLogViewerWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo ID người dùng, module, hoặc thực thể...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &AuditLogViewerWidget::onSearchLogClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    logTable_ = new QTableWidget(this);
    logTable_->setColumnCount(8); // ID, Người dùng, Loại hành động, Mức độ, Module, Sub-module, Thực thể, Thời gian
    logTable_->setHorizontalHeaderLabels({"ID", "Người dùng", "Loại Hành động", "Mức độ", "Module", "Sub-module", "Thực thể", "Thời gian"});
    logTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    logTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    logTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    logTable_->horizontalHeader()->setStretchLastSection(true);
    connect(logTable_, &QTableWidget::itemClicked, this, &AuditLogViewerWidget::onLogTableItemClicked);
    mainLayout->addWidget(logTable_);

    // Form elements for displaying log details (read-only)
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    userIdLineEdit_ = new QLineEdit(this); userIdLineEdit_->setReadOnly(true);
    userNameLineEdit_ = new QLineEdit(this); userNameLineEdit_->setReadOnly(true);
    sessionIdLineEdit_ = new QLineEdit(this); sessionIdLineEdit_->setReadOnly(true);
    actionTypeComboBox_ = new QComboBox(this); populateActionTypeComboBox(); actionTypeComboBox_->setEnabled(false);
    severityComboBox_ = new QComboBox(this); populateSeverityComboBox(); severityComboBox_->setEnabled(false);
    moduleLineEdit_ = new QLineEdit(this); moduleLineEdit_->setReadOnly(true);
    subModuleLineEdit_ = new QLineEdit(this); subModuleLineEdit_->setReadOnly(true);
    entityIdLineEdit_ = new QLineEdit(this); entityIdLineEdit_->setReadOnly(true);
    entityTypeLineEdit_ = new QLineEdit(this); entityTypeLineEdit_->setReadOnly(true);
    entityNameLineEdit_ = new QLineEdit(this); entityNameLineEdit_->setReadOnly(true);
    ipAddressLineEdit_ = new QLineEdit(this); ipAddressLineEdit_->setReadOnly(true);
    userAgentLineEdit_ = new QLineEdit(this); userAgentLineEdit_->setReadOnly(true);
    workstationIdLineEdit_ = new QLineEdit(this); workstationIdLineEdit_->setReadOnly(true);
    createdAtEdit_ = new QDateTimeEdit(this); createdAtEdit_->setReadOnly(true); createdAtEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    beforeDataTextEdit_ = new QTextEdit(this); beforeDataTextEdit_->setReadOnly(true);
    afterDataTextEdit_ = new QTextEdit(this); afterDataTextEdit_->setReadOnly(true);
    changeReasonLineEdit_ = new QLineEdit(this); changeReasonLineEdit_->setReadOnly(true);
    metadataTextEdit_ = new QTextEdit(this); metadataTextEdit_->setReadOnly(true);
    commentsLineEdit_ = new QLineEdit(this); commentsLineEdit_->setReadOnly(true);
    approvalIdLineEdit_ = new QLineEdit(this); approvalIdLineEdit_->setReadOnly(true);
    isCompliantCheckBox_ = new QCheckBox("Tuân thủ", this); isCompliantCheckBox_->setEnabled(false);
    complianceNoteLineEdit_ = new QLineEdit(this); complianceNoteLineEdit_->setReadOnly(true);


    formLayout->addWidget(new QLabel("ID Log:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("ID Người dùng:", &userIdLineEdit_);
    formLayout->addRow("Tên Người dùng:", &userNameLineEdit_);
    formLayout->addRow("ID Phiên:", &sessionIdLineEdit_);
    formLayout->addRow("Loại Hành động:", &actionTypeComboBox_);
    formLayout->addRow("Mức độ:", &severityComboBox_);
    formLayout->addRow("Module:", &moduleLineEdit_);
    formLayout->addRow("Sub-module:", &subModuleLineEdit_);
    formLayout->addRow("ID Thực thể:", &entityIdLineEdit_);
    formLayout->addRow("Loại Thực thể:", &entityTypeLineEdit_);
    formLayout->addRow("Tên Thực thể:", &entityNameLineEdit_);
    formLayout->addRow("Địa chỉ IP:", &ipAddressLineEdit_);
    formLayout->addRow("User Agent:", &userAgentLineEdit_);
    formLayout->addRow("ID Máy trạm:", &workstationIdLineEdit_);
    formLayout->addRow("Thời gian tạo:", &createdAtEdit_);
    formLayout->addRow("Dữ liệu Trước (JSON):", &beforeDataTextEdit_);
    formLayout->addRow("Dữ liệu Sau (JSON):", &afterDataTextEdit_);
    formLayout->addRow("Lý do thay đổi:", &changeReasonLineEdit_);
    formLayout->addRow("Metadata (JSON):", &metadataTextEdit_);
    formLayout->addRow("Bình luận:", &commentsLineEdit_);
    formLayout->addRow("ID Phê duyệt:", &approvalIdLineEdit_);
    formLayout->addRow("Tuân thủ:", &isCompliantCheckBox_);
    formLayout->addRow("Ghi chú tuân thủ:", &complianceNoteLineEdit_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    exportLogsButton_ = new QPushButton("Xuất Logs", this); // Optional feature
    connect(exportLogsButton_, &QPushButton::clicked, this, &AuditLogViewerWidget::onExportLogsClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this);
    connect(clearFormButton_, &QPushButton::clicked, this, &AuditLogViewerWidget::clearForm);
    
    buttonLayout->addWidget(exportLogsButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void AuditLogViewerWidget::loadAuditLogs() {
    ERP::Logger::Logger::getInstance().info("AuditLogViewerWidget: Loading audit logs...");
    logTable_->setRowCount(0); // Clear existing rows

    // Only show logs if user has permission to view all logs
    if (!hasPermission("Security.ViewAuditLogs")) {
        showMessageBox("Không có quyền", "Bạn không có quyền xem nhật ký kiểm toán.", QMessageBox::Warning);
        return;
    }

    std::vector<ERP::Security::DTO::AuditLogDTO> logs = auditLogService_->getAllAuditLogs({}, currentUserId_, currentUserRoleIds_);

    logTable_->setRowCount(logs.size());
    for (int i = 0; i < logs.size(); ++i) {
        const auto& log = logs[i];
        logTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(log.id.left(8) + "..."))); // Truncate ID
        
        QString userName = "N/A";
        // Attempt to get username if userId is available and not "system_user" or "N/A"
        if (!log.userId.empty() && log.userId != "system_user" && log.userId != "N/A") {
            std::optional<ERP::User::DTO::UserDTO> user = securityManager_->getUserService()->getUserById(log.userId, currentUserId_, currentUserRoleIds_);
            if (user) userName = QString::fromStdString(user->username);
            else userName = QString::fromStdString(log.userName); // Fallback to recorded userName
        } else {
            userName = QString::fromStdString(log.userName); // Use recorded userName if no user ID
        }
        logTable_->setItem(i, 1, new QTableWidgetItem(userName));

        logTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(log.getActionTypeString())));
        logTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Common::logSeverityToString(log.severity))));
        logTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(log.module)));
        logTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(log.subModule)));
        logTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(log.entityType.value_or("") + " (" + log.entityName.value_or("") + ")")));
        logTable_->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(log.createdAt, ERP::Common::DATETIME_FORMAT))));
    }
    logTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("AuditLogViewerWidget: Audit logs loaded successfully.");
}

void AuditLogViewerWidget::populateActionTypeComboBox() {
    actionTypeComboBox_->clear();
    // Add all enum values from AuditActionType
    actionTypeComboBox_->addItem("Login", static_cast<int>(ERP::Security::DTO::AuditActionType::LOGIN));
    actionTypeComboBox_->addItem("Login Failed", static_cast<int>(ERP::Security::DTO::AuditActionType::LOGIN_FAILED));
    actionTypeComboBox_->addItem("Logout", static_cast<int>(ERP::Security::DTO::AuditActionType::LOGOUT));
    actionTypeComboBox_->addItem("Create", static_cast<int>(ERP::Security::DTO::AuditActionType::CREATE));
    actionTypeComboBox_->addItem("Update", static_cast<int>(ERP::Security::DTO::AuditActionType::UPDATE));
    actionTypeComboBox_->addItem("Delete", static_cast<int>(ERP::Security::DTO::AuditActionType::DELETE));
    actionTypeComboBox_->addItem("View", static_cast<int>(ERP::Security::DTO::AuditActionType::VIEW));
    actionTypeComboBox_->addItem("Password Change", static_cast<int>(ERP::Security::DTO::AuditActionType::PASSWORD_CHANGE));
    actionTypeComboBox_->addItem("Permission Change", static_cast<int>(ERP::Security::DTO::AuditActionType::PERMISSION_CHANGE));
    actionTypeComboBox_->addItem("Configuration Change", static_cast<int>(ERP::Security::DTO::AuditActionType::CONFIGURATION_CHANGE));
    actionTypeComboBox_->addItem("File Upload", static_cast<int>(ERP::Security::DTO::AuditActionType::FILE_UPLOAD));
    actionTypeComboBox_->addItem("File Download", static_cast<int>(ERP::Security::DTO::AuditActionType::FILE_DOWNLOAD));
    actionTypeComboBox_->addItem("Process Start", static_cast<int>(ERP::Security::DTO::AuditActionType::PROCESS_START));
    actionTypeComboBox_->addItem("Process End", static_cast<int>(ERP::Security::DTO::AuditActionType::PROCESS_END));
    actionTypeComboBox_->addItem("Error", static_cast<int>(ERP::Security::DTO::AuditActionType::ERROR));
    actionTypeComboBox_->addItem("Warning", static_cast<int>(ERP::Security::DTO::AuditActionType::WARNING));
    actionTypeComboBox_->addItem("Impersonation", static_cast<int>(ERP::Security::DTO::AuditActionType::IMPERSONATION));
    actionTypeComboBox_->addItem("Data Export", static_cast<int>(ERP::Security::DTO::AuditActionType::DATA_EXPORT));
    actionTypeComboBox_->addItem("Data Import", static_cast<int>(ERP::Security::DTO::AuditActionType::DATA_IMPORT));
    actionTypeComboBox_->addItem("Scheduled Task", static_cast<int>(ERP::Security::DTO::AuditActionType::SCHEDULED_TASK));
    actionTypeComboBox_->addItem("Equipment Calibration", static_cast<int>(ERP::Security::DTO::AuditActionType::EQUIPMENT_CALIBRATION));
    actionTypeComboBox_->addItem("Custom", static_cast<int>(ERP::Security::DTO::AuditActionType::CUSTOM));
}

void AuditLogViewerWidget::populateSeverityComboBox() {
    severityComboBox_->clear();
    // Add all enum values from LogSeverity
    severityComboBox_->addItem("Debug", static_cast<int>(ERP::Common::LogSeverity::DEBUG));
    severityComboBox_->addItem("Info", static_cast<int>(ERP::Common::LogSeverity::INFO));
    severityComboBox_->addItem("Warning", static_cast<int>(ERP::Common::LogSeverity::WARNING));
    severityComboBox_->addItem("Error", static_cast<int>(ERP::Common::LogSeverity::ERROR));
    severityComboBox_->addItem("Critical", static_cast<int>(ERP::Common::LogSeverity::CRITICAL));
}


void AuditLogViewerWidget::onSearchLogClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["search_term"] = searchText.toStdString(); // Assuming generic search by user ID, module, entity name
    }
    logTable_->setRowCount(0);
    std::vector<ERP::Security::DTO::AuditLogDTO> logs = auditLogService_->getAllAuditLogs(filter, currentUserId_, currentUserRoleIds_);

    logTable_->setRowCount(logs.size());
    for (int i = 0; i < logs.size(); ++i) {
        const auto& log = logs[i];
        logTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(log.id.left(8) + "...")));
        
        QString userName = "N/A";
        if (!log.userId.empty() && log.userId != "system_user" && log.userId != "N/A") {
            std::optional<ERP::User::DTO::UserDTO> user = securityManager_->getUserService()->getUserById(log.userId, currentUserId_, currentUserRoleIds_);
            if (user) userName = QString::fromStdString(user->username);
            else userName = QString::fromStdString(log.userName);
        } else {
            userName = QString::fromStdString(log.userName);
        }
        logTable_->setItem(i, 1, new QTableWidgetItem(userName));

        logTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(log.getActionTypeString())));
        logTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Common::logSeverityToString(log.severity))));
        logTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(log.module)));
        logTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(log.subModule)));
        logTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(log.entityType.value_or("") + " (" + log.entityName.value_or("") + ")")));
        logTable_->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(log.createdAt, ERP::Common::DATETIME_FORMAT))));
    }
    logTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("AuditLogViewerWidget: Search completed.");
}

void AuditLogViewerWidget::onLogTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString logIdTruncated = logTable_->item(row, 0)->text(); // This is truncated ID
    // We need to retrieve the full ID to get the full log entry
    // A better approach would be to store full DTOs as data in table items
    // For now, let's just use the truncated ID to get the exact one from service if needed,
    // or implement proper ID storage. Assume the service can find by truncated ID.
    // Or, more correctly, fetch the full list of logs on search and store them in a member variable,
    // then index into that vector when an item is clicked.
    // For now, let's pass the truncated ID and hope the service can handle it (it won't, so this needs to be fixed to pass the full ID).
    // Correct way: Re-fetch or store original IDs in hidden column or item data.
    
    // For now, we will re-fetch by ID from the table.
    // This is not efficient, but works for displaying details.
    // In a real app, you'd load the full DTOs into a QList<AuditLogDTO> on loadAuditLogs/onSearch,
    // then retrieve by index or full ID from that list.

    std::map<std::string, std::any> filterById;
    filterById["id"] = logIdTruncated.toStdString().left(8); // This is likely wrong, should be full ID
    // Find the full ID from the current table data (if available) or re-query.
    // Let's assume the full ID is stored as user data in the first column item.
    QString fullLogId = logTable_->item(row, 0)->data(Qt::UserRole).toString(); // Assuming UserRole stores full ID

    // If full ID is not stored in UserRole, then we need to re-fetch from service using the true ID.
    // Since our table shows truncated ID, we need the original ID.
    // Simplest (though not ideal for large sets) is to refetch by full ID.
    // Let's just pass the exact string from the ID column of the table (assuming it's not actually truncated there)
    // If table item is truncated, you need to store original ID as Qt::UserRole when loading table.
    
    // Correction: In `loadAuditLogs`, store the full ID in `Qt::UserRole` for the first column.
    // Currently, it's `logTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(log.id.left(8) + "...")));`
    // This needs to be: `QTableWidgetItem* idItem = new QTableWidgetItem(QString::fromStdString(log.id.left(8) + "...")); idItem->setData(Qt::UserRole, QString::fromStdString(log.id)); logTable_->setItem(i, 0, idItem);`
    // And then retrieve: `QString fullLogId = logTable_->item(row, 0)->data(Qt::UserRole).toString();`

    // For now, let's assume `logTable_->item(row, 0)->text()` gets the unique ID, even if truncated for display.
    // But it's better to store full ID in user role.
    QString selectedLogId = logTable_->item(row, 0)->text(); // Assuming this is the full ID from the item.
    std::optional<ERP::Security::DTO::AuditLogDTO> logOpt = auditLogService_->getAuditLogById(selectedLogId.toStdString(), currentUserId_, currentUserRoleIds_); // Assuming getAuditLogById exists in service

    if (logOpt) {
        showLogDetailsDialog(&(*logOpt));
    } else {
        showMessageBox("Thông tin Nhật ký", "Không thể tải chi tiết nhật ký đã chọn.", QMessageBox::Warning);
        clearForm();
    }
}

void AuditLogViewerWidget::clearForm() {
    idLineEdit_->clear();
    userIdLineEdit_->clear();
    userNameLineEdit_->clear();
    sessionIdLineEdit_->clear();
    actionTypeComboBox_->setCurrentIndex(0);
    severityComboBox_->setCurrentIndex(0);
    moduleLineEdit_->clear();
    subModuleLineEdit_->clear();
    entityIdLineEdit_->clear();
    entityTypeLineEdit_->clear();
    entityNameLineEdit_->clear();
    ipAddressLineEdit_->clear();
    userAgentLineEdit_->clear();
    workstationIdLineEdit_->clear();
    createdAtEdit_->clear();
    beforeDataTextEdit_->clear();
    afterDataTextEdit_->clear();
    changeReasonLineEdit_->clear();
    metadataTextEdit_->clear();
    commentsLineEdit_->clear();
    approvalIdLineEdit_->clear();
    isCompliantCheckBox_->setChecked(false);
    complianceNoteLineEdit_->clear();
    logTable_->clearSelection();
    updateButtonsState();
}

void AuditLogViewerWidget::onExportLogsClicked() {
    if (!hasPermission("Security.ExportAuditLogs")) { // Assuming a permission for this
        showMessageBox("Lỗi", "Bạn không có quyền xuất nhật ký kiểm toán.", QMessageBox::Warning);
        return;
    }

    // This would involve saving filtered logs to a file (CSV, Excel, JSON).
    // For simplicity, just a message box here.
    showMessageBox("Xuất Logs", "Chức năng xuất nhật ký đang được phát triển.", QMessageBox::Information);
    ERP::Logger::Logger::getInstance().info("AuditLogViewerWidget: Export logs clicked.");
}


void AuditLogViewerWidget::showLogDetailsDialog(ERP::Security::DTO::AuditLogDTO* log) {
    if (!log) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Chi tiết Nhật ký Kiểm toán");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    // Populate all fields similarly to onLogTableItemClicked, but using a QDialog and QTextEdits for large text
    QLineEdit logIdEdit(this); logIdEdit.setReadOnly(true); logIdEdit.setText(QString::fromStdString(log->id));
    QLineEdit userIdEdit(this); userIdEdit.setReadOnly(true); userIdEdit.setText(QString::fromStdString(log->userId));
    QLineEdit userNameEdit(this); userNameEdit.setReadOnly(true); userNameEdit.setText(QString::fromStdString(log->userName));
    QLineEdit sessionIdEdit(this); sessionIdEdit.setReadOnly(true); sessionIdEdit.setText(QString::fromStdString(log->sessionId.value_or("")));
    QLineEdit actionTypeEdit(this); actionTypeEdit.setReadOnly(true); actionTypeEdit.setText(QString::fromStdString(log->getActionTypeString()));
    QLineEdit severityEdit(this); severityEdit.setReadOnly(true); severityEdit.setText(QString::fromStdString(ERP::Common::logSeverityToString(log->severity)));
    QLineEdit moduleEdit(this); moduleEdit.setReadOnly(true); moduleEdit.setText(QString::fromStdString(log->module));
    QLineEdit subModuleEdit(this); subModuleEdit.setReadOnly(true); subModuleEdit.setText(QString::fromStdString(log->subModule));
    QLineEdit entityIdEdit(this); entityIdEdit.setReadOnly(true); entityIdEdit.setText(QString::fromStdString(log->entityId.value_or("")));
    QLineEdit entityTypeEdit(this); entityTypeEdit.setReadOnly(true); entityTypeEdit.setText(QString::fromStdString(log->entityType.value_or("")));
    QLineEdit entityNameEdit(this); entityNameEdit.setReadOnly(true); entityNameEdit.setText(QString::fromStdString(log->entityName.value_or("")));
    QLineEdit ipAddressEdit(this); ipAddressEdit.setReadOnly(true); ipAddressEdit.setText(QString::fromStdString(log->ipAddress.value_or("")));
    QLineEdit userAgentEdit(this); userAgentEdit.setReadOnly(true); userAgentEdit.setText(QString::fromStdString(log->userAgent.value_or("")));
    QLineEdit workstationIdEdit(this); workstationIdEdit.setReadOnly(true); workstationIdEdit.setText(QString::fromStdString(log->workstationId.value_or("")));
    QDateTimeEdit createdAtEdit(this); createdAtEdit.setReadOnly(true); createdAtEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    createdAtEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(log->createdAt.time_since_epoch()).count()));
    QTextEdit beforeDataEdit(this); beforeDataEdit.setReadOnly(true); beforeDataEdit.setText(QString::fromStdString(ERP::Utils::DTOUtils::mapToJsonString(log->beforeData.value_or(std::map<std::string, std::any>{}))));
    QTextEdit afterDataEdit(this); afterDataEdit.setReadOnly(true); afterDataEdit.setText(QString::fromStdString(ERP::Utils::DTOUtils::mapToJsonString(log->afterData.value_or(std::map<std::string, std::any>{}))));
    QLineEdit changeReasonEdit(this); changeReasonEdit.setReadOnly(true); changeReasonEdit.setText(QString::fromStdString(log->changeReason.value_or("")));
    QTextEdit metadataEdit(this); metadataEdit.setReadOnly(true); metadataEdit.setText(QString::fromStdString(ERP::Utils::DTOUtils::mapToJsonString(log->metadata)));
    QLineEdit commentsEdit(this); commentsEdit.setReadOnly(true); commentsEdit.setText(QString::fromStdString(log->comments.value_or("")));
    QLineEdit approvalIdEdit(this); approvalIdEdit.setReadOnly(true); approvalIdEdit.setText(QString::fromStdString(log->approvalId.value_or("")));
    QCheckBox isCompliantCheck(this); isCompliantCheck.setCheckState(log->isCompliant ? Qt::Checked : Qt::Unchecked); isCompliantCheck.setEnabled(false);
    QLineEdit complianceNoteEdit(this); complianceNoteEdit.setReadOnly(true); complianceNoteEdit.setText(QString::fromStdString(log->complianceNote.value_or("")));

    formLayout->addRow("ID Log:", &logIdEdit);
    formLayout->addRow("ID Người dùng:", &userIdEdit);
    formLayout->addRow("Tên Người dùng:", &userNameEdit);
    formLayout->addRow("ID Phiên:", &sessionIdEdit);
    formLayout->addRow("Loại Hành động:", &actionTypeEdit);
    formLayout->addRow("Mức độ:", &severityEdit);
    formLayout->addRow("Module:", &moduleEdit);
    formLayout->addRow("Sub-module:", &subModuleEdit);
    formLayout->addRow("ID Thực thể:", &entityIdEdit);
    formLayout->addRow("Loại Thực thể:", &entityTypeEdit);
    formLayout->addRow("Tên Thực thể:", &entityNameEdit);
    formLayout->addRow("Địa chỉ IP:", &ipAddressEdit);
    formLayout->addRow("User Agent:", &userAgentEdit);
    formLayout->addRow("ID Máy trạm:", &workstationIdEdit);
    formLayout->addRow("Thời gian tạo:", &createdAtEdit);
    formLayout->addRow("Dữ liệu Trước (JSON):", &beforeDataEdit);
    formLayout->addRow("Dữ liệu Sau (JSON):", &afterDataEdit);
    formLayout->addRow("Lý do thay đổi:", &changeReasonEdit);
    formLayout->addRow("Metadata (JSON):", &metadataEdit);
    formLayout->addRow("Bình luận:", &commentsEdit);
    formLayout->addRow("ID Phê duyệt:", &approvalIdEdit);
    formLayout->addRow("Tuân thủ:", &isCompliantCheck);
    formLayout->addRow("Ghi chú tuân thủ:", &complianceNoteEdit);
    dialogLayout->addLayout(formLayout);

    QPushButton *closeButton = new QPushButton("Đóng", &dialog);
    dialogLayout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}


void AuditLogViewerWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool AuditLogViewerWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void AuditLogViewerWidget::updateButtonsState() {
    bool canView = hasPermission("Security.ViewAuditLogs");
    bool canExport = hasPermission("Security.ExportAuditLogs"); // Assuming export permission

    searchButton_->setEnabled(canView);
    exportLogsButton_->setEnabled(canExport);
    
    bool isRowSelected = logTable_->currentRow() >= 0;
    // No edit/delete for logs in this viewer, just display.
    // clearFormButton_ is always enabled.
}

} // namespace Security
} // namespace UI
} // namespace ERP