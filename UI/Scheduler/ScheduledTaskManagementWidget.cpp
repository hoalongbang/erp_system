// UI/Scheduler/ScheduledTaskManagementWidget.cpp
#include "ScheduledTaskManagementWidget.h" // Standard includes
#include "ScheduledTask.h"                 // ScheduledTask DTO
#include "User.h"                          // User DTO (for display)
#include "ScheduledTaskService.h"          // ScheduledTask Service
#include "ISecurityManager.h"              // Security Manager
#include "Logger.h"                        // Logging
#include "ErrorHandler.h"                  // Error Handling
#include "Common.h"                        // Common Enums/Constants
#include "DateUtils.h"                     // Date Utilities
#include "StringUtils.h"                   // String Utilities
#include "CustomMessageBox.h"              // Custom Message Box
#include "UserService.h"                   // For getting user names
#include "DTOUtils.h"                      // For JSON conversions

#include <QInputDialog>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QJsonObject>

namespace ERP {
namespace UI {
namespace Scheduler {

ScheduledTaskManagementWidget::ScheduledTaskManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IScheduledTaskService> scheduledTaskService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      scheduledTaskService_(scheduledTaskService),
      securityManager_(securityManager) {

    if (!scheduledTaskService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ tác vụ được lên lịch hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("ScheduledTaskManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("ScheduledTaskManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("ScheduledTaskManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadScheduledTasks();
    updateButtonsState();
}

ScheduledTaskManagementWidget::~ScheduledTaskManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void ScheduledTaskManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên tác vụ...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &ScheduledTaskManagementWidget::onSearchTaskClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    taskTable_ = new QTableWidget(this);
    taskTable_->setColumnCount(7); // ID, Tên tác vụ, Loại, Tần suất, Thời gian chạy tiếp theo, Trạng thái, Người giao
    taskTable_->setHorizontalHeaderLabels({"ID", "Tên Tác vụ", "Loại", "Tần suất", "Chạy tiếp theo", "Trạng thái", "Người giao"});
    taskTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    taskTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    taskTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    taskTable_->horizontalHeader()->setStretchLastSection(true);
    connect(taskTable_, &QTableWidget::itemClicked, this, &ScheduledTaskManagementWidget::onTaskTableItemClicked);
    mainLayout->addWidget(taskTable_);

    // Form elements for editing/adding tasks
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    taskNameLineEdit_ = new QLineEdit(this);
    taskTypeLineEdit_ = new QLineEdit(this);
    frequencyComboBox_ = new QComboBox(this); populateFrequencyComboBox();
    cronExpressionLineEdit_ = new QLineEdit(this);
    nextRunTimeEdit_ = new QDateTimeEdit(this); nextRunTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    lastRunTimeEdit_ = new QDateTimeEdit(this); lastRunTimeEdit_->setReadOnly(true); lastRunTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();
    assignedToComboBox_ = new QComboBox(this); populateUserComboBox(assignedToComboBox_);
    lastErrorMessageLineEdit_ = new QLineEdit(this); lastErrorMessageLineEdit_->setReadOnly(true);
    parametersJsonEdit_ = new QLineEdit(this); // For parameters map as JSON string
    startDateEdit_ = new QDateTimeEdit(this); startDateEdit_->setDisplayFormat("yyyy-MM-dd");
    endDateEdit_ = new QDateTimeEdit(this); endDateEdit_->setDisplayFormat("yyyy-MM-dd");


    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("Tên Tác vụ:*", &taskNameLineEdit_);
    formLayout->addRow("Loại Tác vụ:*", &taskTypeLineEdit_);
    formLayout->addRow("Tần suất:*", &frequencyComboBox_);
    formLayout->addRow("Biểu thức Cron (nếu tùy chỉnh):", &cronExpressionLineEdit_);
    formLayout->addRow("Chạy tiếp theo:*", &nextRunTimeEdit_);
    formLayout->addRow("Chạy cuối cùng:", &lastRunTimeEdit_);
    formLayout->addRow("Trạng thái:*", &statusComboBox_);
    formLayout->addRow("Người giao:", &assignedToComboBox_);
    formLayout->addRow("Lỗi cuối cùng:", &lastErrorMessageLineEdit_);
    formLayout->addRow("Tham số (JSON):", &parametersJsonEdit_);
    formLayout->addRow("Ngày bắt đầu hiệu lực:", &startDateEdit_);
    formLayout->addRow("Ngày kết thúc hiệu lực:", &endDateEdit_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addTaskButton_ = new QPushButton("Thêm mới", this); connect(addTaskButton_, &QPushButton::clicked, this, &ScheduledTaskManagementWidget::onAddTaskClicked);
    editTaskButton_ = new QPushButton("Sửa", this); connect(editTaskButton_, &QPushButton::clicked, this, &ScheduledTaskManagementWidget::onEditTaskClicked);
    deleteTaskButton_ = new QPushButton("Xóa", this); connect(deleteTaskButton_, &QPushButton::clicked, this, &ScheduledTaskManagementWidget::onDeleteTaskClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &ScheduledTaskManagementWidget::onUpdateTaskStatusClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &ScheduledTaskManagementWidget::onSearchTaskClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &ScheduledTaskManagementWidget::clearForm);
    
    buttonLayout->addWidget(addTaskButton_);
    buttonLayout->addWidget(editTaskButton_);
    buttonLayout->addWidget(deleteTaskButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void ScheduledTaskManagementWidget::loadScheduledTasks() {
    ERP::Logger::Logger::getInstance().info("ScheduledTaskManagementWidget: Loading scheduled tasks...");
    taskTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Scheduler::DTO::ScheduledTaskDTO> tasks = scheduledTaskService_->getAllScheduledTasks({}, currentUserId_, currentUserRoleIds_);

    taskTable_->setRowCount(tasks.size());
    for (int i = 0; i < tasks.size(); ++i) {
        const auto& task = tasks[i];
        taskTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(task.id)));
        taskTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(task.taskName)));
        taskTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(task.taskType)));
        taskTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(task.getFrequencyString())));
        taskTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(task.nextRunTime, ERP::Common::DATETIME_FORMAT))));
        taskTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(task.getStatusString())));
        
        QString assignedToName = "N/A";
        if (task.assignedToUserId) {
            std::optional<ERP::User::DTO::UserDTO> user = securityManager_->getUserService()->getUserById(*task.assignedToUserId, currentUserId_, currentUserRoleIds_);
            if (user) assignedToName = QString::fromStdString(user->username);
        }
        taskTable_->setItem(i, 6, new QTableWidgetItem(assignedToName));
    }
    taskTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ScheduledTaskManagementWidget: Scheduled tasks loaded successfully.");
}

