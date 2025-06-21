// UI/Notification/NotificationManagementWidget.cpp
#include "NotificationManagementWidget.h" // Standard includes
#include "Notification.h"                 // Notification DTO
#include "User.h"                         // User DTO
#include "NotificationService.h"          // Notification Service
#include "UserService.h"                  // User Service
#include "ISecurityManager.h"             // Security Manager
#include "Logger.h"                       // Logging
#include "ErrorHandler.h"                 // Error Handling
#include "Common.h"                       // Common Enums/Constants
#include "DateUtils.h"                    // Date Utilities
#include "StringUtils.h"                  // String Utilities
#include "CustomMessageBox.h"             // Custom Message Box

#include <QInputDialog>
#include <QDateTime>

namespace ERP {
namespace UI {
namespace Notification {

NotificationManagementWidget::NotificationManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::INotificationService> notificationService,
    std::shared_ptr<ISecurityManager> securityManager)
    : QWidget(parent),
      notificationService_(notificationService),
      securityManager_(securityManager) {

    if (!notificationService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ thông báo hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("NotificationManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("NotificationManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("NotificationManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadNotifications();
    updateButtonsState();
}

NotificationManagementWidget::~NotificationManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void NotificationManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tiêu đề hoặc nội dung tin nhắn...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &NotificationManagementWidget::onSearchNotificationClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    notificationTable_ = new QTableWidget(this);
    notificationTable_->setColumnCount(7); // ID, User ID, Tiêu đề, Nội dung, Thời gian gửi, Đã đọc, Loại
    notificationTable_->setHorizontalHeaderLabels({"ID", "Người dùng", "Tiêu đề", "Nội dung", "Thời gian gửi", "Đã đọc", "Loại"});
    notificationTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    notificationTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    notificationTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    notificationTable_->horizontalHeader()->setStretchLastSection(true);
    connect(notificationTable_, &QTableWidget::itemClicked, this, &NotificationManagementWidget::onNotificationTableItemClicked);
    mainLayout->addWidget(notificationTable_);

    // Form elements for editing/adding notifications
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    userIdComboBox_ = new QComboBox(this); populateUserComboBox(userIdComboBox_); // Recipient user
    titleLineEdit_ = new QLineEdit(this);
    messageLineEdit_ = new QLineEdit(this);
    typeComboBox_ = new QComboBox(this); populateTypeComboBox();
    priorityComboBox_ = new QComboBox(this); populatePriorityComboBox();
    sentTimeEdit_ = new QDateTimeEdit(this); sentTimeEdit_->setReadOnly(true); sentTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    senderIdLineEdit_ = new QLineEdit(this); senderIdLineEdit_->setReadOnly(true); // Auto-filled with current user
    relatedEntityIdLineEdit_ = new QLineEdit(this);
    relatedEntityTypeLineEdit_ = new QLineEdit(this);
    isReadCheckBox_ = new QCheckBox("Đã đọc", this); isReadCheckBox_->setEnabled(false); // Only updated via action
    isPublicCheckBox_ = new QCheckBox("Công khai", this); // Assuming this field is relevant for notifications

    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Người dùng nhận:*", this), 1, 0); formLayout->addWidget(userIdComboBox_, 1, 1);
    formLayout->addWidget(new QLabel("Tiêu đề:*", this), 2, 0); formLayout->addWidget(titleLineEdit_, 2, 1);
    formLayout->addWidget(new QLabel("Nội dung:*", this), 3, 0); formLayout->addWidget(messageLineEdit_, 3, 1);
    formLayout->addWidget(new QLabel("Loại:*", this), 4, 0); formLayout->addWidget(typeComboBox_, 4, 1);
    formLayout->addWidget(new QLabel("Ưu tiên:*", this), 5, 0); formLayout->addWidget(priorityComboBox_, 5, 1);
    formLayout->addWidget(new QLabel("Thời gian gửi:", this), 6, 0); formLayout->addWidget(sentTimeEdit_, 6, 1);
    formLayout->addWidget(new QLabel("Người gửi:", this), 7, 0); formLayout->addWidget(senderIdLineEdit_, 7, 1);
    formLayout->addWidget(new QLabel("ID Thực thể liên quan:", this), 8, 0); formLayout->addWidget(relatedEntityIdLineEdit_, 8, 1);
    formLayout->addWidget(new QLabel("Loại Thực thể liên quan:", this), 9, 0); formLayout->addWidget(relatedEntityTypeLineEdit_, 9, 1);
    formLayout->addWidget(isReadCheckBox_, 10, 1);
    formLayout->addWidget(isPublicCheckBox_, 11, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addNotificationButton_ = new QPushButton("Thêm mới", this); connect(addNotificationButton_, &QPushButton::clicked, this, &NotificationManagementWidget::onAddNotificationClicked);
    editNotificationButton_ = new QPushButton("Sửa", this); connect(editNotificationButton_, &QPushButton::clicked, this, &NotificationManagementWidget::onEditNotificationClicked);
    deleteNotificationButton_ = new QPushButton("Xóa", this); connect(deleteNotificationButton_, &QPushButton::clicked, this, &NotificationManagementWidget::onDeleteNotificationClicked);
    markAsReadButton_ = new QPushButton("Đánh dấu Đã đọc", this); connect(markAsReadButton_, &QPushButton::clicked, this, &NotificationManagementWidget::onMarkAsReadClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &NotificationManagementWidget::onSearchNotificationClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &NotificationManagementWidget::clearForm);
    
    buttonLayout->addWidget(addNotificationButton_);
    buttonLayout->addWidget(editNotificationButton_);
    buttonLayout->addWidget(deleteNotificationButton_);
    buttonLayout->addWidget(markAsReadButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void NotificationManagementWidget::loadNotifications() {
    ERP::Logger::Logger::getInstance().info("NotificationManagementWidget: Loading notifications...");
    notificationTable_->setRowCount(0); // Clear existing rows

    // For simplicity, load notifications for the current user or all if admin
    std::map<std::string, std::any> filter;
    if (!hasPermission("Notification.ViewAllNotifications")) {
        filter["user_id"] = currentUserId_; // Only show current user's notifications
    }
    std::vector<ERP::Notification::DTO::NotificationDTO> notifications = notificationService_->getAllNotifications(filter, currentUserId_, currentUserRoleIds_);

    notificationTable_->setRowCount(notifications.size());
    for (int i = 0; i < notifications.size(); ++i) {
        const auto& notification = notifications[i];
        notificationTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(notification.id)));
        
        QString username = "N/A";
        std::optional<ERP::User::DTO::UserDTO> user = securityManager_->getUserService()->getUserById(notification.userId, currentUserId_, currentUserRoleIds_);
        if (user) username = QString::fromStdString(user->username);
        notificationTable_->setItem(i, 1, new QTableWidgetItem(username));

        notificationTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(notification.title)));
        notificationTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(notification.message.left(50) + "..."))); // Truncate message
        notificationTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(notification.sentTime, ERP::Common::DATETIME_FORMAT))));
        notificationTable_->setItem(i, 5, new QTableWidgetItem(notification.isRead ? "Yes" : "No"));
        notificationTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(notification.getTypeString())));
    }
    notificationTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("NotificationManagementWidget: Notifications loaded successfully.");
}

