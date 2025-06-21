// UI/Warehouse/InventoryTransactionManagementWidget.cpp
#include "InventoryTransactionManagementWidget.h"
#include "CustomMessageBox.h" // For Common::CustomMessageBox

namespace ERP {
namespace UI {
namespace Warehouse {

InventoryTransactionManagementWidget::InventoryTransactionManagementWidget(
    QWidget* parent,
    std::shared_ptr<ERP::Warehouse::Services::IInventoryTransactionService> inventoryTransactionService,
    std::shared_ptr<ERP::Product::Services::IProductService> productService,
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<ERP::Catalog::Services::ILocationService> locationService,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : QWidget(parent),
      inventoryTransactionService_(inventoryTransactionService),
      productService_(productService),
      warehouseService_(warehouseService),
      locationService_(locationService),
      securityManager_(securityManager) {
    
    if (!inventoryTransactionService_ || !productService_ || !warehouseService_ || !locationService_ || !securityManager_) {
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "InventoryTransactionManagementWidget: Initialized with null service.", "Lỗi hệ thống: Một hoặc nhiều dịch vụ không khả dụng.");
        showMessageBox("Lỗi khởi tạo", "Không thể khởi tạo widget quản lý giao dịch tồn kho do lỗi dịch vụ.", QMessageBox::Critical);
        return;
    }

    // Get current user ID and roles from securityManager
    // This is safer as these values are managed by the main application's security context.
    auto authService = securityManager_->getAuthenticationService();
    if (authService) {
        std::string dummySessionId = "current_session_id"; // Placeholder, a real session ID from login
        std::optional<ERP::Security::DTO::SessionDTO> currentSession = authService->validateSession(dummySessionId);
        if (currentSession) {
            currentUserId_ = currentSession->userId;
            currentUserRoleIds_ = securityManager_->getUserService()->getUserRoles(currentUserId_, {});
        } else {
            currentUserId_ = "system_user"; // Fallback for non-logged-in operations or system actions
            currentUserRoleIds_ = {"anonymous"}; // Fallback
            ERP::Logger::Logger::getInstance().warning("InventoryTransactionManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("InventoryTransactionManagementWidget: Authentication Service not available. Running with limited privileges.");
    }
    
    setupUI();
    populateWarehouseFilterComboBox();
    populateLocationFilterComboBox(filterWarehouseComboBox_->currentData().toString()); // Populate based on initial warehouse selection
    populateTransactionTypeFilterComboBox();
    loadInventoryTransactions(); // Load initial data
}

void InventoryTransactionManagementWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // --- Filter Section ---
    QGridLayout* filterGridLayout = new QGridLayout();

    filterGridLayout->addWidget(new QLabel("Tên sản phẩm:"), 0, 0);
    filterProductNameInput_ = new QLineEdit(this);
    filterProductNameInput_->setPlaceholderText("Nhập tên sản phẩm");
    filterGridLayout->addWidget(filterProductNameInput_, 0, 1);

    filterGridLayout->addWidget(new QLabel("Loại GD:"), 0, 2);
    filterTransactionTypeComboBox_ = new QComboBox(this);
    filterTransactionTypeComboBox_->setPlaceholderText("Chọn loại GD");
    filterTransactionTypeComboBox_->addItem("Tất cả loại", QVariant("")); // Option to show all
    filterGridLayout->addWidget(filterTransactionTypeComboBox_, 0, 3);

    filterGridLayout->addWidget(new QLabel("ID Tài liệu tham chiếu:"), 1, 0);
    filterReferenceDocumentIdInput_ = new QLineEdit(this);
    filterReferenceDocumentIdInput_->setPlaceholderText("Nhập ID tài liệu");
    filterGridLayout->addWidget(filterReferenceDocumentIdInput_, 1, 1);

    filterGridLayout->addWidget(new QLabel("Kho hàng:"), 1, 2);
    filterWarehouseComboBox_ = new QComboBox(this);
    filterWarehouseComboBox_->setPlaceholderText("Chọn kho hàng");
    filterWarehouseComboBox_->addItem("Tất cả kho hàng", QVariant(""));
    connect(filterWarehouseComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [&](int index){
        QString selectedWarehouseId = filterWarehouseComboBox_->currentData().toString();
        populateLocationFilterComboBox(selectedWarehouseId);
        // Do not call loadInventoryTransactions here if it is called in populateLocationFilterComboBox as well
        // loadInventoryTransactions();
    });
    filterGridLayout->addWidget(filterWarehouseComboBox_, 1, 3);

    filterGridLayout->addWidget(new QLabel("Vị trí:"), 2, 0);
    filterLocationComboBox_ = new QComboBox(this);
    filterLocationComboBox_->setPlaceholderText("Chọn vị trí");
    filterLocationComboBox_->addItem("Tất cả vị trí", QVariant(""));
    connect(filterLocationComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InventoryTransactionManagementWidget::loadInventoryTransactions);
    filterGridLayout->addWidget(filterLocationComboBox_, 2, 1);

    filterGridLayout->addWidget(new QLabel("Số lô/Serial:"), 2, 2); // NEW ROW FOR LOT/SERIAL
    QHBoxLayout* lotSerialLayout = new QHBoxLayout();
    filterLotNumberInput_ = new QLineEdit(this);
    filterLotNumberInput_->setPlaceholderText("Số lô");
    filterSerialNumberInput_ = new QLineEdit(this);
    filterSerialNumberInput_->setPlaceholderText("Số Serial");
    lotSerialLayout->addWidget(filterLotNumberInput_);
    lotSerialLayout->addWidget(filterSerialNumberInput_);
    filterGridLayout->addLayout(lotSerialLayout, 2, 3);

    filterGridLayout->addWidget(new QLabel("Ngày bắt đầu:"), 3, 0); // Moved to row 3
    filterStartDateEdit_ = new QDateTimeEdit(QDateTime::currentDateTime().addMonths(-1), this);
    filterStartDateEdit_->setCalendarPopup(true);
    filterStartDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    filterGridLayout->addWidget(filterStartDateEdit_, 3, 1);

    filterGridLayout->addWidget(new QLabel("Ngày kết thúc:"), 3, 2); // Moved to row 3
    filterEndDateEdit_ = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    filterEndDateEdit_->setCalendarPopup(true);
    filterEndDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    filterGridLayout->addWidget(filterEndDateEdit_, 3, 3);

    loadTransactionsButton_ = new QPushButton("Tải giao dịch", this);
    filterGridLayout->addWidget(loadTransactionsButton_, 4, 3, 1, 1); // Placed in new row for button

    mainLayout->addLayout(filterGridLayout);

    // --- Transactions Table ---
    transactionsTable_ = new QTableWidget(this);
    transactionsTable_->setColumnCount(13); // ID, Product, Type, Quantity, Unit Cost, Date, Warehouse, Location, Lot, Serial, Ref Doc ID, Ref Doc Type, Notes
    transactionsTable_->setHorizontalHeaderLabels({"ID", "Sản phẩm", "Loại GD", "SL", "Giá vốn ĐV", "Ngày GD", "Kho hàng", "Vị trí", "Số lô", "Số Serial", "ID Tài liệu", "Loại Tài liệu", "Ghi chú"});
    transactionsTable_->horizontalHeader()->setStretchLastSection(true);
    transactionsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    transactionsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(transactionsTable_);

    // Connect signals and slots for filters
    connect(loadTransactionsButton_, &QPushButton::clicked, this, &InventoryTransactionManagementWidget::loadInventoryTransactions);
    connect(filterProductNameInput_, &QLineEdit::returnPressed, this, &InventoryTransactionManagementWidget::loadInventoryTransactions);
    connect(filterLotNumberInput_, &QLineEdit::returnPressed, this, &InventoryTransactionManagementWidget::loadInventoryTransactions);
    connect(filterSerialNumberInput_, &QLineEdit::returnPressed, this, &InventoryTransactionManagementWidget::loadInventoryTransactions);
    connect(filterTransactionTypeComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InventoryTransactionManagementWidget::loadInventoryTransactions);
    connect(filterReferenceDocumentIdInput_, &QLineEdit::returnPressed, this, &InventoryTransactionManagementWidget::loadInventoryTransactions);
    connect(filterStartDateEdit_, &QDateTimeEdit::dateTimeChanged, this, &InventoryTransactionManagementWidget::loadInventoryTransactions);
    connect(filterEndDateEdit_, &QDateTimeEdit::dateTimeChanged, this, &InventoryTransactionManagementWidget::loadInventoryTransactions);
}

void InventoryTransactionManagementWidget::populateWarehouseFilterComboBox() {
    filterWarehouseComboBox_->clear();
    filterWarehouseComboBox_->addItem("Tất cả kho hàng", QVariant(""));
    std::vector<ERP::Catalog::DTO::WarehouseDTO> warehouses = warehouseService_->getAllWarehouses({}, currentUserId_, currentUserRoleIds_); // Pass current user context
    for (const auto& warehouse : warehouses) {
        if (warehouse.status == ERP::Common::EntityStatus::ACTIVE) {
            filterWarehouseComboBox_->addItem(QString::fromStdString(warehouse.name), QVariant(QString::fromStdString(warehouse.id)));
        }
    }
}

void InventoryTransactionManagementWidget::populateLocationFilterComboBox(const QString& warehouseId) {
    filterLocationComboBox_->clear();
    filterLocationComboBox_->addItem("Tất cả vị trí", QVariant(""));

    std::map<std::string, std::any> filter;
    if (!warehouseId.isEmpty()) {
        filter["warehouse_id"] = warehouseId.toStdString();
    }
    std::vector<ERP::Catalog::DTO::LocationDTO> locations = locationService_->getAllLocations(filter, currentUserId_, currentUserRoleIds_); // Pass current user context
    for (const auto& location : locations) {
        if (location.status == ERP::Common::EntityStatus::ACTIVE) {
            filterLocationComboBox_->addItem(QString::fromStdString(location.name), QVariant(QString::fromStdString(location.id)));
        }
    }
    loadInventoryTransactions(); // Reload transactions when location filter changes
}

void InventoryTransactionManagementWidget::populateTransactionTypeFilterComboBox() {
    filterTransactionTypeComboBox_->clear();
    filterTransactionTypeComboBox_->addItem("Tất cả loại", QVariant("")); // Option to show all
    for (int i = static_cast<int>(ERP::Warehouse::DTO::InventoryTransactionType::GOODS_RECEIPT); i <= static_cast<int>(ERP::Warehouse::DTO::InventoryTransactionType::RESERVATION_RELEASE); ++i) {
        ERP::Warehouse::DTO::InventoryTransactionType type = static_cast<ERP::Warehouse::DTO::InventoryTransactionType>(i);
        filterTransactionTypeComboBox_->addItem(QString::fromStdString(ERP::Warehouse::DTO::InventoryTransactionDTO().getTypeString(type)), QVariant(i)); // Use DTO helper
    }
}

void InventoryTransactionManagementWidget::loadInventoryTransactions() {
    // Kiểm tra quyền hạn chi tiết
    if (!securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, "Warehouse.ViewInventoryTransactions")) {
        showMessageBox("Lỗi quyền", "Bạn không có quyền xem giao dịch tồn kho.", QMessageBox::Warning);
        transactionsTable_->setRowCount(0);
        return;
    }

    transactionsTable_->setRowCount(0); // Clear existing rows
    
    std::map<std::string, std::any> filterMap;

    QString productName = filterProductNameInput_->text().trimmed();
    if (!productName.isEmpty()) {
        std::map<std::string, std::any> productFilter;
        productFilter["name_contains"] = productName.toStdString(); // Assuming productService supports "contains"
        std::vector<ERP::Product::DTO::ProductDTO> products = productService_->getAllProducts(productFilter, currentUserId_, currentUserRoleIds_);
        if (!products.empty()) {
            filterMap["product_id"] = products[0].id; // Use ID of the first matching product
        } else {
            // No product found matching filter, so no transactions to display
            return;
        }
    }

    QString selectedWarehouseId = filterWarehouseComboBox_->currentData().toString();
    if (!selectedWarehouseId.isEmpty()) filterMap["warehouse_id"] = selectedWarehouseId.toStdString();
    
    QString selectedLocationId = filterLocationComboBox_->currentData().toString();
    if (!selectedLocationId.isEmpty()) filterMap["location_id"] = selectedLocationId.toStdString();

    QVariant typeVariant = filterTransactionTypeComboBox_->currentData();
    if (typeVariant.isValid() && !typeVariant.toString().isEmpty()) {
        filterMap["type"] = typeVariant.toInt(); // Store enum int value
    }

    if (!filterReferenceDocumentIdInput_->text().isEmpty()) filterMap["reference_document_id"] = filterReferenceDocumentIdInput_->text().toStdString();
    if (!filterLotNumberInput_->text().isEmpty()) filterMap["lot_number"] = filterLotNumberInput_->text().toStdString();
    if (!filterSerialNumberInput_->text().isEmpty()) filterMap["serial_number"] = filterSerialNumberInput_->text().toStdString();

    // Add date range filters
    filterMap["transaction_date_ge"] = ERP::Utils::DateUtils::qDateTimeToTimePoint(filterStartDateEdit_->dateTime());
    filterMap["transaction_date_le"] = ERP::Utils::DateUtils::qDateTimeToTimePoint(filterEndDateEdit_->dateTime());

    std::vector<ERP::Warehouse::DTO::InventoryTransactionDTO> transactions = inventoryTransactionService_->getAllInventoryTransactions(filterMap, currentUserRoleIds_); // Pass full filterMap
    
    transactionsTable_->setRowCount(transactions.size());

    for (int i = 0; i < transactions.size(); ++i) {
        const auto& txn = transactions[i];
        std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(txn.productId, currentUserId_, currentUserRoleIds_);
        QString productName = product ? QString::fromStdString(product->name) : "Không rõ";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(txn.warehouseId, currentUserId_, currentUserRoleIds_);
        QString warehouseName = warehouse ? QString::fromStdString(warehouse->name) : "Không rõ";
        std::optional<ERP::Catalog::DTO::LocationDTO> location = locationService_->getLocationById(txn.locationId, currentUserId_, currentUserRoleIds_);
        QString locationName = location ? QString::fromStdString(location->name) : "Không rõ";

        transactionsTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(txn.id)));
        transactionsTable_->setItem(i, 1, new QTableWidgetItem(productName));
        transactionsTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(txn.getTypeString())));
        transactionsTable_->setItem(i, 3, new QTableWidgetItem(QString::number(txn.quantity)));
        transactionsTable_->setItem(i, 4, new QTableWidgetItem(QString::number(txn.unitCost.value_or(0.0), 'f', 2)));
        transactionsTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(txn.transactionDate, ERP::Common::DATETIME_FORMAT))));
        transactionsTable_->setItem(i, 6, new QTableWidgetItem(warehouseName));
        transactionsTable_->setItem(i, 7, new QTableWidgetItem(locationName));
        transactionsTable_->setItem(i, 8, new QTableWidgetItem(txn.lotNumber ? QString::fromStdString(*txn.lotNumber) : "N/A")); // NEW
        transactionsTable_->setItem(i, 9, new QTableWidgetItem(txn.serialNumber ? QString::fromStdString(*txn.serialNumber) : "N/A")); // NEW
        transactionsTable_->setItem(i, 10, new QTableWidgetItem(txn.referenceDocumentId ? QString::fromStdString(*txn.referenceDocumentId) : "N/A")); // Index change
        transactionsTable_->setItem(i, 11, new QTableWidgetItem(txn.referenceDocumentType ? QString::fromStdString(*txn.referenceDocumentType) : "N/A")); // Index change
        transactionsTable_->setItem(i, 12, new QTableWidgetItem(txn.notes ? QString::fromStdString(*txn.notes) : "")); // Index change
    }
    transactionsTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("UI: Inventory Transactions loaded successfully.");
}

void InventoryTransactionManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

} // namespace Warehouse
} // namespace UI
} // namespace ERP