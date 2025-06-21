// UI/Material/ReceiptSlipManagementWidget.cpp
#include "ReceiptSlipManagementWidget.h" // Đã rút gọn include
#include "ReceiptSlip.h"                 // Đã rút gọn include
#include "ReceiptSlipDetail.h"           // Đã rút gọn include
#include "Product.h"                     // Đã rút gọn include
#include "Warehouse.h"                   // Đã rút gọn include
#include "Location.h"                    // Đã rút gọn include
#include "ReceiptSlipService.h"          // Đã rút gọn include
#include "ProductService.h"              // Đã rút gọn include
#include "WarehouseService.h"            // Đã rút gọn include
#include "InventoryManagementService.h"  // Đã rút gọn include
#include "ISecurityManager.h"            // Đã rút gọn include
#include "Logger.h"                      // Đã rút gọn include
#include "ErrorHandler.h"                // Đã rút gọn include
#include "Common.h"                      // Đã rút gọn include
#include "DateUtils.h"                   // Đã rút gọn include
#include "StringUtils.h"                 // Đã rút gọn include
#include "CustomMessageBox.h"            // Hộp thoại thông báo tùy chỉnh
#include "UserService.h"                 // For getting user names

#include <QInputDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDoubleValidator>
#include <QDialogButtonBox>

namespace ERP {
namespace UI {
namespace Material {

ReceiptSlipManagementWidget::ReceiptSlipManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IReceiptSlipService> receiptSlipService,
    std::shared_ptr<Product::Services::IProductService> productService,
    std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<Warehouse::Services::IInventoryManagementService> inventoryManagementService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      receiptSlipService_(receiptSlipService),
      productService_(productService),
      warehouseService_(warehouseService),
      inventoryManagementService_(inventoryManagementService),
      securityManager_(securityManager) {

    if (!receiptSlipService_ || !productService_ || !warehouseService_ || !inventoryManagementService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ phiếu nhập kho hoặc các dịch vụ phụ thuộc không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("ReceiptSlipManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("ReceiptSlipManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("ReceiptSlipManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadReceiptSlips();
    updateButtonsState();
}

ReceiptSlipManagementWidget::~ReceiptSlipManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void ReceiptSlipManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo số phiếu nhập...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &ReceiptSlipManagementWidget::onSearchSlipClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    slipTable_ = new QTableWidget(this);
    slipTable_->setColumnCount(6); // ID, Số phiếu, Kho hàng, Ngày nhập, Trạng thái, Tài liệu tham chiếu
    slipTable_->setHorizontalHeaderLabels({"ID", "Số Phiếu Nhập", "Kho hàng", "Ngày Nhập", "Trạng thái", "Tài liệu tham chiếu"});
    slipTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    slipTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    slipTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    slipTable_->horizontalHeader()->setStretchLastSection(true);
    connect(slipTable_, &QTableWidget::itemClicked, this, &ReceiptSlipManagementWidget::onSlipTableItemClicked);
    mainLayout->addWidget(slipTable_);

    // Form elements for editing/adding slips
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    receiptNumberLineEdit_ = new QLineEdit(this);
    warehouseComboBox_ = new QComboBox(this); populateWarehouseComboBox();
    receivedByLineEdit_ = new QLineEdit(this); receivedByLineEdit_->setReadOnly(true); // Should be current user
    receiptDateEdit_ = new QDateTimeEdit(this); receiptDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();
    referenceDocumentIdLineEdit_ = new QLineEdit(this);
    referenceDocumentTypeLineEdit_ = new QLineEdit(this);
    notesLineEdit_ = new QLineEdit(this);


    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("Số Phiếu Nhập:*", &receiptNumberLineEdit_);
    formLayout->addRow("Kho hàng:*", &warehouseComboBox_);
    formLayout->addRow("Người nhận:", &receivedByLineEdit_);
    formLayout->addRow("Ngày Nhập:*", &receiptDateEdit_);
    formLayout->addRow("Trạng thái:*", &statusComboBox_);
    formLayout->addRow("ID Tài liệu tham chiếu:", &referenceDocumentIdLineEdit_);
    formLayout->addRow("Loại Tài liệu tham chiếu:", &referenceDocumentTypeLineEdit_);
    formLayout->addRow("Ghi chú:", &notesLineEdit_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addSlipButton_ = new QPushButton("Thêm mới", this); connect(addSlipButton_, &QPushButton::clicked, this, &ReceiptSlipManagementWidget::onAddSlipClicked);
    editSlipButton_ = new QPushButton("Sửa", this); connect(editSlipButton_, &QPushButton::clicked, this, &ReceiptSlipManagementWidget::onEditSlipClicked);
    deleteSlipButton_ = new QPushButton("Xóa", this); connect(deleteSlipButton_, &QPushButton::clicked, this, &ReceiptSlipManagementWidget::onDeleteSlipClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &ReceiptSlipManagementWidget::onUpdateSlipStatusClicked);
    manageDetailsButton_ = new QPushButton("Quản lý Chi tiết", this); connect(manageDetailsButton_, &QPushButton::clicked, this, &ReceiptSlipManagementWidget::onManageDetailsClicked);
    recordReceivedQuantityButton_ = new QPushButton("Ghi nhận SL nhận", this); connect(recordReceivedQuantityButton_, &QPushButton::clicked, this, &ReceiptSlipManagementWidget::onRecordReceivedQuantityClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &ReceiptSlipManagementWidget::onSearchSlipClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &ReceiptSlipManagementWidget::clearForm);
    
    buttonLayout->addWidget(addSlipButton_);
    buttonLayout->addWidget(editSlipButton_);
    buttonLayout->addWidget(deleteSlipButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(manageDetailsButton_);
    buttonLayout->addWidget(recordReceivedQuantityButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void ReceiptSlipManagementWidget::loadReceiptSlips() {
    ERP::Logger::Logger::getInstance().info("ReceiptSlipManagementWidget: Loading receipt slips...");
    slipTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Material::DTO::ReceiptSlipDTO> slips = receiptSlipService_->getAllReceiptSlips({}, currentUserId_, currentUserRoleIds_);

    slipTable_->setRowCount(slips.size());
    for (int i = 0; i < slips.size(); ++i) {
        const auto& slip = slips[i];
        slipTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(slip.id)));
        slipTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(slip.receiptNumber)));
        
        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(slip.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) warehouseName = QString::fromStdString(warehouse->name);
        slipTable_->setItem(i, 2, new QTableWidgetItem(warehouseName));

        slipTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(slip.receiptDate, ERP::Common::DATETIME_FORMAT))));
        slipTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(slip.getStatusString())));
        
