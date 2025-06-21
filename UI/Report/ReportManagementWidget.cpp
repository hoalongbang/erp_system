// UI/Report/ReportManagementWidget.cpp
#include "ReportManagementWidget.h" // Standard includes
#include "Report.h"                 // Report DTOs
#include "User.h"                   // User DTO (for display)
#include "ReportService.h"          // Report Service
#include "ISecurityManager.h"       // Security Manager
#include "Logger.h"                 // Logging
#include "ErrorHandler.h"           // Error Handling
#include "Common.h"                 // Common Enums/Constants
#include "DateUtils.h"              // Date Utilities
#include "StringUtils.h"            // String Utilities
#include "CustomMessageBox.h"       // Custom Message Box
#include "UserService.h"            // For getting user names

#include <QInputDialog>
#include <QDateTime>
#include <QTableWidgetItem>
#include <QDialogButtonBox>

namespace ERP {
namespace UI {
namespace Report {

ReportManagementWidget::ReportManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IReportService> reportService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      reportService_(reportService),
      securityManager_(securityManager) {

    if (!reportService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ báo cáo hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("ReportManagementWidget: Initialized with null dependencies.");
        return;
    }

    auto authService = securityManager_->getAuthenticationService();
    if (authService) {
        std::string dummySessionId = "current_session_id"; // Placeholder
        std::optional<ERP::Security::DTO::SessionDTO> currentSession = authService->validateSession(dummySessionId);
        if (currentSession) {
            currentUserId_ = currentSession->userId;
            currentUserRoleIds_ = securityManager_->getUserService()->getUserRoles(currentUserId_, {});
        } else {
            currentUserId_ = "system_user";
            currentUserRoleIds_ = {"anonymous"};
            ERP::Logger::Logger::getInstance().warning("ReportManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("ReportManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadReportRequests();
    updateButtonsState();
}

ReportManagementWidget::~ReportManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void ReportManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên báo cáo...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &ReportManagementWidget::onSearchRequestClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    requestTable_ = new QTableWidget(this);
    requestTable_->setColumnCount(7); // ID, Tên báo cáo, Loại, Tần suất, Định dạng, Người yêu cầu, Ngày yêu cầu
    requestTable_->setHorizontalHeaderLabels({"ID", "Tên Báo cáo", "Loại", "Tần suất", "Định dạng", "Người YC", "Ngày YC"});
    requestTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    requestTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    requestTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    requestTable_->horizontalHeader()->setStretchLastSection(true);
    connect(requestTable_, &QTableWidget::itemClicked, this, &ReportManagementWidget::onRequestTableItemClicked);
    mainLayout->addWidget(requestTable_);

    // Form elements for editing/adding requests
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    reportNameLineEdit_ = new QLineEdit(this);
    reportTypeLineEdit_ = new QLineEdit(this);
    frequencyComboBox_ = new QComboBox(this); populateFrequencyComboBox();
    formatComboBox_ = new QComboBox(this); populateFormatComboBox();
    requestedByLineEdit_ = new QLineEdit(this); requestedByLineEdit_->setReadOnly(true); // Auto-filled
    requestedTimeEdit_ = new QDateTimeEdit(this); requestedTimeEdit_->setReadOnly(true); requestedTimeEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    outputPathLineEdit_ = new QLineEdit(this);
    scheduleCronExpressionLineEdit_ = new QLineEdit(this);
    emailRecipientsLineEdit_ = new QLineEdit(this);
    statusComboBox_ = new QComboBox(this); populateRequestStatusComboBox(); // Request status based on execution status

    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("Tên Báo cáo:*", &reportNameLineEdit_);
    formLayout->addRow("Loại Báo cáo:*", &reportTypeLineEdit_);
    formLayout->addRow("Tần suất:*", &frequencyComboBox_);
    formLayout->addRow("Định dạng:*", &formatComboBox_);
    formLayout->addRow("Người yêu cầu:", &requestedByLineEdit_);
    formLayout->addRow("Ngày yêu cầu:", &requestedTimeEdit_);
    formLayout->addRow("Đường dẫn đầu ra:", &outputPathLineEdit_);
    formLayout->addRow("Biểu thức Cron (tùy chỉnh):", &scheduleCronExpressionLineEdit_);
    formLayout->addRow("Email người nhận (cách nhau bởi dấu phẩy):", &emailRecipientsLineEdit_);
    formLayout->addRow("Trạng thái YC (conceptual):", &statusComboBox_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addRequestButton_ = new QPushButton("Thêm mới", this); connect(addRequestButton_, &QPushButton::clicked, this, &ReportManagementWidget::onAddRequestClicked);
    editRequestButton_ = new QPushButton("Sửa", this); connect(editRequestButton_, &QPushButton::clicked, this, &ReportManagementWidget::onEditRequestClicked);
    deleteRequestButton_ = new QPushButton("Xóa", this); connect(deleteRequestButton_, &QPushButton::clicked, this, &ReportManagementWidget::onDeleteRequestClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &ReportManagementWidget::onUpdateRequestStatusClicked);
    viewExecutionLogsButton_ = new QPushButton("Xem Nhật ký thực thi", this); connect(viewExecutionLogsButton_, &QPushButton::clicked, this, &ReportManagementWidget::onViewExecutionLogsClicked);
    runReportNowButton_ = new QPushButton("Chạy Báo cáo ngay", this); connect(runReportNowButton_, &QPushButton::clicked, this, &ReportManagementWidget::onRunReportNowClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &ReportManagementWidget::onSearchRequestClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &ReportManagementWidget::clearForm);
    
    buttonLayout->addWidget(addRequestButton_);
    buttonLayout->addWidget(editRequestButton_);
    buttonLayout->addWidget(deleteRequestButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(viewExecutionLogsButton_);
    buttonLayout->addWidget(runReportNowButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void ReportManagementWidget::loadReportRequests() {
    ERP::Logger::Logger::getInstance().info("ReportManagementWidget: Loading report requests...");
    requestTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Report::DTO::ReportRequestDTO> requests = reportService_->getAllReportRequests({}, currentUserId_, currentUserRoleIds_);

    requestTable_->setRowCount(requests.size());
    for (int i = 0; i < requests.size(); ++i) {
        const auto& request = requests[i];
        requestTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(request.id)));
        requestTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(request.reportName)));
        requestTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(request.reportType)));
        requestTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(request.getFrequencyString())));
        requestTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(request.getFormatString())));
        
