// UI/Integration/DeviceManagementWidget.h
#ifndef UI_INTEGRATION_DEVICEMANAGEMENTWIDGET_H
#define UI_INTEGRATION_DEVICEMANAGEMENTWIDGET_H
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
#include "DeviceManagerService.h" // Dịch vụ quản lý thiết bị
#include "ISecurityManager.h"     // Dịch vụ bảo mật
#include "Logger.h"               // Logging
#include "ErrorHandler.h"         // Xử lý lỗi
#include "Common.h"               // Các enum chung
#include "DateUtils.h"            // Xử lý ngày tháng
#include "StringUtils.h"          // Xử lý chuỗi
#include "CustomMessageBox.h"     // Hộp thoại thông báo tùy chỉnh
#include "DeviceConfig.h"         // DeviceConfig DTO
#include "DeviceEventLog.h"       // DeviceEventLog DTO (for viewing events)


namespace ERP {
namespace UI {
namespace Integration {

/**
 * @brief DeviceManagementWidget class provides a UI for managing Device Configurations.
 * This widget allows viewing, registering, updating, deleting device configurations,
 * and managing their connection status. It also supports viewing device event logs.
 */
class DeviceManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for DeviceManagementWidget.
     * @param deviceManagerService Shared pointer to IDeviceManagerService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit DeviceManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IDeviceManagerService> deviceManagerService = nullptr,
        std::shared_ptr<ISecurityManager> securityManager = nullptr);

    ~DeviceManagementWidget();

private slots:
    void loadDeviceConfigs();
    void onRegisterDeviceClicked();
    void onEditDeviceConfigClicked();
    void onDeleteDeviceConfigClicked();
    void onUpdateConnectionStatusClicked();
    void onSearchDeviceConfigClicked();
    void onDeviceConfigTableItemClicked(int row, int column);
    void clearForm();

    void onViewEventLogsClicked(); // New slot for viewing event logs
    void onRecordDeviceEventClicked(); // New slot for manually recording an event

private:
    std::shared_ptr<Services::IDeviceManagerService> deviceManagerService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *deviceConfigTable_;
    QPushButton *registerDeviceButton_;
    QPushButton *editDeviceConfigButton_;
    QPushButton *deleteDeviceConfigButton_;
    QPushButton *updateConnectionStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;
    QPushButton *viewEventLogsButton_;
    QPushButton *recordDeviceEventButton_;

    // Form elements for editing/adding configs
    QLineEdit *idLineEdit_;
    QLineEdit *deviceNameLineEdit_;
    QLineEdit *deviceIdentifierLineEdit_;
    QComboBox *deviceTypeComboBox_; // DeviceType
    QLineEdit *connectionStringLineEdit_;
    QLineEdit *ipAddressLineEdit_;
    QComboBox *connectionStatusComboBox_; // ConnectionStatus
    QLineEdit *locationIdLineEdit_; // Link to Catalog/Location
    QLineEdit *notesLineEdit_;
    QCheckBox *isCriticalCheckBox_;

    // Helper functions
    void setupUI();
    void populateDeviceTypeComboBox();
    void populateConnectionStatusComboBox();
    void populateLocationComboBox(QComboBox* comboBox); // Helper to populate combo with locations
    void showDeviceConfigInputDialog(ERP::Integration::DTO::DeviceConfigDTO* config = nullptr);
    void showUpdateConnectionStatusDialog(ERP::Integration::DTO::DeviceConfigDTO* config);
    void showRecordDeviceEventDialog(ERP::Integration::DTO::DeviceConfigDTO* config);
    void showViewEventLogsDialog(ERP::Integration::DTO::DeviceConfigDTO* config);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Integration
} // namespace UI
} // namespace ERP

#endif // UI_INTEGRATION_DEVICEMANAGEMENTWIDGET_H