        QString refDoc = slip.referenceDocumentId.value_or("") + " (" + slip.referenceDocumentType.value_or("") + ")";
        if (refDoc == " ()") refDoc = "N/A";
        slipTable_->setItem(i, 5, new QTableWidgetItem(refDoc));
    }
    slipTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ReceiptSlipManagementWidget: Receipt slips loaded successfully.");
}

void ReceiptSlipManagementWidget::populateWarehouseComboBox() {
    warehouseComboBox_->clear();
    std::vector<ERP::Catalog::DTO::WarehouseDTO> allWarehouses = warehouseService_->getAllWarehouses({}, currentUserId_, currentUserRoleIds_);
    for (const auto& warehouse : allWarehouses) {
        warehouseComboBox_->addItem(QString::fromStdString(warehouse.name), QString::fromStdString(warehouse.id));
    }
}

void ReceiptSlipManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Draft", static_cast<int>(ERP::Material::DTO::ReceiptSlipStatus::DRAFT));
    statusComboBox_->addItem("Pending Approval", static_cast<int>(ERP::Material::DTO::ReceiptSlipStatus::PENDING_APPROVAL));
    statusComboBox_->addItem("Approved", static_cast<int>(ERP::Material::DTO::ReceiptSlipStatus::APPROVED));
    statusComboBox_->addItem("In Progress", static_cast<int>(ERP::Material::DTO::ReceiptSlipStatus::IN_PROGRESS));
    statusComboBox_->addItem("Completed", static_cast<int>(ERP::Material::DTO::ReceiptSlipStatus::COMPLETED));
    statusComboBox_->addItem("Cancelled", static_cast<int>(ERP::Material::DTO::ReceiptSlipStatus::CANCELLED));
    statusComboBox_->addItem("Rejected", static_cast<int>(ERP::Material::DTO::ReceiptSlipStatus::REJECTED));
}