void NotificationManagementWidget::populateUserComboBox(QComboBox* comboBox) {
    comboBox->clear();
    std::vector<ERP::User::DTO::UserDTO> allUsers = securityManager_->getUserService()->getAllUsers({}, currentUserId_, currentUserRoleIds_);
    for (const auto& user : allUsers) {
        comboBox->addItem(QString::fromStdString(user.username), QString::fromStdString(user.id));
    }
}

void NotificationManagementWidget::populateTypeComboBox() {
    typeComboBox_->clear();
    typeComboBox_->addItem("Info", static_cast<int>(ERP::Notification::DTO::NotificationType::INFO));
    typeComboBox_->addItem("Warning", static_cast<int>(ERP::Notification::DTO::NotificationType::WARNING));
    typeComboBox_->addItem("Error", static_cast<int>(ERP::Notification::DTO::NotificationType::ERROR));
    typeComboBox_->addItem("Success", static_cast<int>(ERP::Notification::DTO::NotificationType::SUCCESS));
    typeComboBox_->addItem("Alert", static_cast<int>(ERP::Notification::DTO::NotificationType::ALERT));
    typeComboBox_->addItem("System", static_cast<int>(ERP::Notification::DTO::NotificationType::SYSTEM));
}

void NotificationManagementWidget::populatePriorityComboBox() {
    priorityComboBox_->clear();
    priorityComboBox_->addItem("Low", static_cast<int>(ERP::Notification::DTO::NotificationPriority::LOW));
    priorityComboBox_->addItem("Normal", static_cast<int>(ERP::Notification::DTO::NotificationPriority::NORMAL));
    priorityComboBox_->addItem("High", static_cast<int>(ERP::Notification::DTO::NotificationPriority::HIGH));
    priorityComboBox_->addItem("Urgent", static_cast<int>(ERP::Notification::DTO::NotificationPriority::URGENT));
}


