// UI/Scheduler/ScheduledTaskManagementWidget.h
#ifndef UI_SCHEDULER_SCHEDULEDTASKMANAGEMENTWIDGET_H
#define UI_SCHEDULER_SCHEDULEDTASKMANAGEMENTWIDGET_H
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
#include <QIntValidator> // For version/sort order
#include <QTextEdit> // For displaying large text fields like JSON parameters

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "ScheduledTaskService.h" // Dịch vụ tác vụ được lên lịch
#include "ISecurityManager.h"     // Dịch vụ bảo mật
#include "Logger.h"               // Logging
#include "ErrorHandler.h"         // Xử lý lỗi
#include "Common.h"               // Các enum chung
#include "DateUtils.h"            // Xử lý ngày tháng
#include "StringUtils.h"          // Xử lý chuỗi
#include "CustomMessageBox.h"     // Hộp thoại thông báo tùy chỉnh
#include "ScheduledTask.h"        // ScheduledTask DTO
#include "User.h"                 // User DTO (for display)

namespace ERP {
    namespace UI {
        namespace Scheduler {

            /**
             * @brief ScheduledTaskManagementWidget class provides a UI for managing Scheduled Tasks.
             * This widget allows viewing, creating, updating, deleting, and changing task status.
             */
            class ScheduledTaskManagementWidget : public QWidget {
                Q_OBJECT

            public:
                /**
                 * @brief Constructor for ScheduledTaskManagementWidget.
                 * @param parent Parent widget.
                 * @param scheduledTaskService Shared pointer to IScheduledTaskService.
                 * @param securityManager Shared pointer to ISecurityManager.
                 */
                explicit ScheduledTaskManagementWidget(
                    QWidget* parent = nullptr,
                    std::shared_ptr<Services::IScheduledTaskService> scheduledTaskService = nullptr,
                    std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

                ~ScheduledTaskManagementWidget();

            private slots:
                void loadScheduledTasks();
                void onAddTaskClicked();
                void onEditTaskClicked();
                void onDeleteTaskClicked();
                void onUpdateTaskStatusClicked();
                void onSearchTaskClicked();
                void onTaskTableItemClicked(int row, int column);
                void clearForm();

            private:
                std::shared_ptr<Services::IScheduledTaskService> scheduledTaskService_;
                std::shared_ptr<ISecurityManager> securityManager_;
                // Current user context
                std::string currentUserId_;
                std::vector<std::string> currentUserRoleIds_;

                QTableWidget* taskTable_;
                QPushButton* addTaskButton_;
                QPushButton* editTaskButton_;
                QPushButton* deleteTaskButton_;
                QPushButton* updateStatusButton_;
                QPushButton* searchButton_;
                QLineEdit* searchLineEdit_;
                QPushButton* clearFormButton_;

                // Form elements for editing/adding tasks
                QLineEdit* idLineEdit_;
                QLineEdit* taskNameLineEdit_;
                QLineEdit* taskTypeLineEdit_;
                QComboBox* frequencyComboBox_; // ScheduleFrequency
                QLineEdit* cronExpressionLineEdit_;
                QDateTimeEdit* nextRunTimeEdit_;
                QDateTimeEdit* lastRunTimeEdit_; // Read-only
                QComboBox* statusComboBox_; // ScheduledTaskStatus
                QComboBox* assignedToComboBox_; // User ID, display name
                QLineEdit* lastErrorMessageLineEdit_; // Read-only
                QTextEdit* parametersJsonEdit_; // For parameters map as JSON string (use QTextEdit for multi-line JSON)
                QDateTimeEdit* startDateEdit_;
                QDateTimeEdit* endDateEdit_;

                // Helper functions
                void setupUI();
                void populateFrequencyComboBox();
                void populateStatusComboBox(); // For scheduled task status
                void populateUserComboBox(QComboBox* comboBox); // For assignedTo user
                void showTaskInputDialog(ERP::Scheduler::DTO::ScheduledTaskDTO* task = nullptr);
                void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
                void updateButtonsState();

                bool hasPermission(const std::string& permission);
            };

        } // namespace Scheduler
    } // namespace UI
} // namespace ERP

#endif // UI_SCHEDULER_SCHEDULEDTASKMANAGEMENTWIDGET_H