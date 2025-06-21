// Modules/UI/Scheduler/TaskExecutionLogManagementWidget.cpp
#include "TaskExecutionLogManagementWidget.h" // Standard includes
#include "TaskExecutionLog.h"                 // TaskExecutionLog DTO
#include "ScheduledTask.h"                    // ScheduledTask DTO
#include "User.h"                             // User DTO
#include "TaskExecutionLogService.h"          // TaskExecutionLog Service
#include "ScheduledTaskService.h"             // ScheduledTask Service
#include "ISecurityManager.h"                 // Security Manager
#include "Logger.h"                           // Logging
#include "ErrorHandler.h"                     // Error Handling
#include "Common.h"                           // Common Enums/Constants
#include "DateUtils.h"                        // Date Utilities
#include "StringUtils.h"                      // String Utilities
#include "CustomMessageBox.h"                 // Custom Message Box
#include "UserService.h"                      // For getting user names
#include "DTOUtils.h"                         // For JSON conversions

#include <QInputDialog>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace ERP {
namespace UI {
namespace Scheduler {

TaskExecutionLogManagementWidget::TaskExecutionLogManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::ITaskExecutionLogService> taskExecutionLogService,
    std::shared_ptr<Services::IScheduledTaskService> scheduledTaskService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      taskExecutionLogService_(taskExecutionLogService),
      scheduledTaskService_(scheduledTaskService),
      securityManager_(securityManager) {

    if (!taskExecutionLogService_ || !scheduledTaskService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ nhật ký thực thi tác vụ, tác vụ được lên lịch hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("TaskExecutionLogManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("TaskExecutionLogManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("TaskExecutionLogManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadExecutionLogs();
    updateButtonsState();
}

TaskExecutionLogManagementWidget::~TaskExecutionLogManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void TaskExecutionLogManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo ID tác vụ hoặc lỗi...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &TaskExecutionLogManagementWidget::onSearchLogClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    logTable_ = new QTableWidget(this);
    logTable_->setColumnCount(7); // ID, Tên tác vụ, Thời gian bắt đầu, Thời gian kết thúc, Trạng thái, Thực hiện bởi, Lỗi
    logTable_->setHorizontalHeaderLabels({"ID Nhật ký", "Tên Tác vụ", "Thời gian bắt đầu", "Thời gian kết thúc", "Trạng thái", "Thực hiện bởi", "Thông báo lỗi"});
    logTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    logTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    logTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    logTable_->horizontalHeader()->setStretchLastSection(true);
    connect(logTable_, &QTableWidget::itemClicked, this, &TaskExecutionLogManagementWidget::onLogTableItemClicked);
    mainLayout->addWidget(logTable_);

    // Form elements for displaying log details (read-only)
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    scheduledTaskIdLineEdit_ = new QLineEdit(this); scheduledTaskIdLineEdit_->setReadOnly(true);
    scheduledTaskNameLineEdit_ = new QLineEdit(this); scheduledTaskNameLineEdit_->setReadOnly(true);
    startTimeEdit_ = new QDateTimeEdit(this); startTimeEdit_->setReadOnly(true); startTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    endTimeEdit_ = new QDateTimeEdit(this); endTimeEdit_->setReadOnly(true); endTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    statusComboBox_ = new QComboBox(this); populateStatusComboBox(); statusComboBox_->setEnabled(false); // Read-only
    executedByLineEdit_ = new QLineEdit(this); executedByLineEdit_->setReadOnly(true);
    logOutputLineEdit_ = new QLineEdit(this); logOutputLineEdit_->setReadOnly(true);
    errorMessageLineEdit_ = new QLineEdit(this); errorMessageLineEdit_->setReadOnly(true);
    parametersJsonEdit_ = new QLineEdit(this); parametersJsonEdit_->setReadOnly(true); // For execution context parameters

    formLayout->addWidget(new QLabel("ID Nhật ký:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("ID Tác vụ lên lịch:", &scheduledTaskIdLineEdit_);
    formLayout->addRow("Tên Tác vụ lên lịch:", &scheduledTaskNameLineEdit_);
    formLayout->addRow("Thời gian bắt đầu:", &startTimeEdit_);
    formLayout->addRow("Thời gian kết thúc:", &endTimeEdit_);
    formLayout->addRow("Trạng thái:", &statusComboBox_);
    formLayout->addRow("Thực hiện bởi:", &executedByLineEdit_);
    formLayout->addRow("Đầu ra Log:", &logOutputLineEdit_);
    formLayout->addRow("Thông báo lỗi:", &errorMessageLineEdit_);
    formLayout->addRow("Tham số Context (JSON):", &parametersJsonEdit_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    deleteLogButton_ = new QPushButton("Xóa Nhật ký", this);
    connect(deleteLogButton_, &QPushButton::clicked, this, &TaskExecutionLogManagementWidget::onDeleteLogClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this);
    connect(clearFormButton_, &QPushButton::clicked, this, &TaskExecutionLogManagementWidget::clearForm);
    
    buttonLayout->addWidget(deleteLogButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void TaskExecutionLogManagementWidget::loadExecutionLogs() {
    ERP::Logger::Logger::getInstance().info("TaskExecutionLogManagementWidget: Loading execution logs...");
    logTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Scheduler::DTO::TaskExecutionLogDTO> logs = taskExecutionLogService_->getAllTaskExecutionLogs({}, currentUserId_, currentUserRoleIds_);

    logTable_->setRowCount(logs.size());
    for (int i = 0; i < logs.size(); ++i) {
        const auto& log = logs[i];
        logTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(log.id)));
        
        QString taskName = "N/A";
        std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> task = scheduledTaskService_->getScheduledTaskById(log.scheduledTaskId, currentUserId_, currentUserRoleIds_);
        if (task) taskName = QString::fromStdString(task->taskName);
        logTable_->setItem(i, 1, new QTableWidgetItem(taskName));

        logTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(log.startTime, ERP::Common::DATETIME_FORMAT))));
        logTable_->setItem(i, 3, new QTableWidgetItem(log.endTime ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*log.endTime, ERP::Common::DATETIME_FORMAT)) : "N/A"));
        logTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(log.getStatusString())));
        
