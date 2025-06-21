// UI/Manufacturing/BillOfMaterialManagementWidget.cpp
#include "BillOfMaterialManagementWidget.h" // Đã rút gọn include
#include "BillOfMaterial.h"         // Đã rút gọn include
#include "BillOfMaterialItem.h"     // Đã rút gọn include
#include "Product.h"                // Đã rút gọn include
#include "UnitOfMeasure.h"          // Đã rút gọn include
#include "BillOfMaterialService.h"  // Đã rút gọn include
#include "ProductService.h"         // Đã rút gọn include
#include "UnitOfMeasureService.h"   // Đã rút gọn include
#include "ISecurityManager.h"       // Đã rút gọn include
#include "Logger.h"                 // Đã rút gọn include
#include "ErrorHandler.h"           // Đã rút gọn include
#include "Common.h"                 // Đã rút gọn include
#include "DateUtils.h"              // Đã rút gọn include
#include "StringUtils.h"            // Đã rút gọn include
#include "CustomMessageBox.h"       // Đã rút gọn include
#include "UserService.h"            // For getting current user roles from SecurityManager

#include <QInputDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDoubleValidator>

namespace ERP {
namespace UI {
namespace Manufacturing {

BillOfMaterialManagementWidget::BillOfMaterialManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IBillOfMaterialService> bomService,
    std::shared_ptr<Product::Services::IProductService> productService,
    std::shared_ptr<Catalog::Services::IUnitOfMeasureService> unitOfMeasureService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      bomService_(bomService),
      productService_(productService),
      unitOfMeasureService_(unitOfMeasureService),
      securityManager_(securityManager) {

    if (!bomService_ || !productService_ || !unitOfMeasureService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ BOM, sản phẩm, đơn vị đo hoặc bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("BillOfMaterialManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("BillOfMaterialManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadBOMs();
    updateButtonsState();
}

BillOfMaterialManagementWidget::~BillOfMaterialManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void BillOfMaterialManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên BOM hoặc ID sản phẩm...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &BillOfMaterialManagementWidget::onSearchBOMClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    bomTable_ = new QTableWidget(this);
    bomTable_->setColumnCount(6); // ID, Tên BOM, Sản phẩm, SL cơ sở, Đơn vị, Trạng thái
    bomTable_->setHorizontalHeaderLabels({"ID", "Tên BOM", "Sản phẩm", "SL cơ sở", "Đơn vị", "Trạng thái"});
    bomTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    bomTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    bomTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    bomTable_->horizontalHeader()->setStretchLastSection(true);
    connect(bomTable_, &QTableWidget::itemClicked, this, &BillOfMaterialManagementWidget::onBOMTableItemClicked);
    mainLayout->addWidget(bomTable_);

    // Form elements for editing/adding
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    bomNameLineEdit_ = new QLineEdit(this);
    productComboBox_ = new QComboBox(this);
    descriptionLineEdit_ = new QLineEdit(this);
    baseQuantityLineEdit_ = new QLineEdit(this); baseQuantityLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    baseQuantityUnitComboBox_ = new QComboBox(this);
    statusComboBox_ = new QComboBox(this);
    populateStatusComboBox(); // Populate BOM status
    versionLineEdit_ = new QLineEdit(this); versionLineEdit_->setValidator(new QIntValidator(0, 99999, this));


    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Tên BOM:*", this), 1, 0); formLayout->addWidget(bomNameLineEdit_, 1, 1);
    formLayout->addWidget(new QLabel("Sản phẩm:*", this), 2, 0); formLayout->addWidget(productComboBox_, 2, 1);
    formLayout->addWidget(new QLabel("Mô tả:", this), 3, 0); formLayout->addWidget(descriptionLineEdit_, 3, 1);
    formLayout->addWidget(new QLabel("SL cơ sở:*", this), 4, 0); formLayout->addWidget(baseQuantityLineEdit_, 4, 1);
    formLayout->addWidget(new QLabel("Đơn vị SL cơ sở:*", this), 5, 0); formLayout->addWidget(baseQuantityUnitComboBox_, 5, 1);
    formLayout->addWidget(new QLabel("Trạng thái:*", this), 6, 0); formLayout->addWidget(statusComboBox_, 6, 1);
    formLayout->addWidget(new QLabel("Phiên bản:", this), 7, 0); formLayout->addWidget(versionLineEdit_, 7, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addBOMButton_ = new QPushButton("Thêm mới", this); connect(addBOMButton_, &QPushButton::clicked, this, &BillOfMaterialManagementWidget::onAddBOMClicked);
    editBOMButton_ = new QPushButton("Sửa", this); connect(editBOMButton_, &QPushButton::clicked, this, &BillOfMaterialManagementWidget::onEditBOMClicked);
    deleteBOMButton_ = new QPushButton("Xóa", this); connect(deleteBOMButton_, &QPushButton::clicked, this, &BillOfMaterialManagementWidget::onDeleteBOMClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &BillOfMaterialManagementWidget::onUpdateBOMStatusClicked);
    manageBOMItemsButton_ = new QPushButton("Quản lý Thành phần", this); connect(manageBOMItemsButton_, &QPushButton::clicked, this, &BillOfMaterialManagementWidget::onManageBOMItemsClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &BillOfMaterialManagementWidget::clearForm);
    buttonLayout->addWidget(addBOMButton_);
    buttonLayout->addWidget(editBOMButton_);
    buttonLayout->addWidget(deleteBOMButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(manageBOMItemsButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void BillOfMaterialManagementWidget::loadBOMs() {
    ERP::Logger::Logger::getInstance().info("BillOfMaterialManagementWidget: Loading BOMs...");
    bomTable_->setRowCount(0);

    std::vector<ERP::Manufacturing::DTO::BillOfMaterialDTO> boms = bomService_->getAllBillOfMaterials({}, currentUserId_, currentUserRoleIds_);

    bomTable_->setRowCount(boms.size());
    for (int i = 0; i < boms.size(); ++i) {
        const auto& bom = boms[i];
        bomTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(bom.id)));
        bomTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(bom.bomName)));
        
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(bom.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);
        bomTable_->setItem(i, 2, new QTableWidgetItem(productName));

        bomTable_->setItem(i, 3, new QTableWidgetItem(QString::number(bom.baseQuantity)));
        
        QString unitName = "N/A";
        std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> unit = unitOfMeasureService_->getUnitOfMeasureById(bom.baseQuantityUnitId, currentUserId_, currentUserRoleIds_);
        if (unit) unitName = QString::fromStdString(unit->name);
        bomTable_->setItem(i, 4, new QTableWidgetItem(unitName));

        bomTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(bom.getStatusString())));
    }
    bomTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("BillOfMaterialManagementWidget: BOMs loaded successfully.");
}

void BillOfMaterialManagementWidget::populateProductComboBox() {
    productComboBox_->clear();
    std::vector<ERP::Product::DTO::ProductDTO> allProducts = productService_->getAllProducts({}, currentUserId_, currentUserRoleIds_);
    for (const auto& product : allProducts) {
        productComboBox_->addItem(QString::fromStdString(product.name), QString::fromStdString(product.id));
    }
}

void BillOfMaterialManagementWidget::populateUnitOfMeasureComboBox() {
    baseQuantityUnitComboBox_->clear();
    std::vector<ERP::Catalog::DTO::UnitOfMeasureDTO> allUoMs = unitOfMeasureService_->getAllUnitsOfMeasure({}, currentUserId_, currentUserRoleIds_);
    for (const auto& uom : allUoMs) {
        baseQuantityUnitComboBox_->addItem(QString::fromStdString(uom.name), QString::fromStdString(uom.id));
    }
}

void BillOfMaterialManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Draft", static_cast<int>(ERP::Manufacturing::DTO::BillOfMaterialStatus::DRAFT));
    statusComboBox_->addItem("Active", static_cast<int>(ERP::Manufacturing::DTO::BillOfMaterialStatus::ACTIVE));
    statusComboBox_->addItem("Inactive", static_cast<int>(ERP::Manufacturing::DTO::BillOfMaterialStatus::INACTIVE));
    statusComboBox_->addItem("Archived", static_cast<int>(ERP::Manufacturing::DTO::BillOfMaterialStatus::ARCHIVED));
}


void BillOfMaterialManagementWidget::onAddBOMClicked() {
    if (!hasPermission("Manufacturing.CreateBillOfMaterial")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm định mức nguyên vật liệu.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateProductComboBox();
    populateUnitOfMeasureComboBox();
    showBOMInputDialog();
}

void BillOfMaterialManagementWidget::onEditBOMClicked() {
    if (!hasPermission("Manufacturing.UpdateBillOfMaterial")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa định mức nguyên vật liệu.", QMessageBox::Warning);
        return;
    }

    int selectedRow = bomTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa BOM", "Vui lòng chọn một định mức nguyên vật liệu để sửa.", QMessageBox::Information);
        return;
    }

    QString bomId = bomTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> bomOpt = bomService_->getBillOfMaterialById(bomId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (bomOpt) {
        populateProductComboBox();
        populateUnitOfMeasureComboBox();
        showBOMInputDialog(&(*bomOpt));
    } else {
        showMessageBox("Sửa BOM", "Không tìm thấy định mức nguyên vật liệu để sửa.", QMessageBox::Critical);
    }
}

void BillOfMaterialManagementWidget::onDeleteBOMClicked() {
    if (!hasPermission("Manufacturing.DeleteBillOfMaterial")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa định mức nguyên vật liệu.", QMessageBox::Warning);
        return;
    }

    int selectedRow = bomTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa BOM", "Vui lòng chọn một định mức nguyên vật liệu để xóa.", QMessageBox::Information);
        return;
    }

    QString bomId = bomTable_->item(selectedRow, 0)->text();
    QString bomName = bomTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa BOM");
    confirmBox.setText("Bạn có chắc chắn muốn xóa định mức nguyên vật liệu '" + bomName + "' (ID: " + bomId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (bomService_->deleteBillOfMaterial(bomId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa BOM", "Định mức nguyên vật liệu đã được xóa thành công.", QMessageBox::Information);
            loadBOMs();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa định mức nguyên vật liệu. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void BillOfMaterialManagementWidget::onUpdateBOMStatusClicked() {
    if (!hasPermission("Manufacturing.UpdateBillOfMaterialStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái định mức nguyên vật liệu.", QMessageBox::Warning);
        return;
    }

    int selectedRow = bomTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một định mức nguyên vật liệu để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString bomId = bomTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> bomOpt = bomService_->getBillOfMaterialById(bomId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!bomOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy định mức nguyên vật liệu để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Manufacturing::DTO::BillOfMaterialDTO currentBOM = *bomOpt;
    // Toggle status between ACTIVE and INACTIVE for simplicity, or show a dialog for specific status
    ERP::Manufacturing::DTO::BillOfMaterialStatus newStatus = (currentBOM.status == ERP::Manufacturing::DTO::BillOfMaterialStatus::ACTIVE) ?
                                                             ERP::Manufacturing::DTO::BillOfMaterialStatus::INACTIVE :
                                                             ERP::Manufacturing::DTO::BillOfMaterialStatus::ACTIVE;

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Cập nhật trạng thái BOM");
    confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái BOM '" + QString::fromStdString(currentBOM.bomName) + "' thành " + QString::fromStdString(currentBOM.getStatusString()) + "?"); // Use getStatusString for new status
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (bomService_->updateBillOfMaterialStatus(bomId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Cập nhật trạng thái", "Trạng thái BOM đã được cập nhật thành công.", QMessageBox::Information);
            loadBOMs();
            clearForm();
        } else {
            showMessageBox("Lỗi", "Không thể cập nhật trạng thái BOM. Vui lòng kiểm tra log.", QMessageBox::Critical);
        }
    }
}


void BillOfMaterialManagementWidget::onSearchBOMClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["name_or_product_id_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    bomTable_->setRowCount(0);
    std::vector<ERP::Manufacturing::DTO::BillOfMaterialDTO> boms = bomService_->getAllBillOfMaterials(filter, currentUserId_, currentUserRoleIds_);

    bomTable_->setRowCount(boms.size());
    for (int i = 0; i < boms.size(); ++i) {
        const auto& bom = boms[i];
        bomTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(bom.id)));
        bomTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(bom.bomName)));
        
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(bom.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);
        bomTable_->setItem(i, 2, new QTableWidgetItem(productName));

        bomTable_->setItem(i, 3, new QTableWidgetItem(QString::number(bom.baseQuantity)));
        
        QString unitName = "N/A";
        std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> unit = unitOfMeasureService_->getUnitOfMeasureById(bom.baseQuantityUnitId, currentUserId_, currentUserRoleIds_);
        if (unit) unitName = QString::fromStdString(unit->name);
        bomTable_->setItem(i, 4, new QTableWidgetItem(unitName));

        bomTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(bom.getStatusString())));
    }
    bomTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("BillOfMaterialManagementWidget: Search completed.");
}

