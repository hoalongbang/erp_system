// UI/Integration/ExternalSystemManagementWidget.h
#ifndef UI_INTEGRATION_EXTERNALSYSTEMMANAGEMENTWIDGET_H
#define UI_INTEGRATION_EXTERNALSYSTEMMANAGEMENTWIDGET_H
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
#include <QCheckBox>
#include <QTextEdit> // For API Parameters/Metadata

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "ExternalSystemService.h"    // Dịch vụ hệ thống bên ngoài
#include "ISecurityManager.h"         // Dịch vụ bảo mật
#include "Logger.h"                   // Logging
#include "ErrorHandler.h"             // Xử lý lỗi
#include "Common.h"                   // Các enum chung
#include "DateUtils.h"                // Xử lý ngày tháng
#include "StringUtils.h"              // Xử lý chuỗi
#include "CustomMessageBox.h"         // Hộp thoại thông báo tùy chỉnh
#include "IntegrationConfig.h"        // IntegrationConfig DTO
#include "APIEndpoint.h"              // APIEndpoint DTO


namespace ERP {
namespace UI {
namespace Integration {

/**
 * @brief ExternalSystemManagementWidget class provides a UI for managing External System Integrations.
 * This widget allows viewing, creating, updating, deleting configurations,
 * and managing associated API Endpoints. It also supports sending test data.
 */
class ExternalSystemManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for ExternalSystemManagementWidget.
     * @param externalSystemService Shared pointer to IExternalSystemService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit ExternalSystemManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IExternalSystemService> externalSystemService = nullptr,
        std::shared_ptr<ISecurityManager> securityManager = nullptr);

    ~ExternalSystemManagementWidget();

private slots:
    void loadIntegrationConfigs();
    void onCreateConfigClicked();
    void onEditConfigClicked();
    void onDeleteConfigClicked();
    void onUpdateConfigStatusClicked();
    void onSearchConfigClicked();
    void onConfigTableItemClicked(int row, int column);
    void clearForm();

    void onManageAPIEndpointsClicked(); // New slot for managing API endpoints
    void onSendDataClicked(); // New slot for sending test data

private:
    std::shared_ptr<Services::IExternalSystemService> externalSystemService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *configTable_;
    QPushButton *createConfigButton_;
    QPushButton *editConfigButton_;
    QPushButton *deleteConfigButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;
    QPushButton *manageAPIEndpointsButton_;
    QPushButton *sendDataButton_;

    // Form elements for editing/adding configs
    QLineEdit *idLineEdit_;
    QLineEdit *systemNameLineEdit_;
    QLineEdit *systemCodeLineEdit_;
    QComboBox *integrationTypeComboBox_; // IntegrationType
    QLineEdit *baseUrlLineEdit_;
    QLineEdit *usernameLineEdit_;
    QLineEdit *passwordLineEdit_; // Password input (securely)
    QCheckBox *isEncryptedCheckBox_; // If password/keys are encrypted
    QTextEdit *metadataTextEdit_; // For additional JSON metadata
    QComboBox *statusComboBox_; // EntityStatus

    // Helper functions
    void setupUI();
    void populateIntegrationTypeComboBox();
    void populateStatusComboBox(); // For entity status
    void showConfigInputDialog(ERP::Integration::DTO::IntegrationConfigDTO* config = nullptr);
    void showManageAPIEndpointsDialog(ERP::Integration::DTO::IntegrationConfigDTO* config);
    void showSendDataDialog(ERP::Integration::DTO::IntegrationConfigDTO* config);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Integration
} // namespace UI
} // namespace ERP

#endif // UI_INTEGRATION_EXTERNALSYSTEMMANAGEMENTWIDGET_H