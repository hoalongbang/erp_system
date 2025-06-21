// UI/Security/SessionManagementWidget.cpp
#include "SessionManagementWidget.h" // Đã rút gọn include
#include "Session.h"           // Đã rút gọn include
#include "User.h"              // Đã rút gọn include
#include "SessionService.h"    // Đã rút gọn include
#include "UserService.h"       // For getting username from userId
#include "ISecurityManager.h"  // Đã rút gọn include
#include "Logger.h"            // Đã rút gọn include
#include "ErrorHandler.h"      // Đã rút gọn include
#include "Common.h"            // Đã rút gọn include
#include "DateUtils.h"         // Đã rút gọn include
#include "StringUtils.h"       // Đã rút gọn include
#include "CustomMessageBox.h"  // Đã rút gọn include

#include <QInputDialog> // For getting input from user

namespace ERP {
namespace UI {
namespace Security {

SessionManagementWidget::SessionManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::ISessionService> sessionService,
    std::shared_ptr<ISecurityManager> securityManager)
    : QWidget(parent),
      sessionService_(sessionService),
      securityManager_(securityManager) {

    if (!sessionService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ phiên hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("SessionManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("SessionManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("SessionManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadSessions();
    updateButtonsState();
}

SessionManagementWidget::~SessionManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void SessionManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo ID phiên hoặc ID người dùng...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &SessionManagementWidget::onSearchSessionClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    sessionTable_ = new QTableWidget(this);
    sessionTable_->setColumnCount(8); // ID, User ID, Tên người dùng, Token, Hết hạn, IP, User Agent, Trạng thái
    sessionTable_->setHorizontalHeaderLabels({"ID Phiên", "ID Người dùng", "Tên người dùng", "Token", "Thời gian hết hạn", "Địa chỉ IP", "User Agent", "Trạng thái"});
    sessionTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    sessionTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    sessionTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    sessionTable_->horizontalHeader()->setStretchLastSection(true);
    connect(sessionTable_, &QTableWidget::itemClicked, this, &SessionManagementWidget::onSessionTableItemClicked);
    mainLayout->addWidget(sessionTable_);

    // Form elements for displaying details (mostly read-only)
    QGridLayout *formLayout = new QGridLayout();
    sessionIdLineEdit_ = new QLineEdit(this); sessionIdLineEdit_->setReadOnly(true);
    userIdLineEdit_ = new QLineEdit(this); userIdLineEdit_->setReadOnly(true);
    usernameLineEdit_ = new QLineEdit(this); usernameLineEdit_->setReadOnly(true);
    tokenLineEdit_ = new QLineEdit(this); tokenLineEdit_->setReadOnly(true);
    expirationTimeEdit_ = new QDateTimeEdit(this); expirationTimeEdit_->setReadOnly(true); expirationTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    ipAddressLineEdit_ = new QLineEdit(this); ipAddressLineEdit_->setReadOnly(true);
    userAgentLineEdit_ = new QLineEdit(this); userAgentLineEdit_->setReadOnly(true);
    deviceInfoLineEdit_ = new QLineEdit(this); deviceInfoLineEdit_->setReadOnly(true);
    statusComboBox_ = new QComboBox(this); statusComboBox_->setEnabled(false); // Read-only via UI
    statusComboBox_->addItem("Active", static_cast<int>(ERP::Common::EntityStatus::ACTIVE));
    statusComboBox_->addItem("Inactive", static_cast<int>(ERP::Common::EntityStatus::INACTIVE));
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Common::EntityStatus::PENDING));
    statusComboBox_->addItem("Deleted", static_cast<int>(ERP::Common::EntityStatus::DELETED));
    isActiveCheckBox_ = new QCheckBox("Phiên hoạt động", this); isActiveCheckBox_->setEnabled(false);

    formLayout->addWidget(new QLabel("ID Phiên:", this), 0, 0); formLayout->addWidget(sessionIdLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("ID Người dùng:", this), 1, 0); formLayout->addWidget(userIdLineEdit_, 1, 1);
    formLayout->addWidget(new QLabel("Tên người dùng:", this), 2, 0); formLayout->addWidget(usernameLineEdit_, 2, 1);
    formLayout->addWidget(new QLabel("Token:", this), 3, 0); formLayout->addWidget(tokenLineEdit_, 3, 1);
    formLayout->addWidget(new QLabel("Thời gian hết hạn:", this), 4, 0); formLayout->addWidget(expirationTimeEdit_, 4, 1);
    formLayout->addWidget(new QLabel("Địa chỉ IP:", this), 5, 0); formLayout->addWidget(ipAddressLineEdit_, 5, 1);
    formLayout->addWidget(new QLabel("User Agent:", this), 6, 0); formLayout->addWidget(userAgentLineEdit_, 6, 1);
    formLayout->addWidget(new QLabel("Thông tin thiết bị:", this), 7, 0); formLayout->addWidget(deviceInfoLineEdit_, 7, 1);
    formLayout->addWidget(new QLabel("Trạng thái:", this), 8, 0); formLayout->addWidget(statusComboBox_, 8, 1);
    formLayout->addWidget(isActiveCheckBox_, 9, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    deactivateSessionButton_ = new QPushButton("Vô hiệu hóa Phiên", this);
    connect(deactivateSessionButton_, &QPushButton::clicked, this, &SessionManagementWidget::onDeactivateSessionClicked);
    deleteSessionButton_ = new QPushButton("Xóa Phiên", this);
    connect(deleteSessionButton_, &QPushButton::clicked, this, &SessionManagementWidget::onDeleteSessionClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this);
    connect(clearFormButton_, &QPushButton::clicked, this, &SessionManagementWidget::clearForm);

    buttonLayout->addWidget(deactivateSessionButton_);
    buttonLayout->addWidget(deleteSessionButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void SessionManagementWidget::loadSessions() {
    ERP::Logger::Logger::getInstance().info("SessionManagementWidget: Loading sessions...");
    sessionTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Security::DTO::SessionDTO> sessions = sessionService_->getAllSessions({}, currentUserId_, currentUserRoleIds_);

    sessionTable_->setRowCount(sessions.size());
    for (int i = 0; i < sessions.size(); ++i) {
        const auto& session = sessions[i];
        sessionTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(session.id)));
        sessionTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(session.userId)));
        
        // Resolve username
        QString username = "N/A";
        std::optional<ERP::User::DTO::UserDTO> user = securityManager_->getUserService()->getUserById(session.userId, currentUserId_, currentUserRoleIds_);
        if (user) {
            username = QString::fromStdString(user->username);
        }
        sessionTable_->setItem(i, 2, new QTableWidgetItem(username));

        sessionTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(session.token.left(10) + "..."))); // Display truncated token
        sessionTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(session.expirationTime, ERP::Common::DATETIME_FORMAT))));
        sessionTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(session.ipAddress.value_or(""))));
        sessionTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(session.userAgent.value_or(""))));
        sessionTable_->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(session.status))));
    }
    sessionTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("SessionManagementWidget: Sessions loaded successfully.");
}