        QString executedByName = "N/A";
        if (log.executedByUserId) {
            std::optional<ERP::User::DTO::UserDTO> user = securityManager_->getUserService()->getUserById(*log.executedByUserId, currentUserId_, currentUserRoleIds_);
            if (user) executedByName = QString::fromStdString(user->username);
        }
        logTable_->setItem(i, 5, new QTableWidgetItem(executedByName));

        logTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(log.errorMessage.value_or(""))));
    }
    logTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("TaskExecutionLogManagementWidget: Execution logs loaded successfully.");
}

void TaskExecutionLogManagementWidget::populateScheduledTaskComboBox(QComboBox* comboBox) {
    comboBox->clear();
    std::vector<ERP::Scheduler::DTO::ScheduledTaskDTO> allTasks = scheduledTaskService_->getAllScheduledTasks({}, currentUserId_, currentUserRoleIds_);
    for (const auto& task : allTasks) {
        comboBox->addItem(QString::fromStdString(task.taskName), QString::fromStdString(task.id));
    }
}

void TaskExecutionLogManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Success", static_cast<int>(ERP::Scheduler::DTO::TaskExecutionStatus::SUCCESS));
    statusComboBox_->addItem("Failed", static_cast<int>(ERP::Scheduler::DTO::TaskExecutionStatus::FAILED));
    statusComboBox_->addItem("Running", static_cast<int>(ERP::Scheduler::DTO::TaskExecutionStatus::RUNNING));
    statusComboBox_->addItem("Skipped", static_cast<int>(ERP::Scheduler::DTO::TaskExecutionStatus::SKIPPED));
}


