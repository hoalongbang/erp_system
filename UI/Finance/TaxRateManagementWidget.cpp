// UI/Finance/TaxRateManagementWidget.cpp
#include "TaxRateManagementWidget.h" // Standard includes
#include "TaxRate.h"                 // TaxRate DTO
#include "TaxService.h"              // Tax Service
#include "ISecurityManager.h"        // Security Manager
#include "Logger.h"                  // Logging
#include "ErrorHandler.h"            // Error Handling
#include "Common.h"                  // Common Enums/Constants
#include "DateUtils.h"               // Date Utilities
#include "StringUtils.h"             // String Utilities
#include "CustomMessageBox.h"        // Custom Message Box
#include "UserService.h"             // For getting user names

#include <QInputDialog>
#include <QDoubleValidator>
#include <QDateTime>

namespace ERP {
namespace UI {
namespace Finance {

TaxRateManagementWidget::TaxRateManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::ITaxService> taxService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      taxService_(taxService),
      securityManager_(securityManager) {

    if (!taxService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ thuế hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("TaxRateManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("TaxRateManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("TaxRateManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadTaxRates();
    updateButtonsState();
}

TaxRateManagementWidget::~TaxRateManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void TaxRateManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên thuế suất...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &TaxRateManagementWidget::onSearchTaxRateClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    taxRateTable_ = new QTableWidget(this);
    taxRateTable_->setColumnCount(5); // ID, Name, Rate, Effective Date, Expiration Date, Status
    taxRateTable_->setHorizontalHeaderLabels({"ID", "Tên Thuế suất", "Thuế suất (%)", "Ngày Hiệu lực", "Ngày Hết hạn", "Trạng thái"});
    taxRateTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    taxRateTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    taxRateTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    taxRateTable_->horizontalHeader()->setStretchLastSection(true);
    connect(taxRateTable_, &QTableWidget::itemClicked, this, &TaxRateManagementWidget::onTaxRateTableItemClicked);
    mainLayout->addWidget(taxRateTable_);

    // Form elements for editing/adding
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    nameLineEdit_ = new QLineEdit(this);
    rateLineEdit_ = new QLineEdit(this); rateLineEdit_->setValidator(new QDoubleValidator(0.0, 100.0, 2, this)); // 0-100%
    descriptionLineEdit_ = new QLineEdit(this);
    effectiveDateEdit_ = new QDateTimeEdit(this); effectiveDateEdit_->setDisplayFormat("yyyy-MM-dd"); effectiveDateEdit_->setCalendarPopup(true);
    expirationDateEdit_ = new QDateTimeEdit(this); expirationDateEdit_->setDisplayFormat("yyyy-MM-dd"); expirationDateEdit_->setCalendarPopup(true);
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();


    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("Tên Thuế suất:*", &nameLineEdit_);
    formLayout->addRow("Thuế suất (%):*", &rateLineEdit_);
    formLayout->addRow("Mô tả:", &descriptionLineEdit_);
    formLayout->addRow("Ngày Hiệu lực:*", &effectiveDateEdit_);
    formLayout->addRow("Ngày Hết hạn:", &expirationDateEdit_);
    formLayout->addRow("Trạng thái:", &statusComboBox_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addTaxRateButton_ = new QPushButton("Thêm mới", this); connect(addTaxRateButton_, &QPushButton::clicked, this, &TaxRateManagementWidget::onAddTaxRateClicked);
    editTaxRateButton_ = new QPushButton("Sửa", this); connect(editTaxRateButton_, &QPushButton::clicked, this, &TaxRateManagementWidget::onEditTaxRateClicked);
    deleteTaxRateButton_ = new QPushButton("Xóa", this); connect(deleteTaxRateButton_, &QPushButton::clicked, this, &TaxRateManagementWidget::onDeleteTaxRateClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &TaxRateManagementWidget::onUpdateTaxRateStatusClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &TaxRateManagementWidget::onSearchTaxRateClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &TaxRateManagementWidget::clearForm);
    
    buttonLayout->addWidget(addTaxRateButton_);
    buttonLayout->addWidget(editTaxRateButton_);
    buttonLayout->addWidget(deleteTaxRateButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void TaxRateManagementWidget::loadTaxRates() {
    ERP::Logger::Logger::getInstance().info("TaxRateManagementWidget: Loading tax rates...");
    taxRateTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Finance::DTO::TaxRateDTO> taxRates = taxService_->getAllTaxRates({}, currentUserId_, currentUserRoleIds_);

    taxRateTable_->setRowCount(taxRates.size());
    for (int i = 0; i < taxRates.size(); ++i) {
        const auto& taxRate = taxRates[i];
        taxRateTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(taxRate.id)));
        taxRateTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(taxRate.name)));
        taxRateTable_->setItem(i, 2, new QTableWidgetItem(QString::number(taxRate.rate, 'f', 2)));
        taxRateTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(taxRate.effectiveDate, ERP::Common::DATETIME_FORMAT))));
        taxRateTable_->setItem(i, 4, new QTableWidgetItem(taxRate.expirationDate ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*taxRate.expirationDate, ERP::Common::DATETIME_FORMAT)) : "N/A"));
        taxRateTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(taxRate.status))));
    }
    taxRateTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("TaxRateManagementWidget: Tax rates loaded successfully.");
}

void TaxRateManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Active", static_cast<int>(ERP::Common::EntityStatus::ACTIVE));
    statusComboBox_->addItem("Inactive", static_cast<int>(ERP::Common::EntityStatus::INACTIVE));
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Common::EntityStatus::PENDING));
    statusComboBox_->addItem("Deleted", static_cast<int>(ERP::Common::EntityStatus::DELETED));
}


void TaxRateManagementWidget::onAddTaxRateClicked() {
    if (!hasPermission("Finance.CreateTaxRate")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm thuế suất.", QMessageBox::Warning);
        return;
    }
    clearForm();
    showTaxRateInputDialog();
}

void TaxRateManagementWidget::onEditTaxRateClicked() {
    if (!hasPermission("Finance.UpdateTaxRate")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa thuế suất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = taxRateTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Thuế suất", "Vui lòng chọn một thuế suất để sửa.", QMessageBox::Information);
        return;
    }

    QString taxRateId = taxRateTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Finance::DTO::TaxRateDTO> taxRateOpt = taxService_->getTaxRateById(taxRateId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (taxRateOpt) {
        showTaxRateInputDialog(&(*taxRateOpt));
    } else {
        showMessageBox("Sửa Thuế suất", "Không tìm thấy thuế suất để sửa.", QMessageBox::Critical);
    }
}

void TaxRateManagementWidget::onDeleteTaxRateClicked() {
    if (!hasPermission("Finance.DeleteTaxRate")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa thuế suất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = taxRateTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Thuế suất", "Vui lòng chọn một thuế suất để xóa.", QMessageBox::Information);
        return;
    }

    QString taxRateId = taxRateTable_->item(selectedRow, 0)->text();
    QString taxRateName = taxRateTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Thuế suất");
    confirmBox.setText("Bạn có chắc chắn muốn xóa thuế suất '" + taxRateName + "' (ID: " + taxRateId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (taxService_->deleteTaxRate(taxRateId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Thuế suất", "Thuế suất đã được xóa thành công.", QMessageBox::Information);
            loadTaxRates();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa thuế suất. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void TaxRateManagementWidget::onUpdateTaxRateStatusClicked() {
    if (!hasPermission("Finance.UpdateTaxRate")) { // Using general update permission for status change
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái thuế suất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = taxRateTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một thuế suất để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString taxRateId = taxRateTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Finance::DTO::TaxRateDTO> taxRateOpt = taxService_->getTaxRateById(taxRateId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!taxRateOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy thuế suất để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Finance::DTO::TaxRateDTO currentTaxRate = *taxRateOpt;
    // Toggle status between ACTIVE and INACTIVE for simplicity, or show a dialog for specific status
    ERP::Common::EntityStatus newStatus = (currentTaxRate.status == ERP::Common::EntityStatus::ACTIVE) ?
                                          ERP::Common::EntityStatus::INACTIVE :
                                          ERP::Common::EntityStatus::ACTIVE;

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Cập nhật trạng thái Thuế suất");
    confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái thuế suất '" + QString::fromStdString(currentTaxRate.name) + "' thành " + QString::fromStdString(ERP::Common::entityStatusToString(newStatus)) + "?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        // TaxService doesn't have updateTaxRateStatus, use updateTaxRate with new status
        ERP::Finance::DTO::TaxRateDTO updatedTaxRate = currentTaxRate;
        updatedTaxRate.status = newStatus;
        if (taxService_->updateTaxRate(updatedTaxRate, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Cập nhật trạng thái", "Trạng thái thuế suất đã được cập nhật thành công.", QMessageBox::Information);
            loadTaxRates();
            clearForm();
        } else {
            showMessageBox("Lỗi", "Không thể cập nhật trạng thái thuế suất. Vui lòng kiểm tra log.", QMessageBox::Critical);
        }
    }
}


void TaxRateManagementWidget::onSearchTaxRateClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["name_contains"] = searchText.toStdString(); // Assuming service supports "contains" filter
    }
    taxRateTable_->setRowCount(0);
    std::vector<ERP::Finance::DTO::TaxRateDTO> taxRates = taxService_->getAllTaxRates(filter, currentUserId_, currentUserRoleIds_);

    taxRateTable_->setRowCount(taxRates.size());
    for (int i = 0; i < taxRates.size(); ++i) {
        const auto& taxRate = taxRates[i];
        taxRateTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(taxRate.id)));
        taxRateTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(taxRate.name)));
        taxRateTable_->setItem(i, 2, new QTableWidgetItem(QString::number(taxRate.rate, 'f', 2)));
        taxRateTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(taxRate.effectiveDate, ERP::Common::DATETIME_FORMAT))));
        taxRateTable_->setItem(i, 4, new QTableWidgetItem(taxRate.expirationDate ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*taxRate.expirationDate, ERP::Common::DATETIME_FORMAT)) : "N/A"));
        taxRateTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(taxRate.status))));
    }
    taxRateTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("TaxRateManagementWidget: Search completed.");
}