void SessionManagementWidget::onDeactivateSessionClicked() {
    if (!hasPermission("Security.DeactivateSession")) {
        showMessageBox("Lỗi", "Bạn không có quyền vô hiệu hóa phiên.", QMessageBox::Warning);
        return;
    }

    int selectedRow = sessionTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Vô hiệu hóa Phiên", "Vui lòng chọn một phiên để vô hiệu hóa.", QMessageBox::Information);
        return;
    }

    QString sessionId = sessionTable_->item(selectedRow, 0)->text();
    QString sessionUserId = sessionTable_->item(selectedRow, 1)->text(); // Get user ID for display/audit
    QString sessionUsername = sessionTable_->item(selectedRow, 2)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Vô hiệu hóa Phiên");
    confirmBox.setText("Bạn có chắc chắn muốn vô hiệu hóa phiên của người dùng '" + sessionUsername + "' (ID: " + sessionId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (sessionService_->deactivateSession(sessionId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Vô hiệu hóa Phiên", "Phiên đã được vô hiệu hóa thành công.", QMessageBox::Information);
            loadSessions();
            clearForm();
        } else {
            showMessageBox("Lỗi", "Không thể vô hiệu hóa phiên. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void SessionManagementWidget::onDeleteSessionClicked() {
    if (!hasPermission("Security.DeleteSession")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa phiên.", QMessageBox::Warning);
        return;
    }

    int selectedRow = sessionTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Phiên", "Vui lòng chọn một phiên để xóa.", QMessageBox::Information);
        return;
    }

    QString sessionId = sessionTable_->item(selectedRow, 0)->text();
    QString sessionUserId = sessionTable_->item(selectedRow, 1)->text(); // Get user ID for display/audit
    QString sessionUsername = sessionTable_->item(selectedRow, 2)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Phiên");
    confirmBox.setText("Bạn có chắc chắn muốn xóa phiên của người dùng '" + sessionUsername + "' (ID: " + sessionId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (sessionService_->deleteSession(sessionId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Phiên", "Phiên đã được xóa thành công.", QMessageBox::Information);
            loadSessions();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa phiên. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void SessionManagementWidget::onSearchSessionClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["search_term"] = searchText.toStdString(); // Assuming service supports generic search
        // Or specific filters: filter["id"] = searchText.toStdString();
        // filter["user_id"] = searchText.toStdString();
    }
    sessionTable_->setRowCount(0);
    std::vector<ERP::Security::DTO::SessionDTO> sessions = sessionService_->getAllSessions(filter, currentUserId_, currentUserRoleIds_);

    sessionTable_->setRowCount(sessions.size());
    for (int i = 0; i < sessions.size(); ++i) {
        const auto& session = sessions[i];
        sessionTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(session.id)));
        sessionTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(session.userId)));
        
        QString username = "N/A";
        std::optional<ERP::User::DTO::UserDTO> user = securityManager_->getUserService()->getUserById(session.userId, currentUserId_, currentUserRoleIds_);
        if (user) {
            username = QString::fromStdString(user->username);
        }
        sessionTable_->setItem(i, 2, new QTableWidgetItem(username));

        sessionTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(session.token.left(10) + "...")));
        sessionTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(session.expirationTime, ERP::Common::DATETIME_FORMAT))));
        sessionTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(session.ipAddress.value_or(""))));
        sessionTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(session.userAgent.value_or(""))));
        sessionTable_->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(session.status))));
    }
    sessionTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("SessionManagementWidget: Search completed.");
}

