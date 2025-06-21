// UI/Material/MaterialRequestSlipManagementWidget.cpp
#include "MaterialRequestSlipManagementWidget.h" // Đã rút gọn include
#include "MaterialRequestSlip.h"         // Đã rút gọn include
#include "MaterialRequestSlipDetail.h"   // Đã rút gọn include
#include "Product.h"                   // Đã rút gọn include
#include "MaterialRequestService.h"    // Đã rút gọn include
#include "ProductService.h"            // Đã rút gọn include
#include "ISecurityManager.h"          // Đã rút gọn include
#include "Logger.h"                    // Đã rút gọn include
#include "ErrorHandler.h"              // Đã rút gọn include
#include "Common.h"                    // Đã rút gọn include
#include "DateUtils.h"                 // Đã rút gọn include
#include "StringUtils.h"               // Đã rút gọn include
#include "CustomMessageBox.h"          // Đã rút gọn include
#include "UserService.h"               // For getting user names

#include <QInputDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDoubleValidator>
#include <QDialogButtonBox>

namespace ERP {
namespace UI {
namespace Material {

MaterialRequestSlipManagementWidget::MaterialRequestSlipManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IMaterialRequestService> materialRequestService,
    std::shared_ptr<Product::Services::IProductService> productService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      materialRequestService_(materialRequestService),
      productService_(productService),
      securityManager_(securityManager) {

    if (!materialRequestService_ || !productService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ yêu cầu vật tư hoặc các dịch vụ phụ thuộc không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("MaterialRequestSlipManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("MaterialRequestSlipManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("MaterialRequestSlipManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadMaterialRequestSlips();
    updateButtonsState();
}

MaterialRequestSlipManagementWidget::~MaterialRequestSlipManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void MaterialRequestSlipManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo số yêu cầu...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &MaterialRequestSlipManagementWidget::onSearchSlipClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    slipTable_ = new QTableWidget(this);
    slipTable_->setColumnCount(6); // ID, Số yêu cầu, Bộ phận, Người yêu cầu, Ngày yêu cầu, Trạng thái
    slipTable_->setHorizontalHeaderLabels({"ID", "Số Yêu cầu", "Bộ phận", "Người YC", "Ngày YC", "Trạng thái"});
    slipTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    slipTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    slipTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    slipTable_->horizontalHeader()->setStretchLastSection(true);
    connect(slipTable_, &QTableWidget::itemClicked, this, &MaterialRequestSlipManagementWidget::onSlipTableItemClicked);
    mainLayout->addWidget(slipTable_);

    // Form elements for editing/adding slips
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    requestNumberLineEdit_ = new QLineEdit(this);
    requestingDepartmentLineEdit_ = new QLineEdit(this);
    requestedByLineEdit_ = new QLineEdit(this); requestedByLineEdit_->setReadOnly(true); // Should be current user
    requestDateEdit_ = new QDateTimeEdit(this); requestDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();
    approvedByLineEdit_ = new QLineEdit(this); // User ID, display name
    approvalDateEdit_ = new QDateTimeEdit(this); approvalDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    notesLineEdit_ = new QLineEdit(this);
    referenceDocumentIdLineEdit_ = new QLineEdit(this);
    referenceDocumentTypeLineEdit_ = new QLineEdit(this);


    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("Số Yêu cầu:*", &requestNumberLineEdit_);
    formLayout->addRow("Bộ phận YC:*", &requestingDepartmentLineEdit_);
    formLayout->addRow("Người YC:", &requestedByLineEdit_);
    formLayout->addRow("Ngày YC:*", &requestDateEdit_);
    formLayout->addRow("Trạng thái:*", &statusComboBox_);
    formLayout->addRow("Người phê duyệt:", &approvedByLineEdit_);
    formLayout->addRow("Ngày phê duyệt:", &approvalDateEdit_);
    formLayout->addRow("Ghi chú:", &notesLineEdit_);
    formLayout->addRow("ID Tài liệu tham chiếu:", &referenceDocumentIdLineEdit_);
    formLayout->addRow("Loại Tài liệu tham chiếu:", &referenceDocumentTypeLineEdit_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addSlipButton_ = new QPushButton("Thêm mới", this); connect(addSlipButton_, &QPushButton::clicked, this, &MaterialRequestSlipManagementWidget::onAddSlipClicked);
    editSlipButton_ = new QPushButton("Sửa", this); connect(editSlipButton_, &QPushButton::clicked, this, &MaterialRequestSlipManagementWidget::onEditSlipClicked);
    deleteSlipButton_ = new QPushButton("Xóa", this); connect(deleteSlipButton_, &QPushButton::clicked, this, &MaterialRequestSlipManagementWidget::onDeleteSlipClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &MaterialRequestSlipManagementWidget::onUpdateSlipStatusClicked);
    manageDetailsButton_ = new QPushButton("Quản lý Chi tiết", this); connect(manageDetailsButton_, &QPushButton::clicked, this, &MaterialRequestSlipManagementWidget::onManageDetailsClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &MaterialRequestSlipManagementWidget::onSearchSlipClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &MaterialRequestSlipManagementWidget::clearForm);
    
    buttonLayout->addWidget(addSlipButton_);
    buttonLayout->addWidget(editSlipButton_);
    buttonLayout->addWidget(deleteSlipButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(manageDetailsButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void MaterialRequestSlipManagementWidget::loadMaterialRequestSlips() {
    ERP::Logger::Logger::getInstance().info("MaterialRequestSlipManagementWidget: Loading material request slips...");
    slipTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Material::DTO::MaterialRequestSlipDTO> slips = materialRequestService_->getAllMaterialRequestSlips({}, currentUserId_, currentUserRoleIds_);

    slipTable_->setRowCount(slips.size());
    for (int i = 0; i < slips.size(); ++i) {
        const auto& slip = slips[i];
        slipTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(slip.id)));
        slipTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(slip.requestNumber)));
        slipTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(slip.requestingDepartment)));
        
        QString requestedByName = "N/A";
        std::optional<ERP::User::DTO::UserDTO> requestedByUser = securityManager_->getUserService()->getUserById(slip.requestedByUserId, currentUserId_, currentUserRoleIds_);
        if (requestedByUser) requestedByName = QString::fromStdString(requestedByUser->username);
        slipTable_->setItem(i, 3, new QTableWidgetItem(requestedByName));

        slipTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(slip.requestDate, ERP::Common::DATETIME_FORMAT))));
        slipTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(slip.getStatusString())));
    }
    slipTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("MaterialRequestSlipManagementWidget: Material request slips loaded successfully.");
}

void MaterialRequestSlipManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Draft", static_cast<int>(ERP::Material::DTO::MaterialRequestSlipStatus::DRAFT));
    statusComboBox_->addItem("Pending Approval", static_cast<int>(ERP::Material::DTO::MaterialRequestSlipStatus::PENDING_APPROVAL));
    statusComboBox_->addItem("Approved", static_cast<int>(ERP::Material::DTO::MaterialRequestSlipStatus::APPROVED));
    statusComboBox_->addItem("In Progress", static_cast<int>(ERP::Material::DTO::MaterialRequestSlipStatus::IN_PROGRESS));
    statusComboBox_->addItem("Completed", static_cast<int>(ERP::Material::DTO::MaterialRequestSlipStatus::COMPLETED));
    statusComboBox_->addItem("Cancelled", static_cast<int>(ERP::Material::DTO::MaterialRequestSlipStatus::CANCELLED));
    statusComboBox_->addItem("Rejected", static_cast<int>(ERP::Material::DTO::MaterialRequestSlipStatus::REJECTED));
}

void MaterialRequestSlipManagementWidget::populateUserComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("None", "");
    std::vector<ERP::User::DTO::UserDTO> allUsers = securityManager_->getUserService()->getAllUsers({}, currentUserId_, currentUserRoleIds_);
    for (const auto& user : allUsers) {
        comboBox->addItem(QString::fromStdString(user.username), QString::fromStdString(user.id));
    }
}