void TaskExecutionLogManagementWidget::onDeleteLogClicked() {
    if (!hasPermission("Scheduler.DeleteTaskExecutionLog")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa nhật ký thực thi tác vụ.", QMessageBox::Warning);
        return;
    }

    int selectedRow = logTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Nhật ký", "Vui lòng chọn một nhật ký để xóa.", QMessageBox::Information);
        return;
    }

    QString logId = logTable_->item(selectedRow, 0)->text();
    QString taskName = logTable_->item(selectedRow, 1)->text(); // Display task name

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Nhật ký Thực thi");
    confirmBox.setText("Bạn có chắc chắn muốn xóa nhật ký thực thi cho tác vụ '" + taskName + "' (ID: " + logId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (taskExecutionLogService_->deleteTaskExecutionLog(logId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Nhật ký", "Nhật ký thực thi đã được xóa thành công.", QMessageBox::Information);
            loadExecutionLogs();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa nhật ký thực thi. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}


void TaskExecutionLogManagementWidget::onSearchLogClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["search_term"] = searchText.toStdString(); // Assuming service supports generic search
    }
    logTable_->setRowCount(0);
    std::vector<ERP::Scheduler::DTO::TaskExecutionLogDTO> logs = taskExecutionLogService_->getAllTaskExecutionLogs(filter, currentUserId_, currentUserRoleIds_);

    logTable_->setRowCount(logs.size());
    for (int i = 0; i < logs.size(); ++i) {
        const auto& log = logs[i];
        logTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(log.id)));
        
        QString taskName = "N/A";
        std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> task = scheduledTaskService_->getScheduledTaskById(log.scheduledTaskId, currentUserId_, currentUserRoleIds_);
        if (task) taskName = QString::fromStdString(task->taskName);
        logTable_->setItem(i, 1, new QTableWidgetItem(taskName));

        logTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(log.startTime, ERP::Common::DATETIME_FORMAT))));
        logTable_->setItem(i, 3, new QTableWidgetItem(log.endTime ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*log.endTime, ERP::Common::DATETIME_FORMAT)) : "N/A"));
        logTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(log.getStatusString())));
        
        QString executedByName = "N/A";
        if (log.executedByUserId) {
            std::optional<ERP::User::DTO::UserDTO> user = securityManager_->getUserService()->getUserById(*log.executedByUserId, currentUserId_, currentUserRoleIds_);
            if (user) executedByName = QString::fromStdString(user->username);
        }
        logTable_->setItem(i, 5, new QTableWidgetItem(executedByName));

        logTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(log.errorMessage.value_or(""))));
    }
    logTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("TaskExecutionLogManagementWidget: Search completed.");
}

void TaskExecutionLogManagementWidget::onLogTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString logId = logTable_->item(row, 0)->text();
    std::optional<ERP::Scheduler::DTO::TaskExecutionLogDTO> logOpt = taskExecutionLogService_->getTaskExecutionLogById(logId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (logOpt) {
        showLogDetailsDialog(&(*logOpt));
    } else {
        showMessageBox("Thông tin Nhật ký", "Không thể tải chi tiết nhật ký đã chọn.", QMessageBox::Warning);
        clearForm();
    }
}

void TaskExecutionLogManagementWidget::clearForm() {
    idLineEdit_->clear();
    scheduledTaskIdLineEdit_->clear();
    scheduledTaskNameLineEdit_->clear();
    startTimeEdit_->clear();
    endTimeEdit_->clear();
    statusComboBox_->setCurrentIndex(0);
    executedByLineEdit_->clear();
    logOutputLineEdit_->clear();
    errorMessageLineEdit_->clear();
    parametersJsonEdit_->clear();
    logTable_->clearSelection();
    updateButtonsState();
}