        QString requestedByName = "N/A";
        std::optional<ERP::User::DTO::UserDTO> requestedByUser = securityManager_->getUserService()->getUserById(request.requestedByUserId, currentUserId_, currentUserRoleIds_);
        if (requestedByUser) requestedByName = QString::fromStdString(requestedByUser->username);
        requestTable_->setItem(i, 5, new QTableWidgetItem(requestedByName));

        requestTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(request.requestedTime, ERP::Common::DATETIME_FORMAT))));
    }
    requestTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ReportManagementWidget: Report requests loaded successfully.");
}

void ReportManagementWidget::populateFrequencyComboBox() {
    frequencyComboBox_->clear();
    frequencyComboBox_->addItem("Once", static_cast<int>(ERP::Report::DTO::ReportFrequency::ONCE));
    frequencyComboBox_->addItem("Hourly", static_cast<int>(ERP::Report::DTO::ReportFrequency::HOURLY));
    frequencyComboBox_->addItem("Daily", static_cast<int>(ERP::Report::DTO::ReportFrequency::DAILY));
    frequencyComboBox_->addItem("Weekly", static_cast<int>(ERP::Report::DTO::ReportFrequency::WEEKLY));
    frequencyComboBox_->addItem("Monthly", static_cast<int>(ERP::Report::DTO::ReportFrequency::MONTHLY));
    frequencyComboBox_->addItem("Quarterly", static_cast<int>(ERP::Report::DTO::ReportFrequency::QUARTERLY));
    frequencyComboBox_->addItem("Yearly", static_cast<int>(ERP::Report::DTO::ReportFrequency::YEARLY));
    frequencyComboBox_->addItem("Custom (Cron)", static_cast<int>(ERP::Report::DTO::ReportFrequency::CUSTOM));
}

void ReportManagementWidget::populateFormatComboBox() {
    formatComboBox_->clear();
    formatComboBox_->addItem("PDF", static_cast<int>(ERP::Report::DTO::ReportFormat::PDF));
    formatComboBox_->addItem("Excel", static_cast<int>(ERP::Report::DTO::ReportFormat::EXCEL));
    formatComboBox_->addItem("CSV", static_cast<int>(ERP::Report::DTO::ReportFormat::CSV));
    formatComboBox_->addItem("HTML", static_cast<int>(ERP::Report::DTO::ReportFormat::HTML));
    formatComboBox_->addItem("JSON", static_cast<int>(ERP::Report::DTO::ReportFormat::JSON));
}

