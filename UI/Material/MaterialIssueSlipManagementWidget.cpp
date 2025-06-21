// UI/Material/MaterialIssueSlipManagementWidget.cpp
#include "MaterialIssueSlipManagementWidget.h" // Đã rút gọn include
#include "MaterialIssueSlip.h"         // Đã rút gọn include
#include "MaterialIssueSlipDetail.h"   // Đã rút gọn include
#include "ProductionOrder.h"           // Đã rút gọn include
#include "Product.h"                   // Đã rút gọn include
#include "Warehouse.h"                 // Đã rút gọn include
#include "MaterialIssueSlipService.h"  // Đã rút gọn include
#include "ProductionOrderService.h"    // Đã rút gọn include
#include "ProductService.h"            // Đã rút gọn include
#include "WarehouseService.h"          // Đã rút gọn include
#include "InventoryManagementService.h"// Đã rút gọn include
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

MaterialIssueSlipManagementWidget::MaterialIssueSlipManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IMaterialIssueSlipService> materialIssueSlipService,
    std::shared_ptr<Manufacturing::Services::IProductionOrderService> productionOrderService,
    std::shared_ptr<Product::Services::IProductService> productService,
    std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<Warehouse::Services::IInventoryManagementService> inventoryManagementService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      materialIssueSlipService_(materialIssueSlipService),
      productionOrderService_(productionOrderService),
      productService_(productService),
      warehouseService_(warehouseService),
      inventoryManagementService_(inventoryManagementService),
      securityManager_(securityManager) {

    if (!materialIssueSlipService_ || !productionOrderService_ || !productService_ || !warehouseService_ || !inventoryManagementService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ phiếu xuất vật tư sản xuất hoặc các dịch vụ phụ thuộc không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("MaterialIssueSlipManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadMaterialIssueSlips();
    updateButtonsState();
}

MaterialIssueSlipManagementWidget::~MaterialIssueSlipManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void MaterialIssueSlipManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo số phiếu xuất...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &MaterialIssueSlipManagementWidget::onSearchSlipClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    slipTable_ = new QTableWidget(this);
    slipTable_->setColumnCount(6); // ID, Số phiếu, Lệnh sản xuất, Kho hàng, Ngày xuất, Trạng thái
    slipTable_->setHorizontalHeaderLabels({"ID", "Số Phiếu Xuất", "Lệnh SX", "Kho hàng", "Ngày Xuất", "Trạng thái"});
    slipTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    slipTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    slipTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    slipTable_->horizontalHeader()->setStretchLastSection(true);
    connect(slipTable_, &QTableWidget::itemClicked, this, &MaterialIssueSlipManagementWidget::onSlipTableItemClicked);
    mainLayout->addWidget(slipTable_);

    // Form elements for editing/adding slips
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    issueNumberLineEdit_ = new QLineEdit(this);
    productionOrderComboBox_ = new QComboBox(this); populateProductionOrderComboBox();
    warehouseComboBox_ = new QComboBox(this); populateWarehouseComboBox();
    issuedByLineEdit_ = new QLineEdit(this); issuedByLineEdit_->setReadOnly(true); // Should be current user
    issueDateEdit_ = new QDateTimeEdit(this); issueDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();
    notesLineEdit_ = new QLineEdit(this);


    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Số Phiếu Xuất:*", this), 1, 0); formLayout->addWidget(issueNumberLineEdit_, 1, 1);
    formLayout->addWidget(new QLabel("Lệnh SX:*", this), 2, 0); formLayout->addWidget(productionOrderComboBox_, 2, 1);
    formLayout->addWidget(new QLabel("Kho hàng:*", this), 3, 0); formLayout->addWidget(warehouseComboBox_, 3, 1);
    formLayout->addWidget(new QLabel("Người xuất:", this), 4, 0); formLayout->addWidget(issuedByLineEdit_, 4, 1);
    formLayout->addWidget(new QLabel("Ngày Xuất:*", this), 5, 0); formLayout->addWidget(issueDateEdit_, 5, 1);
    formLayout->addWidget(new QLabel("Trạng thái:*", this), 6, 0); formLayout->addWidget(statusComboBox_, 6, 1);
    formLayout->addWidget(new QLabel("Ghi chú:", this), 7, 0); formLayout->addWidget(notesLineEdit_, 7, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addSlipButton_ = new QPushButton("Thêm mới", this); connect(addSlipButton_, &QPushButton::clicked, this, &MaterialIssueSlipManagementWidget::onAddSlipClicked);
    editSlipButton_ = new QPushButton("Sửa", this); connect(editSlipButton_, &QPushButton::clicked, this, &MaterialIssueSlipManagementWidget::onEditSlipClicked);
    deleteSlipButton_ = new QPushButton("Xóa", this); connect(deleteSlipButton_, &QPushButton::clicked, this, &MaterialIssueSlipManagementWidget::onDeleteSlipClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &MaterialIssueSlipManagementWidget::onUpdateSlipStatusClicked);
    manageDetailsButton_ = new QPushButton("Quản lý Chi tiết", this); connect(manageDetailsButton_, &QPushButton::clicked, this, &MaterialIssueSlipManagementWidget::onManageDetailsClicked);
    recordIssuedQuantityButton_ = new QPushButton("Ghi nhận SL xuất", this); connect(recordIssuedQuantityButton_, &QPushButton::clicked, this, &MaterialIssueSlipManagementWidget::onRecordIssuedQuantityClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &MaterialIssueSlipManagementWidget::onSearchSlipClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &MaterialIssueSlipManagementWidget::clearForm);
    
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

void MaterialIssueSlipManagementWidget::loadMaterialIssueSlips() {
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipManagementWidget: Loading material issue slips...");
    slipTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Material::DTO::MaterialIssueSlipDTO> slips = materialIssueSlipService_->getAllMaterialIssueSlips({}, currentUserId_, currentUserRoleIds_);

    slipTable_->setRowCount(slips.size());
    for (int i = 0; i < slips.size(); ++i) {
        const auto& slip = slips[i];
        slipTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(slip.id)));
        slipTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(slip.issueNumber)));
        
        QString productionOrderNumber = "N/A";
        std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> po = productionOrderService_->getProductionOrderById(slip.productionOrderId, currentUserId_, currentUserRoleIds_);
        if (po) productionOrderNumber = QString::fromStdString(po->orderNumber);
        slipTable_->setItem(i, 2, new QTableWidgetItem(productionOrderNumber));

        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(slip.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) warehouseName = QString::fromStdString(warehouse->name);
        slipTable_->setItem(i, 3, new QTableWidgetItem(warehouseName));

        slipTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(slip.issueDate, ERP::Common::DATETIME_FORMAT))));
        slipTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(slip.getStatusString())));
    }
    slipTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipManagementWidget: Material issue slips loaded successfully.");
}

