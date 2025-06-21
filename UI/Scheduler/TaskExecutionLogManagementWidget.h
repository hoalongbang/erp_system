// UI/Scheduler/TaskExecutionLogManagementWidget.h
#ifndef UI_SCHEDULER_TASKEXECUTIONLOGMANAGEMENTWIDGET_H
#define UI_SCHEDULER_TASKEXECUTIONLOGMANAGEMENTWIDGET_H
#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QDialog>
#include <QDateTimeEdit>

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "TaskExecutionLogService.h" // Dịch vụ nhật ký thực thi tác vụ
#include "ScheduledTaskService.h"    // Dịch vụ tác vụ được lên lịch (để hiển thị tên tác vụ)
#include "ISecurityManager.h"        // Dịch vụ bảo mật
#include "Logger.h"                  // Logging
#include "ErrorHandler.h"            // Xử lý lỗi
#include "Common.h"                  // Các enum chung
#include "DateUtils.h"               // Xử lý ngày tháng
#include "StringUtils.h"             // Xử lý chuỗi
#include "CustomMessageBox.h"        // Hộp thoại thông báo tùy chỉnh
#include "TaskExecutionLog.h"        // TaskExecutionLog DTO
#include "ScheduledTask.h"           // ScheduledTask DTO (for display)
#include "User.h"                    // User DTO (for display)


namespace ERP {
namespace UI {
namespace Scheduler {

/**
 * @brief TaskExecutionLogManagementWidget class provides a UI for viewing Task Execution Logs.
 * This widget allows viewing and deleting execution logs.
 */
class TaskExecutionLogManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for TaskExecutionLogManagementWidget.
     * @param taskExecutionLogService Shared pointer to ITaskExecutionLogService.
     * @param scheduledTaskService Shared pointer to IScheduledTaskService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit TaskExecutionLogManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::ITaskExecutionLogService> taskExecutionLogService = nullptr,
        std::shared_ptr<Services::IScheduledTaskService> scheduledTaskService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~TaskExecutionLogManagementWidget();

private slots:
    void loadExecutionLogs();
    void onDeleteLogClicked();
    void onSearchLogClicked();
    void onLogTableItemClicked(int row, int column);
    void clearForm();

private:
    std::shared_ptr<Services::ITaskExecutionLogService> taskExecutionLogService_;
    std::shared_ptr<Services::IScheduledTaskService> scheduledTaskService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *logTable_;
    QPushButton *deleteLogButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;

    // Form elements for displaying log details (read-only)
    QLineEdit *idLineEdit_;
    QLineEdit *scheduledTaskIdLineEdit_; // Linked task ID, display name
    QLineEdit *scheduledTaskNameLineEdit_; // Display name of scheduled task
    QDateTimeEdit *startTimeEdit_;
    QDateTimeEdit *endTimeEdit_;
    QComboBox *statusComboBox_; // Execution Status
    QLineEdit *executedByLineEdit_; // User ID, display name
    QLineEdit *logOutputLineEdit_; // Truncated or view in detail dialog
    QLineEdit *errorMessageLineEdit_; // Truncated or view in detail dialog
    QLineEdit *executionContextJsonLineEdit_; // JSON string, view in detail dialog

    // Helper functions
    void setupUI();
    void populateScheduledTaskComboBox(QComboBox* comboBox); // For filtering/displaying
    void populateStatusComboBox(); // For execution status
    void showLogDetailsDialog(ERP::Scheduler::DTO::TaskExecutionLogDTO* log);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Scheduler
} // namespace UI
} // namespace ERP

#endif // UI_SCHEDULER_TASKEXECUTIONLOGMANAGEMENTWIDGET_H