// UI/Material/IssueSlipManagementWidget.cpp
#include "IssueSlipManagementWidget.h" // Đã rút gọn include
#include "IssueSlip.h"                 // Đã rút gọn include
#include "IssueSlipDetail.h"           // Đã rút gọn include
#include "Product.h"                   // Đã rút gọn include
#include "Warehouse.h"                 // Đã rút gọn include
#include "Location.h"                  // Đã rút gọn include
#include "MaterialRequestSlip.h"       // Đã rút gọn include
#include "IssueSlipService.h"          // Đã rút gọn include
#include "ProductService.h"            // Đã rút gọn include
#include "WarehouseService.h"          // Đã rút gọn include
#include "InventoryManagementService.h"// Đã rút gọn include
#include "MaterialRequestService.h"    // Đã rút gọn include
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

IssueSlipManagementWidget::IssueSlipManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IIssueSlipService> issueSlipService,
    std::shared_ptr<Product::Services::IProductService> productService,
    std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<Warehouse::Services::IInventoryManagementService> inventoryManagementService,
    std::shared_ptr<Services::IMaterialRequestService> materialRequestService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      issueSlipService_(issueSlipService),
      productService_(productService),
      warehouseService_(warehouseService),
      inventoryManagementService_(inventoryManagementService),
      materialRequestService_(materialRequestService),
      securityManager_(securityManager) {

    if (!issueSlipService_ || !productService_ || !warehouseService_ || !inventoryManagementService_ || !materialRequestService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ phiếu xuất kho hoặc các dịch vụ phụ thuộc không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("IssueSlipManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("IssueSlipManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("IssueSlipManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadIssueSlips();
    updateButtonsState();
}

IssueSlipManagementWidget::~IssueSlipManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void IssueSlipManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo số phiếu xuất...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &IssueSlipManagementWidget::onSearchSlipClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    slipTable_ = new QTableWidget(this);
    slipTable_->setColumnCount(6); // ID, Số phiếu, Kho hàng, Ngày xuất, Trạng thái, Yêu cầu vật tư liên kết
    slipTable_->setHorizontalHeaderLabels({"ID", "Số Phiếu Xuất", "Kho hàng", "Ngày Xuất", "Trạng thái", "YC Vật tư"});
    slipTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    slipTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    slipTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    slipTable_->horizontalHeader()->setStretchLastSection(true);
    connect(slipTable_, &QTableWidget::itemClicked, this, &IssueSlipManagementWidget::onSlipTableItemClicked);
    mainLayout->addWidget(slipTable_);

    // Form elements for editing/adding slips
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    issueNumberLineEdit_ = new QLineEdit(this);
    warehouseComboBox_ = new QComboBox(this); populateWarehouseComboBox();
    issuedByLineEdit_ = new QLineEdit(this); issuedByLineEdit_->setReadOnly(true); // Should be current user
    issueDateEdit_ = new QDateTimeEdit(this); issueDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    materialRequestSlipComboBox_ = new QComboBox(this); populateMaterialRequestSlipComboBox();
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();
    referenceDocumentIdLineEdit_ = new QLineEdit(this);
    referenceDocumentTypeLineEdit_ = new QLineEdit(this);
    notesLineEdit_ = new QLineEdit(this);


    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Số Phiếu Xuất:*", this), 1, 0); formLayout->addWidget(issueNumberLineEdit_, 1, 1);
    formLayout->addWidget(new QLabel("Kho hàng:*", this), 2, 0); formLayout->addWidget(warehouseComboBox_, 2, 1);
    formLayout->addWidget(new QLabel("Người xuất:", this), 3, 0); formLayout->addWidget(issuedByLineEdit_, 3, 1);
    formLayout->addWidget(new QLabel("Ngày Xuất:*", this), 4, 0); formLayout->addWidget(issueDateEdit_, 4, 1);
    formLayout->addWidget(new QLabel("YC Vật tư liên kết:", this), 5, 0); formLayout->addWidget(materialRequestSlipComboBox_, 5, 1);
    formLayout->addWidget(new QLabel("Trạng thái:*", this), 6, 0); formLayout->addWidget(statusComboBox_, 6, 1);
    formLayout->addWidget(new QLabel("ID Tài liệu tham chiếu:", this), 7, 0); formLayout->addWidget(referenceDocumentIdLineEdit_, 7, 1);
    formLayout->addWidget(new QLabel("Loại Tài liệu tham chiếu:", this), 8, 0); formLayout->addWidget(referenceDocumentTypeLineEdit_, 8, 1);
    formLayout->addWidget(new QLabel("Ghi chú:", this), 9, 0); formLayout->addWidget(notesLineEdit_, 9, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addSlipButton_ = new QPushButton("Thêm mới", this); connect(addSlipButton_, &QPushButton::clicked, this, &IssueSlipManagementWidget::onAddSlipClicked);
    editSlipButton_ = new QPushButton("Sửa", this); connect(editSlipButton_, &QPushButton::clicked, this, &IssueSlipManagementWidget::onEditSlipClicked);
    deleteSlipButton_ = new QPushButton("Xóa", this); connect(deleteSlipButton_, &QPushButton::clicked, this, &IssueSlipManagementWidget::onDeleteSlipClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &IssueSlipManagementWidget::onUpdateSlipStatusClicked);
    manageDetailsButton_ = new QPushButton("Quản lý Chi tiết", this); connect(manageDetailsButton_, &QPushButton::clicked, this, &IssueSlipManagementWidget::onManageDetailsClicked);
    recordIssuedQuantityButton_ = new QPushButton("Ghi nhận SL xuất", this); connect(recordIssuedQuantityButton_, &QPushButton::clicked, this, &IssueSlipManagementWidget::onRecordIssuedQuantityClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &IssueSlipManagementWidget::onSearchSlipClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &IssueSlipManagementWidget::clearForm);
    
    buttonLayout->addWidget(addSlipButton_);
    buttonLayout->addWidget(editSlipButton_);
    buttonLayout->addWidget(deleteSlipButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(manageDetailsButton_);
    buttonLayout->addWidget(recordIssuedQuantityButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void IssueSlipManagementWidget::loadIssueSlips() {
    ERP::Logger::Logger::getInstance().info("IssueSlipManagementWidget: Loading issue slips...");
    slipTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Material::DTO::IssueSlipDTO> slips = issueSlipService_->getAllIssueSlips({}, currentUserId_, currentUserRoleIds_);

    slipTable_->setRowCount(slips.size());
    for (int i = 0; i < slips.size(); ++i) {
        const auto& slip = slips[i];
        slipTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(slip.id)));
        slipTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(slip.issueNumber)));
        
        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(slip.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) warehouseName = QString::fromStdString(warehouse->name);
        slipTable_->setItem(i, 2, new QTableWidgetItem(warehouseName));

        slipTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(slip.issueDate, ERP::Common::DATETIME_FORMAT))));
        slipTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(slip.getStatusString())));
        
        QString mrsNumber = "N/A";
        if (slip.materialRequestSlipId) {
            std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> mrs = materialRequestService_->getMaterialRequestSlipById(*slip.materialRequestSlipId, currentUserId_, currentUserRoleIds_);
            if (mrs) mrsNumber = QString::fromStdString(mrs->requestNumber);
        }
        slipTable_->setItem(i, 5, new QTableWidgetItem(mrsNumber));
    }
    slipTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("IssueSlipManagementWidget: Issue slips loaded successfully.");
}

