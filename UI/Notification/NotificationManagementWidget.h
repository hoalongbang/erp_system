// UI/Notification/NotificationManagementWidget.h
#ifndef UI_NOTIFICATION_NOTIFICATIONMANAGEMENTWIDGET_H
#define UI_NOTIFICATION_NOTIFICATIONMANAGEMENTWIDGET_H
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
#include <QCheckBox>

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "NotificationService.h" // Dịch vụ thông báo
#include "UserService.h"         // Dịch vụ người dùng (để hiển thị tên người dùng)
#include "ISecurityManager.h"    // Dịch vụ bảo mật
#include "Logger.h"              // Logging
#include "ErrorHandler.h"        // Xử lý lỗi
#include "Common.h"              // Các enum chung
#include "DateUtils.h"           // Xử lý ngày tháng
#include "StringUtils.h"         // Xử lý chuỗi
#include "CustomMessageBox.h"    // Hộp thoại thông báo tùy chỉnh
#include "Notification.h"        // Notification DTO
#include "User.h"                // User DTO (for display)

namespace ERP {
namespace UI {
namespace Notification {

/**
 * @brief NotificationManagementWidget class provides a UI for managing Notifications.
 * This widget allows viewing, creating, updating, and deleting notifications.
 */
class NotificationManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for NotificationManagementWidget.
     * @param notificationService Shared pointer to INotificationService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit NotificationManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::INotificationService> notificationService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~NotificationManagementWidget();

private slots:
    void loadNotifications();
    void onAddNotificationClicked();
    void onEditNotificationClicked(); // For updating metadata/notes etc.
    void onDeleteNotificationClicked();
    void onMarkAsReadClicked(); // New slot for marking as read
    void onSearchNotificationClicked();
    void onNotificationTableItemClicked(int row, int column);
    void clearForm();

private:
    std::shared_ptr<Services::INotificationService> notificationService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *notificationTable_;
    QPushButton *addNotificationButton_;
    QPushButton *editNotificationButton_;
    QPushButton *deleteNotificationButton_;
    QPushButton *markAsReadButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;

    // Form elements for editing/adding
    QLineEdit *idLineEdit_;
    QComboBox *userIdComboBox_; // Recipient User ID, display username
    QLineEdit *titleLineEdit_;
    QLineEdit *messageLineEdit_;
    QComboBox *typeComboBox_; // NotificationType
    QComboBox *priorityComboBox_; // NotificationPriority
    QDateTimeEdit *sentTimeEdit_; // Read-only, set by service
    QLineEdit *senderIdLineEdit_; // User ID, display username
    QLineEdit *relatedEntityIdLineEdit_;
    QLineEdit *relatedEntityTypeLineEdit_;
    QCheckBox *isReadCheckBox_; // Display status, update via MarkAsRead action
    QCheckBox *isPublicCheckBox_; // For is_public (if applicable)

    // Helper functions
    void setupUI();
    void populateUserComboBox(QComboBox* comboBox); // For recipient and sender
    void populateTypeComboBox();
    void populatePriorityComboBox();
    void showNotificationInputDialog(ERP::Notification::DTO::NotificationDTO* notification = nullptr);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Notification
} // namespace UI
} // namespace ERP

#endif // UI_NOTIFICATION_NOTIFICATIONMANAGEMENTWIDGET_H