void ReportManagementWidget::populateRequestStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Report::DTO::ReportExecutionStatus::PENDING));
    statusComboBox_->addItem("In Progress", static_cast<int>(ERP::Report::DTO::ReportExecutionStatus::IN_PROGRESS));
    statusComboBox_->addItem("Completed", static_cast<int>(ERP::Report::DTO::ReportExecutionStatus::COMPLETED));
    statusComboBox_->addItem("Failed", static_cast<int>(ERP::Report::DTO::ReportExecutionStatus::FAILED));
    statusComboBox_->addItem("Cancelled", static_cast<int>(ERP::Report::DTO::ReportExecutionStatus::CANCELLED));
}


void ReportManagementWidget::onAddRequestClicked() {
    if (!hasPermission("Report.CreateReportRequest")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm yêu cầu báo cáo.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateFrequencyComboBox();
    populateFormatComboBox();
    showRequestInputDialog();
}

void ReportManagementWidget::onEditRequestClicked() {
    if (!hasPermission("Report.UpdateReportRequest")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa yêu cầu báo cáo.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Yêu Cầu Báo Cáo", "Vui lòng chọn một yêu cầu báo cáo để sửa.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Report::DTO::ReportRequestDTO> requestOpt = reportService_->getReportRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (requestOpt) {
        populateFrequencyComboBox();
        populateFormatComboBox();
        showRequestInputDialog(&(*requestOpt));
    } else {
        showMessageBox("Sửa Yêu Cầu Báo Cáo", "Không tìm thấy yêu cầu báo cáo để sửa.", QMessageBox::Critical);
    }
}

void ReportManagementWidget::onDeleteRequestClicked() {
    if (!hasPermission("Report.DeleteReportRequest")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa yêu cầu báo cáo.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Yêu Cầu Báo Cáo", "Vui lòng chọn một yêu cầu báo cáo để xóa.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    QString reportName = requestTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Yêu Cầu Báo Cáo");
    confirmBox.setText("Bạn có chắc chắn muốn xóa yêu cầu báo cáo '" + reportName + "' (ID: " + requestId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (reportService_->deleteReportRequest(requestId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Yêu Cầu Báo Cáo", "Yêu cầu báo cáo đã được xóa thành công.", QMessageBox::Information);
            loadReportRequests();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa yêu cầu báo cáo. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void ReportManagementWidget::onUpdateRequestStatusClicked() {
    if (!hasPermission("Report.UpdateReportRequestStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái yêu cầu báo cáo.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một yêu cầu báo cáo để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Report::DTO::ReportRequestDTO> requestOpt = reportService_->getReportRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!requestOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy yêu cầu báo cáo để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Report::DTO::ReportRequestDTO currentRequest = *requestOpt;
    // Use a QInputDialog or custom dialog to get the new status
    QDialog statusDialog(this);
    statusDialog.setWindowTitle("Chọn Trạng Thái Mới");
    QVBoxLayout *layout = new QVBoxLayout(&statusDialog);
    QComboBox newStatusCombo;
    populateRequestStatusComboBox(); // Populate it
    for(int i = 0; i < statusComboBox_->count(); ++i) { // Copy items
        newStatusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));
    }
    // Set current status as default selected (based on currentReport.metadata["last_execution_status"])
    // This requires converting std::any back to QVariant for findData
    if (currentRequest.metadata.count("last_execution_status")) {
        if (currentRequest.metadata.at("last_execution_status").type() == typeid(int)) {
             int currentExecStatusInt = std::any_cast<int>(currentRequest.metadata.at("last_execution_status"));
             int currentStatusIndex = newStatusCombo.findData(currentExecStatusInt);
             if (currentStatusIndex != -1) newStatusCombo.setCurrentIndex(currentStatusIndex);
        }
    }


    layout->addWidget(new QLabel("Chọn trạng thái mới:", &statusDialog));
    layout->addWidget(&newStatusCombo);
    QPushButton *okButton = new QPushButton("Cập nhật", &statusDialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &statusDialog);
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(okButton);
    btnLayout->addWidget(cancelButton);
    layout->addLayout(btnLayout);

    connect(okButton, &QPushButton::clicked, &statusDialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (statusDialog.exec() == QDialog::Accepted) {
        ERP::Report::DTO::ReportExecutionStatus newStatus = static_cast<ERP::Report::DTO::ReportExecutionStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái yêu cầu báo cáo");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái yêu cầu báo cáo '" + QString::fromStdString(currentRequest.reportName) + "' thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (reportService_->updateReportRequestStatus(requestId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái yêu cầu báo cáo đã được cập nhật thành công.", QMessageBox::Information);
                loadReportRequests();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái yêu cầu báo cáo. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}

void ReportManagementWidget::onViewExecutionLogsClicked() {
    if (!hasPermission("Report.ViewReportExecutionLogs")) { // Assuming this permission
        showMessageBox("Lỗi", "Bạn không có quyền xem nhật ký thực thi báo cáo.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xem Nhật ký", "Vui lòng chọn một yêu cầu báo cáo để xem nhật ký thực thi.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Report::DTO::ReportRequestDTO> requestOpt = reportService_->getReportRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (requestOpt) {
        showViewExecutionLogsDialog(&(*requestOpt));
    } else {
        showMessageBox("Xem Nhật ký", "Không tìm thấy yêu cầu báo cáo để xem nhật ký thực thi.", QMessageBox::Critical);
    }
}

void ReportManagementWidget::onRunReportNowClicked() {
    if (!hasPermission("Report.RunReportNow")) { // Assuming this permission
        showMessageBox("Lỗi", "Bạn không có quyền chạy báo cáo ngay lập tức.", QMessageBox::Warning);
        return;
    }

    int selectedRow = requestTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Chạy Báo cáo", "Vui lòng chọn một yêu cầu báo cáo để chạy.", QMessageBox::Information);
        return;
    }

    QString requestId = requestTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Report::DTO::ReportRequestDTO> requestOpt = reportService_->getReportRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!requestOpt) {
        showMessageBox("Chạy Báo cáo", "Không tìm thấy yêu cầu báo cáo để chạy.", QMessageBox::Critical);
        return;
    }
    
    ERP::Report::DTO::ReportRequestDTO reportToRun = *requestOpt;

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Chạy Báo cáo ngay");
    confirmBox.setText("Bạn có chắc chắn muốn chạy báo cáo '" + QString::fromStdString(reportToRun.reportName) + "' ngay bây giờ không?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        // Here, you would ideally submit a task to the TaskEngine.
        // For simplicity, let's just record a "COMPLETED" execution log directly and update status.
        ERP::Report::DTO::ReportExecutionLogDTO newLog;
        newLog.id = ERP::Utils::generateUUID();
        newLog.reportRequestId = reportToRun.id;
        newLog.executionTime = ERP::Utils::DateUtils::now();
        newLog.status = ERP::Report::DTO::ReportExecutionStatus::COMPLETED;
        newLog.executedByUserId = currentUserId_;
        newLog.actualOutputPath = reportToRun.outputPath.value_or("N/A") + "_" + QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(newLog.executionTime, "%Y%m%d%H%M%S")).toStdString() + "." + QString::fromStdString(reportToRun.getFormatString()).toLower().toStdString();
        newLog.errorMessage = std::nullopt; // Success
        newLog.executionMetadata = reportToRun.parameters; // Copy parameters as execution context

        if (reportService_->createReportExecutionLog(newLog, currentUserId_, currentUserRoleIds_)) { // Assuming this method exists and calls DAO
            // Update the request status to reflect last execution
            reportService_->updateReportRequestStatus(reportToRun.id, newLog.status, currentUserId_, currentUserRoleIds_);
            showMessageBox("Chạy Báo cáo", "Báo cáo đã được chạy thành công. Xem nhật ký thực thi để biết chi tiết.", QMessageBox::Information);
            loadReportRequests(); // Refresh table
        } else {
            showMessageBox("Lỗi", "Không thể chạy báo cáo. Vui lòng kiểm tra log.", QMessageBox::Critical);
        }
    }
}


void ReportManagementWidget::onSearchRequestClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["name_contains"] = searchText.toStdString(); // Assuming service supports "contains" filter
    }
    requestTable_->setRowCount(0);
    std::vector<ERP::Report::DTO::ReportRequestDTO> requests = reportService_->getAllReportRequests(filter, currentUserId_, currentUserRoleIds_);

    requestTable_->setRowCount(requests.size());
    for (int i = 0; i < requests.size(); ++i) {
        const auto& request = requests[i];
        requestTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(request.id)));
        requestTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(request.reportName)));
        requestTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(request.reportType)));
        requestTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(request.getFrequencyString())));
        requestTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(request.getFormatString())));
        
        QString requestedByName = "N/A";
        std::optional<ERP::User::DTO::UserDTO> requestedByUser = securityManager_->getUserService()->getUserById(request.requestedByUserId, currentUserId_, currentUserRoleIds_);
        if (requestedByUser) requestedByName = QString::fromStdString(requestedByUser->username);
        requestTable_->setItem(i, 5, new QTableWidgetItem(requestedByName));

        requestTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(request.requestedTime, ERP::Common::DATETIME_FORMAT))));
    }
    requestTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ReportManagementWidget: Search completed.");
}

