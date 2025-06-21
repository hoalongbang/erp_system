// UI/Security/AuditLogViewerWidget.h
#ifndef UI_SECURITY_AUDITLOGVIEWERWIDGET_H
#define UI_SECURITY_AUDITLOGVIEWERWIDGET_H
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
#include <QTextEdit> // For displaying large log output/JSON

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "AuditLogService.h" // Dịch vụ nhật ký kiểm toán
#include "UserService.h"     // Dịch vụ người dùng (để hiển thị tên người dùng)
#include "ISecurityManager.h"  // Dịch vụ bảo mật
#include "Logger.h"            // Logging
#include "ErrorHandler.h"    // Xử lý lỗi
#include "Common.h"          // Các enum chung
#include "DateUtils.h"       // Xử lý ngày tháng
#include "StringUtils.h"     // Xử lý chuỗi
#include "CustomMessageBox.h" // Hộp thoại thông báo tùy chỉnh
#include "AuditLog.h"        // AuditLog DTO
#include "User.h"            // User DTO (for display)

namespace ERP {
namespace UI {
namespace Security {

/**
 * @brief AuditLogViewerWidget class provides a UI for viewing Audit Logs.
 * This widget allows filtering, viewing details, and potentially managing (e.g., archiving) logs.
 */
class AuditLogViewerWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for AuditLogViewerWidget.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit AuditLogViewerWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IAuditLogService> auditLogService = nullptr,
        std::shared_ptr<ISecurityManager> securityManager = nullptr);

    ~AuditLogViewerWidget();

private slots:
    void loadAuditLogs();
    void onSearchLogClicked();
    void onLogTableItemClicked(int row, int column);
    void clearForm();
    void onExportLogsClicked(); // New slot for exporting logs (optional)

private:
    std::shared_ptr<Services::IAuditLogService> auditLogService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *logTable_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;
    QPushButton *exportLogsButton_;

    // Form elements for displaying log details (read-only)
    QLineEdit *idLineEdit_;
    QLineEdit *userIdLineEdit_;
    QLineEdit *userNameLineEdit_;
    QLineEdit *sessionIdLineEdit_;
    QComboBox *actionTypeComboBox_; // AuditActionType
    QComboBox *severityComboBox_; // LogSeverity
    QLineEdit *moduleLineEdit_;
    QLineEdit *subModuleLineEdit_;
    QLineEdit *entityIdLineEdit_;
    QLineEdit *entityTypeLineEdit_;
    QLineEdit *entityNameLineEdit_;
    QLineEdit *ipAddressLineEdit_;
    QLineEdit *userAgentLineEdit_;
    QLineEdit *workstationIdLineEdit_;
    QDateTimeEdit *createdAtEdit_;
    QTextEdit *beforeDataTextEdit_; // JSON text
    QTextEdit *afterDataTextEdit_; // JSON text
    QLineEdit *changeReasonLineEdit_;
    QTextEdit *metadataTextEdit_; // JSON text
    QLineEdit *commentsLineEdit_;
    QLineEdit *approvalIdLineEdit_;
    QCheckBox *isCompliantCheckBox_;
    QLineEdit *complianceNoteLineEdit_;

    // Helper functions
    void setupUI();
    void populateActionTypeComboBox();
    void populateSeverityComboBox();
    void showLogDetailsDialog(ERP::Security::DTO::AuditLogDTO* log);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Security
} // namespace UI
} // namespace ERP

#endif // UI_SECURITY_AUDITLOGVIEWERWIDGET_H