void MaterialIssueSlipManagementWidget::populateProductionOrderComboBox() {
    productionOrderComboBox_->clear();
    std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> allPOs = productionOrderService_->getAllProductionOrders({}, currentUserId_, currentUserRoleIds_);
    for (const auto& po : allPOs) {
        productionOrderComboBox_->addItem(QString::fromStdString(po.orderNumber), QString::fromStdString(po.id));
    }
}

void MaterialIssueSlipManagementWidget::populateWarehouseComboBox() {
    warehouseComboBox_->clear();
    std::vector<ERP::Catalog::DTO::WarehouseDTO> allWarehouses = warehouseService_->getAllWarehouses({}, currentUserId_, currentUserRoleIds_);
    for (const auto& warehouse : allWarehouses) {
        warehouseComboBox_->addItem(QString::fromStdString(warehouse.name), QString::fromStdString(warehouse.id));
    }
}

void MaterialIssueSlipManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Draft", static_cast<int>(ERP::Material::DTO::MaterialIssueSlipStatus::DRAFT));
    statusComboBox_->addItem("Pending Approval", static_cast<int>(ERP::Material::DTO::MaterialIssueSlipStatus::PENDING_APPROVAL));
    statusComboBox_->addItem("Approved", static_cast<int>(ERP::Material::DTO::MaterialIssueSlipStatus::APPROVED));
    statusComboBox_->addItem("Issued", static_cast<int>(ERP::Material::DTO::MaterialIssueSlipStatus::ISSUED));
    statusComboBox_->addItem("Completed", static_cast<int>(ERP::Material::DTO::MaterialIssueSlipStatus::COMPLETED));
    statusComboBox_->addItem("Cancelled", static_cast<int>(ERP::Material::DTO::MaterialIssueSlipStatus::CANCELLED));
    statusComboBox_->addItem("Rejected", static_cast<int>(ERP::Material::DTO::MaterialIssueSlipStatus::REJECTED));
}