void ReportManagementWidget::onRequestTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString requestId = requestTable_->item(row, 0)->text();
    std::optional<ERP::Report::DTO::ReportRequestDTO> requestOpt = reportService_->getReportRequestById(requestId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (requestOpt) {
        idLineEdit_->setText(QString::fromStdString(requestOpt->id));
        reportNameLineEdit_->setText(QString::fromStdString(requestOpt->reportName));
        reportTypeLineEdit_->setText(QString::fromStdString(requestOpt->reportType));
        
        populateFrequencyComboBox();
        int freqIndex = frequencyComboBox_->findData(static_cast<int>(requestOpt->frequency));
        if (freqIndex != -1) frequencyComboBox_->setCurrentIndex(freqIndex);

        populateFormatComboBox();
        int formatIndex = formatComboBox_->findData(static_cast<int>(requestOpt->format));
        if (formatIndex != -1) formatComboBox_->setCurrentIndex(formatIndex);

        requestedByLineEdit_->setText(QString::fromStdString(requestOpt->requestedByUserId)); // Display ID
        requestedTimeEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(requestOpt->requestedTime.time_since_epoch()).count()));
        
        outputPathLineEdit_->setText(QString::fromStdString(requestOpt->outputPath.value_or("")));
        scheduleCronExpressionLineEdit_->setText(QString::fromStdString(requestOpt->scheduleCronExpression.value_or("")));
        emailRecipientsLineEdit_->setText(QString::fromStdString(requestOpt->emailRecipients.value_or("")));
        
        populateRequestStatusComboBox();
        // The statusComboBox here is for last execution status, stored in metadata
        if (requestOpt->metadata.count("last_execution_status")) {
            if (requestOpt->metadata.at("last_execution_status").type() == typeid(int)) {
                 int lastExecStatus = std::any_cast<int>(requestOpt->metadata.at("last_execution_status"));
                 int statusIndex = statusComboBox_->findData(lastExecStatus);
                 if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);
            }
        } else {
            statusComboBox_->setCurrentIndex(0); // Default to Pending
        }

    } else {
        showMessageBox("Thông tin Yêu Cầu Báo Cáo", "Không thể tải chi tiết yêu cầu báo cáo đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void ReportManagementWidget::clearForm() {
    idLineEdit_->clear();
    reportNameLineEdit_->clear();
    reportTypeLineEdit_->clear();
    frequencyComboBox_->setCurrentIndex(0);
    formatComboBox_->setCurrentIndex(0);
    requestedByLineEdit_->clear();
    requestedTimeEdit_->clear();
    outputPathLineEdit_->clear();
    scheduleCronExpressionLineEdit_->clear();
    emailRecipientsLineEdit_->clear();
    statusComboBox_->setCurrentIndex(0);
    requestTable_->clearSelection();
    updateButtonsState();
}

void ReportManagementWidget::showViewExecutionLogsDialog(ERP::Report::DTO::ReportRequestDTO* request) {
    if (!request) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Nhật ký thực thi cho báo cáo: " + QString::fromStdString(request->reportName));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *logsTable = new QTableWidget(&dialog);
    logsTable->setColumnCount(6); // Execution Time, Status, Executed By, Output Path, Error Message, Metadata
    logsTable->setHorizontalHeaderLabels({"Thời gian thực thi", "Trạng thái", "Thực hiện bởi", "Đường dẫn đầu ra", "Thông báo lỗi", "Metadata"});
    logsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    logsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    logsTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(logsTable);

    // Load execution logs
    std::vector<ERP::Report::DTO::ReportExecutionLogDTO> logs = reportService_->getReportExecutionLogsByRequestId(request->id, currentUserId_, currentUserRoleIds_);
    logsTable->setRowCount(logs.size());
    for (int i = 0; i < logs.size(); ++i) {
        const auto& log = logs[i];
        logsTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(log.executionTime, ERP::Common::DATETIME_FORMAT))));
        logsTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(log.getStatusString())));
        
        QString executedByName = "N/A";
        if (log.executedByUserId) {
            std::optional<ERP::User::DTO::UserDTO> executedByUser = securityManager_->getUserService()->getUserById(*log.executedByUserId, currentUserId_, currentUserRoleIds_);
            if (executedByUser) executedByName = QString::fromStdString(executedByUser->username);
        }
        logsTable->setItem(i, 2, new QTableWidgetItem(executedByName));

        logsTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(log.actualOutputPath.value_or(""))));
        logsTable->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(log.errorMessage.value_or(""))));
        logsTable->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DTOUtils::mapToJsonString(log.executionMetadata)))); // Convert map to JSON string for display
    }
    logsTable->resizeColumnsToContents();

    QPushButton *closeButton = new QPushButton("Đóng", &dialog);
    dialogLayout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}