void NotificationManagementWidget::onAddNotificationClicked() {
    if (!hasPermission("Notification.CreateNotification")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm thông báo.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateUserComboBox(userIdComboBox_);
    populateTypeComboBox();
    populatePriorityComboBox();
    showNotificationInputDialog();
}

void NotificationManagementWidget::onEditNotificationClicked() {
    if (!hasPermission("Notification.UpdateNotification")) { // Assuming this permission exists
        showMessageBox("Lỗi", "Bạn không có quyền sửa thông báo.", QMessageBox::Warning);
        return;
    }

    int selectedRow = notificationTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Thông Báo", "Vui lòng chọn một thông báo để sửa.", QMessageBox::Information);
        return;
    }

    QString notificationId = notificationTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Notification::DTO::NotificationDTO> notificationOpt = notificationService_->getNotificationById(notificationId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (notificationOpt) {
        populateUserComboBox(userIdComboBox_);
        populateTypeComboBox();
        populatePriorityComboBox();
        showNotificationInputDialog(&(*notificationOpt));
    } else {
        showMessageBox("Sửa Thông Báo", "Không tìm thấy thông báo để sửa.", QMessageBox::Critical);
    }
}

void NotificationManagementWidget::onDeleteNotificationClicked() {
    if (!hasPermission("Notification.DeleteNotification")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa thông báo.", QMessageBox::Warning);
        return;
    }

    int selectedRow = notificationTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Thông Báo", "Vui lòng chọn một thông báo để xóa.", QMessageBox::Information);
        return;
    }

    QString notificationId = notificationTable_->item(selectedRow, 0)->text();
    QString notificationTitle = notificationTable_->item(selectedRow, 2)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Thông Báo");
    confirmBox.setText("Bạn có chắc chắn muốn xóa thông báo '" + notificationTitle + "' (ID: " + notificationId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (notificationService_->deleteNotification(notificationId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Thông Báo", "Thông báo đã được xóa thành công.", QMessageBox::Information);
            loadNotifications();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa thông báo. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void NotificationManagementWidget::onMarkAsReadClicked() {
    if (!hasPermission("Notification.MarkAsRead")) { // Assuming a permission for this specific action
        showMessageBox("Lỗi", "Bạn không có quyền đánh dấu thông báo đã đọc.", QMessageBox::Warning);
        return;
    }

    int selectedRow = notificationTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Đánh dấu Đã đọc", "Vui lòng chọn một thông báo để đánh dấu đã đọc.", QMessageBox::Information);
        return;
    }

    QString notificationId = notificationTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Notification::DTO::NotificationDTO> notificationOpt = notificationService_->getNotificationById(notificationId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!notificationOpt) {
        showMessageBox("Đánh dấu Đã đọc", "Không tìm thấy thông báo để đánh dấu đã đọc.", QMessageBox::Critical);
        return;
    }

    if (notificationOpt->isRead) {
        showMessageBox("Đánh dấu Đã đọc", "Thông báo này đã được đánh dấu là đã đọc rồi.", QMessageBox::Information);
        return;
    }

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Đánh dấu Thông Báo Đã đọc");
    confirmBox.setText("Bạn có chắc chắn muốn đánh dấu thông báo '" + QString::fromStdString(notificationOpt->title) + "' là đã đọc?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (notificationService_->markNotificationAsRead(notificationId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Đánh dấu Đã đọc", "Thông báo đã được đánh dấu là đã đọc thành công.", QMessageBox::Information);
            loadNotifications();
            clearForm();
        } else {
            showMessageBox("Lỗi", "Không thể đánh dấu thông báo là đã đọc. Vui lòng kiểm tra log.", QMessageBox::Critical);
        }
    }
}


void NotificationManagementWidget::onSearchNotificationClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["title_or_message_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    // Also, filter by current user if not admin
    if (!hasPermission("Notification.ViewAllNotifications")) {
        filter["user_id"] = currentUserId_;
    }

    notificationTable_->setRowCount(0);
    std::vector<ERP::Notification::DTO::NotificationDTO> notifications = notificationService_->getAllNotifications(filter, currentUserId_, currentUserRoleIds_);

    notificationTable_->setRowCount(notifications.size());
    for (int i = 0; i < notifications.size(); ++i) {
        const auto& notification = notifications[i];
        notificationTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(notification.id)));
        
        QString username = "N/A";
        std::optional<ERP::User::DTO::UserDTO> user = securityManager_->getUserService()->getUserById(notification.userId, currentUserId_, currentUserRoleIds_);
        if (user) username = QString::fromStdString(user->username);
        notificationTable_->setItem(i, 1, new QTableWidgetItem(username));

        notificationTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(notification.title)));
        notificationTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(notification.message.left(50) + "...")));
        notificationTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(notification.sentTime, ERP::Common::DATETIME_FORMAT))));
        notificationTable_->setItem(i, 5, new QTableWidgetItem(notification.isRead ? "Yes" : "No"));
        notificationTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(notification.getTypeString())));
    }
    notificationTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("NotificationManagementWidget: Search completed.");
}