void TaskExecutionLogManagementWidget::showLogDetailsDialog(ERP::Scheduler::DTO::TaskExecutionLogDTO* log) {
    if (!log) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Chi tiết Nhật ký Thực thi Tác vụ");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit idEdit(this); idEdit.setReadOnly(true);
    QLineEdit scheduledTaskIdEdit(this); scheduledTaskIdEdit.setReadOnly(true);
    QLineEdit scheduledTaskNameEdit(this); scheduledTaskNameEdit.setReadOnly(true);
    QDateTimeEdit startTimeEdit(this); startTimeEdit.setReadOnly(true); startTimeEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QDateTimeEdit endTimeEdit(this); endTimeEdit.setReadOnly(true); endTimeEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QLineEdit statusEdit(this); statusEdit.setReadOnly(true);
    QLineEdit executedByEdit(this); executedByEdit.setReadOnly(true);
    QTextEdit logOutputEdit(this); logOutputEdit.setReadOnly(true);
    QTextEdit errorMessageEdit(this); errorMessageEdit.setReadOnly(true);
    QTextEdit executionContextEdit(this); executionContextEdit.setReadOnly(true);

    idEdit.setText(QString::fromStdString(log->id));
    scheduledTaskIdEdit.setText(QString::fromStdString(log->scheduledTaskId));
    
    QString taskName = "N/A";
    std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> task = scheduledTaskService_->getScheduledTaskById(log->scheduledTaskId, currentUserId_, currentUserRoleIds_);
    if (task) taskName = QString::fromStdString(task->taskName);
    scheduledTaskNameEdit.setText(taskName);

    startTimeEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(log->startTime.time_since_epoch()).count()));
    if (log->endTime) endTimeEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(log->endTime->time_since_epoch()).count()));
    statusEdit.setText(QString::fromStdString(log->getStatusString()));
    
    QString executedByName = "N/A";
    if (log->executedByUserId) {
        std::optional<ERP::User::DTO::UserDTO> user = securityManager_->getUserService()->getUserById(*log->executedByUserId, currentUserId_, currentUserRoleIds_);
        if (user) executedByName = QString::fromStdString(user->username);
    }
    executedByEdit.setText(executedByName);
    logOutputEdit.setText(QString::fromStdString(log->logOutput.value_or("")));
    errorMessageEdit.setText(QString::fromStdString(log->errorMessage.value_or("")));
    executionContextEdit.setText(QString::fromStdString(ERP::Utils::DTOUtils::mapToJsonString(log->executionContext))); // Convert map to JSON string for display

    formLayout->addRow("ID Nhật ký:", &idEdit);
    formLayout->addRow("ID Tác vụ lên lịch:", &scheduledTaskIdEdit);
    formLayout->addRow("Tên Tác vụ lên lịch:", &scheduledTaskNameEdit);
    formLayout->addRow("Thời gian bắt đầu:", &startTimeEdit);
    formLayout->addRow("Thời gian kết thúc:", &endTimeEdit);
    formLayout->addRow("Trạng thái:", &statusEdit);
    formLayout->addRow("Thực hiện bởi:", &executedByEdit);
    formLayout->addRow("Đầu ra Log:", &logOutputEdit);
    formLayout->addRow("Thông báo lỗi:", &errorMessageEdit);
    formLayout->addRow("Tham số Context (JSON):", &executionContextEdit);
    dialogLayout->addLayout(formLayout);

    QPushButton *closeButton = new QPushButton("Đóng", &dialog);
    dialogLayout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}


void TaskExecutionLogManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool TaskExecutionLogManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void TaskExecutionLogManagementWidget::updateButtonsState() {
    bool canDelete = hasPermission("Scheduler.DeleteTaskExecutionLog");
    bool canView = hasPermission("Scheduler.ViewTaskExecutionLogs");

    searchButton_->setEnabled(canView);

    bool isRowSelected = logTable_->currentRow() >= 0;
    deleteLogButton_->setEnabled(isRowSelected && canDelete);
    
    // All form fields are read-only, no need to manage their enabled state based on selection.
    // clearFormButton_ is always enabled to clear the display.
}

} // namespace Scheduler
} // namespace UI
} // namespace ERP