void SessionManagementWidget::onSessionTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString sessionId = sessionTable_->item(row, 0)->text();
    std::optional<ERP::Security::DTO::SessionDTO> sessionOpt = sessionService_->getSessionById(sessionId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (sessionOpt) {
        sessionIdLineEdit_->setText(QString::fromStdString(sessionOpt->id));
        userIdLineEdit_->setText(QString::fromStdString(sessionOpt->userId));
        
        QString username = "N/A";
        std::optional<ERP::User::DTO::UserDTO> user = securityManager_->getUserService()->getUserById(sessionOpt->userId, currentUserId_, currentUserRoleIds_);
        if (user) {
            username = QString::fromStdString(user->username);
        }
        usernameLineEdit_->setText(username);

        tokenLineEdit_->setText(QString::fromStdString(sessionOpt->token)); // Show full token here
        expirationTimeEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(sessionOpt->expirationTime.time_since_epoch()).count()));
        ipAddressLineEdit_->setText(QString::fromStdString(sessionOpt->ipAddress.value_or("")));
        userAgentLineEdit_->setText(QString::fromStdString(sessionOpt->userAgent.value_or("")));
        deviceInfoLineEdit_->setText(QString::fromStdString(sessionOpt->deviceInfo.value_or("")));
        
        int statusIndex = statusComboBox_->findData(static_cast<int>(sessionOpt->status));
        if (statusIndex != -1) {
            statusComboBox_->setCurrentIndex(statusIndex);
        }
        isActiveCheckBox_->setChecked(sessionOpt->status == ERP::Common::EntityStatus::ACTIVE);

    } else {
        showMessageBox("Thông tin Phiên", "Không thể tải chi tiết phiên đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void SessionManagementWidget::clearForm() {
    sessionIdLineEdit_->clear();
    userIdLineEdit_->clear();
    usernameLineEdit_->clear();
    tokenLineEdit_->clear();
    expirationTimeEdit_->clear();
    ipAddressLineEdit_->clear();
    userAgentLineEdit_->clear();
    deviceInfoLineEdit_->clear();
    statusComboBox_->setCurrentIndex(0); // Default to Active
    isActiveCheckBox_->setChecked(false);
    sessionTable_->clearSelection();
    updateButtonsState();
}

void SessionManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool SessionManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void SessionManagementWidget::updateButtonsState() {
    bool canDeactivate = hasPermission("Security.DeactivateSession");
    bool canDelete = hasPermission("Security.DeleteSession");
    bool canView = hasPermission("Security.ViewSessions"); // For search and table view

    searchButton_->setEnabled(canView);

    bool isRowSelected = sessionTable_->currentRow() >= 0;
    deactivateSessionButton_->setEnabled(isRowSelected && canDeactivate);
    deleteSessionButton_->setEnabled(isRowSelected && canDelete);

    // Form fields are read-only for sessions, so no need to enable/disable based on update permission
}

} // namespace Security
} // namespace UI
} // namespace ERP