void MaterialIssueSlipManagementWidget::onAddSlipClicked() {
    if (!hasPermission("Material.CreateMaterialIssueSlip")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm phiếu xuất vật tư sản xuất.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateProductionOrderComboBox();
    populateWarehouseComboBox();
    showSlipInputDialog();
}

void MaterialIssueSlipManagementWidget::onEditSlipClicked() {
    if (!hasPermission("Material.UpdateMaterialIssueSlip")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa phiếu xuất vật tư sản xuất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Phiếu Xuất Vật tư SX", "Vui lòng chọn một phiếu xuất vật tư sản xuất để sửa.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> slipOpt = materialIssueSlipService_->getMaterialIssueSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (slipOpt) {
        populateProductionOrderComboBox();
        populateWarehouseComboBox();
        showSlipInputDialog(&(*slipOpt));
    } else {
        showMessageBox("Sửa Phiếu Xuất Vật tư SX", "Không tìm thấy phiếu xuất vật tư sản xuất để sửa.", QMessageBox::Critical);
    }
}

void MaterialIssueSlipManagementWidget::onDeleteSlipClicked() {
    if (!hasPermission("Material.DeleteMaterialIssueSlip")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa phiếu xuất vật tư sản xuất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Phiếu Xuất Vật tư SX", "Vui lòng chọn một phiếu xuất vật tư sản xuất để xóa.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    QString slipNumber = slipTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Phiếu Xuất Vật tư SX");
    confirmBox.setText("Bạn có chắc chắn muốn xóa phiếu xuất vật tư sản xuất '" + slipNumber + "' (ID: " + slipId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (materialIssueSlipService_->deleteMaterialIssueSlip(slipId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Phiếu Xuất Vật tư SX", "Phiếu xuất vật tư sản xuất đã được xóa thành công.", QMessageBox::Information);
            loadMaterialIssueSlips();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa phiếu xuất vật tư sản xuất. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void MaterialIssueSlipManagementWidget::onUpdateSlipStatusClicked() {
    if (!hasPermission("Material.UpdateMaterialIssueSlipStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái phiếu xuất vật tư sản xuất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một phiếu xuất vật tư sản xuất để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> slipOpt = materialIssueSlipService_->getMaterialIssueSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!slipOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy phiếu xuất vật tư sản xuất để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Material::DTO::MaterialIssueSlipDTO currentSlip = *slipOpt;
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
        ERP::Material::DTO::MaterialIssueSlipStatus newStatus = static_cast<ERP::Material::DTO::MaterialIssueSlipStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái phiếu xuất vật tư sản xuất");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái phiếu xuất vật tư sản xuất '" + QString::fromStdString(currentSlip.issueNumber) + "' thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (materialIssueSlipService_->updateMaterialIssueSlipStatus(slipId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái phiếu xuất vật tư sản xuất đã được cập nhật thành công.", QMessageBox::Information);
                loadMaterialIssueSlips();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái phiếu xuất vật tư sản xuất. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}


void MaterialIssueSlipManagementWidget::onSearchSlipClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["issue_number_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    slipTable_->setRowCount(0);
    std::vector<ERP::Material::DTO::MaterialIssueSlipDTO> slips = materialIssueSlipService_->getAllMaterialIssueSlips(filter, currentUserId_, currentUserRoleIds_);

    slipTable_->setRowCount(slips.size());
    for (int i = 0; i < slips.size(); ++i) {
        const auto& slip = slips[i];
        slipTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(slip.id)));
        slipTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(slip.issueNumber)));
        
        QString productionOrderNumber = "N/A";
        std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> po = productionOrderService_->getProductionOrderById(slip.productionOrderId, currentUserId_, currentUserRoleIds_);
        if (po) productionOrderNumber = QString::fromStdString(po->orderNumber);
        slipTable_->setItem(i, 2, new QTableWidgetItem(productionOrderNumber));

        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(slip.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) warehouseName = QString::fromStdString(warehouse->name);
        slipTable_->setItem(i, 3, new QTableWidgetItem(warehouseName));

        slipTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(slip.issueDate, ERP::Common::DATETIME_FORMAT))));
        slipTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(slip.getStatusString())));
    }
    slipTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipManagementWidget: Search completed.");
}