void ScheduledTaskManagementWidget::populateFrequencyComboBox() {
    frequencyComboBox_->clear();
    frequencyComboBox_->addItem("Once", static_cast<int>(ERP::Scheduler::DTO::ScheduleFrequency::ONCE));
    frequencyComboBox_->addItem("Hourly", static_cast<int>(ERP::Scheduler::DTO::ScheduleFrequency::HOURLY));
    frequencyComboBox_->addItem("Daily", static_cast<int>(ERP::Scheduler::DTO::ScheduleFrequency::DAILY));
    frequencyComboBox_->addItem("Weekly", static_cast<int>(ERP::Scheduler::DTO::ScheduleFrequency::WEEKLY));
    frequencyComboBox_->addItem("Monthly", static_cast<int>(ERP::Scheduler::DTO::ScheduleFrequency::MONTHLY));
    frequencyComboBox_->addItem("Yearly", static_cast<int>(ERP::Scheduler::DTO::ScheduleFrequency::YEARLY));
    frequencyComboBox_->addItem("Custom (Cron)", static_cast<int>(ERP::Scheduler::DTO::ScheduleFrequency::CUSTOM_CRON));
}

void ScheduledTaskManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Active", static_cast<int>(ERP::Scheduler::DTO::ScheduledTaskStatus::ACTIVE));
    statusComboBox_->addItem("Inactive", static_cast<int>(ERP::Scheduler::DTO::ScheduledTaskStatus::INACTIVE));
    statusComboBox_->addItem("Suspended", static_cast<int>(ERP::Scheduler::DTO::ScheduledTaskStatus::SUSPENDED));
    statusComboBox_->addItem("Completed", static_cast<int>(ERP::Scheduler::DTO::ScheduledTaskStatus::COMPLETED));
    statusComboBox_->addItem("Failed", static_cast<int>(ERP::Scheduler::DTO::ScheduledTaskStatus::FAILED));
}