void IssueSlipManagementWidget::populateWarehouseComboBox() {
    warehouseComboBox_->clear();
    std::vector<ERP::Catalog::DTO::WarehouseDTO> allWarehouses = warehouseService_->getAllWarehouses({}, currentUserId_, currentUserRoleIds_);
    for (const auto& warehouse : allWarehouses) {
        warehouseComboBox_->addItem(QString::fromStdString(warehouse.name), QString::fromStdString(warehouse.id));
    }
}

void IssueSlipManagementWidget::populateMaterialRequestSlipComboBox() {
    materialRequestSlipComboBox_->clear();
    materialRequestSlipComboBox_->addItem("None", "");
    std::vector<ERP::Material::DTO::MaterialRequestSlipDTO> allMrs = materialRequestService_->getAllMaterialRequestSlips({}, currentUserId_, currentUserRoleIds_);
    for (const auto& mrs : allMrs) {
        materialRequestSlipComboBox_->addItem(QString::fromStdString(mrs.requestNumber), QString::fromStdString(mrs.id));
    }
}

void IssueSlipManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Draft", static_cast<int>(ERP::Material::DTO::IssueSlipStatus::DRAFT));
    statusComboBox_->addItem("Pending Approval", static_cast<int>(ERP::Material::DTO::IssueSlipStatus::PENDING_APPROVAL));
    statusComboBox_->addItem("Approved", static_cast<int>(ERP::Material::DTO::IssueSlipStatus::APPROVED));
    statusComboBox_->addItem("In Progress", static_cast<int>(ERP::Material::DTO::IssueSlipStatus::IN_PROGRESS));
    statusComboBox_->addItem("Completed", static_cast<int>(ERP::Material::DTO::IssueSlipStatus::COMPLETED));
    statusComboBox_->addItem("Cancelled", static_cast<int>(ERP::Material::DTO::IssueSlipStatus::CANCELLED));
    statusComboBox_->addItem("Rejected", static_cast<int>(ERP::Material::DTO::IssueSlipStatus::REJECTED));
}


