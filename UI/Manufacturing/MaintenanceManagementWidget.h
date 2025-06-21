// UI/Manufacturing/MaintenanceManagementWidget.h
#ifndef UI_MANUFACTURING_MAINTENANCEMANAGEMENTWIDGET_H
#define UI_MANUFACTURING_MAINTENANCEMANAGEMENTWIDGET_H
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
#include "MaintenanceManagementService.h" // Dịch vụ bảo trì
#include "AssetManagementService.h"       // Dịch vụ tài sản
#include "ISecurityManager.h"           // Dịch vụ bảo mật
#include "Logger.h"                     // Logging
#include "ErrorHandler.h"               // Xử lý lỗi
#include "Common.h"                     // Các enum chung
#include "DateUtils.h"                  // Xử lý ngày tháng
#include "StringUtils.h"                // Xử lý chuỗi
#include "CustomMessageBox.h"           // Hộp thoại thông báo tùy chỉnh
#include "MaintenanceManagement.h"      // DTOs bảo trì
#include "Asset.h"                      // Asset DTO (for display)

namespace ERP {
namespace UI {
namespace Manufacturing {

/**
 * @brief MaintenanceManagementWidget class provides a UI for managing Maintenance Requests and Activities.
 * This widget allows viewing, creating, updating, deleting, and changing request status.
 * It also supports recording maintenance activities.
 */
class MaintenanceManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for MaintenanceManagementWidget.
     * @param maintenanceService Shared pointer to IMaintenanceManagementService.
     * @param assetService Shared pointer to IAssetManagementService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit MaintenanceManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IMaintenanceManagementService> maintenanceService = nullptr,
        std::shared_ptr<Asset::Services::IAssetManagementService> assetService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~MaintenanceManagementWidget();

private slots:
    void loadMaintenanceRequests();
    void onAddRequestClicked();
    void onEditRequestClicked();
    void onDeleteRequestClicked();
    void onUpdateRequestStatusClicked();
    void onSearchRequestClicked();
    void onRequestTableItemClicked(int row, int column);
    void clearForm();

    void onRecordActivityClicked(); // New slot for recording maintenance activities
    void onViewActivitiesClicked(); // New slot for viewing activities

private:
    std::shared_ptr<Services::IMaintenanceManagementService> maintenanceService_;
    std::shared_ptr<Asset::Services::IAssetManagementService> assetService_;
    std::shared_ptr<Security::ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *requestTable_;
    QPushButton *addRequestButton_;
    QPushButton *editRequestButton_;
    QPushButton *deleteRequestButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;
    QPushButton *recordActivityButton_;
    QPushButton *viewActivitiesButton_;

    // Form elements for editing/adding requests
    QLineEdit *idLineEdit_;
    QComboBox *assetComboBox_;
    QComboBox *requestTypeComboBox_;
    QComboBox *priorityComboBox_;
    QComboBox *statusComboBox_; // Request Status
    QLineEdit *descriptionLineEdit_;
    QLineEdit *requestedByLineEdit_; // User ID, display name
    QDateTimeEdit *requestedDateEdit_;
    QDateTimeEdit *scheduledDateEdit_;
    QLineEdit *assignedToLineEdit_; // User ID, display name
    QLineEdit *failureReasonLineEdit_;

    // Helper functions
    void setupUI();
    void populateAssetComboBox();
    void populateRequestTypeComboBox();
    void populatePriorityComboBox();
    void populateRequestStatusComboBox();
    void populateUserComboBox(QComboBox* comboBox); // Helper to populate combo with users
    void showRequestInputDialog(ERP::Manufacturing::DTO::MaintenanceRequestDTO* request = nullptr);
    void showRecordActivityDialog(ERP::Manufacturing::DTO::MaintenanceRequestDTO* request);
    void showViewActivitiesDialog(ERP::Manufacturing::DTO::MaintenanceRequestDTO* request);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Manufacturing
} // namespace UI
} // namespace ERP

#endif // UI_MANUFACTURING_MAINTENANCEMANAGEMENTWIDGET_H