void ScheduledTaskManagementWidget::populateUserComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("None", "");
    std::vector<ERP::User::DTO::UserDTO> allUsers = securityManager_->getUserService()->getAllUsers({}, currentUserId_, currentUserRoleIds_);
    for (const auto& user : allUsers) {
        comboBox->addItem(QString::fromStdString(user.username), QString::fromStdString(user.id));
    }
}


void ScheduledTaskManagementWidget::onAddTaskClicked() {
    if (!hasPermission("Scheduler.CreateScheduledTask")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm tác vụ được lên lịch.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateUserComboBox(assignedToComboBox_);
    populateFrequencyComboBox();
    populateStatusComboBox();
    showTaskInputDialog();
}

void ScheduledTaskManagementWidget::onEditTaskClicked() {
    if (!hasPermission("Scheduler.UpdateScheduledTask")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa tác vụ được lên lịch.", QMessageBox::Warning);
        return;
    }

    int selectedRow = taskTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Tác vụ", "Vui lòng chọn một tác vụ được lên lịch để sửa.", QMessageBox::Information);
        return;
    }

    QString taskId = taskTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> taskOpt = scheduledTaskService_->getScheduledTaskById(taskId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (taskOpt) {
        populateUserComboBox(assignedToComboBox_);
        populateFrequencyComboBox();
        populateStatusComboBox();
        showTaskInputDialog(&(*taskOpt));
    } else {
        showMessageBox("Sửa Tác vụ", "Không tìm thấy tác vụ được lên lịch để sửa.", QMessageBox::Critical);
    }
}

void ScheduledTaskManagementWidget::onDeleteTaskClicked() {
    if (!hasPermission("Scheduler.DeleteScheduledTask")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa tác vụ được lên lịch.", QMessageBox::Warning);
        return;
    }

    int selectedRow = taskTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Tác vụ", "Vui lòng chọn một tác vụ được lên lịch để xóa.", QMessageBox::Information);
        return;
    }

    QString taskId = taskTable_->item(selectedRow, 0)->text();
    QString taskName = taskTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Tác vụ");
    confirmBox.setText("Bạn có chắc chắn muốn xóa tác vụ được lên lịch '" + taskName + "' (ID: " + taskId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (scheduledTaskService_->deleteScheduledTask(taskId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Tác vụ", "Tác vụ đã được xóa thành công.", QMessageBox::Information);
            loadScheduledTasks();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa tác vụ. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void ScheduledTaskManagementWidget::onUpdateTaskStatusClicked() {
    if (!hasPermission("Scheduler.UpdateScheduledTaskStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái tác vụ được lên lịch.", QMessageBox::Warning);
        return;
    }

    int selectedRow = taskTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một tác vụ được lên lịch để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString taskId = taskTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> taskOpt = scheduledTaskService_->getScheduledTaskById(taskId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!taskOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy tác vụ được lên lịch để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Scheduler::DTO::ScheduledTaskDTO currentTask = *taskOpt;
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
    int currentStatusIndex = newStatusCombo.findData(static_cast<int>(currentTask.status));
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
        ERP::Scheduler::DTO::ScheduledTaskStatus newStatus = static_cast<ERP::Scheduler::DTO::ScheduledTaskStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái tác vụ");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái tác vụ '" + QString::fromStdString(currentTask.taskName) + "' thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (scheduledTaskService_->updateScheduledTaskStatus(taskId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái tác vụ đã được cập nhật thành công.", QMessageBox::Information);
                loadScheduledTasks();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái tác vụ. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}


void ScheduledTaskManagementWidget::onSearchTaskClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["name_or_type_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    taskTable_->setRowCount(0);
    std::vector<ERP::Scheduler::DTO::ScheduledTaskDTO> tasks = scheduledTaskService_->getAllScheduledTasks(filter, currentUserId_, currentUserRoleIds_);

    taskTable_->setRowCount(tasks.size());
    for (int i = 0; i < tasks.size(); ++i) {
        const auto& task = tasks[i];
        taskTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(task.id)));
        taskTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(task.taskName)));
        taskTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(task.taskType)));
        taskTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(task.getFrequencyString())));
        taskTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(task.nextRunTime, ERP::Common::DATETIME_FORMAT))));
        taskTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(task.getStatusString())));
        
        QString assignedToName = "N/A";
        if (task.assignedToUserId) {
            std::optional<ERP::User::DTO::UserDTO> user = securityManager_->getUserService()->getUserById(*task.assignedToUserId, currentUserId_, currentUserRoleIds_);
            if (user) assignedToName = QString::fromStdString(user->username);
        }
        taskTable_->setItem(i, 6, new QTableWidgetItem(assignedToName));
    }
    taskTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ScheduledTaskManagementWidget: Search completed.");
}