void IssueSlipManagementWidget::onAddSlipClicked() {
    if (!hasPermission("Material.CreateIssueSlip")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm phiếu xuất kho.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateWarehouseComboBox();
    populateMaterialRequestSlipComboBox();
    showSlipInputDialog();
}

void IssueSlipManagementWidget::onEditSlipClicked() {
    if (!hasPermission("Material.UpdateIssueSlip")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa phiếu xuất kho.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Phiếu Xuất Kho", "Vui lòng chọn một phiếu xuất kho để sửa.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Material::DTO::IssueSlipDTO> slipOpt = issueSlipService_->getIssueSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (slipOpt) {
        populateWarehouseComboBox();
        populateMaterialRequestSlipComboBox();
        showSlipInputDialog(&(*slipOpt));
    } else {
        showMessageBox("Sửa Phiếu Xuất Kho", "Không tìm thấy phiếu xuất kho để sửa.", QMessageBox::Critical);
    }
}

void IssueSlipManagementWidget::onDeleteSlipClicked() {
    if (!hasPermission("Material.DeleteIssueSlip")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa phiếu xuất kho.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Phiếu Xuất Kho", "Vui lòng chọn một phiếu xuất kho để xóa.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    QString slipNumber = slipTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Phiếu Xuất Kho");
    confirmBox.setText("Bạn có chắc chắn muốn xóa phiếu xuất kho '" + slipNumber + "' (ID: " + slipId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (issueSlipService_->deleteIssueSlip(slipId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Phiếu Xuất Kho", "Phiếu xuất kho đã được xóa thành công.", QMessageBox::Information);
            loadIssueSlips();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa phiếu xuất kho. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void IssueSlipManagementWidget::onUpdateSlipStatusClicked() {
    if (!hasPermission("Material.UpdateIssueSlipStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái phiếu xuất kho.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một phiếu xuất kho để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Material::DTO::IssueSlipDTO> slipOpt = issueSlipService_->getIssueSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!slipOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy phiếu xuất kho để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Material::DTO::IssueSlipDTO currentSlip = *slipOpt;
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
        ERP::Material::DTO::IssueSlipStatus newStatus = static_cast<ERP::Material::DTO::IssueSlipStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái phiếu xuất kho");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái phiếu xuất kho '" + QString::fromStdString(currentSlip.issueNumber) + "' thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (issueSlipService_->updateIssueSlipStatus(slipId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái phiếu xuất kho đã được cập nhật thành công.", QMessageBox::Information);
                loadIssueSlips();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái phiếu xuất kho. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}


void IssueSlipManagementWidget::onSearchSlipClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["issue_number_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    slipTable_->setRowCount(0);
    std::vector<ERP::Material::DTO::IssueSlipDTO> slips = issueSlipService_->getAllIssueSlips(filter, currentUserId_, currentUserRoleIds_);

    slipTable_->setRowCount(slips.size());
    for (int i = 0; i < slips.size(); ++i) {
        const auto& slip = slips[i];
        slipTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(slip.id)));
        slipTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(slip.issueNumber)));
        
        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(slip.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) warehouseName = QString::fromStdString(warehouse->name);
        slipTable_->setItem(i, 2, new QTableWidgetItem(warehouseName));

        slipTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(slip.issueDate, ERP::Common::DATETIME_FORMAT))));
        slipTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(slip.getStatusString())));
        
        QString mrsNumber = "N/A";
        if (slip.materialRequestSlipId) {
            std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> mrs = materialRequestService_->getMaterialRequestSlipById(*slip.materialRequestSlipId, currentUserId_, currentUserRoleIds_);
            if (mrs) mrsNumber = QString::fromStdString(mrs->requestNumber);
        }
        slipTable_->setItem(i, 5, new QTableWidgetItem(mrsNumber));
    }
    slipTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("IssueSlipManagementWidget: Search completed.");
}