void BillOfMaterialManagementWidget::onBOMTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString bomId = bomTable_->item(row, 0)->text();
    std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> bomOpt = bomService_->getBillOfMaterialById(bomId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (bomOpt) {
        idLineEdit_->setText(QString::fromStdString(bomOpt->id));
        bomNameLineEdit_->setText(QString::fromStdString(bomOpt->bomName));
        
        populateProductComboBox();
        int productIndex = productComboBox_->findData(QString::fromStdString(bomOpt->productId));
        if (productIndex != -1) productComboBox_->setCurrentIndex(productIndex);

        descriptionLineEdit_->setText(QString::fromStdString(bomOpt->description.value_or("")));
        baseQuantityLineEdit_->setText(QString::number(bomOpt->baseQuantity));
        
        populateUnitOfMeasureComboBox();
        int unitIndex = baseQuantityUnitComboBox_->findData(QString::fromStdString(bomOpt->baseQuantityUnitId));
        if (unitIndex != -1) baseQuantityUnitComboBox_->setCurrentIndex(unitIndex);

        int statusIndex = statusComboBox_->findData(static_cast<int>(bomOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);
        
        versionLineEdit_->setText(QString::number(bomOpt->version.value_or(0))); // Optional int

    } else {
        showMessageBox("Thông tin BOM", "Không thể tải chi tiết định mức nguyên vật liệu đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void BillOfMaterialManagementWidget::clearForm() {
    idLineEdit_->clear();
    bomNameLineEdit_->clear();
    productComboBox_->clear(); // Will be repopulated when needed
    descriptionLineEdit_->clear();
    baseQuantityLineEdit_->clear();
    baseQuantityUnitComboBox_->clear(); // Will be repopulated when needed
    statusComboBox_->setCurrentIndex(0); // Default to Draft
    versionLineEdit_->clear();
    bomTable_->clearSelection();
    updateButtonsState();
}

void BillOfMaterialManagementWidget::onManageBOMItemsClicked() {
    if (!hasPermission("Manufacturing.ManageBillOfMaterialItems")) { // New permission for BOM items
        showMessageBox("Lỗi", "Bạn không có quyền quản lý thành phần định mức nguyên vật liệu.", QMessageBox::Warning);
        return;
    }

    int selectedRow = bomTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Quản lý Thành phần BOM", "Vui lòng chọn một BOM để quản lý thành phần.", QMessageBox::Information);
        return;
    }

    QString bomId = bomTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> bomOpt = bomService_->getBillOfMaterialById(bomId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (bomOpt) {
        showManageBOMItemsDialog(&(*bomOpt));
    } else {
        showMessageBox("Quản lý Thành phần BOM", "Không tìm thấy BOM để quản lý thành phần.", QMessageBox::Critical);
    }
}


void BillOfMaterialManagementWidget::showBOMInputDialog(ERP::Manufacturing::DTO::BillOfMaterialDTO* bom) {
    QDialog dialog(this);
    dialog.setWindowTitle(bom ? "Sửa BOM" : "Thêm BOM Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit bomNameEdit(this);
    QComboBox productCombo(this); populateProductComboBox();
    for (int i = 0; i < productComboBox_->count(); ++i) productCombo.addItem(productComboBox_->itemText(i), productComboBox_->itemData(i));
    
    QLineEdit descriptionEdit(this);
    QLineEdit baseQuantityEdit(this); baseQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    QComboBox baseQuantityUnitCombo(this); populateUnitOfMeasureComboBox();
    for (int i = 0; i < baseQuantityUnitComboBox_->count(); ++i) baseQuantityUnitCombo.addItem(baseQuantityUnitComboBox_->itemText(i), baseQuantityUnitComboBox_->itemData(i));
    
    QComboBox statusCombo(this); populateStatusComboBox();
    for (int i = 0; i < statusComboBox_->count(); ++i) statusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));

    QLineEdit versionEdit(this); versionEdit.setValidator(new QIntValidator(0, 99999, this));

    if (bom) {
        bomNameEdit.setText(QString::fromStdString(bom->bomName));
        int productIndex = productCombo.findData(QString::fromStdString(bom->productId));
        if (productIndex != -1) productCombo.setCurrentIndex(productIndex);
        descriptionEdit.setText(QString::fromStdString(bom->description.value_or("")));
        baseQuantityEdit.setText(QString::number(bom->baseQuantity));
        int unitIndex = baseQuantityUnitCombo.findData(QString::fromStdString(bom->baseQuantityUnitId));
        if (unitIndex != -1) baseQuantityUnitCombo.setCurrentIndex(unitIndex);
        int statusIndex = statusCombo.findData(static_cast<int>(bom->status));
        if (statusIndex != -1) statusCombo.setCurrentIndex(statusIndex);
        versionEdit.setText(QString::number(bom->version.value_or(0)));
    } else {
        baseQuantityEdit.setText("1.0");
        versionEdit.setText("1");
    }

    formLayout->addRow("Tên BOM:*", &bomNameEdit);
    formLayout->addRow("Sản phẩm:*", &productCombo);
    formLayout->addRow("Mô tả:", &descriptionEdit);
    formLayout->addRow("SL cơ sở:*", &baseQuantityEdit);
    formLayout->addRow("Đơn vị SL cơ sở:*", &baseQuantityUnitCombo);
    formLayout->addRow("Trạng thái:*", &statusCombo);
    formLayout->addRow("Phiên bản:", &versionEdit);

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(bom ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Manufacturing::DTO::BillOfMaterialDTO newBOMData;
        if (bom) {
            newBOMData = *bom;
        }

        newBOMData.bomName = bomNameEdit.text().toStdString();
        newBOMData.productId = productCombo.currentData().toString().toStdString();
        newBOMData.description = descriptionEdit.text().isEmpty() ? std::nullopt : std::make_optional(descriptionEdit.text().toStdString());
        newBOMData.baseQuantity = baseQuantityEdit.text().toDouble();
        newBOMData.baseQuantityUnitId = baseQuantityUnitCombo.currentData().toString().toStdString();
        newBOMData.status = static_cast<ERP::Manufacturing::DTO::BillOfMaterialStatus>(statusCombo.currentData().toInt());
        newBOMData.version = versionEdit.text().toInt();

        // For new BOMs, items are added in a separate dialog after creation
        std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO> currentItems;
        if (bom) { // When editing, load existing items first
             currentItems = bomService_->getBillOfMaterialItems(bom->id, currentUserId_, currentUserRoleIds_);
        }


        bool success = false;
        if (bom) {
            success = bomService_->updateBillOfMaterial(newBOMData, currentItems, currentUserId_, currentUserRoleIds_); // Pass existing items if not modified
            if (success) showMessageBox("Sửa BOM", "BOM đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật BOM. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> createdBOM = bomService_->createBillOfMaterial(newBOMData, {}, currentUserId_, currentUserRoleIds_); // Create with empty items
            if (createdBOM) {
                showMessageBox("Thêm BOM", "BOM mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm BOM mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadBOMs();
            clearForm();
        }
    }
}

void BillOfMaterialManagementWidget::showManageBOMItemsDialog(ERP::Manufacturing::DTO::BillOfMaterialDTO* bom) {
    if (!bom) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Quản lý Thành phần cho BOM: " + QString::fromStdString(bom->bomName));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *itemsTable = new QTableWidget(&dialog);
    itemsTable->setColumnCount(4); // Product, Quantity, Unit, Notes
    itemsTable->setHorizontalHeaderLabels({"Sản phẩm", "Số lượng", "Đơn vị", "Ghi chú"});
    itemsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    itemsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    itemsTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(itemsTable);

    // Load existing items
    std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO> currentItems = bomService_->getBillOfMaterialItems(bom->id, currentUserId_, currentUserRoleIds_);
    itemsTable->setRowCount(currentItems.size());
    for (int i = 0; i < currentItems.size(); ++i) {
        const auto& item = currentItems[i];
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(item.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);

        QString unitName = "N/A";
        std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> unit = unitOfMeasureService_->getUnitOfMeasureById(item.unitOfMeasureId, currentUserId_, currentUserRoleIds_);
        if (unit) unitName = QString::fromStdString(unit->name);

        itemsTable->setItem(i, 0, new QTableWidgetItem(productName));
        itemsTable->setItem(i, 1, new QTableWidgetItem(QString::number(item.quantity)));
        itemsTable->setItem(i, 2, new QTableWidgetItem(unitName));
        itemsTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(item.notes.value_or(""))));
        itemsTable->item(i, 0)->setData(Qt::UserRole, QString::fromStdString(item.id)); // Store item ID
    }


    QHBoxLayout *itemButtonsLayout = new QHBoxLayout();
    QPushButton *addItemButton = new QPushButton("Thêm Thành phần", &dialog);
    QPushButton *editItemButton = new QPushButton("Sửa Thành phần", &dialog);
    QPushButton *deleteItemButton = new QPushButton("Xóa Thành phần", &dialog);
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
        // Show dialog to add a single item
        QDialog itemDialog(&dialog);
        itemDialog.setWindowTitle("Thêm Thành phần BOM");
        QFormLayout itemFormLayout;
        QComboBox productCombo; populateProductComboBox();
        for (int i = 0; i < productComboBox_->count(); ++i) productCombo.addItem(productComboBox_->itemText(i), productComboBox_->itemData(i));
        QLineEdit quantityEdit; quantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QComboBox unitCombo; populateUnitOfMeasureComboBox();
        for (int i = 0; i < baseQuantityUnitComboBox_->count(); ++i) unitCombo.addItem(baseQuantityUnitComboBox_->itemText(i), baseQuantityUnitComboBox_->itemData(i));
        QLineEdit notesEdit;

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Số lượng:*", &quantityEdit);
        itemFormLayout.addRow("Đơn vị:*", &unitCombo);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);

        QPushButton *okItemButton = new QPushButton("Thêm", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (productCombo.currentData().isNull() || quantityEdit.text().isEmpty() || unitCombo.currentData().isNull()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin thành phần.", QMessageBox::Warning);
                return;
            }
            // Add to table
            int newRow = itemsTable->rowCount();
            itemsTable->insertRow(newRow);
            itemsTable->setItem(newRow, 0, new QTableWidgetItem(productCombo.currentText()));
            itemsTable->setItem(newRow, 1, new QTableWidgetItem(quantityEdit.text()));
            itemsTable->setItem(newRow, 2, new QTableWidgetItem(unitCombo.currentText()));
            itemsTable->setItem(newRow, 3, new QTableWidgetItem(notesEdit.text()));
            itemsTable->item(newRow, 0)->setData(Qt::UserRole, productCombo.currentData()); // Store product ID
            itemsTable->item(newRow, 2)->setData(Qt::UserRole, unitCombo.currentData());   // Store unit ID
        }
    });

    connect(editItemButton, &QPushButton::clicked, [&]() {
        int selectedItemRow = itemsTable->currentRow();
        if (selectedItemRow < 0) {
            showMessageBox("Sửa Thành phần", "Vui lòng chọn một thành phần để sửa.", QMessageBox::Information);
            return;
        }

        QDialog itemDialog(&dialog);
        itemDialog.setWindowTitle("Sửa Thành phần BOM");
        QFormLayout itemFormLayout;
        QComboBox productCombo; populateProductComboBox();
        for (int i = 0; i < productComboBox_->count(); ++i) productCombo.addItem(productComboBox_->itemText(i), productComboBox_->itemData(i));
        QLineEdit quantityEdit; quantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QComboBox unitCombo; populateUnitOfMeasureComboBox();
        for (int i = 0; i < baseQuantityUnitComboBox_->count(); ++i) unitCombo.addItem(baseQuantityUnitComboBox_->itemText(i), baseQuantityUnitComboBox_->itemData(i));
        QLineEdit notesEdit;

        // Populate with current item data
        QString currentProductId = itemsTable->item(selectedItemRow, 0)->data(Qt::UserRole).toString();
        int productIndex = productCombo.findData(currentProductId);
        if (productIndex != -1) productCombo.setCurrentIndex(productIndex);

        quantityEdit.setText(itemsTable->item(selectedItemRow, 1)->text());

        QString currentUnitId = itemsTable->item(selectedItemRow, 2)->data(Qt::UserRole).toString();
        int unitIndex = unitCombo.findData(currentUnitId);
        if (unitIndex != -1) unitCombo.setCurrentIndex(unitIndex);

        notesEdit.setText(itemsTable->item(selectedItemRow, 3)->text());

        itemFormLayout.addRow("Sản phẩm:*", &productCombo);
        itemFormLayout.addRow("Số lượng:*", &quantityEdit);
        itemFormLayout.addRow("Đơn vị:*", &unitCombo);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);

        QPushButton *okItemButton = new QPushButton("Lưu", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (productCombo.currentData().isNull() || quantityEdit.text().isEmpty() || unitCombo.currentData().isNull()) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin thành phần.", QMessageBox::Warning);
                return;
            }
            // Update table row
            itemsTable->item(selectedItemRow, 0)->setText(productCombo.currentText());
            itemsTable->item(selectedItemRow, 1)->setText(quantityEdit.text());
            itemsTable->item(selectedItemRow, 2)->setText(unitCombo.currentText());
            itemsTable->item(selectedItemRow, 3)->setText(notesEdit.text());
            itemsTable->item(selectedItemRow, 0)->setData(Qt::UserRole, productCombo.currentData());
            itemsTable->item(selectedItemRow, 2)->setData(Qt::UserRole, unitCombo.currentData());
        }
    });

    connect(deleteItemButton, &QPushButton::clicked, [&]() {
        int selectedItemRow = itemsTable->currentRow();
        if (selectedItemRow < 0) {
            showMessageBox("Xóa Thành phần", "Vui lòng chọn một thành phần để xóa.", QMessageBox::Information);
            return;
        }
        Common::CustomMessageBox confirmDelBox(&dialog);
        confirmDelBox.setWindowTitle("Xóa Thành phần BOM");
        confirmDelBox.setText("Bạn có chắc chắn muốn xóa thành phần này?");
        confirmDelBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmDelBox.exec() == QMessageBox::Yes) {
            itemsTable->removeRow(selectedItemRow);
        }
    });


    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Collect all items from the table
        std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO> updatedItems;
        for (int i = 0; i < itemsTable->rowCount(); ++i) {
            ERP::Manufacturing::DTO::BillOfMaterialItemDTO item;
            item.id = ERP::Utils::generateUUID(); // New ID for all items, as we are replacing them
            item.productId = itemsTable->item(i, 0)->data(Qt::UserRole).toString().toStdString();
            item.quantity = itemsTable->item(i, 1)->text().toDouble();
            item.unitOfMeasureId = itemsTable->item(i, 2)->data(Qt::UserRole).toString().toStdString();
            item.notes = itemsTable->item(i, 3)->text().isEmpty() ? std::nullopt : std::make_optional(itemsTable->item(i, 3)->text().toStdString());
            updatedItems.push_back(item);
        }

        // Call service to update BOM with new item list
        if (bomService_->updateBillOfMaterial(*bom, updatedItems, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Quản lý Thành phần BOM", "Thành phần BOM đã được cập nhật thành công.", QMessageBox::Information);
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật thành phần BOM. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void BillOfMaterialManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool BillOfMaterialManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void BillOfMaterialManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Manufacturing.CreateBillOfMaterial");
    bool canUpdate = hasPermission("Manufacturing.UpdateBillOfMaterial");
    bool canDelete = hasPermission("Manufacturing.DeleteBillOfMaterial");
    bool canChangeStatus = hasPermission("Manufacturing.UpdateBillOfMaterialStatus");
    bool canManageItems = hasPermission("Manufacturing.ManageBillOfMaterialItems");

    addBOMButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Manufacturing.ViewBillOfMaterial"));

    bool isRowSelected = bomTable_->currentRow() >= 0;
    editBOMButton_->setEnabled(isRowSelected && canUpdate);
    deleteBOMButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    manageBOMItemsButton_->setEnabled(isRowSelected && canManageItems);

    bool enableForm = isRowSelected && canUpdate;
    bomNameLineEdit_->setEnabled(enableForm);
    productComboBox_->setEnabled(enableForm);
    descriptionLineEdit_->setEnabled(enableForm);
    baseQuantityLineEdit_->setEnabled(enableForm);
    baseQuantityUnitComboBox_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    versionLineEdit_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        bomNameLineEdit_->clear();
        productComboBox_->clear();
        descriptionLineEdit_->clear();
        baseQuantityLineEdit_->clear();
        baseQuantityUnitComboBox_->clear();
        statusComboBox_->setCurrentIndex(0);
        versionLineEdit_->clear();
    }
}


} // namespace Manufacturing
} // namespace UI
} // namespace ERP