void ReportManagementWidget::showRequestInputDialog(ERP::Report::DTO::ReportRequestDTO* request) {
    QDialog dialog(this);
    dialog.setWindowTitle(request ? "Sửa Yêu Cầu Báo Cáo" : "Thêm Yêu Cầu Báo Cáo Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit reportNameEdit(this);
    QLineEdit reportTypeEdit(this);
    QComboBox frequencyCombo(this); populateFrequencyComboBox();
    for(int i = 0; i < frequencyComboBox_->count(); ++i) frequencyCombo.addItem(frequencyComboBox_->itemText(i), frequencyComboBox_->itemData(i));
    QComboBox formatCombo(this); populateFormatComboBox();
    for(int i = 0; i < formatComboBox_->count(); ++i) formatCombo.addItem(formatComboBox_->itemText(i), formatComboBox_->itemData(i));
    
    QLineEdit outputPathEdit(this);
    QLineEdit scheduleCronExpressionEdit(this);
    QLineEdit emailRecipientsEdit(this);
    // Parameters need a more complex widget (e.g., QTableWidget for key-value pairs)
    // For simplicity, let's just show/edit parameters as a single JSON string if it was in metadata.
    QLineEdit parametersJsonEdit(this); // Assuming parameters are represented as a JSON string

    if (request) {
        reportNameEdit.setText(QString::fromStdString(request->reportName));
        reportTypeEdit.setText(QString::fromStdString(request->reportType));
        int freqIndex = frequencyCombo.findData(static_cast<int>(request->frequency));
        if (freqIndex != -1) frequencyCombo.setCurrentIndex(freqIndex);
        int formatIndex = formatCombo.findData(static_cast<int>(request->format));
        if (formatIndex != -1) formatCombo.setCurrentIndex(formatIndex);
        outputPathEdit.setText(QString::fromStdString(request->outputPath.value_or("")));
        scheduleCronExpressionEdit.setText(QString::fromStdString(request->scheduleCronExpression.value_or("")));
        emailRecipientsEdit.setText(QString::fromStdString(request->emailRecipients.value_or("")));
        parametersJsonEdit.setText(QString::fromStdString(ERP::Utils::DTOUtils::mapToJsonString(request->parameters)));
    } else {
        // Defaults for new
        parametersJsonEdit.setText("{}"); // Default empty JSON object
    }

    formLayout->addRow("Tên Báo cáo:*", &reportNameEdit);
    formLayout->addRow("Loại Báo cáo:*", &reportTypeEdit);
    formLayout->addRow("Tần suất:*", &frequencyCombo);
    formLayout->addRow("Định dạng:*", &formatCombo);
    formLayout->addRow("Đường dẫn đầu ra:", &outputPathEdit);
    formLayout->addRow("Biểu thức Cron (tùy chỉnh):", &scheduleCronExpressionEdit);
    formLayout->addRow("Email người nhận:", &emailRecipientsEdit);
    formLayout->addRow("Tham số (JSON):", &parametersJsonEdit);
    
    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(request ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Report::DTO::ReportRequestDTO newRequestData;
        if (request) {
            newRequestData = *request;
        } else {
            newRequestData.id = ERP::Utils::generateUUID(); // New ID for new request
        }

        newRequestData.reportName = reportNameEdit.text().toStdString();
        newRequestData.reportType = reportTypeEdit.text().toStdString();
        newRequestData.frequency = static_cast<ERP::Report::DTO::ReportFrequency>(frequencyCombo.currentData().toInt());
        newRequestData.format = static_cast<ERP::Report::DTO::ReportFormat>(formatCombo.currentData().toInt());
        newRequestData.outputPath = outputPathEdit.text().isEmpty() ? std::nullopt : std::make_optional(outputPathEdit.text().toStdString());
        newRequestData.scheduleCronExpression = scheduleCronExpressionEdit.text().isEmpty() ? std::nullopt : std::make_optional(scheduleCronExpressionEdit.text().toStdString());
        newRequestData.emailRecipients = emailRecipientsEdit.text().isEmpty() ? std::nullopt : std::make_optional(emailRecipientsEdit.text().toStdString());
        
        // Parse parameters JSON string to map
        newRequestData.parameters = ERP::Utils::DTOUtils::jsonStringToMap(parametersJsonEdit.text().toStdString());

        newRequestData.requestedByUserId = currentUserId_; // Set requested by current user
        newRequestData.requestedTime = ERP::Utils::DateUtils::now(); // Set current time
        newRequestData.status = ERP::Common::EntityStatus::ACTIVE; // Default active

        bool success = false;
        if (request) {
            success = reportService_->updateReportRequest(newRequestData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Yêu Cầu Báo Cáo", "Yêu cầu báo cáo đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật yêu cầu báo cáo. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Report::DTO::ReportRequestDTO> createdRequest = reportService_->createReportRequest(newRequestData, currentUserId_, currentUserRoleIds_);
            if (createdRequest) {
                showMessageBox("Thêm Yêu Cầu Báo Cáo", "Yêu cầu báo cáo mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm yêu cầu báo cáo mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadReportRequests();
            clearForm();
        }
    }
}


void ReportManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool ReportManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void ReportManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Report.CreateReportRequest");
    bool canUpdate = hasPermission("Report.UpdateReportRequest");
    bool canDelete = hasPermission("Report.DeleteReportRequest");
    bool canChangeStatus = hasPermission("Report.UpdateReportRequestStatus");
    bool canViewLogs = hasPermission("Report.ViewReportExecutionLogs");
    bool canRunNow = hasPermission("Report.RunReportNow");

    addRequestButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Report.ViewReportRequests"));

    bool isRowSelected = requestTable_->currentRow() >= 0;
    editRequestButton_->setEnabled(isRowSelected && canUpdate);
    deleteRequestButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    viewExecutionLogsButton_->setEnabled(isRowSelected && canViewLogs);
    runReportNowButton_->setEnabled(isRowSelected && canRunNow);


    bool enableForm = isRowSelected && canUpdate;
    reportNameLineEdit_->setEnabled(enableForm);
    reportTypeLineEdit_->setEnabled(enableForm);
    frequencyComboBox_->setEnabled(enableForm);
    formatComboBox_->setEnabled(enableForm);
    outputPathLineEdit_->setEnabled(enableForm);
    scheduleCronExpressionLineEdit_->setEnabled(enableForm);
    emailRecipientsLineEdit_->setEnabled(enableForm);
    // statusComboBox_ is read-only (reflects metadata)
    // parametersJsonEdit_ is also handled directly in showRequestInputDialog as QLineEdit.

    // Read-only fields
    idLineEdit_->setEnabled(false);
    requestedByLineEdit_->setEnabled(false);
    requestedTimeEdit_->setEnabled(false);

    if (!isRowSelected) {
        idLineEdit_->clear();
        reportNameLineEdit_->clear();
        reportTypeLineEdit_->clear();
        frequencyComboBox_->setCurrentIndex(0);
        formatComboBox_->setCurrentIndex(0);
        requestedByLineEdit_->clear();
        requestedTimeEdit_->clear();
        outputPathLineEdit_->clear();
        scheduleCronExpressionLineEdit_->clear();
        emailRecipientsLineEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
    }
}


} // namespace Report
} // namespace UI
} // namespace ERP