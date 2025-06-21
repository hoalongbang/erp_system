// Modules/UI/Warehouse/InventoryManagementWidget.cpp
#include "InventoryManagementWidget.h" // Standard includes
#include "Inventory.h"                 // Inventory DTO
#include "InventoryTransaction.h"      // InventoryTransaction DTO
#include "Product.h"                   // Product DTO
#include "Warehouse.h"                 // Warehouse DTO
#include "Location.h"                  // Location DTO
#include "InventoryManagementService.h"// InventoryManagement Service
#include "ProductService.h"            // Product Service
#include "WarehouseService.h"          // Warehouse Service
#include "ISecurityManager.h"          // Security Manager
#include "Logger.h"                    // Logging
#include "ErrorHandler.h"              // Error Handling
#include "Common.h"                    // Common Enums/Constants
#include "DateUtils.h"                 // Date Utilities
#include "StringUtils.h"               // String Utilities
#include "CustomMessageBox.h"          // Custom Message Box
#include "UserService.h"               // For getting user names

#include <QInputDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDoubleValidator>
#include <QDialogButtonBox>

namespace ERP {
namespace UI {
namespace Warehouse {

InventoryManagementWidget::InventoryManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IInventoryManagementService> inventoryService,
    std::shared_ptr<Product::Services::IProductService> productService,
    std::shared_ptr<Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      inventoryService_(inventoryService),
      productService_(productService),
      warehouseService_(warehouseService),
      securityManager_(securityManager) {

    if (!inventoryService_ || !productService_ || !warehouseService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ quản lý tồn kho, sản phẩm, kho hàng hoặc bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("InventoryManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("InventoryManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("InventoryManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadInventory();
    updateButtonsState();
}

InventoryManagementWidget::~InventoryManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void InventoryManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo sản phẩm, kho, vị trí...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &InventoryManagementWidget::onSearchInventoryClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    inventoryTable_ = new QTableWidget(this);
    inventoryTable_->setColumnCount(10); // Product, Warehouse, Location, Quantity, Reserved, Available, Unit Cost, Lot/Serial, Mfg Date, Exp Date
    inventoryTable_->setHorizontalHeaderLabels({"Sản phẩm", "Kho hàng", "Vị trí", "SL", "SL Đặt trước", "SL Khả dụng", "Giá đơn vị", "Số lô/Serial", "Ngày SX", "Ngày HH"});
    inventoryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    inventoryTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    inventoryTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    inventoryTable_->horizontalHeader()->setStretchLastSection(true);
    connect(inventoryTable_, &QTableWidget::itemClicked, this, &InventoryManagementWidget::onInventoryTableItemClicked);
    mainLayout->addWidget(inventoryTable_);

    // Form elements for displaying inventory details (read-only)
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    productIdLineEdit_ = new QLineEdit(this); productIdLineEdit_->setReadOnly(true);
    productNameLineEdit_ = new QLineEdit(this); productNameLineEdit_->setReadOnly(true);
    warehouseIdLineEdit_ = new QLineEdit(this); warehouseIdLineEdit_->setReadOnly(true);
    warehouseNameLineEdit_ = new QLineEdit(this); warehouseNameLineEdit_->setReadOnly(true);
    locationIdLineEdit_ = new QLineEdit(this); locationIdLineEdit_->setReadOnly(true);
    locationNameLineEdit_ = new QLineEdit(this); locationNameLineEdit_->setReadOnly(true);
    quantityLineEdit_ = new QLineEdit(this); quantityLineEdit_->setReadOnly(true);
    reservedQuantityLineEdit_ = new QLineEdit(this); reservedQuantityLineEdit_->setReadOnly(true);
    availableQuantityLineEdit_ = new QLineEdit(this); availableQuantityLineEdit_->setReadOnly(true);
    unitCostLineEdit_ = new QLineEdit(this); unitCostLineEdit_->setReadOnly(true);
    lotNumberLineEdit_ = new QLineEdit(this); lotNumberLineEdit_->setReadOnly(true);
    serialNumberLineEdit_ = new QLineEdit(this); serialNumberLineEdit_->setReadOnly(true);
    manufactureDateEdit_ = new QDateTimeEdit(this); manufactureDateEdit_->setReadOnly(true); manufactureDateEdit_->setDisplayFormat("yyyy-MM-dd");
    expirationDateEdit_ = new QDateTimeEdit(this); expirationDateEdit_->setReadOnly(true); expirationDateEdit_->setDisplayFormat("yyyy-MM-dd");
    reorderLevelLineEdit_ = new QLineEdit(this); reorderLevelLineEdit_->setReadOnly(true);
    reorderQuantityLineEdit_ = new QLineEdit(this); reorderQuantityLineEdit_->setReadOnly(true);


    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addRow("ID Sản phẩm:", &productIdLineEdit_);
    formLayout->addRow("Tên Sản phẩm:", &productNameLineEdit_);
    formLayout->addRow("ID Kho hàng:", &warehouseIdLineEdit_);
    formLayout->addRow("Tên Kho hàng:", &warehouseNameLineEdit_);
    formLayout->addRow("ID Vị trí:", &locationIdLineEdit_);
    formLayout->addRow("Tên Vị trí:", &locationNameLineEdit_);
    formLayout->addRow("Số lượng:", &quantityLineEdit_);
    formLayout->addRow("SL Đặt trước:", &reservedQuantityLineEdit_);
    formLayout->addRow("SL Khả dụng:", &availableQuantityLineEdit_);
    formLayout->addRow("Giá đơn vị:", &unitCostLineEdit_);
    formLayout->addRow("Số lô:", &lotNumberLineEdit_);
    formLayout->addRow("Số Serial:", &serialNumberLineEdit_);
    formLayout->addRow("Ngày SX:", &manufactureDateEdit_);
    formLayout->addRow("Ngày HH:", &expirationDateEdit_);
    formLayout->addRow("Mức đặt hàng lại:", &reorderLevelLineEdit_);
    formLayout->addRow("SL đặt hàng lại:", &reorderQuantityLineEdit_);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    recordGoodsReceiptButton_ = new QPushButton("Ghi nhận Nhập kho", this);
    connect(recordGoodsReceiptButton_, &QPushButton::clicked, this, [&]() { showGoodsMovementDialog(ERP::Warehouse::DTO::InventoryTransactionType::GOODS_RECEIPT); });
    recordGoodsIssueButton_ = new QPushButton("Ghi nhận Xuất kho", this);
    connect(recordGoodsIssueButton_, &QPushButton::clicked, this, [&]() { showGoodsMovementDialog(ERP::Warehouse::DTO::InventoryTransactionType::GOODS_ISSUE); });
    adjustInventoryButton_ = new QPushButton("Điều chỉnh Tồn kho", this);
    connect(adjustInventoryButton_, &QPushButton::clicked, this, [&]() { showGoodsMovementDialog(ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_IN); }); // Can be IN/OUT
    transferStockButton_ = new QPushButton("Chuyển kho", this);
    connect(transferStockButton_, &QPushButton::clicked, this, &InventoryManagementWidget::showTransferStockDialog);
    clearFormButton_ = new QPushButton("Xóa Form", this);
    connect(clearFormButton_, &QPushButton::clicked, this, &InventoryManagementWidget::clearForm);
    
    buttonLayout->addWidget(recordGoodsReceiptButton_);
    buttonLayout->addWidget(recordGoodsIssueButton_);
    buttonLayout->addWidget(adjustInventoryButton_);
    buttonLayout->addWidget(transferStockButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void InventoryManagementWidget::loadInventory() {
    ERP::Logger::Logger::getInstance().info("InventoryManagementWidget: Loading inventory...");
    inventoryTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Warehouse::DTO::InventoryDTO> inventories = inventoryService_->getInventory({}, currentUserId_, currentUserRoleIds_);

    inventoryTable_->setRowCount(inventories.size());
    for (int i = 0; i < inventories.size(); ++i) {
        const auto& inventory = inventories[i];
        
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(inventory.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);

        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(inventory.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) warehouseName = QString::fromStdString(warehouse->name);

        QString locationName = "N/A";
        std::optional<ERP::Catalog::DTO::LocationDTO> location = warehouseService_->getLocationById(inventory.locationId, currentUserId_, currentUserRoleIds_);
        if (location) locationName = QString::fromStdString(location->name);

        inventoryTable_->setItem(i, 0, new QTableWidgetItem(productName));
        inventoryTable_->setItem(i, 1, new QTableWidgetItem(warehouseName));
        inventoryTable_->setItem(i, 2, new QTableWidgetItem(locationName));
        inventoryTable_->setItem(i, 3, new QTableWidgetItem(QString::number(inventory.quantity)));
        inventoryTable_->setItem(i, 4, new QTableWidgetItem(QString::number(inventory.reservedQuantity.value_or(0.0))));
        inventoryTable_->setItem(i, 5, new QTableWidgetItem(QString::number(inventory.availableQuantity.value_or(0.0))));
        inventoryTable_->setItem(i, 6, new QTableWidgetItem(QString::number(inventory.unitCost.value_or(0.0), 'f', 2)));
        inventoryTable_->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(inventory.lotNumber.value_or("") + "/" + inventory.serialNumber.value_or(""))));
        inventoryTable_->setItem(i, 8, new QTableWidgetItem(inventory.manufactureDate ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*inventory.manufactureDate, "yyyy-MM-dd")) : "N/A"));
        inventoryTable_->setItem(i, 9, new QTableWidgetItem(inventory.expirationDate ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*inventory.expirationDate, "yyyy-MM-dd")) : "N/A"));
        