void IssueSlipManagementWidget::onSlipTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString slipId = slipTable_->item(row, 0)->text();
    std::optional<ERP::Material::DTO::IssueSlipDTO> slipOpt = issueSlipService_->getIssueSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (slipOpt) {
        idLineEdit_->setText(QString::fromStdString(slipOpt->id));
        issueNumberLineEdit_->setText(QString::fromStdString(slipOpt->issueNumber));
        
        populateWarehouseComboBox();
        int warehouseIndex = warehouseComboBox_->findData(QString::fromStdString(slipOpt->warehouseId));
        if (warehouseIndex != -1) warehouseComboBox_->setCurrentIndex(warehouseIndex);

        issuedByLineEdit_->setText(QString::fromStdString(slipOpt->issuedByUserId)); // Display ID
        issueDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(slipOpt->issueDate.time_since_epoch()).count()));
        
        populateMaterialRequestSlipComboBox();
        if (slipOpt->materialRequestSlipId) {
            int mrsIndex = materialRequestSlipComboBox_->findData(QString::fromStdString(*slipOpt->materialRequestSlipId));
            if (mrsIndex != -1) materialRequestSlipComboBox_->setCurrentIndex(mrsIndex);
            else materialRequestSlipComboBox_->setCurrentIndex(0); // "None"
        } else {
            materialRequestSlipComboBox_->setCurrentIndex(0); // "None"
        }

        int statusIndex = statusComboBox_->findData(static_cast<int>(slipOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);
        
        referenceDocumentIdLineEdit_->setText(QString::fromStdString(slipOpt->referenceDocumentId.value_or("")));
        referenceDocumentTypeLineEdit_->setText(QString::fromStdString(slipOpt->referenceDocumentType.value_or("")));
        notesLineEdit_->setText(QString::fromStdString(slipOpt->notes.value_or("")));

    } else {
        showMessageBox("Thông tin Phiếu Xuất Kho", "Không tìm thấy phiếu xuất kho đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void IssueSlipManagementWidget::clearForm() {
    idLineEdit_->clear();
    issueNumberLineEdit_->clear();
    warehouseComboBox_->clear(); // Repopulated on demand
    issuedByLineEdit_->clear();
    issueDateEdit_->clear();
    materialRequestSlipComboBox_->clear(); // Repopulated on demand
    statusComboBox_->setCurrentIndex(0); // Default to Draft
    referenceDocumentIdLineEdit_->clear();
    referenceDocumentTypeLineEdit_->clear();
    notesLineEdit_->clear();
    slipTable_->clearSelection();
    updateButtonsState();
}

void IssueSlipManagementWidget::onManageDetailsClicked() {
    if (!hasPermission("Material.ManageIssueSlipDetails")) { // Assuming this permission
        showMessageBox("Lỗi", "Bạn không có quyền quản lý chi tiết phiếu xuất kho.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Quản lý Chi tiết", "Vui lòng chọn một phiếu xuất kho để quản lý chi tiết.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Material::DTO::IssueSlipDTO> slipOpt = issueSlipService_->getIssueSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (slipOpt) {
        showManageDetailsDialog(&(*slipOpt));
    } else {
        showMessageBox("Quản lý Chi tiết", "Không tìm thấy phiếu xuất kho để quản lý chi tiết.", QMessageBox::Critical);
    }
}

void IssueSlipManagementWidget::onRecordIssuedQuantityClicked() {
    if (!hasPermission("Material.RecordIssuedQuantity")) {
        showMessageBox("Lỗi", "Bạn không có quyền ghi nhận số lượng xuất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Ghi nhận SL xuất", "Vui lòng chọn một phiếu xuất kho trước.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Material::DTO::IssueSlipDTO> parentSlipOpt = issueSlipService_->getIssueSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);
    if (!parentSlipOpt) {
        showMessageBox("Ghi nhận SL xuất", "Không tìm thấy phiếu xuất kho.", QMessageBox::Critical);
        return;
    }

    // Show a dialog to select a detail and input quantity
    QDialog dialog(this);
    dialog.setWindowTitle("Ghi nhận Số lượng Xuất Thực tế");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox detailComboBox;
    // Populate with details of the selected slip
    std::vector<ERP::Material::DTO::IssueSlipDetailDTO> details = issueSlipService_->getIssueSlipDetails(slipId.toStdString(), currentUserId_, currentUserRoleIds_);
    for (const auto& detail : details) {
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(detail.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);
        detailComboBox.addItem(productName + " (YC: " + QString::number(detail.requestedQuantity) + ", Đã xuất: " + QString::number(detail.issuedQuantity) + ")", QString::fromStdString(detail.id));
    }

    QLineEdit quantityLineEdit;
    quantityLineEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));

    formLayout->addRow("Chọn Chi tiết:", &detailComboBox);
    formLayout->addRow("Số lượng Xuất Thực tế:*", &quantityLineEdit);
    layout->addLayout(formLayout);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString selectedDetailId = detailComboBox.currentData().toString();
        double quantity = quantityLineEdit.text().toDouble();

        std::optional<ERP::Material::DTO::IssueSlipDetailDTO> selectedDetailOpt = issueSlipService_->getIssueSlipDetailById(selectedDetailId.toStdString());
        if (!selectedDetailOpt) {
            showMessageBox("Lỗi", "Không tìm thấy chi tiết phiếu xuất kho đã chọn.", QMessageBox::Critical);
            return;
        }

        if (issueSlipService_->recordIssuedQuantity(selectedDetailId.toStdString(), quantity, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Ghi nhận SL xuất", "Số lượng xuất đã được ghi nhận thành công.", QMessageBox::Information);
            loadIssueSlips(); // Refresh parent table
            clearForm();
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể ghi nhận số lượng xuất. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void IssueSlipManagementWidget::showSlipInputDialog(ERP::Material::DTO::IssueSlipDTO* slip) {
    QDialog dialog(this);
    dialog.setWindowTitle(slip ? "Sửa Phiếu Xuất Kho" : "Thêm Phiếu Xuất Kho Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit issueNumberEdit(this);
    QComboBox warehouseCombo(this); populateWarehouseComboBox();
    for (int i = 0; i < warehouseComboBox_->count(); ++i) warehouseCombo.addItem(warehouseComboBox_->itemText(i), warehouseComboBox_->itemData(i));
    
    QLineEdit issuedByEdit(this); issuedByEdit.setReadOnly(true); // Auto-filled
    QDateTimeEdit issueDateEdit(this); issueDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QComboBox materialRequestSlipCombo(this); populateMaterialRequestSlipComboBox();
    for (int i = 0; i < materialRequestSlipComboBox_->count(); ++i) materialRequestSlipCombo.addItem(materialRequestSlipComboBox_->itemText(i), materialRequestSlipComboBox_->itemData(i));

    QComboBox statusCombo(this); populateStatusComboBox();
    for (int i = 0; i < statusComboBox_->count(); ++i) statusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));

    QLineEdit referenceDocumentIdEdit(this);
    QLineEdit referenceDocumentTypeEdit(this);
    QLineEdit notesEdit(this);

    if (slip) {
        issueNumberEdit.setText(QString::fromStdString(slip->issueNumber));
        int warehouseIndex = warehouseCombo.findData(QString::fromStdString(slip->warehouseId));
        if (warehouseIndex != -1) warehouseCombo.setCurrentIndex(warehouseIndex);
        issuedByEdit.setText(QString::fromStdString(slip->issuedByUserId));
        issueDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(slip->issueDate.time_since_epoch()).count()));
        if (slip->materialRequestSlipId) {
            int mrsIndex = materialRequestSlipCombo.findData(QString::fromStdString(*slip->materialRequestSlipId));
            if (mrsIndex != -1) materialRequestSlipCombo.setCurrentIndex(mrsIndex);
            else materialRequestSlipCombo.setCurrentIndex(0);
        } else {
            materialRequestSlipCombo.setCurrentIndex(0);
        }
        int statusIndex = statusCombo.findData(static_cast<int>(slip->status));
        if (statusIndex != -1) statusCombo.setCurrentIndex(statusIndex);
        referenceDocumentIdEdit.setText(QString::fromStdString(slip->referenceDocumentId.value_or("")));
        referenceDocumentTypeEdit.setText(QString::fromStdString(slip->referenceDocumentType.value_or("")));
        notesEdit.setText(QString::fromStdString(slip->notes.value_or("")));

        issueNumberEdit.setReadOnly(true); // Don't allow changing issue number for existing
    } else {
        issueNumberEdit.setText("IS-" + QString::fromStdString(ERP::Utils::generateUUID().substr(0, 8))); // Auto-generate
        issueDateEdit.setDateTime(QDateTime::currentDateTime());
        issuedByEdit.setText(QString::fromStdString(currentUserId_)); // Auto-fill current user
    }

    formLayout->addRow("Số Phiếu Xuất:*", &issueNumberEdit);
    formLayout->addRow("Kho hàng:*", &warehouseCombo);
    formLayout->addRow("Người xuất:", &issuedByEdit);
    formLayout->addRow("Ngày Xuất:*", &issueDateEdit);
    formLayout->addRow("YC Vật tư liên kết:", &materialRequestSlipCombo);
    formLayout->addRow("Trạng thái:*", &statusCombo);
    formLayout->addRow("ID Tài liệu tham chiếu:", &referenceDocumentIdEdit);
    formLayout->addRow("Loại Tài liệu tham chiếu:", &referenceDocumentTypeEdit);
    formLayout->addRow("Ghi chú:", &notesEdit);
    
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
        ERP::Material::DTO::IssueSlipDTO newSlipData;
        if (slip) {
            newSlipData = *slip;
        }

        newSlipData.issueNumber = issueNumberEdit.text().toStdString();
        newSlipData.warehouseId = warehouseCombo.currentData().toString().toStdString();
        newSlipData.issuedByUserId = issuedByEdit.text().toStdString();
        newSlipData.issueDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(issueDateEdit.dateTime());
        
        QString selectedMrsId = materialRequestSlipCombo.currentData().toString();
        newSlipData.materialRequestSlipId = selectedMrsId.isEmpty() ? std::nullopt : std::make_optional(selectedMrsId.toStdString());

        newSlipData.status = static_cast<ERP::Material::DTO::IssueSlipStatus>(statusCombo.currentData().toInt());
        newSlipData.referenceDocumentId = referenceDocumentIdEdit.text().isEmpty() ? std::nullopt : std::make_optional(referenceDocumentIdEdit.text().toStdString());
        newSlipData.referenceDocumentType = referenceDocumentTypeEdit.text().isEmpty() ? std::nullopt : std::make_optional(referenceDocumentTypeEdit.text().toStdString());
        newSlipData.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());

        // For new slips, details are added in a separate dialog after creation
        std::vector<ERP::Material::DTO::IssueSlipDetailDTO> currentDetails;
        if (slip) { // When editing, load existing details first
             currentDetails = issueSlipService_->getIssueSlipDetails(slip->id, currentUserId_, currentUserRoleIds_);
        }

        bool success = false;
        if (slip) {
            success = issueSlipService_->updateIssueSlip(newSlipData, currentDetails, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Phiếu Xuất Kho", "Phiếu xuất kho đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật phiếu xuất kho. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Material::DTO::IssueSlipDTO> createdSlip = issueSlipService_->createIssueSlip(newSlipData, {}, currentUserId_, currentUserRoleIds_); // Create with empty details
            if (createdSlip) {
                showMessageBox("Thêm Phiếu Xuất Kho", "Phiếu xuất kho mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm phiếu xuất kho mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadIssueSlips();
            clearForm();
        }
    }
}

void IssueSlipManagementWidget::showManageDetailsDialog(ERP::Material::DTO::IssueSlipDTO* slip) {
    if (!slip) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Quản lý Chi tiết Phiếu Xuất Kho: " + QString::fromStdString(slip->issueNumber));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *detailsTable = new QTableWidget(&dialog);
    detailsTable->setColumnCount(7); // Product, Location, Req Qty, Issued Qty, Lot/Serial, Notes, Is Fully Issued
    detailsTable->setHorizontalHeaderLabels({"Sản phẩm", "Vị trí", "SL YC", "SL Xuất", "Số lô/Serial", "Ghi chú", "Đã xuất đủ"});
    detailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    detailsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    detailsTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(detailsTable);

    // Load existing details
    std::vector<ERP::Material::DTO::IssueSlipDetailDTO> currentDetails = issueSlipService_->getIssueSlipDetails(slip->id, currentUserId_, currentUserRoleIds_);
    detailsTable->setRowCount(currentDetails.size());
    for (int i = 0; i < currentDetails.size(); ++i) {
        const auto& detail = currentDetails[i];
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(detail.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);

        QString locationName = "N/A";
        std::optional<ERP::Catalog::DTO::LocationDTO> location = warehouseService_->getLocationById(detail.locationId, currentUserId_, currentUserRoleIds_);
        if (location) locationName = QString::fromStdString(location->name);

        detailsTable->setItem(i, 0, new QTableWidgetItem(productName));
        detailsTable->setItem(i, 1, new QTableWidgetItem(locationName));
        detailsTable->setItem(i, 2, new QTableWidgetItem(QString::number(detail.requestedQuantity)));
        detailsTable->setItem(i, 3, new QTableWidgetItem(QString::number(detail.issuedQuantity)));
        detailsTable->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(detail.lotNumber.value_or("") + "/" + detail.serialNumber.value_or(""))));
        detailsTable->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(detail.notes.value_or(""))));
        detailsTable->setItem(i, 6, new QTableWidgetItem(detail.isFullyIssued ? "Yes" : "No"));
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
        itemDialog.setWindowTitle("Thêm Chi tiết Phiếu Xuất Kho");
        QFormLayout itemFormLayout;
        QComboBox productCombo; populateProductComboBox();
        for (int i = 0; i < productService_->getAllProducts({}).size(); ++i) productCombo.addItem(productComboBox_->itemText(i), productComboBox_->itemData(i));
        QComboBox locationCombo;
        // Populate locations based on selected warehouse (from parent slip)
        std::vector<ERP::Catalog::DTO::LocationDTO> locationsInWarehouse = warehouseService_->getLocationsByWarehouse(slip->warehouseId, currentUserId_, currentUserRoleIds_);
        for (const auto& loc : locationsInWarehouse) locationCombo.addItem(QString::fromStdString(loc.name), QString::fromStdString(loc.id));

        QLineEdit requestedQuantityEdit; requestedQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit lotNumberEdit;
        QLineEdit serialNumberEdit;
        QLineEdit notesEdit;

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Vị trí:*", &locationCombo);
        itemFormLayout.addRow("Số lượng YC:*", &requestedQuantityEdit);
        itemFormLayout.addRow("Số lô:", &lotNumberEdit);
        itemFormLayout.addRow("Số Serial:", &serialNumberEdit);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);

        QPushButton *okItemButton = new QPushButton("Thêm", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (productCombo.currentData().isNull() || locationCombo.currentData().isNull() || requestedQuantityEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Add to table
            int newRow = detailsTable->rowCount();
            detailsTable->insertRow(newRow);
            detailsTable->setItem(newRow, 0, new QTableWidgetItem(productCombo.currentText()));
            detailsTable->setItem(newRow, 1, new QTableWidgetItem(locationCombo.currentText()));
            detailsTable->setItem(newRow, 2, new QTableWidgetItem(requestedQuantityEdit.text()));
            detailsTable->setItem(newRow, 3, new QTableWidgetItem("0.0")); // Issued Qty
            detailsTable->setItem(newRow, 4, new QTableWidgetItem(lotNumberEdit.text() + "/" + serialNumberEdit.text()));
            detailsTable->setItem(newRow, 5, new QTableWidgetItem(notesEdit.text()));
            detailsTable->setItem(newRow, 6, new QTableWidgetItem("No")); // Is Fully Issued
            detailsTable->item(newRow, 0)->setData(Qt::UserRole, productCombo.currentData()); // Store product ID
            detailsTable->item(newRow, 1)->setData(Qt::UserRole, locationCombo.currentData()); // Store location ID
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
        itemDialog.setWindowTitle("Sửa Chi tiết Phiếu Xuất Kho");
        QFormLayout itemFormLayout;
        QComboBox productCombo; populateProductComboBox();
        for (int i = 0; i < productService_->getAllProducts({}).size(); ++i) productCombo.addItem(productComboBox_->itemText(i), productComboBox_->itemData(i));
        QComboBox locationCombo;
        std::vector<ERP::Catalog::DTO::LocationDTO> locationsInWarehouse = warehouseService_->getLocationsByWarehouse(slip->warehouseId, currentUserId_, currentUserRoleIds_);
        for (const auto& loc : locationsInWarehouse) locationCombo.addItem(QString::fromStdString(loc.name), QString::fromStdString(loc.id));

        QLineEdit requestedQuantityEdit; requestedQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit lotNumberEdit;
        QLineEdit serialNumberEdit;
        QLineEdit notesEdit;

        // Populate with current item data
        QString currentProductId = detailsTable->item(selectedItemRow, 0)->data(Qt::UserRole).toString();
        int productIndex = productCombo.findData(currentProductId);
        if (productIndex != -1) productCombo.setCurrentIndex(productIndex);

        QString currentLocationId = detailsTable->item(selectedItemRow, 1)->data(Qt::UserRole).toString();
        int locationIndex = locationCombo.findData(currentLocationId);
        if (locationIndex != -1) locationCombo.setCurrentIndex(locationIndex);

        requestedQuantityEdit.setText(detailsTable->item(selectedItemRow, 2)->text());
        // Issued quantity is read-only in this dialog (updated via recordIssuedQuantity)
        
        QString lotSerialText = detailsTable->item(selectedItemRow, 4)->text();
        QStringList lotSerialParts = lotSerialText.split("/");
        if (lotSerialParts.size() > 0) lotNumberEdit.setText(lotSerialParts[0]);
        if (lotSerialParts.size() > 1) serialNumberEdit.setText(lotSerialParts[1]);

        notesEdit.setText(detailsTable->item(selectedItemRow, 5)->text());

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Vị trí:*", &locationCombo);
        itemFormLayout.addRow("Số lượng YC:*", &requestedQuantityEdit);
        itemFormLayout.addRow("Số lô:", &lotNumberEdit);
        itemFormLayout.addRow("Số Serial:", &serialNumberEdit);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);

        QPushButton *okItemButton = new QPushButton("Lưu", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (productCombo.currentData().isNull() || locationCombo.currentData().isNull() || requestedQuantityEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Update table row
            detailsTable->item(selectedItemRow, 0)->setText(productCombo.currentText());
            detailsTable->item(selectedItemRow, 1)->setText(locationCombo.currentText());
            detailsTable->item(selectedItemRow, 2)->setText(requestedQuantityEdit.text());
            // detailsTable->item(selectedItemRow, 3) is issued qty, not updated here
            detailsTable->item(selectedItemRow, 4)->setText(lotNumberEdit.text() + "/" + serialNumberEdit.text());
            detailsTable->item(selectedItemRow, 5)->setText(notesEdit.text());
            // detailsTable->item(selectedItemRow, 6) is fully issued, not updated here
            detailsTable->item(selectedItemRow, 0)->setData(Qt::UserRole, productCombo.currentData());
            detailsTable->item(selectedItemRow, 1)->setData(Qt::UserRole, locationCombo.currentData());
        }
    });

    connect(deleteItemButton, &QPushButton::clicked, [&]() {
        int selectedItemRow = detailsTable->currentRow();
        if (selectedItemRow < 0) {
            showMessageBox("Xóa Chi tiết", "Vui lòng chọn một chi tiết để xóa.", QMessageBox::Information);
            return;
        }
        Common::CustomMessageBox confirmDelBox(&dialog);
        confirmDelBox.setWindowTitle("Xóa Chi tiết Phiếu Xuất Kho");
        confirmDelBox.setText("Bạn có chắc chắn muốn xóa chi tiết phiếu xuất kho này?");
        confirmDelBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmDelBox.exec() == QMessageBox::Yes) {
            detailsTable->removeRow(selectedItemRow);
        }
    });

    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Collect all items from the table
        std::vector<ERP::Material::DTO::IssueSlipDetailDTO> updatedDetails;
        for (int i = 0; i < detailsTable->rowCount(); ++i) {
            ERP::Material::DTO::IssueSlipDetailDTO detail;
            // If item had an ID (from existing data), reuse it, otherwise generate new
            QString existingId = detailsTable->item(i, 0)->data(Qt::UserRole).toString();
            detail.id = existingId.isEmpty() ? ERP::Utils::generateUUID() : existingId.toStdString();

            detail.issueSlipId = slip->id;
            detail.productId = detailsTable->item(i, 0)->data(Qt::UserRole).toString().toStdString();
            detail.locationId = detailsTable->item(i, 1)->data(Qt::UserRole).toString().toStdString();
            detail.requestedQuantity = detailsTable->item(i, 2)->text().toDouble();
            detail.issuedQuantity = detailsTable->item(i, 3)->text().toDouble(); // Keep current issued qty
            
            QString lotSerialText = detailsTable->item(i, 4)->text();
            QStringList lotSerialParts = lotSerialText.split("/");
            if (lotSerialParts.size() > 0) detail.lotNumber = lotSerialParts[0].toStdString();
            if (lotSerialParts.size() > 1) detail.serialNumber = lotSerialParts[1].toStdString();

            detail.notes = detailsTable->item(i, 5)->text().isEmpty() ? std::nullopt : std::make_optional(detailsTable->item(i, 5)->text().toStdString());
            detail.isFullyIssued = detailsTable->item(i, 6)->text() == "Yes"; // Keep current status
            
            // Assuming creation/update logic will handle createdAt/createdBy/status from parent slip
            updatedDetails.push_back(detail);
        }

        // Call service to update Issue Slip with new detail list
        if (issueSlipService_->updateIssueSlip(*slip, updatedDetails, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Quản lý Chi tiết", "Chi tiết phiếu xuất kho đã được cập nhật thành công.", QMessageBox::Information);
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật chi tiết phiếu xuất kho. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void IssueSlipManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool IssueSlipManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void IssueSlipManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Material.CreateIssueSlip");
    bool canUpdate = hasPermission("Material.UpdateIssueSlip");
    bool canDelete = hasPermission("Material.DeleteIssueSlip");
    bool canChangeStatus = hasPermission("Material.UpdateIssueSlipStatus");
    bool canManageDetails = hasPermission("Material.ManageIssueSlipDetails"); // Assuming this permission
    bool canRecordQuantity = hasPermission("Material.RecordIssuedQuantity");

    addSlipButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Material.ViewIssueSlips"));

    bool isRowSelected = slipTable_->currentRow() >= 0;
    editSlipButton_->setEnabled(isRowSelected && canUpdate);
    deleteSlipButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    manageDetailsButton_->setEnabled(isRowSelected && canManageDetails);
    recordIssuedQuantityButton_->setEnabled(isRowSelected && canRecordQuantity);


    bool enableForm = isRowSelected && canUpdate;
    issueNumberLineEdit_->setEnabled(enableForm); // Will be read-only for existing anyway
    warehouseComboBox_->setEnabled(enableForm);
    // issuedByLineEdit_ is read-only
    issueDateEdit_->setEnabled(enableForm);
    materialRequestSlipComboBox_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    referenceDocumentIdLineEdit_->setEnabled(enableForm);
    referenceDocumentTypeLineEdit_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        issueNumberLineEdit_->clear();
        warehouseComboBox_->clear();
        issuedByLineEdit_->clear();
        issueDateEdit_->clear();
        materialRequestSlipComboBox_->clear();
        statusComboBox_->setCurrentIndex(0);
        referenceDocumentIdLineEdit_->clear();
        referenceDocumentTypeLineEdit_->clear();
        notesLineEdit_->clear();
    }
}


} // namespace Material
} // namespace UI
} // namespace ERP