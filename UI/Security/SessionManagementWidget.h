// UI/Security/SessionManagementWidget.h
#ifndef UI_SECURITY_SESSIONMANAGEMENTWIDGET_H
#define UI_SECURITY_SESSIONMANAGEMENTWIDGET_H
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
#include <QCheckBox>
#include <QDateTimeEdit>

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "SessionService.h"    // Dịch vụ quản lý phiên
#include "ISecurityManager.h"  // Dịch vụ bảo mật
#include "Logger.h"            // Logging
#include "ErrorHandler.h"    // Xử lý lỗi
#include "Common.h"          // Các enum chung
#include "DateUtils.h"       // Xử lý ngày tháng
#include "StringUtils.h"     // Xử lý chuỗi
#include "CustomMessageBox.h" // Hộp thoại thông báo tùy chỉnh
#include "Session.h"         // Session DTO (for enums etc.)
#include "User.h"            // User DTO (for display username)

namespace ERP {
namespace UI {
namespace Security {

/**
 * @brief SessionManagementWidget class provides a UI for managing user sessions.
 * This widget allows viewing, updating, deactivating, and deleting sessions.
 */
class SessionManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for SessionManagementWidget.
     * @param sessionService Shared pointer to ISessionService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit SessionManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::ISessionService> sessionService = nullptr,
        std::shared_ptr<ISecurityManager> securityManager = nullptr);

    ~SessionManagementWidget();

private slots:
    void loadSessions();
    void onDeactivateSessionClicked();
    void onDeleteSessionClicked();
    void onSearchSessionClicked();
    void onSessionTableItemClicked(int row, int column);
    void clearForm();

private:
    std::shared_ptr<Services::ISessionService> sessionService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *sessionTable_;
    QPushButton *deactivateSessionButton_;
    QPushButton *deleteSessionButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;

    // Form elements for displaying session details (mostly read-only)
    QLineEdit *sessionIdLineEdit_;
    QLineEdit *userIdLineEdit_;
    QLineEdit *usernameLineEdit_; // Display username
    QLineEdit *tokenLineEdit_;
    QDateTimeEdit *expirationTimeEdit_;
    QLineEdit *ipAddressLineEdit_;
    QLineEdit *userAgentLineEdit_;
    QLineEdit *deviceInfoLineEdit_;
    QComboBox *statusComboBox_; // EntityStatus
    QCheckBox *isActiveCheckBox_; // Based on status

    // Helper functions
    void setupUI();
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState(); // Control button enable/disable based on selection and permissions
    
    // Permission checking helper
    bool hasPermission(const std::string& permission);
};

} // namespace Security
} // namespace UI
} // namespace ERP

#endif // UI_SECURITY_SESSIONMANAGEMENTWIDGET_H