void TaxRateManagementWidget::onTaxRateTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString taxRateId = taxRateTable_->item(row, 0)->text();
    std::optional<ERP::Finance::DTO::TaxRateDTO> taxRateOpt = taxService_->getTaxRateById(taxRateId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (taxRateOpt) {
        idLineEdit_->setText(QString::fromStdString(taxRateOpt->id));
        nameLineEdit_->setText(QString::fromStdString(taxRateOpt->name));
        rateLineEdit_->setText(QString::number(taxRateOpt->rate, 'f', 2));
        descriptionLineEdit_->setText(QString::fromStdString(taxRateOpt->description.value_or("")));
        effectiveDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(taxRateOpt->effectiveDate.time_since_epoch()).count()));
        if (taxRateOpt->expirationDate) expirationDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(taxRateOpt->expirationDate->time_since_epoch()).count()));
        else expirationDateEdit_->clear();
        
        populateStatusComboBox();
        int statusIndex = statusComboBox_->findData(static_cast<int>(taxRateOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);

    } else {
        showMessageBox("Thông tin Thuế suất", "Không thể tải chi tiết thuế suất đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void TaxRateManagementWidget::clearForm() {
    idLineEdit_->clear();
    nameLineEdit_->clear();
    rateLineEdit_->clear();
    descriptionLineEdit_->clear();
    effectiveDateEdit_->clear();
    expirationDateEdit_->clear();
    statusComboBox_->setCurrentIndex(0); // Default to Active
    taxRateTable_->clearSelection();
    updateButtonsState();
}


void TaxRateManagementWidget::showTaxRateInputDialog(ERP::Finance::DTO::TaxRateDTO* taxRate) {
    QDialog dialog(this);
    dialog.setWindowTitle(taxRate ? "Sửa Thuế suất" : "Thêm Thuế suất Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit nameEdit(this);
    QLineEdit rateEdit(this); rateEdit.setValidator(new QDoubleValidator(0.0, 100.0, 2, this));
    QLineEdit descriptionEdit(this);
    QDateTimeEdit effectiveDateEdit(this); effectiveDateEdit.setDisplayFormat("yyyy-MM-dd"); effectiveDateEdit.setCalendarPopup(true);
    QDateTimeEdit expirationDateEdit(this); expirationDateEdit.setDisplayFormat("yyyy-MM-dd"); expirationDateEdit.setCalendarPopup(true);

    if (taxRate) {
        nameEdit.setText(QString::fromStdString(taxRate->name));
        rateEdit.setText(QString::number(taxRate->rate, 'f', 2));
        descriptionEdit.setText(QString::fromStdString(taxRate->description.value_or("")));
        effectiveDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(taxRate->effectiveDate.time_since_epoch()).count()));
        if (taxRate->expirationDate) expirationDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(taxRate->expirationDate->time_since_epoch()).count()));
        else expirationDateEdit.clear();
        
        nameEdit.setReadOnly(true); // Don't allow changing name for existing
    } else {
        rateEdit.setText("0.00");
        effectiveDateEdit.setDateTime(QDateTime::currentDateTime().date()); // Default to today
    }

    formLayout->addRow("Tên Thuế suất:*", &nameEdit);
    formLayout->addRow("Thuế suất (%):*", &rateEdit);
    formLayout->addRow("Mô tả:", &descriptionEdit);
    formLayout->addRow("Ngày Hiệu lực:*", &effectiveDateEdit);
    formLayout->addRow("Ngày Hết hạn:", &expirationDateEdit);
    
    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(taxRate ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Finance::DTO::TaxRateDTO newTaxRateData;
        if (taxRate) {
            newTaxRateData = *taxRate;
        }

        newTaxRateData.name = nameEdit.text().toStdString();
        newTaxRateData.rate = rateEdit.text().toDouble();
        newTaxRateData.description = descriptionEdit.text().isEmpty() ? std::nullopt : std::make_optional(descriptionEdit.text().toStdString());
        newTaxRateData.effectiveDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(effectiveDateEdit.dateTime());
        newTaxRateData.expirationDate = expirationDateEdit.dateTime().isNull() ? std::nullopt : std::make_optional(ERP::Utils::DateUtils::qDateTimeToTimePoint(expirationDateEdit.dateTime()));
        newTaxRateData.status = ERP::Common::EntityStatus::ACTIVE; // Always set to active on creation/update via form

        bool success = false;
        if (taxRate) {
            success = taxService_->updateTaxRate(newTaxRateData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Thuế suất", "Thuế suất đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật thuế suất. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Finance::DTO::TaxRateDTO> createdTaxRate = taxService_->createTaxRate(newTaxRateData, currentUserId_, currentUserRoleIds_);
            if (createdTaxRate) {
                showMessageBox("Thêm Thuế suất", "Thuế suất mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandler::getLastUserMessage().value_or("Không thể thêm thuế suất mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadTaxRates();
            clearForm();
        }
    }
}


void TaxRateManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool TaxRateManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void TaxRateManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Finance.CreateTaxRate");
    bool canUpdate = hasPermission("Finance.UpdateTaxRate");
    bool canDelete = hasPermission("Finance.DeleteTaxRate");
    // Assuming update permission covers status change for tax rates

    addTaxRateButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Finance.ViewTaxRates"));

    bool isRowSelected = taxRateTable_->currentRow() >= 0;
    editTaxRateButton_->setEnabled(isRowSelected && canUpdate);
    deleteTaxRateButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canUpdate); // Assuming update permission covers status change


    bool enableForm = isRowSelected && canUpdate;
    nameLineEdit_->setEnabled(enableForm); // Will be read-only for existing anyway
    rateLineEdit_->setEnabled(enableForm);
    descriptionLineEdit_->setEnabled(enableForm);
    effectiveDateEdit_->setEnabled(enableForm);
    expirationDateEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        nameLineEdit_->clear();
        rateLineEdit_->clear();
        descriptionLineEdit_->clear();
        effectiveDateEdit_->clear();
        expirationDateEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
    }
}


} // namespace Finance
} // namespace UI
} // namespace ERP