void MaterialRequestSlipManagementWidget::onAddSlipClicked() {
    if (!hasPermission("Material.CreateMaterialRequest")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm phiếu yêu cầu vật tư.", QMessageBox::Warning);
        return;
    }
    clearForm();
    showSlipInputDialog();
}

void MaterialRequestSlipManagementWidget::onEditSlipClicked() {
    if (!hasPermission("Material.UpdateMaterialRequest")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa phiếu yêu cầu vật tư.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Phiếu Yêu cầu Vật tư", "Vui lòng chọn một phiếu yêu cầu vật tư để sửa.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> slipOpt = materialRequestService_->getMaterialRequestSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (slipOpt) {
        showSlipInputDialog(&(*slipOpt));
    } else {
        showMessageBox("Sửa Phiếu Yêu cầu Vật tư", "Không tìm thấy phiếu yêu cầu vật tư để sửa.", QMessageBox::Critical);
    }
}

void MaterialRequestSlipManagementWidget::onDeleteSlipClicked() {
    if (!hasPermission("Material.DeleteMaterialRequest")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa phiếu yêu cầu vật tư.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Phiếu Yêu cầu Vật tư", "Vui lòng chọn một phiếu yêu cầu vật tư để xóa.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    QString slipNumber = slipTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Phiếu Yêu cầu Vật tư");
    confirmBox.setText("Bạn có chắc chắn muốn xóa phiếu yêu cầu vật tư '" + slipNumber + "' (ID: " + slipId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (materialRequestService_->deleteMaterialRequestSlip(slipId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Phiếu Yêu cầu Vật tư", "Phiếu yêu cầu vật tư đã được xóa thành công.", QMessageBox::Information);
            loadMaterialRequestSlips();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa phiếu yêu cầu vật tư. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void MaterialRequestSlipManagementWidget::onUpdateSlipStatusClicked() {
    if (!hasPermission("Material.UpdateMaterialRequestStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái phiếu yêu cầu vật tư.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một phiếu yêu cầu vật tư để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> slipOpt = materialRequestService_->getMaterialRequestSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!slipOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy phiếu yêu cầu vật tư để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Material::DTO::MaterialRequestSlipDTO currentSlip = *slipOpt;
    // Use a QInputDialog or custom dialog to get the new status
    QDialog statusDialog(this);
    statusDialog.setWindowTitle("Chọn Trạng Thái Mới");
    QVBoxLayout *layout = new QVBoxLayout(&statusDialog);
    QComboBox newStatusCombo;
    populateStatusComboBox(); // Populate it
    for(int i = 0; i < statusComboBox_->count(); ++i) { // Copy items
        newStatusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));
    }
    // Set current status as default selected
    int currentStatusIndex = newStatusCombo.findData(static_cast<int>(currentSlip.status));
    if (currentStatusIndex != -1) newStatusCombo.setCurrentIndex(currentStatusIndex);

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
        ERP::Material::DTO::MaterialRequestSlipStatus newStatus = static_cast<ERP::Material::DTO::MaterialRequestSlipStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái phiếu yêu cầu vật tư");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái phiếu yêu cầu vật tư '" + QString::fromStdString(currentSlip.requestNumber) + "' thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (materialRequestService_->updateMaterialRequestSlipStatus(slipId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái phiếu yêu cầu vật tư đã được cập nhật thành công.", QMessageBox::Information);
                loadMaterialRequestSlips();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái phiếu yêu cầu vật tư. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}


void MaterialRequestSlipManagementWidget::onSearchSlipClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["request_number_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    slipTable_->setRowCount(0);
    std::vector<ERP::Material::DTO::MaterialRequestSlipDTO> slips = materialRequestService_->getAllMaterialRequestSlips(filter, currentUserId_, currentUserRoleIds_);

    slipTable_->setRowCount(slips.size());
    for (int i = 0; i < slips.size(); ++i) {
        const auto& slip = slips[i];
        slipTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(slip.id)));
        slipTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(slip.requestNumber)));
        slipTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(slip.requestingDepartment)));
        
        QString requestedByName = "N/A";
        std::optional<ERP::User::DTO::UserDTO> requestedByUser = securityManager_->getUserService()->getUserById(slip.requestedByUserId, currentUserId_, currentUserRoleIds_);
        if (requestedByUser) requestedByName = QString::fromStdString(requestedByUser->username);
        slipTable_->setItem(i, 3, new QTableWidgetItem(requestedByName));

        slipTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(slip.requestDate, ERP::Common::DATETIME_FORMAT))));
        slipTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(slip.getStatusString())));
    }
    slipTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("MaterialRequestSlipManagementWidget: Search completed.");
}

