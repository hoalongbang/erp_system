// UI/loginform.cpp
#include "loginform.h" // Đã rút gọn include
#include "./ui_loginform.h"
#include "Logger.h" // Đã rút gọn include
#include "ErrorHandler.h" // Đã rút gọn include
#include "IAuthenticationService.h" // Đã rút gọn include
#include "IUserService.h" // Đã rút gọn include

#include <QHostInfo> // For IP address
#include <QNetworkInterface> // For IP address
#include <QMessageBox> // For QMessageBox

namespace ERP {
namespace UI {

LoginForm::LoginForm(QWidget *parent,
                     std::shared_ptr<ERP::Security::Service::IAuthenticationService> authenticationService,
                     std::shared_ptr<ERP::User::Services::IUserService> userService)
    : QWidget(parent),
      ui(new Ui::LoginForm),
      authenticationService_(authenticationService),
      userService_(userService) {
    ui->setupUi(this);

    if (!authenticationService_ || !userService_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ xác thực hoặc dịch vụ người dùng không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("LoginForm: Initialized with null authenticationService or userService.");
        // Consider disabling login functionality or exiting gracefully
    }
}

LoginForm::~LoginForm() {
    delete ui;
}

void LoginForm::on_loginButton_clicked() {
    if (!authenticationService_ || !userService_) {
        showMessageBox("Lỗi", "Dịch vụ xác thực hoặc dịch vụ người dùng không khả dụng.", QMessageBox::Critical);
        return;
    }

    QString username = ui->usernameLineEdit->text();
    QString password = ui->passwordLineEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        showMessageBox("Lỗi Đăng Nhập", "Vui lòng nhập tên đăng nhập và mật khẩu.", QMessageBox::Warning);
        return;
    }

    // Get client IP address and user agent (example placeholder)
    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // find non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
            ipAddressesList.at(i).toIPv4Address() != 0) {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }
    if (ipAddress.isEmpty()) {
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
    }
    
    QString userAgent = "DesktopApp/1.0 (Qt)"; // Example user agent
    QString deviceInfo = "OS: " + QSysInfo::prettyProductName() + "; Arch: " + QSysInfo::currentCpuArchitecture();

    ERP::Logger::Logger::getInstance().info("LoginForm: Attempting login for user: " + username.toStdString());

    std::optional<ERP::Security::DTO::SessionDTO> session =
        authenticationService_->authenticate(
            username.toStdString(),
            password.toStdString(),
            ipAddress.toStdString(),
            userAgent.toStdString(),
            deviceInfo.toStdString()
        );

    if (session) {
        showMessageBox("Đăng Nhập Thành Công", "Chào mừng, " + username + "!", QMessageBox::Information);
        ui->usernameLineEdit->clear();
        ui->passwordLineEdit->clear();
        emit loginSuccess(username, QString::fromStdString(session->userId), QString::fromStdString(session->id));
    } else {
        // ErrorHandler already handles showing specific error messages
        // if (ERP::ErrorHandler::getLastErrorMessage()) {
        //    showMessageBox("Lỗi Đăng Nhập", QString::fromStdString(*ERP::ErrorHandler::getLastErrorMessage()), QMessageBox::Critical);
        // } else {
        //    showMessageBox("Lỗi Đăng Nhập", "Tên đăng nhập hoặc mật khẩu không đúng.", QMessageBox::Critical);
        // }
        // The ErrorHandler should implicitly show the message box if configured globally.
        // If not, explicitly show here:
        QString errorMessage = "Tên đăng nhập hoặc mật khẩu không đúng.";
        if (ERP::ErrorHandling::ErrorHandler::getLastUserMessage().has_value()) {
            errorMessage = QString::fromStdString(*ERP::ErrorHandling::ErrorHandler::getLastUserMessage());
        }
        showMessageBox("Lỗi Đăng Nhập", errorMessage, QMessageBox::Critical);
    }
}

void LoginForm::on_registerButton_clicked() {
    emit registerRequested();
}

void LoginForm::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

} // namespace UI
} // namespace ERP