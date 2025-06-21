// UI/registerform.h
#ifndef UI_REGISTERFORM_H
#define UI_REGISTERFORM_H
#include <QWidget>
#include <QMessageBox> // For QMessageBox

// Rút gọn includes
#include "IUserService.h"     // Interface for UserService
#include "User.h"             // For UserDTO
#include "CustomMessageBox.h" // Custom message box implementation

QT_BEGIN_NAMESPACE
namespace Ui { class RegisterForm; }
QT_END_NAMESPACE

namespace ERP {
namespace UI {
/**
 * @brief The RegisterForm class provides the user interface for new user registration.
 * It interacts with the UserService to create new user accounts.
 */
class RegisterForm : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for RegisterForm.
     * @param parent Parent widget.
     * @param userService Shared pointer to IUserService.
     */
    explicit RegisterForm(QWidget *parent = nullptr,
                          std::shared_ptr<ERP::User::Services::IUserService> userService = nullptr);
    ~RegisterForm();

signals:
    void backToLoginRequested();

private slots:
    void on_registerButton_clicked();
    void on_backToLoginButton_clicked();

private:
    Ui::RegisterForm *ui;
    std::shared_ptr<ERP::User::Services::IUserService> userService_;

    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
};
} // namespace UI
} // namespace ERP
#endif // UI_REGISTERFORM_H