        // Store IDs for later use
        inventoryTable_->item(i, 0)->setData(Qt::UserRole, QString::fromStdString(inventory.productId));
        inventoryTable_->item(i, 1)->setData(Qt::UserRole, QString::fromStdString(inventory.warehouseId));
        inventoryTable_->item(i, 2)->setData(Qt::UserRole, QString::fromStdString(inventory.locationId));
        inventoryTable_->item(i, 3)->setData(Qt::UserRole+1, QString::fromStdString(inventory.id)); // Store Inventory ID
    }
    inventoryTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("InventoryManagementWidget: Inventory loaded successfully.");
}

void InventoryManagementWidget::populateProductComboBox(QComboBox* comboBox) {
    comboBox->clear();
    std::vector<ERP::Product::DTO::ProductDTO> allProducts = productService_->getAllProducts({}, currentUserId_, currentUserRoleIds_);
    for (const auto& product : allProducts) {
        comboBox->addItem(QString::fromStdString(product.name + " (" + product.productCode + ")"), QString::fromStdString(product.id));
    }
}

void InventoryManagementWidget::populateWarehouseComboBox(QComboBox* comboBox) {
    comboBox->clear();
    std::vector<ERP::Catalog::DTO::WarehouseDTO> allWarehouses = warehouseService_->getAllWarehouses({}, currentUserId_, currentUserRoleIds_);
    for (const auto& warehouse : allWarehouses) {
        comboBox->addItem(QString::fromStdString(warehouse.name), QString::fromStdString(warehouse.id));
    }
}

void InventoryManagementWidget::populateLocationComboBox(QComboBox* comboBox, const std::string& warehouseId) {
    comboBox->clear();
    std::vector<ERP::Catalog::DTO::LocationDTO> locations;
    if (!warehouseId.empty()) {
        locations = warehouseService_->getLocationsByWarehouse(warehouseId, currentUserId_, currentUserRoleIds_);
    }
    for (const auto& location : locations) {
        comboBox->addItem(QString::fromStdString(location.name), QString::fromStdString(location.id));
    }
}


void InventoryManagementWidget::onRecordGoodsReceiptClicked() {
    if (!hasPermission("Warehouse.RecordGoodsReceipt")) {
        showMessageBox("Lỗi", "Bạn không có quyền ghi nhận nhập kho.", QMessageBox::Warning);
        return;
    }
    showGoodsMovementDialog(ERP::Warehouse::DTO::InventoryTransactionType::GOODS_RECEIPT);
}

void InventoryManagementWidget::onRecordGoodsIssueClicked() {
    if (!hasPermission("Warehouse.RecordGoodsIssue")) {
        showMessageBox("Lỗi", "Bạn không có quyền ghi nhận xuất kho.", QMessageBox::Warning);
        return;
    }
    showGoodsMovementDialog(ERP::Warehouse::DTO::InventoryTransactionType::GOODS_ISSUE);
}

void InventoryManagementWidget::onAdjustInventoryClicked() {
    if (!hasPermission("Warehouse.AdjustInventoryManual")) {
        showMessageBox("Lỗi", "Bạn không có quyền điều chỉnh tồn kho.", QMessageBox::Warning);
        return;
    }
    // For simplicity, using ADJUSTMENT_IN as default in dialog. User can specify sign.
    showGoodsMovementDialog(ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_IN);
}

void InventoryManagementWidget::onTransferStockClicked() {
    if (!hasPermission("Warehouse.TransferStock")) {
        showMessageBox("Lỗi", "Bạn không có quyền chuyển kho.", QMessageBox::Warning);
        return;
    }
    showTransferStockDialog();
}

void InventoryManagementWidget::onSearchInventoryClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["search_term"] = searchText.toStdString(); // Assuming generic search by product, warehouse, location names/codes
    }
    loadInventory(); // Reloads with current filter if any
    ERP::Logger::Logger::getInstance().info("InventoryManagementWidget: Search completed.");
}