void ReceiptSlipManagementWidget::onAddSlipClicked() {
    if (!hasPermission("Material.CreateReceiptSlip")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm phiếu nhập kho.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateWarehouseComboBox();
    showSlipInputDialog();
}

void ReceiptSlipManagementWidget::onEditSlipClicked() {
    if (!hasPermission("Material.UpdateReceiptSlip")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa phiếu nhập kho.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Phiếu Nhập Kho", "Vui lòng chọn một phiếu nhập kho để sửa.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Material::DTO::ReceiptSlipDTO> slipOpt = receiptSlipService_->getReceiptSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (slipOpt) {
        populateWarehouseComboBox();
        showSlipInputDialog(&(*slipOpt));
    } else {
        showMessageBox("Sửa Phiếu Nhập Kho", "Không tìm thấy phiếu nhập kho để sửa.", QMessageBox::Critical);
    }
}

void ReceiptSlipManagementWidget::onDeleteSlipClicked() {
    if (!hasPermission("Material.DeleteReceiptSlip")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa phiếu nhập kho.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Phiếu Nhập Kho", "Vui lòng chọn một phiếu nhập kho để xóa.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    QString slipNumber = slipTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Phiếu Nhập Kho");
    confirmBox.setText("Bạn có chắc chắn muốn xóa phiếu nhập kho '" + slipNumber + "' (ID: " + slipId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (receiptSlipService_->deleteReceiptSlip(slipId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Phiếu Nhập Kho", "Phiếu nhập kho đã được xóa thành công.", QMessageBox::Information);
            loadReceiptSlips();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa phiếu nhập kho. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void ReceiptSlipManagementWidget::onUpdateSlipStatusClicked() {
    if (!hasPermission("Material.UpdateReceiptSlipStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái phiếu nhập kho.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một phiếu nhập kho để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Material::DTO::ReceiptSlipDTO> slipOpt = receiptSlipService_->getReceiptSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!slipOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy phiếu nhập kho để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Material::DTO::ReceiptSlipDTO currentSlip = *slipOpt;
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
        ERP::Material::DTO::ReceiptSlipStatus newStatus = static_cast<ERP::Material::DTO::ReceiptSlipStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái phiếu nhập kho");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái phiếu nhập kho '" + QString::fromStdString(currentSlip.receiptNumber) + "' thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (receiptSlipService_->updateReceiptSlipStatus(slipId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái phiếu nhập kho đã được cập nhật thành công.", QMessageBox::Information);
                loadReceiptSlips();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái phiếu nhập kho. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}


void ReceiptSlipManagementWidget::onSearchSlipClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["receipt_number_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    slipTable_->setRowCount(0);
    std::vector<ERP::Material::DTO::ReceiptSlipDTO> slips = receiptSlipService_->getAllReceiptSlips(filter, currentUserId_, currentUserRoleIds_);

    slipTable_->setRowCount(slips.size());
    for (int i = 0; i < slips.size(); ++i) {
        const auto& slip = slips[i];
        slipTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(slip.id)));
        slipTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(slip.receiptNumber)));
        
        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(slip.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) warehouseName = QString::fromStdString(warehouse->name);
        slipTable_->setItem(i, 2, new QTableWidgetItem(warehouseName));

        slipTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(slip.receiptDate, ERP::Common::DATETIME_FORMAT))));
        slipTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(slip.getStatusString())));
        
        QString refDoc = slip.referenceDocumentId.value_or("") + " (" + slip.referenceDocumentType.value_or("") + ")";
        if (refDoc == " ()") refDoc = "N/A";
        slipTable_->setItem(i, 5, new QTableWidgetItem(refDoc));
    }
    slipTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ReceiptSlipManagementWidget: Search completed.");
}