void ScheduledTaskManagementWidget::onTaskTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString taskId = taskTable_->item(row, 0)->text();
    std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> taskOpt = scheduledTaskService_->getScheduledTaskById(taskId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (taskOpt) {
        idLineEdit_->setText(QString::fromStdString(taskOpt->id));
        taskNameLineEdit_->setText(QString::fromStdString(taskOpt->taskName));
        taskTypeLineEdit_->setText(QString::fromStdString(taskOpt->taskType));
        
        populateFrequencyComboBox();
        int freqIndex = frequencyComboBox_->findData(static_cast<int>(taskOpt->frequency));
        if (freqIndex != -1) frequencyComboBox_->setCurrentIndex(freqIndex);

        cronExpressionLineEdit_->setText(QString::fromStdString(taskOpt->cronExpression.value_or("")));
        nextRunTimeEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(taskOpt->nextRunTime.time_since_epoch()).count()));
        if (taskOpt->lastRunTime) lastRunTimeEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(taskOpt->lastRunTime->time_since_epoch()).count()));
        else lastRunTimeEdit_->clear();
        
        populateStatusComboBox();
        int statusIndex = statusComboBox_->findData(static_cast<int>(taskOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);

        populateUserComboBox(assignedToComboBox_);
        if (taskOpt->assignedToUserId) {
            int userIndex = assignedToComboBox_->findData(QString::fromStdString(*taskOpt->assignedToUserId));
            if (userIndex != -1) assignedToComboBox_->setCurrentIndex(userIndex);
            else assignedToComboBox_->setCurrentIndex(0); // "None"
        } else {
            assignedToComboBox_->setCurrentIndex(0); // "None"
        }

        lastErrorMessageLineEdit_->setText(QString::fromStdString(taskOpt->lastErrorMessage.value_or("")));
        parametersJsonEdit_->setText(QString::fromStdString(ERP::Utils::DTOUtils::mapToJsonString(taskOpt->parameters)));
        if (taskOpt->startDate) startDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(taskOpt->startDate->time_since_epoch()).count()));
        else startDateEdit_->clear();
        if (taskOpt->endDate) endDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(taskOpt->endDate->time_since_epoch()).count()));
        else endDateEdit_->clear();

    } else {
        showMessageBox("Thông tin Tác vụ", "Không tìm thấy tác vụ được lên lịch đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void ScheduledTaskManagementWidget::clearForm() {
    idLineEdit_->clear();
    taskNameLineEdit_->clear();
    taskTypeLineEdit_->clear();
    frequencyComboBox_->setCurrentIndex(0);
    cronExpressionLineEdit_->clear();
    nextRunTimeEdit_->clear();
    lastRunTimeEdit_->clear();
    statusComboBox_->setCurrentIndex(0);
    assignedToComboBox_->clear();
    lastErrorMessageLineEdit_->clear();
    parametersJsonEdit_->clear();
    startDateEdit_->clear();
    endDateEdit_->clear();
    taskTable_->clearSelection();
    updateButtonsState();
}


void ScheduledTaskManagementWidget::showTaskInputDialog(ERP::Scheduler::DTO::ScheduledTaskDTO* task) {
    QDialog dialog(this);
    dialog.setWindowTitle(task ? "Sửa Tác vụ" : "Thêm Tác vụ Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit taskNameEdit(this);
    QLineEdit taskTypeEdit(this);
    QComboBox frequencyCombo(this); populateFrequencyComboBox();
    for(int i = 0; i < frequencyComboBox_->count(); ++i) frequencyCombo.addItem(frequencyComboBox_->itemText(i), frequencyComboBox_->itemData(i));
    QLineEdit cronExpressionEdit(this);
    QDateTimeEdit nextRunTimeEdit(this); nextRunTimeEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QComboBox statusCombo(this); populateStatusComboBox();
    for(int i = 0; i < statusComboBox_->count(); ++i) statusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));
    QComboBox assignedToCombo(this); populateUserComboBox(assignedToCombo); // Re-populate for dialog
    QLineEdit parametersJsonEdit(this);
    QDateTimeEdit startDateEdit(this); startDateEdit.setDisplayFormat("yyyy-MM-dd"); startDateEdit.setCalendarPopup(true);
    QDateTimeEdit endDateEdit(this); endDateEdit.setDisplayFormat("yyyy-MM-dd"); endDateEdit.setCalendarPopup(true);

    if (task) {
        taskNameEdit.setText(QString::fromStdString(task->taskName));
        taskTypeEdit.setText(QString::fromStdString(task->taskType));
        int freqIndex = frequencyCombo.findData(static_cast<int>(task->frequency));
        if (freqIndex != -1) frequencyCombo.setCurrentIndex(freqIndex);
        cronExpressionEdit.setText(QString::fromStdString(task->cronExpression.value_or("")));
        nextRunTimeEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(task->nextRunTime.time_since_epoch()).count()));
        int statusIndex = statusCombo.findData(static_cast<int>(task->status));
        if (statusIndex != -1) statusCombo.setCurrentIndex(statusIndex);
        if (task->assignedToUserId) {
            int userIndex = assignedToCombo.findData(QString::fromStdString(*task->assignedToUserId));
            if (userIndex != -1) assignedToCombo.setCurrentIndex(userIndex);
            else assignedToCombo.setCurrentIndex(0);
        } else {
            assignedToCombo.setCurrentIndex(0);
        }
        parametersJsonEdit.setText(QString::fromStdString(ERP::Utils::DTOUtils::mapToJsonString(task->parameters)));
        if (task->startDate) startDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(task->startDate->time_since_epoch()).count()));
        else startDateEdit.clear();
        if (task->endDate) endDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(task->endDate->time_since_epoch()).count()));
        else endDateEdit.clear();
        
        taskNameEdit.setReadOnly(true); // Don't allow changing name for existing
    } else {
        nextRunTimeEdit.setDateTime(QDateTime::currentDateTime());
        startDateEdit.setDateTime(QDateTime::currentDateTime().date());
    }

    formLayout->addRow("Tên Tác vụ:*", &taskNameEdit);
    formLayout->addRow("Loại Tác vụ:*", &taskTypeEdit);
    formLayout->addRow("Tần suất:*", &frequencyCombo);
    formLayout->addRow("Biểu thức Cron (nếu tùy chỉnh):", &cronExpressionEdit);
    formLayout->addRow("Chạy tiếp theo:*", &nextRunTimeEdit);
    formLayout->addRow("Trạng thái:*", &statusCombo);
    formLayout->addRow("Người giao:", &assignedToCombo);
    formLayout->addRow("Tham số (JSON):", &parametersJsonEdit);
    formLayout->addRow("Ngày bắt đầu hiệu lực:", &startDateEdit);
    formLayout->addRow("Ngày kết thúc hiệu lực:", &endDateEdit);
    
    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(task ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Scheduler::DTO::ScheduledTaskDTO newTaskData;
        if (task) {
            newTaskData = *task;
        } else {
            newTaskData.id = ERP::Utils::generateUUID(); // New ID for new task
        }

        newTaskData.taskName = taskNameEdit.text().toStdString();
        newTaskData.taskType = taskTypeEdit.text().toStdString();
        newTaskData.frequency = static_cast<ERP::Scheduler::DTO::ScheduleFrequency>(frequencyCombo.currentData().toInt());
        newTaskData.cronExpression = cronExpressionEdit.text().isEmpty() ? std::nullopt : std::make_optional(cronExpressionEdit.text().toStdString());
        newTaskData.nextRunTime = ERP::Utils::DateUtils::qDateTimeToTimePoint(nextRunTimeEdit.dateTime());
        newTaskData.status = static_cast<ERP::Scheduler::DTO::ScheduledTaskStatus>(statusCombo.currentData().toInt());
        
        QString selectedAssignedToId = assignedToCombo.currentData().toString();
        newTaskData.assignedToUserId = selectedAssignedToId.isEmpty() ? std::nullopt : std::make_optional(selectedAssignedToId.toStdString());

        newTaskData.parameters = ERP::Utils::DTOUtils::jsonStringToMap(parametersJsonEdit.text().toStdString());
        newTaskData.startDate = startDateEdit.dateTime().isNull() ? std::nullopt : std::make_optional(ERP::Utils::DateUtils::qDateTimeToTimePoint(startDateEdit.dateTime()));
        newTaskData.endDate = endDateEdit.dateTime().isNull() ? std::nullopt : std::make_optional(ERP::Utils::DateUtils::qDateTimeToTimePoint(endDateEdit.dateTime()));

        bool success = false;
        if (task) {
            success = scheduledTaskService_->updateScheduledTask(newTaskData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Tác vụ", "Tác vụ được lên lịch đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật tác vụ. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> createdTask = scheduledTaskService_->createScheduledTask(newTaskData, currentUserId_, currentUserRoleIds_); // Create with empty details
            if (createdTask) {
                showMessageBox("Thêm Tác vụ", "Tác vụ được lên lịch mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm tác vụ mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadScheduledTasks();
            clearForm();
        }
    }
}


void ScheduledTaskManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool ScheduledTaskManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void ScheduledTaskManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Scheduler.CreateScheduledTask");
    bool canUpdate = hasPermission("Scheduler.UpdateScheduledTask");
    bool canDelete = hasPermission("Scheduler.DeleteScheduledTask");
    bool canChangeStatus = hasPermission("Scheduler.UpdateScheduledTaskStatus");

    addTaskButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Scheduler.ViewScheduledTasks"));

    bool isRowSelected = taskTable_->currentRow() >= 0;
    editTaskButton_->setEnabled(isRowSelected && canUpdate);
    deleteTaskButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);


    bool enableForm = isRowSelected && canUpdate;
    taskNameLineEdit_->setEnabled(enableForm); // Will be read-only for existing anyway
    taskTypeLineEdit_->setEnabled(enableForm);
    frequencyComboBox_->setEnabled(enableForm);
    cronExpressionLineEdit_->setEnabled(enableForm);
    nextRunTimeEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    assignedToComboBox_->setEnabled(enableForm);
    parametersJsonEdit_->setEnabled(enableForm);
    startDateEdit_->setEnabled(enableForm);
    endDateEdit_->setEnabled(enableForm);

    // Read-only fields
    idLineEdit_->setEnabled(false);
    lastRunTimeEdit_->setEnabled(false);
    lastErrorMessageLineEdit_->setEnabled(false);

    if (!isRowSelected) {
        idLineEdit_->clear();
        taskNameLineEdit_->clear();
        taskTypeLineEdit_->clear();
        frequencyComboBox_->setCurrentIndex(0);
        cronExpressionLineEdit_->clear();
        nextRunTimeEdit_->clear();
        lastRunTimeEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
        assignedToComboBox_->clear();
        lastErrorMessageLineEdit_->clear();
        parametersJsonEdit_->clear();
        startDateEdit_->clear();
        endDateEdit_->clear();
    }
}


} // namespace Scheduler
} // namespace UI
} // namespace ERP