void InventoryManagementWidget::onInventoryTableItemClicked(int row, int column) {
    if (row < 0) return;
    
    // Retrieve full product ID, warehouse ID, location ID from hidden data
    QString productId = inventoryTable_->item(row, 0)->data(Qt::UserRole).toString();
    QString warehouseId = inventoryTable_->item(row, 1)->data(Qt::UserRole).toString();
    QString locationId = inventoryTable_->item(row, 2)->data(Qt::UserRole).toString();
    QString inventoryId = inventoryTable_->item(row, 3)->data(Qt::UserRole+1).toString(); // Get Inventory ID

    std::optional<ERP::Warehouse::DTO::InventoryDTO> inventoryOpt;
    if (!inventoryId.isEmpty()) {
        // More robust to query by Inventory ID if DAOBase supports it directly
        // Currently, getInventoryByProductLocation is available
        inventoryOpt = inventoryService_->getInventoryByProductLocation(productId.toStdString(), warehouseId.toStdString(), locationId.toStdString(), currentUserId_, currentUserRoleIds_);
    }


    if (inventoryOpt) {
        idLineEdit_->setText(QString::fromStdString(inventoryOpt->id));
        productIdLineEdit_->setText(QString::fromStdString(inventoryOpt->productId));
        
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(inventoryOpt->productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);
        productNameLineEdit_->setText(productName);

        warehouseIdLineEdit_->setText(QString::fromStdString(inventoryOpt->warehouseId));
        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(inventoryOpt->warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) warehouseName = QString::fromStdString(warehouse->name);
        warehouseNameLineEdit_->setText(warehouseName);

        locationIdLineEdit_->setText(QString::fromStdString(inventoryOpt->locationId));
        QString locationName = "N/A";
        std::optional<ERP::Catalog::DTO::LocationDTO> location = warehouseService_->getLocationById(inventoryOpt->locationId, currentUserId_, currentUserRoleIds_);
        if (location) locationName = QString::fromStdString(location->name);
        locationNameLineEdit_->setText(locationName);

        quantityLineEdit_->setText(QString::number(inventoryOpt->quantity));
        reservedQuantityLineEdit_->setText(QString::number(inventoryOpt->reservedQuantity.value_or(0.0)));
        availableQuantityLineEdit_->setText(QString::number(inventoryOpt->availableQuantity.value_or(0.0)));
        unitCostLineEdit_->setText(QString::number(inventoryOpt->unitCost.value_or(0.0), 'f', 2));
        lotNumberLineEdit_->setText(QString::fromStdString(inventoryOpt->lotNumber.value_or("")));
        serialNumberLineEdit_->setText(QString::fromStdString(inventoryOpt->serialNumber.value_or("")));
        if (inventoryOpt->manufactureDate) manufactureDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(inventoryOpt->manufactureDate->time_since_epoch()).count()));
        else manufactureDateEdit_->clear();
        if (inventoryOpt->expirationDate) expirationDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(inventoryOpt->expirationDate->time_since_epoch()).count()));
        else expirationDateEdit_->clear();
        reorderLevelLineEdit_->setText(QString::number(inventoryOpt->reorderLevel.value_or(0.0)));
        reorderQuantityLineEdit_->setText(QString::number(inventoryOpt->reorderQuantity.value_or(0.0)));

    } else {
        showMessageBox("Thông tin Tồn kho", "Không thể tải chi tiết tồn kho đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void InventoryManagementWidget::clearForm() {
    idLineEdit_->clear();
    productIdLineEdit_->clear();
    productNameLineEdit_->clear();
    warehouseIdLineEdit_->clear();
    warehouseNameLineEdit_->clear();
    locationIdLineEdit_->clear();
    locationNameLineEdit_->clear();
    quantityLineEdit_->clear();
    reservedQuantityLineEdit_->clear();
    availableQuantityLineEdit_->clear();
    unitCostLineEdit_->clear();
    lotNumberLineEdit_->clear();
    serialNumberLineEdit_->clear();
    manufactureDateEdit_->clear();
    expirationDateEdit_->clear();
    reorderLevelLineEdit_->clear();
    reorderQuantityLineEdit_->clear();
    inventoryTable_->clearSelection();
    updateButtonsState();
}

void InventoryManagementWidget::showGoodsMovementDialog(ERP::Warehouse::DTO::InventoryTransactionType type) {
    QDialog dialog(this);
    QString dialogTitle;
    if (type == ERP::Warehouse::DTO::InventoryTransactionType::GOODS_RECEIPT) dialogTitle = "Ghi nhận Nhập kho";
    else if (type == ERP::Warehouse::DTO::InventoryTransactionType::GOODS_ISSUE) dialogTitle = "Ghi nhận Xuất kho";
    else if (type == ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_IN) dialogTitle = "Điều chỉnh Tồn kho"; // For Adjustments
    dialog.setWindowTitle(dialogTitle);
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox productCombo(this); populateProductComboBox(&productCombo);
    QComboBox warehouseCombo(this); populateWarehouseComboBox(&warehouseCombo);
    QComboBox locationCombo(this); // Populated dynamically based on warehouse selection
    connect(&warehouseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index){
        QString selectedWarehouseId = warehouseCombo.currentData().toString();
        populateLocationComboBox(&locationCombo, selectedWarehouseId.toStdString());
    });
    // Initial populate for location based on default/first warehouse
    if (warehouseCombo.count() > 0) populateLocationComboBox(&locationCombo, warehouseCombo.itemData(0).toString().toStdString());


    QLineEdit quantityEdit(this); quantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    if (type == ERP::Warehouse::DTO::InventoryTransactionType::GOODS_ISSUE || type == ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_OUT) {
        quantityEdit.setPlaceholderText("Số lượng (âm cho điều chỉnh giảm)");
    } else {
        quantityEdit.setPlaceholderText("Số lượng");
    }
    
    QLineEdit unitCostEdit(this); unitCostEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    QLineEdit lotNumberEdit(this);
    QLineEdit serialNumberEdit(this);
    QDateTimeEdit manufactureDateEdit(this); manufactureDateEdit.setDisplayFormat("yyyy-MM-dd"); manufactureDateEdit.setCalendarPopup(true);
    QDateTimeEdit expirationDateEdit(this); expirationDateEdit.setDisplayFormat("yyyy-MM-dd"); expirationDateEdit.setCalendarPopup(true);
    QLineEdit referenceDocIdEdit(this);
    QLineEdit referenceDocTypeEdit(this);
    QLineEdit notesEdit(this);

    formLayout->addRow("Sản phẩm:*", &productCombo);
    formLayout->addRow("Kho hàng:*", &warehouseCombo);
    formLayout->addRow("Vị trí:*", &locationCombo);
    formLayout->addRow("Số lượng:*", &quantityEdit);
    // Unit Cost is primarily for goods receipt or adjustments where cost changes
    if (type == ERP::Warehouse::DTO::InventoryTransactionType::GOODS_RECEIPT || type == ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_IN) {
        formLayout->addRow("Giá đơn vị:", &unitCostEdit);
    }
    formLayout->addRow("Số lô:", &lotNumberEdit);
    formLayout->addRow("Số Serial:", &serialNumberEdit);
    formLayout->addRow("Ngày SX:", &manufactureDateEdit);
    formLayout->addRow("Ngày HH:", &expirationDateEdit);
    formLayout->addRow("ID Tài liệu tham chiếu:", &referenceDocIdEdit);
    formLayout->addRow("Loại Tài liệu tham chiếu:", &referenceDocTypeEdit);
    formLayout->addRow("Ghi chú:", &notesEdit);
    
    dialogLayout->addLayout(formLayout);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    dialogLayout->addWidget(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        if (productCombo.currentData().isNull() || warehouseCombo.currentData().isNull() || locationCombo.currentData().isNull() || quantityEdit.text().isEmpty()) {
            showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin bắt buộc (Sản phẩm, Kho, Vị trí, Số lượng).", QMessageBox::Warning);
            return;
        }
        
        ERP::Warehouse::DTO::InventoryTransactionDTO transaction;
        transaction.productId = productCombo.currentData().toString().toStdString();
        transaction.warehouseId = warehouseCombo.currentData().toString().toStdString();
        transaction.locationId = locationCombo.currentData().toString().toStdString();
        transaction.type = type; // Set the type from dialog context
        transaction.quantity = quantityEdit.text().toDouble();
        transaction.unitCost = unitCostEdit.text().isEmpty() ? 0.0 : unitCostEdit.text().toDouble();
        transaction.lotNumber = lotNumberEdit.text().isEmpty() ? std::nullopt : std::make_optional(lotNumberEdit.text().toStdString());
        transaction.serialNumber = serialNumberEdit.text().isEmpty() ? std::nullopt : std::make_optional(serialNumberEdit.text().toStdString());
        transaction.manufactureDate = manufactureDateEdit.dateTime().isNull() ? std::nullopt : std::make_optional(ERP::Utils::DateUtils::qDateTimeToTimePoint(manufactureDateEdit.dateTime()));
        transaction.expirationDate = expirationDateEdit.dateTime().isNull() ? std::nullopt : std::make_optional(ERP::Utils::DateUtils::qDateTimeToTimePoint(expirationDateEdit.dateTime()));
        transaction.referenceDocumentId = referenceDocIdEdit.text().isEmpty() ? std::nullopt : std::make_optional(referenceDocIdEdit.text().toStdString());
        transaction.referenceDocumentType = referenceDocTypeEdit.text().isEmpty() ? std::nullopt : std::make_optional(referenceDocTypeEdit.text().toStdString());
        transaction.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());
        // Other BaseDTO fields (id, createdAt, createdBy, status) handled by service

        bool success = false;
        if (type == ERP::Warehouse::DTO::InventoryTransactionType::GOODS_RECEIPT) {
            success = inventoryService_->recordGoodsReceipt(transaction, currentUserId_, currentUserRoleIds_);
        } else if (type == ERP::Warehouse::DTO::InventoryTransactionType::GOODS_ISSUE) {
            success = inventoryService_->recordGoodsIssue(transaction, currentUserId_, currentUserRoleIds_);
        } else if (type == ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_IN || type == ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_OUT) {
            // Adjustments need to explicitly set type based on quantity sign if only one dialog for both
            // For simplicity, we use type as ADJUSTMENT_IN and rely on service to interpret quantity sign
            transaction.type = (transaction.quantity >= 0) ? ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_IN : ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_OUT;
            success = inventoryService_->adjustInventory(transaction, currentUserId_, currentUserRoleIds_);
        }

        if (success) {
            showMessageBox(dialogTitle, "Thao tác tồn kho đã được ghi nhận thành công.", QMessageBox::Information);
            loadInventory();
            clearForm();
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể ghi nhận thao tác tồn kho. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}

void InventoryManagementWidget::showTransferStockDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Chuyển kho");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox productCombo(this); populateProductComboBox(&productCombo);
    QLineEdit quantityEdit(this); quantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    
    QLabel fromLabel("Từ:");
    QComboBox fromWarehouseCombo(this); populateWarehouseComboBox(&fromWarehouseCombo);
    QComboBox fromLocationCombo(this);
    connect(&fromWarehouseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index){
        QString selectedWarehouseId = fromWarehouseCombo.currentData().toString();
        populateLocationComboBox(&fromLocationCombo, selectedWarehouseId.toStdString());
    });
    if (fromWarehouseCombo.count() > 0) populateLocationComboBox(&fromLocationCombo, fromWarehouseCombo.itemData(0).toString().toStdString());

    QLabel toLabel("Đến:");
    QComboBox toWarehouseCombo(this); populateWarehouseComboBox(&toWarehouseCombo);
    QComboBox toLocationCombo(this);
    connect(&toWarehouseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index){
        QString selectedWarehouseId = toWarehouseCombo.currentData().toString();
        populateLocationComboBox(&toLocationCombo, selectedWarehouseId.toStdString());
    });
    if (toWarehouseCombo.count() > 0) populateLocationComboBox(&toLocationCombo, toWarehouseCombo.itemData(0).toString().toStdString());


    formLayout->addRow("Sản phẩm:*", &productCombo);
    formLayout->addRow("Số lượng:*", &quantityEdit);
    formLayout->addRow(fromLabel);
    formLayout->addRow("Kho hàng:", &fromWarehouseCombo);
    formLayout->addRow("Vị trí:", &fromLocationCombo);
    formLayout->addRow(toLabel);
    formLayout->addRow("Kho hàng:", &toWarehouseCombo);
    formLayout->addRow("Vị trí:", &toLocationCombo);

    dialogLayout->addLayout(formLayout);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    dialogLayout->addWidget(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        if (productCombo.currentData().isNull() || quantityEdit.text().isEmpty() ||
            fromWarehouseCombo.currentData().isNull() || fromLocationCombo.currentData().isNull() ||
            toWarehouseCombo.currentData().isNull() || toLocationCombo.currentData().isNull()) {
            showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chuyển kho.", QMessageBox::Warning);
            return;
        }

        std::string productId = productCombo.currentData().toString().toStdString();
        double quantity = quantityEdit.text().toDouble();
        std::string fromWarehouseId = fromWarehouseCombo.currentData().toString().toStdString();
        std::string fromLocationId = fromLocationCombo.currentData().toString().toStdString();
        std::string toWarehouseId = toWarehouseCombo.currentData().toString().toStdString();
        std::string toLocationId = toLocationCombo.currentData().toString().toStdString();

        if (inventoryService_->transferStock(productId, quantity, fromWarehouseId, fromLocationId, toWarehouseId, toLocationId, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Chuyển kho", "Chuyển kho thành công.", QMessageBox::Information);
            loadInventory();
            clearForm();
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể chuyển kho. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void InventoryManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool InventoryManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void InventoryManagementWidget::updateButtonsState() {
    bool canRecordReceipt = hasPermission("Warehouse.RecordGoodsReceipt");
    bool canRecordIssue = hasPermission("Warehouse.RecordGoodsIssue");
    bool canAdjust = hasPermission("Warehouse.AdjustInventoryManual");
    bool canTransfer = hasPermission("Warehouse.TransferStock");
    bool canView = hasPermission("Warehouse.ViewInventory");

    recordGoodsReceiptButton_->setEnabled(canRecordReceipt);
    recordGoodsIssueButton_->setEnabled(canRecordIssue);
    adjustInventoryButton_->setEnabled(canAdjust);
    transferStockButton_->setEnabled(canTransfer);
    searchButton_->setEnabled(canView);

    bool isRowSelected = inventoryTable_->currentRow() >= 0;
    // All form fields are read-only, no need to enable/disable based on selection for direct editing.
    // clearFormButton_ is always enabled.
}

} // namespace Warehouse
} // namespace UI
} // namespace ERP