void MaterialIssueSlipManagementWidget::onSlipTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString slipId = slipTable_->item(row, 0)->text();
    std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> slipOpt = materialIssueSlipService_->getMaterialIssueSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (slipOpt) {
        idLineEdit_->setText(QString::fromStdString(slipOpt->id));
        issueNumberLineEdit_->setText(QString::fromStdString(slipOpt->issueNumber));
        
        populateProductionOrderComboBox();
        int poIndex = productionOrderComboBox_->findData(QString::fromStdString(slipOpt->productionOrderId));
        if (poIndex != -1) productionOrderComboBox_->setCurrentIndex(poIndex);

        populateWarehouseComboBox();
        int warehouseIndex = warehouseComboBox_->findData(QString::fromStdString(slipOpt->warehouseId));
        if (warehouseIndex != -1) warehouseComboBox_->setCurrentIndex(warehouseIndex);

        issuedByLineEdit_->setText(QString::fromStdString(slipOpt->issuedByUserId)); // Display ID
        issueDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(slipOpt->issueDate.time_since_epoch()).count()));
        
        int statusIndex = statusComboBox_->findData(static_cast<int>(slipOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);
        
        notesLineEdit_->setText(QString::fromStdString(slipOpt->notes.value_or("")));

    } else {
        showMessageBox("Thông tin Phiếu Xuất Vật tư SX", "Không tìm thấy phiếu xuất vật tư sản xuất đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void MaterialIssueSlipManagementWidget::clearForm() {
    idLineEdit_->clear();
    issueNumberLineEdit_->clear();
    productionOrderComboBox_->clear(); // Repopulated on demand
    warehouseComboBox_->clear(); // Repopulated on demand
    issuedByLineEdit_->clear();
    issueDateEdit_->clear();
    statusComboBox_->setCurrentIndex(0); // Default to Draft
    notesLineEdit_->clear();
    slipTable_->clearSelection();
    updateButtonsState();
}

void MaterialIssueSlipManagementWidget::onManageDetailsClicked() {
    if (!hasPermission("Material.ManageMaterialIssueSlipDetails")) { // Assuming this permission
        showMessageBox("Lỗi", "Bạn không có quyền quản lý chi tiết phiếu xuất vật tư sản xuất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Quản lý Chi tiết", "Vui lòng chọn một phiếu xuất vật tư sản xuất để quản lý chi tiết.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> slipOpt = materialIssueSlipService_->getMaterialIssueSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (slipOpt) {
        showManageDetailsDialog(&(*slipOpt));
    } else {
        showMessageBox("Quản lý Chi tiết", "Không tìm thấy phiếu xuất vật tư sản xuất để quản lý chi tiết.", QMessageBox::Critical);
    }
}

void MaterialIssueSlipManagementWidget::onRecordIssuedQuantityClicked() {
    if (!hasPermission("Material.RecordMaterialIssueQuantity")) {
        showMessageBox("Lỗi", "Bạn không có quyền ghi nhận số lượng xuất vật tư sản xuất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = slipTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Ghi nhận SL xuất", "Vui lòng chọn một phiếu xuất vật tư sản xuất trước.", QMessageBox::Information);
        return;
    }

    QString slipId = slipTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> parentSlipOpt = materialIssueSlipService_->getMaterialIssueSlipById(slipId.toStdString(), currentUserId_, currentUserRoleIds_);
    if (!parentSlipOpt) {
        showMessageBox("Ghi nhận SL xuất", "Không tìm thấy phiếu xuất vật tư sản xuất.", QMessageBox::Critical);
        return;
    }

    // Show a dialog to select a detail and input quantity
    QDialog dialog(this);
    dialog.setWindowTitle("Ghi nhận Số lượng Xuất Vật tư SX Thực tế");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox detailComboBox;
    // Populate with details of the selected slip
    std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> details = materialIssueSlipService_->getMaterialIssueSlipDetails(slipId.toStdString(), currentUserId_, currentUserRoleIds_);
    for (const auto& detail : details) {
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(detail.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);
        detailComboBox.addItem(productName + " (Đã xuất: " + QString::number(detail.issuedQuantity) + ")", QString::fromStdString(detail.id));
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

        std::optional<ERP::Material::DTO::MaterialIssueSlipDetailDTO> selectedDetailOpt = materialIssueSlipService_->getMaterialIssueSlipDetailById(selectedDetailId.toStdString(), currentUserId_, currentUserRoleIds_);
        if (!selectedDetailOpt) {
            showMessageBox("Lỗi", "Không tìm thấy chi tiết phiếu xuất vật tư sản xuất đã chọn.", QMessageBox::Critical);
            return;
        }

        if (materialIssueSlipService_->recordIssuedQuantity(selectedDetailId.toStdString(), quantity, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Ghi nhận SL xuất", "Số lượng xuất đã được ghi nhận thành công.", QMessageBox::Information);
            loadMaterialIssueSlips(); // Refresh parent table
            clearForm();
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể ghi nhận số lượng xuất. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void MaterialIssueSlipManagementWidget::showSlipInputDialog(ERP::Material::DTO::MaterialIssueSlipDTO* slip) {
    QDialog dialog(this);
    dialog.setWindowTitle(slip ? "Sửa Phiếu Xuất Vật tư SX" : "Thêm Phiếu Xuất Vật tư SX Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit issueNumberEdit(this);
    QComboBox productionOrderCombo(this); populateProductionOrderComboBox();
    for (int i = 0; i < productionOrderComboBox_->count(); ++i) productionOrderCombo.addItem(productionOrderComboBox_->itemText(i), productionOrderComboBox_->itemData(i));
    
    QComboBox warehouseCombo(this); populateWarehouseComboBox();
    for (int i = 0; i < warehouseComboBox_->count(); ++i) warehouseCombo.addItem(warehouseComboBox_->itemText(i), warehouseComboBox_->itemData(i));
    
    QLineEdit issuedByEdit(this); issuedByEdit.setReadOnly(true); // Auto-filled
    QDateTimeEdit issueDateEdit(this); issueDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QComboBox statusCombo(this); populateStatusComboBox();
    for (int i = 0; i < statusComboBox_->count(); ++i) statusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));
    
    QLineEdit notesEdit(this);

    if (slip) {
        issueNumberEdit.setText(QString::fromStdString(slip->issueNumber));
        int poIndex = productionOrderCombo.findData(QString::fromStdString(slip->productionOrderId));
        if (poIndex != -1) productionOrderCombo.setCurrentIndex(poIndex);
        int warehouseIndex = warehouseCombo.findData(QString::fromStdString(slip->warehouseId));
        if (warehouseIndex != -1) warehouseCombo.setCurrentIndex(warehouseIndex);
        issuedByEdit.setText(QString::fromStdString(slip->issuedByUserId));
        issueDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(slip->issueDate.time_since_epoch()).count()));
        int statusIndex = statusCombo.findData(static_cast<int>(slip->status));
        if (statusIndex != -1) statusCombo.setCurrentIndex(statusIndex);
        notesEdit.setText(QString::fromStdString(slip->notes.value_or("")));

        issueNumberEdit.setReadOnly(true); // Don't allow changing issue number for existing
    } else {
        issueNumberEdit.setText("MIS-" + QString::fromStdString(ERP::Utils::generateUUID().substr(0, 8))); // Auto-generate
        issueDateEdit.setDateTime(QDateTime::currentDateTime());
        issuedByEdit.setText(QString::fromStdString(currentUserId_)); // Auto-fill current user
    }

    formLayout->addRow("Số Phiếu Xuất:*", &issueNumberEdit);
    formLayout->addRow("Lệnh SX:*", &productionOrderCombo);
    formLayout->addRow("Kho hàng:*", &warehouseCombo);
    formLayout->addRow("Người xuất:", &issuedByEdit);
    formLayout->addRow("Ngày Xuất:*", &issueDateEdit);
    formLayout->addRow("Trạng thái:*", &statusCombo);
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
        ERP::Material::DTO::MaterialIssueSlipDTO newSlipData;
        if (slip) {
            newSlipData = *slip;
        }

        newSlipData.issueNumber = issueNumberEdit.text().toStdString();
        newSlipData.productionOrderId = productionOrderCombo.currentData().toString().toStdString();
        newSlipData.warehouseId = warehouseCombo.currentData().toString().toStdString();
        newSlipData.issuedByUserId = issuedByEdit.text().toStdString();
        newSlipData.issueDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(issueDateEdit.dateTime());
        newSlipData.status = static_cast<ERP::Material::DTO::MaterialIssueSlipStatus>(statusCombo.currentData().toInt());
        newSlipData.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());

        // For new slips, details are added in a separate dialog after creation
        std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> currentDetails;
        if (slip) { // When editing, load existing details first
             currentDetails = materialIssueSlipService_->getMaterialIssueSlipDetails(slip->id, currentUserId_, currentUserRoleIds_);
        }

        bool success = false;
        if (slip) {
            success = materialIssueSlipService_->updateMaterialIssueSlip(newSlipData, currentDetails, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Phiếu Xuất Vật tư SX", "Phiếu xuất vật tư sản xuất đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật phiếu xuất vật tư sản xuất. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> createdSlip = materialIssueSlipService_->createMaterialIssueSlip(newSlipData, {}, currentUserId_, currentUserRoleIds_); // Create with empty details
            if (createdSlip) {
                showMessageBox("Thêm Phiếu Xuất Vật tư SX", "Phiếu xuất vật tư sản xuất mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm phiếu xuất vật tư sản xuất mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadMaterialIssueSlips();
            clearForm();
        }
    }
}

void MaterialIssueSlipManagementWidget::showManageDetailsDialog(ERP::Material::DTO::MaterialIssueSlipDTO* slip) {
    if (!slip) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Quản lý Chi tiết Phiếu Xuất Vật tư SX: " + QString::fromStdString(slip->issueNumber));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *detailsTable = new QTableWidget(&dialog);
    detailsTable->setColumnCount(6); // Product, Issued Qty, Lot/Serial, Notes
    detailsTable->setHorizontalHeaderLabels({"Sản phẩm", "SL Xuất", "Số lô/Serial", "Ghi chú", "ID Giao dịch Tồn kho", "Trạng thái"});
    detailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    detailsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    detailsTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(detailsTable);

    // Load existing details
    std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> currentDetails = materialIssueSlipService_->getMaterialIssueSlipDetails(slip->id, currentUserId_, currentUserRoleIds_);
    detailsTable->setRowCount(currentDetails.size());
    for (int i = 0; i < currentDetails.size(); ++i) {
        const auto& detail = currentDetails[i];
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(detail.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);

        detailsTable->setItem(i, 0, new QTableWidgetItem(productName));
        detailsTable->setItem(i, 1, new QTableWidgetItem(QString::number(detail.issuedQuantity)));
        detailsTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(detail.lotNumber.value_or("") + "/" + detail.serialNumber.value_or(""))));
        detailsTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(detail.notes.value_or(""))));
        detailsTable->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(detail.inventoryTransactionId.value_or(""))));
        detailsTable->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(detail.status))));
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
        itemDialog.setWindowTitle("Thêm Chi tiết Phiếu Xuất Vật tư SX");
        QFormLayout itemFormLayout;
        QComboBox productCombo; populateProductComboBox();
        for (int i = 0; i < productService_->getAllProducts({}).size(); ++i) productCombo.addItem(productComboBox_->itemText(i), productComboBox_->itemData(i));
        QLineEdit issuedQuantityEdit; issuedQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit lotNumberEdit;
        QLineEdit serialNumberEdit;
        QLineEdit notesEdit;

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Số lượng Xuất:*", &issuedQuantityEdit);
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
            if (productCombo.currentData().isNull() || issuedQuantityEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Add to table
            int newRow = detailsTable->rowCount();
            detailsTable->insertRow(newRow);
            detailsTable->setItem(newRow, 0, new QTableWidgetItem(productCombo.currentText()));
            detailsTable->setItem(newRow, 1, new QTableWidgetItem(issuedQuantityEdit.text()));
            detailsTable->setItem(newRow, 2, new QTableWidgetItem(lotNumberEdit.text() + "/" + serialNumberEdit.text()));
            detailsTable->setItem(newRow, 3, new QTableWidgetItem(notesEdit.text()));
            detailsTable->setItem(newRow, 4, new QTableWidgetItem("")); // Inventory Transaction ID
            detailsTable->setItem(newRow, 5, new QTableWidgetItem("Active")); // Status
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
        itemDialog.setWindowTitle("Sửa Chi tiết Phiếu Xuất Vật tư SX");
        QFormLayout itemFormLayout;
        QComboBox productCombo; populateProductComboBox();
        for (int i = 0; i < productService_->getAllProducts({}).size(); ++i) productCombo.addItem(productComboBox_->itemText(i), productComboBox_->itemData(i));
        QLineEdit issuedQuantityEdit; issuedQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit lotNumberEdit;
        QLineEdit serialNumberEdit;
        QLineEdit notesEdit;

        // Populate with current item data
        QString currentProductId = detailsTable->item(selectedItemRow, 0)->data(Qt::UserRole).toString();
        int productIndex = productCombo.findData(currentProductId);
        if (productIndex != -1) productCombo.setCurrentIndex(productIndex);

        issuedQuantityEdit.setText(detailsTable->item(selectedItemRow, 1)->text());
        
        QString lotSerialText = detailsTable->item(selectedItemRow, 2)->text();
        QStringList lotSerialParts = lotSerialText.split("/");
        if (lotSerialParts.size() > 0) lotNumberEdit.setText(lotSerialParts[0]);
        if (lotSerialParts.size() > 1) serialNumberEdit.setText(lotSerialParts[1]);

        notesEdit.setText(detailsTable->item(selectedItemRow, 3)->text());

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Số lượng Xuất:*", &issuedQuantityEdit);
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
            if (productCombo.currentData().isNull() || issuedQuantityEdit.text().isEmpty()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết.", QMessageBox::Warning);
                return;
            }
            // Update table row
            detailsTable->item(selectedItemRow, 0)->setText(productCombo.currentText());
            detailsTable->item(selectedItemRow, 1)->setText(issuedQuantityEdit.text());
            detailsTable->item(selectedItemRow, 2)->setText(lotNumberEdit.text() + "/" + serialNumberEdit.text());
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
        confirmDelBox.setWindowTitle("Xóa Chi tiết Phiếu Xuất Vật tư SX");
        confirmDelBox.setText("Bạn có chắc chắn muốn xóa chi tiết phiếu xuất vật tư sản xuất này?");
        confirmDelBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmDelBox.exec() == QMessageBox::Yes) {
            detailsTable->removeRow(selectedItemRow);
        }
    });

    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Collect all items from the table
        std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> updatedDetails;
        for (int i = 0; i < detailsTable->rowCount(); ++i) {
            ERP::Material::DTO::MaterialIssueSlipDetailDTO detail;
            // If item had an ID (from existing data), reuse it, otherwise generate new
            QString existingId = detailsTable->item(i, 0)->data(Qt::UserRole).toString();
            detail.id = existingId.isEmpty() ? ERP::Utils::generateUUID() : existingId.toStdString();

            detail.materialIssueSlipId = slip->id;
            detail.productId = detailsTable->item(i, 0)->data(Qt::UserRole).toString().toStdString();
            detail.issuedQuantity = detailsTable->item(i, 1)->text().toDouble();
            
            QString lotSerialText = detailsTable->item(i, 2)->text();
            QStringList lotSerialParts = lotSerialText.split("/");
            if (lotSerialParts.size() > 0) detail.lotNumber = lotSerialParts[0].toStdString();
            if (lotSerialParts.size() > 1) detail.serialNumber = lotSerialParts[1].toStdString();

            detail.notes = detailsTable->item(i, 3)->text().isEmpty() ? std::nullopt : std::make_optional(detailsTable->item(i, 3)->text().toStdString());
            
            // For now, these are not directly editable in the details table itself, but handled by recordIssuedQuantity
            // detail.inventoryTransactionId = detailsTable->item(i, 4)->text().isEmpty() ? std::nullopt : std::make_optional(detailsTable->item(i, 4)->text().toStdString());
            detail.status = detailsTable->item(i, 5)->text() == "Active" ? ERP::Common::EntityStatus::ACTIVE : ERP::Common::EntityStatus::INACTIVE;
            
            updatedDetails.push_back(detail);
        }

        // Call service to update Material Issue Slip with new detail list
        if (materialIssueSlipService_->updateMaterialIssueSlip(*slip, updatedDetails, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Quản lý Chi tiết", "Chi tiết phiếu xuất vật tư sản xuất đã được cập nhật thành công.", QMessageBox::Information);
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật chi tiết phiếu xuất vật tư sản xuất. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void MaterialIssueSlipManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool MaterialIssueSlipManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void MaterialIssueSlipManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Material.CreateMaterialIssueSlip");
    bool canUpdate = hasPermission("Material.UpdateMaterialIssueSlip");
    bool canDelete = hasPermission("Material.DeleteMaterialIssueSlip");
    bool canChangeStatus = hasPermission("Material.UpdateMaterialIssueSlipStatus");
    bool canManageDetails = hasPermission("Material.ManageMaterialIssueSlipDetails"); // Assuming this permission
    bool canRecordQuantity = hasPermission("Material.RecordMaterialIssueQuantity");

    addSlipButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Material.ViewMaterialIssueSlips"));

    bool isRowSelected = slipTable_->currentRow() >= 0;
    editSlipButton_->setEnabled(isRowSelected && canUpdate);
    deleteSlipButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    manageDetailsButton_->setEnabled(isRowSelected && canManageDetails);
    recordIssuedQuantityButton_->setEnabled(isRowSelected && canRecordQuantity);


    bool enableForm = isRowSelected && canUpdate;
    issueNumberLineEdit_->setEnabled(enableForm); // Will be read-only for existing anyway
    productionOrderComboBox_->setEnabled(enableForm);
    warehouseComboBox_->setEnabled(enableForm);
    // issuedByLineEdit_ is read-only
    issueDateEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        issueNumberLineEdit_->clear();
        productionOrderComboBox_->clear();
        warehouseComboBox_->clear();
        issuedByLineEdit_->clear();
        issueDateEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
        notesLineEdit_->clear();
    }
}


} // namespace Material
} // namespace UI
} // namespace ERP