void MaterialRequestSlipManagementWidget::onSlipTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString slipId = slipTable_->item(row, 0)->text();
    std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> slipOpt = materialRequestService_->getMaterialRequestSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (slipOpt) {
        idLineEdit_->setText(QString::fromStdString(slipOpt->id));
        requestNumberLineEdit_->setText(QString::fromStdString(slipOpt->requestNumber));
        requestingDepartmentLineEdit_->setText(QString::fromStdString(slipOpt->requestingDepartment));
        
        requestedByLineEdit_->setText(QString::fromStdString(slipOpt->requestedByUserId)); // Display ID
        requestDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(slipOpt->requestDate.time_since_epoch()).count()));
        
        populateStatusComboBox();
        int statusIndex = statusComboBox_->findData(static_cast<int>(slipOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);
        
        approvedByLineEdit_->setText(QString::fromStdString(slipOpt->approvedByUserId.value_or("")));
        if (slipOpt->approvalDate) approvalDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(slipOpt->approvalDate->time_since_epoch()).count()));
        else approvalDateEdit_->clear();
        notesLineEdit_->setText(QString::fromStdString(slipOpt->notes.value_or("")));
        referenceDocumentIdLineEdit_->setText(QString::fromStdString(slipOpt->referenceDocumentId.value_or("")));
        referenceDocumentTypeLineEdit_->setText(QString::fromStdString(slipOpt->referenceDocumentType.value_or("")));

    } else {
        showMessageBox("Thông tin Phiếu Yêu cầu Vật tư", "Không tìm thấy phiếu yêu cầu vật tư đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void MaterialRequestSlipManagementWidget::clearForm() {
    idLineEdit_->clear();
    requestNumberLineEdit_->clear();
    requestingDepartmentLineEdit_->clear();
    requestedByLineEdit_->clear();
    requestDateEdit_->clear();
    statusComboBox_->setCurrentIndex(0); // Default to Draft
    approvedByLineEdit_->clear();
    approvalDateEdit_->clear();
    notesLineEdit_->clear();
    referenceDocumentIdLineEdit_->clear();
    referenceDocumentTypeLineEdit_->clear();
    slipTable_->clearSelection();
    updateButtonsState();
}

void MaterialRequestSlipManagementWidget::onManageDetailsClicked() {
    if (!hasPermission("Material.ManageMaterialRequestSlipDetails")) { // Assuming this permission
        showMessageBox("Lỗi", "Bạn không có quyền quản lý chi tiết phiếu yêu cầu vật tư.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Quản lý Chi tiết", "Vui lòng chọn một phiếu yêu cầu vật tư để quản lý chi tiết.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> slipOpt = materialRequestService_->getMaterialRequestSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (slipOpt) {
        showManageDetailsDialog(&(*slipOpt));
    } else {
        showMessageBox("Quản lý Chi tiết", "Không tìm thấy phiếu yêu cầu vật tư để quản lý chi tiết.", QMessageBox::Critical);
    }
}


void MaterialRequestSlipManagementWidget::showSlipInputDialog(ERP::Material::DTO::MaterialRequestSlipDTO* slip) {
    QDialog dialog(this);
    dialog.setWindowTitle(slip ? "Sửa Phiếu Yêu cầu Vật tư" : "Thêm Phiếu Yêu cầu Vật tư Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit requestNumberEdit(this);
    QLineEdit requestingDepartmentEdit(this);
    QLineEdit requestedByEdit(this); requestedByEdit.setReadOnly(true); // Auto-filled
    QDateTimeEdit requestDateEdit(this); requestDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QComboBox statusCombo(this); populateStatusComboBox();
    for (int i = 0; i < statusComboBox_->count(); ++i) statusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));
    
    QComboBox approvedByCombo(this); populateUserComboBox(&approvedByCombo);
    QDateTimeEdit approvalDateEdit(this); approvalDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QLineEdit notesEdit(this);
    QLineEdit referenceDocumentIdEdit(this);
    QLineEdit referenceDocumentTypeEdit(this);

    if (slip) {
        requestNumberEdit.setText(QString::fromStdString(slip->requestNumber));
        requestingDepartmentEdit.setText(QString::fromStdString(slip->requestingDepartment));
        requestedByEdit.setText(QString::fromStdString(slip->requestedByUserId));
        requestDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(slip->requestDate.time_since_epoch()).count()));
        int statusIndex = statusCombo.findData(static_cast<int>(slip->status));
        if (statusIndex != -1) statusCombo.setCurrentIndex(statusIndex);
        if (slip->approvedByUserId) {
            int userIndex = approvedByCombo.findData(QString::fromStdString(*slip->approvedByUserId));
            if (userIndex != -1) approvedByCombo.setCurrentIndex(userIndex);
            else approvedByCombo.setCurrentIndex(0); // "None"
        } else {
            approvedByCombo.setCurrentIndex(0); // "None"
        }
        if (slip->approvalDate) approvalDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(slip->approvalDate->time_since_epoch()).count()));
        else approvalDateEdit.clear();
        notesEdit.setText(QString::fromStdString(slip->notes.value_or("")));
        referenceDocumentIdEdit.setText(QString::fromStdString(slip->referenceDocumentId.value_or("")));
        referenceDocumentTypeEdit.setText(QString::fromStdString(slip->referenceDocumentType.value_or("")));

        requestNumberEdit.setReadOnly(true); // Don't allow changing request number for existing
    } else {
        requestNumberEdit.setText("MRS-" + QString::fromStdString(ERP::Utils::generateUUID().substr(0, 8))); // Auto-generate
        requestDateEdit.setDateTime(QDateTime::currentDateTime());
        requestedByEdit.setText(QString::fromStdString(currentUserId_)); // Auto-fill current user
    }

    formLayout->addRow("Số Yêu cầu:*", &requestNumberEdit);
    formLayout->addRow("Bộ phận YC:*", &requestingDepartmentEdit);
    formLayout->addRow("Người YC:", &requestedByEdit);
    formLayout->addRow("Ngày YC:*", &requestDateEdit);
    formLayout->addRow("Trạng thái:*", &statusCombo);
    formLayout->addRow("Người phê duyệt:", &approvedByCombo);
    formLayout->addRow("Ngày phê duyệt:", &approvalDateEdit);
    formLayout->addRow("Ghi chú:", &notesEdit);
    formLayout->addRow("ID Tài liệu tham chiếu:", &referenceDocumentIdEdit);
    formLayout->addRow("Loại Tài liệu tham chiếu:", &referenceDocumentTypeEdit);
    
    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(slip ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Material::DTO::MaterialRequestSlipDTO newSlipData;
        if (slip) {
            newSlipData = *slip;
        } else {
            newSlipData.id = ERP::Utils::generateUUID(); // New ID for new slip
        }

        newSlipData.requestNumber = requestNumberEdit.text().toStdString();
        newSlipData.requestingDepartment = requestingDepartmentEdit.text().toStdString();
        newSlipData.requestedByUserId = requestedByEdit.text().toStdString();
        newSlipData.requestDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(requestDateEdit.dateTime());
        newSlipData.status = static_cast<ERP::Material::DTO::MaterialRequestSlipStatus>(statusCombo.currentData().toInt());
        
        QString selectedApprovedById = approvedByCombo.currentData().toString();
        newSlipData.approvedByUserId = selectedApprovedById.isEmpty() ? std::nullopt : std::make_optional(selectedApprovedById.toStdString());
        
        newSlipData.approvalDate = approvalDateEdit.dateTime().isNull() ? std::nullopt : std::make_optional(ERP::Utils::DateUtils::qDateTimeToTimePoint(approvalDateEdit.dateTime()));
        newSlipData.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());
        newSlipData.referenceDocumentId = referenceDocumentIdEdit.text().isEmpty() ? std::nullopt : std::make_optional(referenceDocumentIdEdit.text().toStdString());
        newSlipData.referenceDocumentType = referenceDocumentTypeEdit.text().isEmpty() ? std::nullopt : std::make_optional(referenceDocumentTypeEdit.text().toStdString());

        // For new slips, details are added in a separate dialog after creation
        std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> currentDetails;
        if (slip) { // When editing, load existing details first
             currentDetails = materialRequestService_->getMaterialRequestSlipDetails(slip->id, currentUserId_, currentUserRoleIds_);
        }

        bool success = false;
        if (slip) {
            success = materialRequestService_->updateMaterialRequestSlip(newSlipData, currentDetails, currentUserId_, currentUserRoleIds_); // Pass existing details if not modified
            if (success) showMessageBox("Sửa Phiếu Yêu cầu Vật tư", "Phiếu yêu cầu vật tư đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật phiếu yêu cầu vật tư. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> createdSlip = materialRequestService_->createMaterialRequestSlip(newSlipData, {}, currentUserId_, currentUserRoleIds_); // Create with empty details
            if (createdSlip) {
                showMessageBox("Thêm Phiếu Yêu cầu Vật tư", "Phiếu yêu cầu vật tư mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm phiếu yêu cầu vật tư mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadMaterialRequestSlips();
            clearForm();
        }
    }
}

void MaterialRequestSlipManagementWidget::showManageDetailsDialog(ERP::Material::DTO::MaterialRequestSlipDTO* slip) {
    if (!slip) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Quản lý Chi tiết Phiếu Yêu cầu Vật tư: " + QString::fromStdString(slip->requestNumber));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *detailsTable = new QTableWidget(&dialog);
    detailsTable->setColumnCount(5); // Product, Requested Qty, Issued Qty, Notes, Is Fully Issued
    detailsTable->setHorizontalHeaderLabels({"Sản phẩm", "SL YC", "SL Đã xuất", "Ghi chú", "Đã xuất đủ"});
    detailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    detailsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    detailsTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(detailsTable);

    // Load existing details
    std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> currentDetails = materialRequestService_->getMaterialRequestSlipDetails(slip->id, currentUserId_, currentUserRoleIds_);
    detailsTable->setRowCount(currentDetails.size());
    for (int i = 0; i < currentDetails.size(); ++i) {
        const auto& detail = currentDetails[i];
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = securityManager_->getProductService()->getProductById(detail.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);

        detailsTable->setItem(i, 0, new QTableWidgetItem(productName));
        detailsTable->setItem(i, 1, new QTableWidgetItem(QString::number(detail.requestedQuantity)));
        detailsTable->setItem(i, 2, new QTableWidgetItem(QString::number(detail.issuedQuantity)));
        detailsTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(detail.notes.value_or(""))));
        detailsTable->setItem(i, 4, new QTableWidgetItem(detail.isFullyIssued ? "Yes" : "No"));
        detailsTable->item(i, 0)->setData(Qt::UserRole, QString::fromStdString(detail.id)); // Store detail ID
    }


    QHBoxLayout *itemButtonsLayout = new QHBoxLayout();
    QPushButton *addItemButton = new QPushButton("Thêm Chi tiết", &dialog);
    QPushButton *editItemButton = new QPushButton("Sửa Chi tiết", &dialog);
    QPushButton *deleteItemButton = new QPushButton("Xóa Chi tiết", &dialog);
    itemButtonsLayout->addWidget(addItemButton);
    itemButtonsLayout->addWidget(editItemButton);
    itemButtonsLayout->addWidget(deleteItemButton);
    dialogLayout->addLayout(itemButtonsLayout);

    QPushButton *saveButton = new QPushButton("Lưu", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *actionButtonsLayout = new QHBoxLayout();
    actionButtonsLayout->addWidget(saveButton);
    actionButtonsLayout->addWidget(cancelButton);
    dialogLayout->addLayout(actionButtonsLayout);

    // Connect item management buttons (logic will be in lambdas for simplicity)
    connect(addItemButton, &QPushButton::clicked, [&]() {
        QDialog itemDialog(&dialog);
        itemDialog.setWindowTitle("Thêm Chi tiết Phiếu Yêu cầu Vật tư");
        QFormLayout itemFormLayout;
        QComboBox productCombo;
        std::vector<ERP::Product::DTO::ProductDTO> allProducts = securityManager_->getProductService()->getAllProducts({}, currentUserId_, currentUserRoleIds_);
        for (const auto& prod : allProducts) productCombo.addItem(QString::fromStdString(prod.name), QString::fromStdString(prod.id));

        QLineEdit requestedQuantityEdit; requestedQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit notesEdit;

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Số lượng YC:*", &requestedQuantityEdit);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);

        QPushButton *okItemButton = new QPushButton("Thêm", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (productCombo.currentData().isNull() || requestedQuantityEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Add to table
            int newRow = detailsTable->rowCount();
            detailsTable->insertRow(newRow);
            detailsTable->setItem(newRow, 0, new QTableWidgetItem(productCombo.currentText()));
            detailsTable->setItem(newRow, 1, new QTableWidgetItem(requestedQuantityEdit.text()));
            detailsTable->setItem(newRow, 2, new QTableWidgetItem("0.0")); // Issued Qty
            detailsTable->setItem(newRow, 3, new QTableWidgetItem(notesEdit.text()));
            detailsTable->setItem(newRow, 4, new QTableWidgetItem("No")); // Is Fully Issued
            detailsTable->item(newRow, 0)->setData(Qt::UserRole, productCombo.currentData()); // Store product ID
            // No detail ID assigned yet until saved
        }
    });

    connect(editItemButton, &QPushButton::clicked, [&]() {
        int selectedItemRow = detailsTable->currentRow();
        if (selectedItemRow < 0) {
            showMessageBox("Sửa Chi tiết", "Vui lòng chọn một chi tiết để sửa.", QMessageBox::Information);
            return;
        }

        QDialog itemDialog(&dialog);
        itemDialog.setWindowTitle("Sửa Chi tiết Phiếu Yêu cầu Vật tư");
        QFormLayout itemFormLayout;
        QComboBox productCombo;
        std::vector<ERP::Product::DTO::ProductDTO> allProducts = securityManager_->getProductService()->getAllProducts({}, currentUserId_, currentUserRoleIds_);
        for (const auto& prod : allProducts) productCombo.addItem(QString::fromStdString(prod.name), QString::fromStdString(prod.id));
        
        QLineEdit requestedQuantityEdit; requestedQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit notesEdit;

        // Populate with current item data
        QString currentProductId = detailsTable->item(selectedItemRow, 0)->data(Qt::UserRole).toString();
        int productIndex = productCombo.findData(currentProductId);
        if (productIndex != -1) productCombo.setCurrentIndex(productIndex);

        requestedQuantityEdit.setText(detailsTable->item(selectedItemRow, 1)->text());
        notesEdit.setText(detailsTable->item(selectedItemRow, 3)->text());

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Số lượng YC:*", &requestedQuantityEdit);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);

        QPushButton *okItemButton = new QPushButton("Lưu", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (productCombo.currentData().isNull() || requestedQuantityEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Update table row
            detailsTable->item(selectedItemRow, 0)->setText(productCombo.currentText());
            detailsTable->item(selectedItemRow, 1)->setText(requestedQuantityEdit.text());
            detailsTable->item(selectedItemRow, 3)->setText(notesEdit.text());
            detailsTable->item(selectedItemRow, 0)->setData(Qt::UserRole, productCombo.currentData());
        }
    });

    connect(deleteItemButton, &QPushButton::clicked, [&]() {
        int selectedItemRow = detailsTable->currentRow();
        if (selectedItemRow < 0) {
            showMessageBox("Xóa Chi tiết", "Vui lòng chọn một chi tiết để xóa.", QMessageBox::Information);
            return;
        }
        Common::CustomMessageBox confirmDelBox(&dialog);
        confirmDelBox.setWindowTitle("Xóa Chi tiết Phiếu Yêu cầu Vật tư");
        confirmDelBox.setText("Bạn có chắc chắn muốn xóa chi tiết phiếu yêu cầu vật tư này?");
        confirmDelBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmDelBox.exec() == QMessageBox::Yes) {
            detailsTable->removeRow(selectedItemRow);
        }
    });

    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Collect all items from the table
        std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> updatedDetails;
        for (int i = 0; i < detailsTable->rowCount(); ++i) {
            ERP::Material::DTO::MaterialRequestSlipDetailDTO detail;
            // If item had an ID (from existing data), reuse it, otherwise generate new
            QString existingId = detailsTable->item(i, 0)->data(Qt::UserRole).toString();
            detail.id = existingId.isEmpty() ? ERP::Utils::generateUUID() : existingId.toStdString();

            detail.materialRequestSlipId = slip->id;
            detail.productId = detailsTable->item(i, 0)->data(Qt::UserRole).toString().toStdString();
            detail.requestedQuantity = detailsTable->item(i, 1)->text().toDouble();
            detail.issuedQuantity = detailsTable->item(i, 2)->text().toDouble(); // Keep current issued qty
            detail.notes = detailsTable->item(i, 3)->text().isEmpty() ? std::nullopt : std::make_optional(detailsTable->item(i, 3)->text().toStdString());
            detail.isFullyIssued = detailsTable->item(i, 4)->text() == "Yes"; // Keep current status
            
            updatedDetails.push_back(detail);
        }

        // Call service to update Material Request Slip with new detail list
        if (materialRequestService_->updateMaterialRequestSlip(*slip, updatedDetails, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Quản lý Chi tiết", "Chi tiết phiếu yêu cầu vật tư đã được cập nhật thành công.", QMessageBox::Information);
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật chi tiết phiếu yêu cầu vật tư. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void MaterialRequestSlipManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool MaterialRequestSlipManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void MaterialRequestSlipManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Material.CreateMaterialRequest");
    bool canUpdate = hasPermission("Material.UpdateMaterialRequest");
    bool canDelete = hasPermission("Material.DeleteMaterialRequest");
    bool canChangeStatus = hasPermission("Material.UpdateMaterialRequestStatus");
    bool canManageDetails = hasPermission("Material.ManageMaterialRequestSlipDetails"); // Assuming this permission

    addSlipButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Material.ViewMaterialRequests"));

    bool isRowSelected = slipTable_->currentRow() >= 0;
    editSlipButton_->setEnabled(isRowSelected && canUpdate);
    deleteSlipButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    manageDetailsButton_->setEnabled(isRowSelected && canManageDetails);


    bool enableForm = isRowSelected && canUpdate;
    requestNumberLineEdit_->setEnabled(enableForm); // Will be read-only for existing anyway
    requestingDepartmentLineEdit_->setEnabled(enableForm);
    // requestedByLineEdit_ is read-only
    requestDateEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    approvedByLineEdit_->setEnabled(enableForm);
    approvalDateEdit_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);
    referenceDocumentIdLineEdit_->setEnabled(enableForm);
    referenceDocumentTypeLineEdit_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        requestNumberLineEdit_->clear();
        requestingDepartmentLineEdit_->clear();
        requestedByLineEdit_->clear();
        requestDateEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
        approvedByLineEdit_->clear();
        approvalDateEdit_->clear();
        notesLineEdit_->clear();
        referenceDocumentIdLineEdit_->clear();
        referenceDocumentTypeLineEdit_->clear();
    }
}


} // namespace Material
} // namespace UI
} // namespace ERP