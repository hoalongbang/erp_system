// UI/Report/ReportManagementWidget.h
#ifndef UI_REPORT_REPORTMANAGEMENTWIDGET_H
#define UI_REPORT_REPORTMANAGEMENTWIDGET_H
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
#include "ReportService.h"         // Dịch vụ báo cáo
#include "ISecurityManager.h"      // Dịch vụ bảo mật
#include "Logger.h"                // Logging
#include "ErrorHandler.h"          // Xử lý lỗi
#include "Common.h"                // Các enum chung
#include "DateUtils.h"             // Xử lý ngày tháng
#include "StringUtils.h"           // Xử lý chuỗi
#include "CustomMessageBox.h"      // Hộp thoại thông báo tùy chỉnh
#include "Report.h"                // Report DTOs
#include "User.h"                  // User DTO (for display)

namespace ERP {
namespace UI {
namespace Report {

/**
 * @brief ReportManagementWidget class provides a UI for managing Report Requests.
 * This widget allows viewing, creating, updating, deleting, and changing request status.
 * It also supports viewing execution logs.
 */
class ReportManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for ReportManagementWidget.
     * @param reportService Shared pointer to IReportService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit ReportManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IReportService> reportService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~ReportManagementWidget();

private slots:
    void loadReportRequests();
    void onAddRequestClicked();
    void onEditRequestClicked();
    void onDeleteRequestClicked();
    void onUpdateRequestStatusClicked();
    void onSearchRequestClicked();
    void onRequestTableItemClicked(int row, int column);
    void clearForm();

    void onViewExecutionLogsClicked(); // New slot for viewing execution logs
    void onRunReportNowClicked(); // New slot for manual report execution

private:
    std::shared_ptr<Services::IReportService> reportService_;
    std::shared_ptr<ISecurityManager> securityManager_;
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
    QPushButton *viewExecutionLogsButton_;
    QPushButton *runReportNowButton_;

    // Form elements for editing/adding requests
    QLineEdit *idLineEdit_;
    QLineEdit *reportNameLineEdit_;
    QLineEdit *reportTypeLineEdit_;
    QComboBox *frequencyComboBox_; // ReportFrequency
    QComboBox *formatComboBox_; // ReportFormat
    QLineEdit *requestedByLineEdit_; // User ID, display name
    QDateTimeEdit *requestedTimeEdit_; // Read-only
    QLineEdit *outputPathLineEdit_;
    QLineEdit *scheduleCronExpressionLineEdit_;
    QLineEdit *emailRecipientsLineEdit_;
    QComboBox *statusComboBox_; // ReportExecutionStatus for the request (conceptual)

    // Helper functions
    void setupUI();
    void populateFrequencyComboBox();
    void populateFormatComboBox();
    void populateRequestStatusComboBox(); // For report request status (using execution status enum)
    void showRequestInputDialog(ERP::Report::DTO::ReportRequestDTO* request = nullptr);
    void showViewExecutionLogsDialog(ERP::Report::DTO::ReportRequestDTO* request);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Report
} // namespace UI
} // namespace ERP

#endif // UI_REPORT_REPORTMANAGEMENTWIDGET_H