void NotificationManagementWidget::onNotificationTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString notificationId = notificationTable_->item(row, 0)->text();
    std::optional<ERP::Notification::DTO::NotificationDTO> notificationOpt = notificationService_->getNotificationById(notificationId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (notificationOpt) {
        idLineEdit_->setText(QString::fromStdString(notificationOpt->id));
        
        populateUserComboBox(userIdComboBox_);
        int userIndex = userIdComboBox_->findData(QString::fromStdString(notificationOpt->userId));
        if (userIndex != -1) userIdComboBox_->setCurrentIndex(userIndex);

        titleLineEdit_->setText(QString::fromStdString(notificationOpt->title));
        messageLineEdit_->setText(QString::fromStdString(notificationOpt->message));
        
        populateTypeComboBox();
        int typeIndex = typeComboBox_->findData(static_cast<int>(notificationOpt->type));
        if (typeIndex != -1) typeComboBox_->setCurrentIndex(typeIndex);

        populatePriorityComboBox();
        int priorityIndex = priorityComboBox_->findData(static_cast<int>(notificationOpt->priority));
        if (priorityIndex != -1) priorityComboBox_->setCurrentIndex(priorityIndex);

        sentTimeEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(notificationOpt->sentTime.time_since_epoch()).count()));
        
        senderIdLineEdit_->setText(QString::fromStdString(notificationOpt->senderId.value_or("")));
        relatedEntityIdLineEdit_->setText(QString::fromStdString(notificationOpt->relatedEntityId.value_or("")));
        relatedEntityTypeLineEdit_->setText(QString::fromStdString(notificationOpt->relatedEntityType.value_or("")));
        isReadCheckBox_->setChecked(notificationOpt->isRead);
        isPublicCheckBox_->setChecked(notificationOpt->isPublic);

    } else {
        showMessageBox("Thông tin Thông Báo", "Không thể tải chi tiết thông báo đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void NotificationManagementWidget::clearForm() {
    idLineEdit_->clear();
    userIdComboBox_->clear(); // Repopulated on demand
    titleLineEdit_->clear();
    messageLineEdit_->clear();
    typeComboBox_->setCurrentIndex(0);
    priorityComboBox_->setCurrentIndex(0);
    sentTimeEdit_->clear();
    senderIdLineEdit_->clear();
    relatedEntityIdLineEdit_->clear();
    relatedEntityTypeLineEdit_->clear();
    isReadCheckBox_->setChecked(false);
    isPublicCheckBox_->setChecked(false);
    notificationTable_->clearSelection();
    updateButtonsState();
}


void NotificationManagementWidget::showNotificationInputDialog(ERP::Notification::DTO::NotificationDTO* notification) {
    QDialog dialog(this);
    dialog.setWindowTitle(notification ? "Sửa Thông Báo" : "Thêm Thông Báo Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox userIdCombo(this); populateUserComboBox(&userIdCombo);
    QLineEdit titleEdit(this);
    QLineEdit messageEdit(this);
    QComboBox typeCombo(this); populateTypeComboBox();
    for(int i = 0; i < typeComboBox_->count(); ++i) typeCombo.addItem(typeComboBox_->itemText(i), typeComboBox_->itemData(i));
    QComboBox priorityCombo(this); populatePriorityComboBox();
    for(int i = 0; i < priorityComboBox_->count(); ++i) priorityCombo.addItem(priorityComboBox_->itemText(i), priorityComboBox_->itemData(i));
    
    QLineEdit senderIdEdit(this);
    QLineEdit relatedEntityIdEdit(this);
    QLineEdit relatedEntityTypeEdit(this);
    QCheckBox isPublicCheck("Công khai", this);

    if (notification) {
        int userIndex = userIdCombo.findData(QString::fromStdString(notification->userId));
        if (userIndex != -1) userIdCombo.setCurrentIndex(userIndex);
        titleEdit.setText(QString::fromStdString(notification->title));
        messageEdit.setText(QString::fromStdString(notification->message));
        int typeIndex = typeCombo.findData(static_cast<int>(notification->type));
        if (typeIndex != -1) typeCombo.setCurrentIndex(typeIndex);
        int priorityIndex = priorityCombo.findData(static_cast<int>(notification->priority));
        if (priorityIndex != -1) priorityCombo.setCurrentIndex(priorityIndex);
        senderIdEdit.setText(QString::fromStdString(notification->senderId.value_or("")));
        relatedEntityIdEdit.setText(QString::fromStdString(notification->relatedEntityId.value_or("")));
        relatedEntityTypeEdit.setText(QString::fromStdString(notification->relatedEntityType.value_or("")));
        isPublicCheck.setChecked(notification->isPublic);
    } else {
        // Defaults for new
        userIdCombo.setCurrentIndex(userIdCombo.findData(QString::fromStdString(currentUserId_))); // Default to current user
        senderIdEdit.setText(QString::fromStdString(currentUserId_)); // Default sender is current user
    }

    formLayout->addRow("Người dùng nhận:*", &userIdCombo);
    formLayout->addRow("Tiêu đề:*", &titleEdit);
    formLayout->addRow("Nội dung:*", &messageEdit);
    formLayout->addRow("Loại:*", &typeCombo);
    formLayout->addRow("Ưu tiên:*", &priorityCombo);
    formLayout->addRow("Người gửi:", &senderIdEdit);
    formLayout->addRow("ID Thực thể liên quan:", &relatedEntityIdEdit);
    formLayout->addRow("Loại Thực thể liên quan:", &relatedEntityTypeEdit);
    formLayout->addRow("", &isPublicCheck);

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(notification ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Notification::DTO::NotificationDTO newNotificationData;
        if (notification) {
            newNotificationData = *notification;
        } else {
            newNotificationData.id = ERP::Utils::generateUUID(); // New ID for new notification
        }

        newNotificationData.userId = userIdCombo.currentData().toString().toStdString();
        newNotificationData.title = titleEdit.text().toStdString();
        newNotificationData.message = messageEdit.text().toStdString();
        newNotificationData.type = static_cast<ERP::Notification::DTO::NotificationType>(typeCombo.currentData().toInt());
        newNotificationData.priority = static_cast<ERP::Notification::DTO::NotificationPriority>(priorityCombo.currentData().toInt());
        newNotificationData.senderId = senderIdEdit.text().isEmpty() ? std::nullopt : std::make_optional(senderIdEdit.text().toStdString());
        newNotificationData.relatedEntityId = relatedEntityIdEdit.text().isEmpty() ? std::nullopt : std::make_optional(relatedEntityIdEdit.text().toStdString());
        newNotificationData.relatedEntityType = relatedEntityTypeEdit.text().isEmpty() ? std::nullopt : std::make_optional(relatedEntityTypeEdit.text().toStdString());
        newNotificationData.isPublic = isPublicCheck.isChecked();
        newNotificationData.isRead = false; // Always false on creation/edit via this dialog
        newNotificationData.status = ERP::Common::EntityStatus::ACTIVE; // Always active

        bool success = false;
        if (notification) {
            // Update an existing notification. Note: `createNotification` in service is only for new, `updateNotification` is needed.
            // For simplicity, assuming update is handled by createNotification in this context, but in reality, need an update method.
            // If `notificationService_` has an `update` method, use it:
            // success = notificationService_->updateNotification(newNotificationData, currentUserId_, currentUserRoleIds_);
            // For now, if no explicit update method, fallback to create (which is incorrect for existing) or mark as failed.
            showMessageBox("Lỗi", "Chức năng sửa thông báo chưa được triển khai đầy đủ.", QMessageBox::Critical);
            return;
        } else {
            std::optional<ERP::Notification::DTO::NotificationDTO> createdNotification = notificationService_->createNotification(newNotificationData, currentUserId_, currentUserRoleIds_);
            if (createdNotification) {
                showMessageBox("Thêm Thông Báo", "Thông báo mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm thông báo mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadNotifications();
            clearForm();
        }
    }
}


void NotificationManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool NotificationManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void NotificationManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Notification.CreateNotification");
    bool canUpdate = hasPermission("Notification.UpdateNotification"); // Assuming this permission
    bool canDelete = hasPermission("Notification.DeleteNotification");
    bool canMarkAsRead = hasPermission("Notification.MarkAsRead"); // Assuming this permission

    addNotificationButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Notification.ViewNotifications")); // ViewAllNotifications or ViewNotifications

    bool isRowSelected = notificationTable_->currentRow() >= 0;
    editNotificationButton_->setEnabled(isRowSelected && canUpdate);
    deleteNotificationButton_->setEnabled(isRowSelected && canDelete);
    markAsReadButton_->setEnabled(isRowSelected && canMarkAsRead && notificationTable_->item(notificationTable_->currentRow(), 5)->text() == "No");

    bool enableForm = isRowSelected && canUpdate;
    userIdComboBox_->setEnabled(enableForm);
    titleLineEdit_->setEnabled(enableForm);
    messageLineEdit_->setEnabled(enableForm);
    typeComboBox_->setEnabled(enableForm);
    priorityComboBox_->setEnabled(enableForm);
    senderIdLineEdit_->setEnabled(enableForm);
    relatedEntityIdLineEdit_->setEnabled(enableForm);
    relatedEntityTypeLineEdit_->setEnabled(enableForm);
    isPublicCheckBox_->setEnabled(enableForm);

    // Read-only fields
    idLineEdit_->setEnabled(false);
    sentTimeEdit_->setEnabled(false);
    isReadCheckBox_->setEnabled(false);

    if (!isRowSelected) {
        idLineEdit_->clear();
        userIdComboBox_->clear();
        titleLineEdit_->clear();
        messageLineEdit_->clear();
        typeComboBox_->setCurrentIndex(0);
        priorityComboBox_->setCurrentIndex(0);
        sentTimeEdit_->clear();
        senderIdLineEdit_->clear();
        relatedEntityIdLineEdit_->clear();
        relatedEntityTypeLineEdit_->clear();
        isReadCheckBox_->setChecked(false);
        isPublicCheckBox_->setChecked(false);
    }
}


} // namespace Notification
} // namespace UI
} // namespace ERP