void ReceiptSlipManagementWidget::onSlipTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString slipId = slipTable_->item(row, 0)->text();
    std::optional<ERP::Material::DTO::ReceiptSlipDTO> slipOpt = receiptSlipService_->getReceiptSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (slipOpt) {
        idLineEdit_->setText(QString::fromStdString(slipOpt->id));
        receiptNumberLineEdit_->setText(QString::fromStdString(slipOpt->receiptNumber));
        
        populateWarehouseComboBox();
        int warehouseIndex = warehouseComboBox_->findData(QString::fromStdString(slipOpt->warehouseId));
        if (warehouseIndex != -1) warehouseComboBox_->setCurrentIndex(warehouseIndex);

        receivedByLineEdit_->setText(QString::fromStdString(slipOpt->receivedByUserId)); // Display ID
        receiptDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(slipOpt->receiptDate.time_since_epoch()).count()));
        
        int statusIndex = statusComboBox_->findData(static_cast<int>(slipOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);
        
        referenceDocumentIdLineEdit_->setText(QString::fromStdString(slipOpt->referenceDocumentId.value_or("")));
        referenceDocumentTypeLineEdit_->setText(QString::fromStdString(slipOpt->referenceDocumentType.value_or("")));
        notesLineEdit_->setText(QString::fromStdString(slipOpt->notes.value_or("")));

    } else {
        showMessageBox("Thông tin Phiếu Nhập Kho", "Không tìm thấy phiếu nhập kho đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void ReceiptSlipManagementWidget::clearForm() {
    idLineEdit_->clear();
    receiptNumberLineEdit_->clear();
    warehouseComboBox_->clear(); // Repopulated on demand
    receivedByLineEdit_->clear();
    receiptDateEdit_->clear();
    statusComboBox_->setCurrentIndex(0); // Default to Draft
    referenceDocumentIdLineEdit_->clear();
    referenceDocumentTypeLineEdit_->clear();
    notesLineEdit_->clear();
    slipTable_->clearSelection();
    updateButtonsState();
}

void ReceiptSlipManagementWidget::onManageDetailsClicked() {
    if (!hasPermission("Material.ManageReceiptSlipDetails")) { // Assuming this permission
        showMessageBox("Lỗi", "Bạn không có quyền quản lý chi tiết phiếu nhập kho.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Quản lý Chi tiết", "Vui lòng chọn một phiếu nhập kho để quản lý chi tiết.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Material::DTO::ReceiptSlipDTO> slipOpt = receiptSlipService_->getReceiptSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (slipOpt) {
        showManageDetailsDialog(&(*slipOpt));
    } else {
        showMessageBox("Quản lý Chi tiết", "Không tìm thấy phiếu nhập kho để quản lý chi tiết.", QMessageBox::Critical);
    }
}

void ReceiptSlipManagementWidget::onRecordReceivedQuantityClicked() {
    if (!hasPermission("Material.RecordReceivedQuantity")) {
        showMessageBox("Lỗi", "Bạn không có quyền ghi nhận số lượng nhận.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Ghi nhận SL nhận", "Vui lòng chọn một phiếu nhập kho trước.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Material::DTO::ReceiptSlipDTO> parentSlipOpt = receiptSlipService_->getReceiptSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);
    if (!parentSlipOpt) {
        showMessageBox("Ghi nhận SL nhận", "Không tìm thấy phiếu nhập kho.", QMessageBox::Critical);
        return;
    }

    // Show a dialog to select a detail and input quantity
    QDialog dialog(this);
    dialog.setWindowTitle("Ghi nhận Số lượng Nhận Thực tế");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox detailComboBox;
    // Populate with details of the selected slip
    std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> details = receiptSlipService_->getReceiptSlipDetails(slipId.toStdString(), currentUserId_, currentUserRoleIds_);
    for (const auto& detail : details) {
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(detail.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);
        QString locationName = "N/A";
        std::optional<ERP::Catalog::DTO::LocationDTO> location = warehouseService_->getLocationById(detail.locationId, currentUserId_, currentUserRoleIds_);
        if (location) locationName = QString::fromStdString(location->name);
        detailComboBox.addItem(productName + " (" + locationName + ") (Dự kiến: " + QString::number(detail.expectedQuantity) + ", Đã nhận: " + QString::number(detail.receivedQuantity) + ")", QString::fromStdString(detail.id));
    }

    QLineEdit quantityLineEdit;
    quantityLineEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));

    formLayout->addRow("Chọn Chi tiết:", &detailComboBox);
    formLayout->addRow("Số lượng Nhận Thực tế:*", &quantityLineEdit);
    layout->addLayout(formLayout);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString selectedDetailId = detailComboBox.currentData().toString();
        double quantity = quantityLineEdit.text().toDouble();

        std::optional<ERP::Material::DTO::ReceiptSlipDetailDTO> selectedDetailOpt = receiptSlipService_->getReceiptSlipDetailById(selectedDetailId.toStdString());
        if (!selectedDetailOpt) {
            showMessageBox("Lỗi", "Không tìm thấy chi tiết phiếu nhập kho đã chọn.", QMessageBox::Critical);
            return;
        }

        if (receiptSlipService_->recordReceivedQuantity(selectedDetailId.toStdString(), quantity, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Ghi nhận SL nhận", "Số lượng nhận đã được ghi nhận thành công.", QMessageBox::Information);
            loadReceiptSlips(); // Refresh parent table
            clearForm();
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể ghi nhận số lượng nhận. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void ReceiptSlipManagementWidget::showSlipInputDialog(ERP::Material::DTO::ReceiptSlipDTO* slip) {
    QDialog dialog(this);
    dialog.setWindowTitle(slip ? "Sửa Phiếu Nhập Kho" : "Thêm Phiếu Nhập Kho Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit receiptNumberEdit(this);
    QComboBox warehouseCombo(this); populateWarehouseComboBox();
    for (int i = 0; i < warehouseComboBox_->count(); ++i) warehouseCombo.addItem(warehouseComboBox_->itemText(i), warehouseComboBox_->itemData(i));
    
    QLineEdit receivedByEdit(this); receivedByEdit.setReadOnly(true); // Auto-filled
    QDateTimeEdit receiptDateEdit(this); receiptDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QComboBox statusCombo(this); populateStatusComboBox();
    for (int i = 0; i < statusComboBox_->count(); ++i) statusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));
    
    QLineEdit referenceDocumentIdEdit(this);
    QLineEdit referenceDocumentTypeEdit(this);
    QLineEdit notesEdit(this);

    if (slip) {
        receiptNumberEdit.setText(QString::fromStdString(slip->receiptNumber));
        int warehouseIndex = warehouseCombo.findData(QString::fromStdString(slip->warehouseId));
        if (warehouseIndex != -1) warehouseCombo.setCurrentIndex(warehouseIndex);
        receivedByEdit.setText(QString::fromStdString(slip->receivedByUserId));
        receiptDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(slip->receiptDate.time_since_epoch()).count()));
        int statusIndex = statusCombo.findData(static_cast<int>(slip->status));
        if (statusIndex != -1) statusCombo.setCurrentIndex(statusIndex);
        referenceDocumentIdEdit.setText(QString::fromStdString(slip->referenceDocumentId.value_or("")));
        referenceDocumentTypeEdit.setText(QString::fromStdString(slip->referenceDocumentType.value_or("")));
        notesEdit.setText(QString::fromStdString(slip->notes.value_or("")));

        receiptNumberEdit.setReadOnly(true); // Don't allow changing receipt number for existing
    } else {
        receiptNumberEdit.setText("RS-" + QString::fromStdString(ERP::Utils::generateUUID().substr(0, 8))); // Auto-generate
        receiptDateEdit.setDateTime(QDateTime::currentDateTime());
        receivedByEdit.setText(QString::fromStdString(currentUserId_)); // Auto-fill current user
    }

    formLayout->addRow("Số Phiếu Nhập:*", &receiptNumberEdit);
    formLayout->addRow("Kho hàng:*", &warehouseCombo);
    formLayout->addRow("Người nhận:", &receivedByEdit);
    formLayout->addRow("Ngày Nhập:*", &receiptDateEdit);
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
        ERP::Material::DTO::ReceiptSlipDTO newSlipData;
        if (slip) {
            newSlipData = *slip;
        }

        newSlipData.receiptNumber = receiptNumberEdit.text().toStdString();
        newSlipData.warehouseId = warehouseCombo.currentData().toString().toStdString();
        newSlipData.receivedByUserId = receivedByEdit.text().toStdString();
        newSlipData.receiptDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(receiptDateEdit.dateTime());
        newSlipData.status = static_cast<ERP::Material::DTO::ReceiptSlipStatus>(statusCombo.currentData().toInt());
        newSlipData.referenceDocumentId = referenceDocumentIdEdit.text().isEmpty() ? std::nullopt : std::make_optional(referenceDocumentIdEdit.text().toStdString());
        newSlipData.referenceDocumentType = referenceDocumentTypeEdit.text().isEmpty() ? std::nullopt : std::make_optional(referenceDocumentTypeEdit.text().toStdString());
        newSlipData.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());

        // For new slips, details are added in a separate dialog after creation
        std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> currentDetails;
        if (slip) { // When editing, load existing details first
             currentDetails = receiptSlipService_->getReceiptSlipDetails(slip->id, currentUserId_, currentUserRoleIds_);
        }

        bool success = false;
        if (slip) {
            success = receiptSlipService_->updateReceiptSlip(newSlipData, currentDetails, currentUserId_, currentUserRoleIds_); // Pass existing details if not modified
            if (success) showMessageBox("Sửa Phiếu Nhập Kho", "Phiếu nhập kho đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật phiếu nhập kho. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Material::DTO::ReceiptSlipDTO> createdSlip = receiptSlipService_->createReceiptSlip(newSlipData, {}, currentUserId_, currentUserRoleIds_); // Create with empty details
            if (createdSlip) {
                showMessageBox("Thêm Phiếu Nhập Kho", "Phiếu nhập kho mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm phiếu nhập kho mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadReceiptSlips();
            clearForm();
        }
    }
}

void ReceiptSlipManagementWidget::showManageDetailsDialog(ERP::Material::DTO::ReceiptSlipDTO* slip) {
    if (!slip) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Quản lý Chi tiết Phiếu Nhập Kho: " + QString::fromStdString(slip->receiptNumber));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *detailsTable = new QTableWidget(&dialog);
    detailsTable->setColumnCount(8); // Product, Location, Expected Qty, Received Qty, Lot/Serial, Mfg Date, Exp Date, Unit Cost, Notes, Is Fully Received
    detailsTable->setHorizontalHeaderLabels({"Sản phẩm", "Vị trí", "SL Dự kiến", "SL Nhận", "Số lô/Serial", "Ngày SX", "Ngày HH", "Giá đơn vị", "Ghi chú", "Đã nhận đủ"});
    detailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    detailsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    detailsTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(detailsTable);

    // Load existing details
    std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> currentDetails = receiptSlipService_->getReceiptSlipDetails(slip->id, currentUserId_, currentUserRoleIds_);
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
        detailsTable->setItem(i, 2, new QTableWidgetItem(QString::number(detail.expectedQuantity)));
        detailsTable->setItem(i, 3, new QTableWidgetItem(QString::number(detail.receivedQuantity)));
        detailsTable->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(detail.lotNumber.value_or("") + "/" + detail.serialNumber.value_or(""))));
        detailsTable->setItem(i, 5, new QTableWidgetItem(detail.manufactureDate ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*detail.manufactureDate, ERP::Common::DATETIME_FORMAT)) : "N/A"));
        detailsTable->setItem(i, 6, new QTableWidgetItem(detail.expirationDate ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*detail.expirationDate, ERP::Common::DATETIME_FORMAT)) : "N/A"));
        detailsTable->setItem(i, 7, new QTableWidgetItem(QString::number(detail.unitCost, 'f', 2)));
        detailsTable->setItem(i, 8, new QTableWidgetItem(QString::fromStdString(detail.notes.value_or(""))));
        detailsTable->setItem(i, 9, new QTableWidgetItem(detail.isFullyReceived ? "Yes" : "No"));
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
        itemDialog.setWindowTitle("Thêm Chi tiết Phiếu Nhập Kho");
        QFormLayout itemFormLayout;
        QComboBox productCombo;
        std::vector<ERP::Product::DTO::ProductDTO> allProducts = productService_->getAllProducts({}, currentUserId_, currentUserRoleIds_);
        for (const auto& prod : allProducts) productCombo.addItem(QString::fromStdString(prod.name), QString::fromStdString(prod.id));
        
        QComboBox locationCombo;
        std::vector<ERP::Catalog::DTO::LocationDTO> locationsInWarehouse = warehouseService_->getLocationsByWarehouse(slip->warehouseId, currentUserId_, currentUserRoleIds_);
        for (const auto& loc : locationsInWarehouse) locationCombo.addItem(QString::fromStdString(loc.name), QString::fromStdString(loc.id));

        QLineEdit expectedQuantityEdit; expectedQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit lotNumberEdit;
        QLineEdit serialNumberEdit;
        QDateTimeEdit manufactureDateEdit; manufactureDateEdit.setDisplayFormat("yyyy-MM-dd"); manufactureDateEdit.setCalendarPopup(true);
        QDateTimeEdit expirationDateEdit; expirationDateEdit.setDisplayFormat("yyyy-MM-dd"); expirationDateEdit.setCalendarPopup(true);
        QLineEdit unitCostEdit; unitCostEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit notesEdit;

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Vị trí:*", &locationCombo);
        itemFormLayout.addRow("SL Dự kiến:*", &expectedQuantityEdit);
        itemFormLayout.addRow("Số lô:", &lotNumberEdit);
        itemFormLayout.addRow("Số Serial:", &serialNumberEdit);
        itemFormLayout.addRow("Ngày SX:", &manufactureDateEdit);
        itemFormLayout.addRow("Ngày HH:", &expirationDateEdit);
        itemFormLayout.addRow("Giá đơn vị:*", &unitCostEdit);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);

        QPushButton *okItemButton = new QPushButton("Thêm", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (productCombo.currentData().isNull() || locationCombo.currentData().isNull() || expectedQuantityEdit.text().isEmpty() || unitCostEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Add to table
            int newRow = detailsTable->rowCount();
            detailsTable->insertRow(newRow);
            detailsTable->setItem(newRow, 0, new QTableWidgetItem(productCombo.currentText()));
            detailsTable->setItem(newRow, 1, new QTableWidgetItem(locationCombo.currentText()));
            detailsTable->setItem(newRow, 2, new QTableWidgetItem(expectedQuantityEdit.text()));
            detailsTable->setItem(newRow, 3, new QTableWidgetItem("0.0")); // Received Qty
            detailsTable->setItem(newRow, 4, new QTableWidgetItem(lotNumberEdit.text() + "/" + serialNumberEdit.text()));
            detailsTable->setItem(newRow, 5, new QTableWidgetItem(manufactureDateEdit.dateTime().toString("yyyy-MM-dd")));
            detailsTable->setItem(newRow, 6, new QTableWidgetItem(expirationDateEdit.dateTime().toString("yyyy-MM-dd")));
            detailsTable->setItem(newRow, 7, new QTableWidgetItem(unitCostEdit.text()));
            detailsTable->setItem(newRow, 8, new QTableWidgetItem(notesEdit.text()));
            detailsTable->setItem(newRow, 9, new QTableWidgetItem("No")); // Is Fully Received
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
        itemDialog.setWindowTitle("Sửa Chi tiết Phiếu Nhập Kho");
        QFormLayout itemFormLayout;
        QComboBox productCombo;
        std::vector<ERP::Product::DTO::ProductDTO> allProducts = productService_->getAllProducts({}, currentUserId_, currentUserRoleIds_);
        for (const auto& prod : allProducts) productCombo.addItem(QString::fromStdString(prod.name), QString::fromStdString(prod.id));
        
        QComboBox locationCombo;
        std::vector<ERP::Catalog::DTO::LocationDTO> locationsInWarehouse = warehouseService_->getLocationsByWarehouse(slip->warehouseId, currentUserId_, currentUserRoleIds_);
        for (const auto& loc : locationsInWarehouse) locationCombo.addItem(QString::fromStdString(loc.name), QString::fromStdString(loc.id));

        QLineEdit expectedQuantityEdit; expectedQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit lotNumberEdit;
        QLineEdit serialNumberEdit;
        QDateTimeEdit manufactureDateEdit; manufactureDateEdit.setDisplayFormat("yyyy-MM-dd"); manufactureDateEdit.setCalendarPopup(true);
        QDateTimeEdit expirationDateEdit; expirationDateEdit.setDisplayFormat("yyyy-MM-dd"); expirationDateEdit.setCalendarPopup(true);
        QLineEdit unitCostEdit; unitCostEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit notesEdit;

        // Populate with current item data
        QString currentProductId = detailsTable->item(selectedItemRow, 0)->data(Qt::UserRole).toString();
        int productIndex = productCombo.findData(currentProductId);
        if (productIndex != -1) productCombo.setCurrentIndex(productIndex);

        QString currentLocationId = detailsTable->item(selectedItemRow, 1)->data(Qt::UserRole).toString();
        int locationIndex = locationCombo.findData(currentLocationId);
        if (locationIndex != -1) locationCombo.setCurrentIndex(locationIndex);

        expectedQuantityEdit.setText(detailsTable->item(selectedItemRow, 2)->text());
        lotNumberEdit.setText(detailsTable->item(selectedItemRow, 4)->text().split("/")[0]);
        serialNumberEdit.setText(detailsTable->item(selectedItemRow, 4)->text().split("/").size() > 1 ? detailsTable->item(selectedItemRow, 4)->text().split("/")[1] : "");
        manufactureDateEdit.setDateTime(QDateTime::fromString(detailsTable->item(selectedItemRow, 5)->text(), "yyyy-MM-dd"));
        expirationDateEdit.setDateTime(QDateTime::fromString(detailsTable->item(selectedItemRow, 6)->text(), "yyyy-MM-dd"));
        unitCostEdit.setText(detailsTable->item(selectedItemRow, 7)->text());
        notesEdit.setText(detailsTable->item(selectedItemRow, 8)->text());


        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Vị trí:*", &locationCombo);
        itemFormLayout.addRow("SL Dự kiến:*", &expectedQuantityEdit);
        itemFormLayout.addRow("Số lô:", &lotNumberEdit);
        itemFormLayout.addRow("Số Serial:", &serialNumberEdit);
        itemFormLayout.addRow("Ngày SX:", &manufactureDateEdit);
        itemFormLayout.addRow("Ngày HH:", &expirationDateEdit);
        itemFormLayout.addRow("Giá đơn vị:*", &unitCostEdit);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);

        QPushButton *okItemButton = new QPushButton("Lưu", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (productCombo.currentData().isNull() || locationCombo.currentData().isNull() || expectedQuantityEdit.text().isEmpty() || unitCostEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Update table row
            detailsTable->setItem(selectedItemRow, 0, new QTableWidgetItem(productCombo.currentText()));
            detailsTable->setItem(selectedItemRow, 1, new QTableWidgetItem(locationCombo.currentText()));
            detailsTable->setItem(selectedItemRow, 2, new QTableWidgetItem(expectedQuantityEdit.text()));
            // detailsTable->item(selectedItemRow, 3) is received qty, not updated here
            detailsTable->setItem(selectedItemRow, 4, new QTableWidgetItem(lotNumberEdit.text() + "/" + serialNumberEdit.text()));
            detailsTable->setItem(selectedItemRow, 5, new QTableWidgetItem(manufactureDateEdit.dateTime().toString("yyyy-MM-dd")));
            detailsTable->setItem(selectedItemRow, 6, new QTableWidgetItem(expirationDateEdit.dateTime().toString("yyyy-MM-dd")));
            detailsTable->setItem(selectedItemRow, 7, new QTableWidgetItem(unitCostEdit.text()));
            detailsTable->setItem(selectedItemRow, 8, new QTableWidgetItem(notesEdit.text()));
            // detailsTable->item(selectedItemRow, 9) is fully received, not updated here
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
        confirmDelBox.setWindowTitle("Xóa Chi tiết Phiếu Nhập Kho");
        confirmDelBox.setText("Bạn có chắc chắn muốn xóa chi tiết phiếu nhập kho này?");
        confirmDelBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmDelBox.exec() == QMessageBox::Yes) {
            detailsTable->removeRow(selectedItemRow);
        }
    });

    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Collect all items from the table
        std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> updatedDetails;
        for (int i = 0; i < detailsTable->rowCount(); ++i) {
            ERP::Material::DTO::ReceiptSlipDetailDTO detail;
            // If item had an ID (from existing data), reuse it, otherwise generate new
            QString existingId = detailsTable->item(i, 0)->data(Qt::UserRole).toString();
            detail.id = existingId.isEmpty() ? ERP::Utils::generateUUID() : existingId.toStdString();

            detail.receiptSlipId = slip->id;
            detail.productId = detailsTable->item(i, 0)->data(Qt::UserRole).toString().toStdString();
            detail.locationId = detailsTable->item(i, 1)->data(Qt::UserRole).toString().toStdString();
            detail.expectedQuantity = detailsTable->item(i, 2)->text().toDouble();
            detail.receivedQuantity = detailsTable->item(i, 3)->text().toDouble(); // Keep current received qty
            
            QString lotSerialText = detailsTable->item(i, 4)->text();
            QStringList lotSerialParts = lotSerialText.split("/");
            if (lotSerialParts.size() > 0) detail.lotNumber = lotSerialParts[0].toStdString();
            if (lotSerialParts.size() > 1) detail.serialNumber = lotSerialParts[1].toStdString();

            if (!detailsTable->item(i, 5)->text().isEmpty()) { // Manufacture Date
                 detail.manufactureDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(QDateTime::fromString(detailsTable->item(i, 5)->text(), "yyyy-MM-dd"));
            } else { detail.manufactureDate = std::nullopt; }

            if (!detailsTable->item(i, 6)->text().isEmpty()) { // Expiration Date
                 detail.expirationDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(QDateTime::fromString(detailsTable->item(i, 6)->text(), "yyyy-MM-dd"));
            } else { detail.expirationDate = std::nullopt; }

            detail.unitCost = detailsTable->item(i, 7)->text().toDouble();
            detail.notes = detailsTable->item(i, 8)->text().isEmpty() ? std::nullopt : std::make_optional(detailsTable->item(i, 8)->text().toStdString());
            detail.isFullyReceived = detailsTable->item(i, 9)->text() == "Yes"; // Keep current status
            
            updatedDetails.push_back(detail);
        }

        // Call service to update Receipt Slip with new detail list
        if (receiptSlipService_->updateReceiptSlip(*slip, updatedDetails, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Quản lý Chi tiết", "Chi tiết phiếu nhập kho đã được cập nhật thành công.", QMessageBox::Information);
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật chi tiết phiếu nhập kho. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void ReceiptSlipManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool ReceiptSlipManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void ReceiptSlipManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Material.CreateReceiptSlip");
    bool canUpdate = hasPermission("Material.UpdateReceiptSlip");
    bool canDelete = hasPermission("Material.DeleteReceiptSlip");
    bool canChangeStatus = hasPermission("Material.UpdateReceiptSlipStatus");
    bool canManageDetails = hasPermission("Material.ManageReceiptSlipDetails"); // Assuming this permission
    bool canRecordQuantity = hasPermission("Material.RecordReceivedQuantity");

    addSlipButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Material.ViewReceiptSlips"));

    bool isRowSelected = slipTable_->currentRow() >= 0;
    editSlipButton_->setEnabled(isRowSelected && canUpdate);
    deleteSlipButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    manageDetailsButton_->setEnabled(isRowSelected && canManageDetails);
    recordReceivedQuantityButton_->setEnabled(isRowSelected && canRecordQuantity);


    bool enableForm = isRowSelected && canUpdate;
    receiptNumberLineEdit_->setEnabled(enableForm); // Will be read-only for existing anyway
    warehouseComboBox_->setEnabled(enableForm);
    // receivedByLineEdit_ is read-only
    receiptDateEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    referenceDocumentIdLineEdit_->setEnabled(enableForm);
    referenceDocumentTypeLineEdit_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        receiptNumberLineEdit_->clear();
        warehouseComboBox_->clear();
        receivedByLineEdit_->clear();
        receiptDateEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
        referenceDocumentIdLineEdit_->clear();
        referenceDocumentTypeLineEdit_->clear();
        notesLineEdit_->clear();
    }
}


} // namespace Material
} // namespace UI
} // namespace ERP