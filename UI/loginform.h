// UI/loginform.h
#ifndef UI_LOGINFORM_H
#define UI_LOGINFORM_H
#include <QWidget>
#include <QMessageBox> // For QMessageBox

// Rút gọn includes
#include "IAuthenticationService.h" // Interface for AuthenticationService
#include "IUserService.h"           // Interface for UserService
#include "Session.h"                // For SessionDTO
#include "User.h"                   // For UserDTO
#include "CustomMessageBox.h"       // Custom message box implementation

QT_BEGIN_NAMESPACE
namespace Ui { class LoginForm; }
QT_END_NAMESPACE

namespace ERP {
namespace UI {
/**
 * @brief The LoginForm class provides the user interface for login.
 * It interacts with the AuthenticationService to authenticate users.
 */
class LoginForm : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for LoginForm.
     * @param parent Parent widget.
     * @param authenticationService Shared pointer to IAuthenticationService.
     * @param userService Shared pointer to IUserService.
     */
    explicit LoginForm(QWidget *parent = nullptr,
                       std::shared_ptr<ERP::Security::Service::IAuthenticationService> authenticationService = nullptr,
                       std::shared_ptr<ERP::User::Services::IUserService> userService = nullptr);
    ~LoginForm();

signals:
    void loginSuccess(const QString& username, const QString& userId, const QString& sessionId);
    void registerRequested();

private slots:
    void on_loginButton_clicked();
    void on_registerButton_clicked();

private:
    Ui::LoginForm *ui;
    std::shared_ptr<ERP::Security::Service::IAuthenticationService> authenticationService_;
    std::shared_ptr<ERP::User::Services::IUserService> userService_; // Used to get current user ID for audit logging, etc.

    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
};
} // namespace UI
} // namespace ERP
#endif // UI_LOGINFORM_H