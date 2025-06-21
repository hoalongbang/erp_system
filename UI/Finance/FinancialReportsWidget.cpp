// UI/Finance/FinancialReportsWidget.cpp
#include "FinancialReportsWidget.h"
#include "GLAccountBalance.h" // For DTOs if needed for detailed balance processing
#include "JournalEntry.h"     // For DTOs if needed for detailed transaction processing
#include "UserService.h"      // For getting user roles

#include <QHeaderView> // For reportTable header stretching

namespace ERP {
namespace UI {
namespace Finance {

FinancialReportsWidget::FinancialReportsWidget(
    QWidget* parent,
    std::shared_ptr<ERP::Finance::Services::IGeneralLedgerService> generalLedgerService,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : QWidget(parent),
      generalLedgerService_(generalLedgerService),
      securityManager_(securityManager) {
    
    if (!generalLedgerService_ || !securityManager_) {
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "FinancialReportsWidget: Initialized with null service.", "Lỗi hệ thống: Một hoặc nhiều dịch vụ không khả dụng.");
        showMessageBox("Lỗi khởi tạo", "Không thể khởi tạo widget báo cáo tài chính do lỗi dịch vụ.", QMessageBox::Critical);
        return;
    }

    // Initialize currentUserId_ and currentUserRoleIds_ from securityManager
    // This is safer as these values are managed by the main application's security context.
    // Assuming a way to get the current logged-in user's ID from securityManager or a global context.
    // For now, let's use a dummy ID and roles if not explicitly set, or rely on MainWindow to set them.
    // In a real application, MainWindow would pass these or SecurityManager would provide a 'getCurrentUser()' like method.
    // For this UI widget, we pull it from SecurityManager's UserService.
    // This requires that SecurityManager is fully initialized and can provide UserService.
    auto authService = securityManager_->getAuthenticationService();
    if (authService) {
        // Attempt to get current user ID and roles from securityManager
        // This is a simplified approach; in a real app, user context would be passed securely.
        std::string dummySessionId = "current_session_id"; // Placeholder, a real session ID from login
        std::optional<ERP::Security::DTO::SessionDTO> currentSession = authService->validateSession(dummySessionId);
        if (currentSession) {
            currentUserId_ = currentSession->userId;
            currentUserRoleIds_ = securityManager_->getUserService()->getUserRoles(currentUserId_, {});
        } else {
            currentUserId_ = "system_user"; // Fallback for non-logged-in operations or system actions
            currentUserRoleIds_ = {"anonymous"}; // Fallback
            ERP::Logger::Logger::getInstance().warning("FinancialReportsWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("FinancialReportsWidget: Authentication Service not available. Running with limited privileges.");
    }
    
    setupUI();
}

void FinancialReportsWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // --- Controls Section ---
    QGridLayout* controlsLayout = new QGridLayout();
    controlsLayout->addWidget(new QLabel("Loại báo cáo:", this), 0, 0);
    reportTypeComboBox_ = new QComboBox(this);
    reportTypeComboBox_->addItem("Bảng cân đối thử (Trial Balance)", QVariant("TrialBalance"));
    reportTypeComboBox_->addItem("Bảng cân đối kế toán (Balance Sheet)", QVariant("BalanceSheet"));
    reportTypeComboBox_->addItem("Báo cáo kết quả hoạt động kinh doanh (Income Statement)", QVariant("IncomeStatement"));
    reportTypeComboBox_->addItem("Báo cáo lưu chuyển tiền tệ (Cash Flow Statement)", QVariant("CashFlowStatement"));
    controlsLayout->addWidget(reportTypeComboBox_, 0, 1);

    controlsLayout->addWidget(new QLabel("Ngày bắt đầu:", this), 1, 0);
    startDateEdit_ = new QDateEdit(QDate::currentDate().addYears(-1), this); // Default to 1 year ago
    startDateEdit_->setCalendarPopup(true);
    startDateEdit_->setDisplayFormat("yyyy-MM-dd");
    controlsLayout->addWidget(startDateEdit_, 1, 1);

    controlsLayout->addWidget(new QLabel("Ngày kết thúc:", this), 2, 0);
    endDateEdit_ = new QDateEdit(QDate::currentDate(), this); // Default to today
    endDateEdit_->setCalendarPopup(true);
    endDateEdit_->setDisplayFormat("yyyy-MM-dd");
    controlsLayout->addWidget(endDateEdit_, 2, 1);

    controlsLayout->addWidget(new QLabel("Ngày đến:", this), 3, 0);
    asOfDateEdit_ = new QDateEdit(QDate::currentDate(), this); // Default to today for Balance Sheet
    asOfDateEdit_->setCalendarPopup(true);
    asOfDateEdit_->setDisplayFormat("yyyy-MM-dd");
    controlsLayout->addWidget(asOfDateEdit_, 3, 1);

    generateReportButton_ = new QPushButton("Tạo báo cáo", this);
    controlsLayout->addWidget(generateReportButton_, 4, 1);

    mainLayout->addLayout(controlsLayout);

    // --- Report Display Section ---
    reportTitleLabel_ = new QLabel("<h3>Báo cáo tài chính</h3>", this);
    reportTitleLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(reportTitleLabel_);

    reportTable_ = new QTableWidget(this);
    reportTable_->horizontalHeader()->setStretchLastSection(true);
    reportTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    reportTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(reportTable_);

    // Connect signals and slots
    connect(reportTypeComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FinancialReportsWidget::updateDateControlsVisibility);
    connect(generateReportButton_, &QPushButton::clicked, this, &FinancialReportsWidget::generateReport);

    // Initial visibility update
    updateDateControlsVisibility();
}

void FinancialReportsWidget::updateDateControlsVisibility() {
    QString selectedReportType = reportTypeComboBox_->currentData().toString();
    // Use QLabels for direct access, as qobject_cast on layout item might be tricky
    // Find labels by iterating layout or create them as member variables.
    // For simplicity, directly set visibility of QDateEdit widgets and assume labels are handled together
    // This would ideally require more complex layout management to hide/show labels cleanly.

    // A simpler way to manage labels is to make them member variables as well, or assign them names.
    // For now, let's directly manage the QDateEdit visibility as the main concern.

    bool isPeriodReport = (selectedReportType == "TrialBalance" || selectedReportType == "IncomeStatement" || selectedReportType == "CashFlowStatement");
    
    startDateEdit_->setVisible(isPeriodReport);
    endDateEdit_->setVisible(isPeriodReport);
    asOfDateEdit_->setVisible(!isPeriodReport);

    // If labels were dynamic:
    // QLayoutItem* item1 = controlsLayout->itemAtPosition(1,0); if(item1 && item1->widget()) item1->widget()->setVisible(isPeriodReport);
    // QLayoutItem* item2 = controlsLayout->itemAtPosition(2,0); if(item2 && item2->widget()) item2->widget()->setVisible(isPeriodReport);
    // QLayoutItem* item3 = controlsLayout->itemAtPosition(3,0); if(item3 && item3->widget()) item3->widget()->setVisible(!isPeriodReport);
}

void FinancialReportsWidget::generateReport() {
    QString selectedReportType = reportTypeComboBox_->currentData().toString();
    reportTable_->setRowCount(0);
    reportTable_->setColumnCount(0);

    // Check permissions
    std::string requiredPermission = "";
    if (selectedReportType == "TrialBalance") requiredPermission = "Finance.ViewTrialBalance";
    else if (selectedReportType == "BalanceSheet") requiredPermission = "Finance.ViewBalanceSheet";
    else if (selectedReportType == "IncomeStatement") requiredPermission = "Finance.ViewIncomeStatement";
    else if (selectedReportType == "CashFlowStatement") requiredPermission = "Finance.ViewCashFlowStatement";

    if (!securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, requiredPermission)) {
        showMessageBox("Lỗi quyền", "Bạn không có quyền xem báo cáo '" + reportTypeComboBox_->currentText() + "'.", QMessageBox::Warning);
        return;
    }

    try {
        if (selectedReportType == "TrialBalance") {
            reportTitleLabel_->setText("<h3>Bảng cân đối thử</h3>");
            reportTable_->setColumnCount(2);
            reportTable_->setHorizontalHeaderLabels({"Tài khoản", "Số dư ròng"});

            std::chrono::system_clock::time_point startDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(startDateEdit_->dateTime());
            std::chrono::system_clock::time_point endDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(endDateEdit_->dateTime());
            
            std::map<std::string, double> reportData = generalLedgerService_->generateTrialBalance(startDate, endDate, currentUserRoleIds_);
            reportTable_->setRowCount(reportData.size());
            int row = 0;
            for (const auto& pair : reportData) {
                reportTable_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(pair.first)));
                reportTable_->setItem(row, 1, new QTableWidgetItem(QString::number(pair.second, 'f', 2)));
                row++;
            }
        } else if (selectedReportType == "BalanceSheet") {
            reportTitleLabel_->setText("<h3>Bảng cân đối kế toán</h3>");
            reportTable_->setColumnCount(2);
            reportTable_->setHorizontalHeaderLabels({"Khoản mục", "Số tiền"});

            std::chrono::system_clock::time_point asOfDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(asOfDateEdit_->dateTime());
            
            std::map<std::string, double> reportData = generalLedgerService_->generateBalanceSheet(asOfDate, currentUserRoleIds_);
            reportTable_->setRowCount(reportData.size());
            int row = 0;
            for (const auto& pair : reportData) {
                reportTable_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(pair.first)));
                reportTable_->setItem(row, 1, new QTableWidgetItem(QString::number(pair.second, 'f', 2)));
                row++;
            }
        } else if (selectedReportType == "IncomeStatement") {
            reportTitleLabel_->setText("<h3>Báo cáo kết quả hoạt động kinh doanh</h3>");
            reportTable_->setColumnCount(2);
            reportTable_->setHorizontalHeaderLabels({"Khoản mục", "Số tiền"});

            std::chrono::system_clock::time_point startDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(startDateEdit_->dateTime());
            std::chrono::system_clock::time_point endDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(endDateEdit_->dateTime());
            
            std::map<std::string, double> reportData = generalLedgerService_->generateIncomeStatement(startDate, endDate, currentUserRoleIds_);
            reportTable_->setRowCount(reportData.size());
            int row = 0;
            for (const auto& pair : reportData) {
                reportTable_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(pair.first)));
                reportTable_->setItem(row, 1, new QTableWidgetItem(QString::number(pair.second, 'f', 2)));
                row++;
            }
        } else if (selectedReportType == "CashFlowStatement") {
            reportTitleLabel_->setText("<h3>Báo cáo lưu chuyển tiền tệ</h3>");
            reportTable_->setColumnCount(2);
            reportTable_->setHorizontalHeaderLabels({"Hoạt động", "Số tiền"});

            std::chrono::system_clock::time_point startDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(startDateEdit_->dateTime());
            std::chrono::system_clock::time_point endDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(endDateEdit_->dateTime());
            
            std::map<std::string, double> reportData = generalLedgerService_->generateCashFlowStatement(startDate, endDate, currentUserRoleIds_);
            reportTable_->setRowCount(reportData.size());
            int row = 0;
            for (const auto& pair : reportData) {
                reportTable_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(pair.first)));
                reportTable_->setItem(row, 1, new QTableWidgetItem(QString::number(pair.second, 'f', 2)));
                row++;
            }
        }
        reportTable_->resizeColumnsToContents();
        ERP::Logger::Logger::getInstance().info("UI: Report '" + selectedReportType.toStdString() + "' generated successfully.");
    } catch (const std::exception& e) {
        showMessageBox("Lỗi tạo báo cáo", "Đã xảy ra lỗi khi tạo báo cáo: " + QString::fromStdString(e.what()) + ". Vui lòng kiểm tra log để biết chi tiết.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().error("UI: Exception during report generation: " + std::string(e.what()));
    }
}

void FinancialReportsWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

} // namespace Finance
} // namespace UI
} // namespace ERP