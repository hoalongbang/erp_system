// main.cpp
#include <QApplication>
#include <QMessageBox>
#include <QDebug> // For qDebug, qInfo etc.

// Standard C++ headers (no direct Qt dependency where not needed)
#include <iostream>
#include <memory>
#include <stdexcept>

// --- Project Includes (Rút gọn) ---
// DAOs
#include "UserDAO.h"
#include "SessionDAO.h"
#include "RoleDAO.h"
#include "PermissionDAO.h"
#include "CategoryDAO.h"
#include "LocationDAO.h"
#include "UnitOfMeasureDAO.h"
#include "WarehouseDAO.h"
#include "ProductDAO.h"
#include "CustomerDAO.h"
#include "SupplierDAO.h"
#include "InventoryDAO.h"
#include "InventoryTransactionDAO.h"
#include "InventoryCostLayerDAO.h"
#include "ReceiptSlipDAO.h"
#include "IssueSlipDAO.h"
#include "MaterialRequestSlipDAO.h"
#include "MaterialIssueSlipDAO.h"
#include "SalesOrderDAO.h"
#include "InvoiceDAO.h"
#include "PaymentDAO.h"
#include "ShipmentDAO.h"
#include "QuotationDAO.h"
#include "ReturnDAO.h"
#include "GeneralLedgerDAO.h"
#include "AccountReceivableDAO.h"
#include "AccountReceivableTransactionDAO.h"
#include "TaxRateDAO.h"
#include "AuditLogDAO.h"
#include "ConfigDAO.h"
#include "DocumentDAO.h"
#include "DeviceConfigDAO.h"
#include "APIEndpointDAO.h"
#include "ProductionOrderDAO.h"
#include "BillOfMaterialDAO.h"
#include "ProductionLineDAO.h"
#include "MaintenanceManagementDAO.h"
#include "NotificationDAO.h"
#include "ReportDAO.h"
#include "ScheduledTaskDAO.h"
#include "TaskExecutionLogDAO.h"
#include "TaskLogDAO.h"

// Services Interfaces
#include "IAuthenticationService.h"
#include "IAuthorizationService.h"
#include "IAuditLogService.h"
#include "ISecurityManager.h"
#include "IUserService.h"
#include "ICategoryService.h"
#include "ILocationService.h"
#include "IUnitOfMeasureService.h"
#include "IWarehouseService.h"
#include "IRoleService.h"
#include "IPermissionService.h"
#include "IProductService.h"
#include "ICustomerService.h"
#include "ISupplierService.h"
#include "IInventoryManagementService.h"
#include "IPickingService.h"
#include "IStocktakeService.h"
#include "IInventoryTransactionService.h"
#include "IReceiptSlipService.h"
#include "IIssueSlipService.h"
#include "IMaterialRequestService.h"
#include "IMaterialIssueSlipService.h"
#include "ISalesOrderService.h"
#include "IInvoiceService.h"
#include "IPaymentService.h"
#include "IShipmentService.h"
#include "IQuotationService.h"
#include "IReturnService.h"
#include "IAccountReceivableService.h"
#include "IGeneralLedgerService.h"
#include "ITaxService.h"
#include "IDocumentService.h"
#include "IAssetManagementService.h"
#include "IConfigService.h"
#include "IDeviceManagerService.h"
#include "IExternalSystemService.h"
#include "INotificationService.h"
#include "IBillOfMaterialService.h"
#include "IMaintenanceManagementService.h"
#include "IProductionLineService.h"
#include "IProductionOrderService.h"
#include "IReportService.h"
#include "IScheduledTaskService.h"
#include "ITaskExecutionLogService.h"
#include "ITaskExecutorService.h"
#include "ISessionService.h"

// Services Implementations
#include "AuthenticationService.h"
#include "AuthorizationService.h"
#include "AuditLogService.h"
#include "SecurityManager.h"
#include "UserService.h"
#include "CategoryService.h"
#include "LocationService.h"
#include "UnitOfMeasureService.h"
#include "WarehouseService.h"
#include "RoleService.h"
#include "PermissionService.h"
#include "ProductService.h"
#include "CustomerService.h"
#include "SupplierService.h"
#include "InventoryManagementService.h"
#include "PickingService.h"
#include "StocktakeService.h"
#include "InventoryTransactionService.h"
#include "ReceiptSlipService.h"
#include "IssueSlipService.h"
#include "MaterialRequestService.h"
#include "MaterialIssueSlipService.h"
#include "SalesOrderService.h"
#include "SalesInvoiceService.h"
#include "SalesPaymentService.h"
#include "SalesShipmentService.h"
#include "SalesQuotationService.h"
#include "SalesReturnService.h"
#include "AccountReceivableService.h"
#include "GeneralLedgerService.h"
#include "TaxService.h"
#include "DocumentService.h"
#include "AssetManagementService.h"
#include "ConfigService.h"
#include "DeviceManagerService.h"
#include "ExternalSystemService.h"
#include "NotificationService.h"
#include "BillOfMaterialService.h"
#include "MaintenanceManagementService.h"
#include "ProductionLineService.h"
#include "ProductionOrderService.h"
#include "ReportService.h"
#include "ScheduledTaskService.h"
#include "TaskExecutionLogService.h"
#include "TaskEngine.h" // Singleton

// Utilities and Common
#include "Logger.h"
#include "ErrorHandler.h"
#include "DatabaseConfig.h"
#include "ConnectionPool.h"
#include "DatabaseInitializer.h"
#include "EventBus.h"
#include "PasswordHasher.h"
#include "EncryptionService.h"

// UI
#include "mainwindow.h"
#include "loginform.h"
#include "registerform.h" // Include register form

// UI Modules (Specific Widgets)
#include "CategoryManagementWidget.h"
#include "LocationManagementWidget.h"
#include "UnitOfMeasureManagementWidget.h"
#include "WarehouseManagementWidget.h"
#include "RoleManagementWidget.h"
#include "PermissionManagementWidget.h"
#include "ProductManagementWidget.h"
#include "CustomerManagementWidget.h"
#include "SupplierManagementWidget.h"
#include "SessionManagementWidget.h" // Security Module UI
#include "AuditLogViewerWidget.h" // Security Module UI
#include "InvoiceManagementWidget.h"
#include "PaymentManagementWidget.h"
#include "QuotationManagementWidget.h"
#include "SalesOrderManagementWidget.h"
#include "ShipmentManagementWidget.h"
#include "BillOfMaterialManagementWidget.h"
#include "MaintenanceManagementWidget.h"
#include "ProductionLineManagementWidget.h"
#include "ProductionOrderManagementWidget.h"
#include "ReceiptSlipManagementWidget.h"
#include "IssueSlipManagementWidget.h"
#include "MaterialRequestSlipManagementWidget.h"
#include "MaterialIssueSlipManagementWidget.h"
#include "InventoryManagementWidget.h"
#include "PickingRequestManagementWidget.h"
#include "StocktakeRequestManagementWidget.h"
#include "InventoryTransactionManagementWidget.h"
#include "AccountReceivableManagementWidget.h"
#include "GeneralLedgerManagementWidget.h"
#include "TaxRateManagementWidget.h"
#include "FinancialReportsWidget.h"
#include "NotificationManagementWidget.h"
#include "ReportManagementWidget.h"
#include "ScheduledTaskManagementWidget.h"
#include "TaskExecutionLogManagementWidget.h"
#include "DeviceManagementWidget.h"
#include "ExternalSystemManagementWidget.h"
#include "ReturnManagementWidget.h"


// DTO Headers (for explicit object creation, or if not all DTOs are included via DataObjects.h)
#include "User.h"
#include "Role.h"
#include "Permission.h"
#include "Return.h"


int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    // 1. Initialize Logger
    ERP::Logger::Logger::getInstance().setLogLevel(ERP::Logger::LogLevel::INFO);
    ERP::Logger::Logger::getInstance().info("Application started.");

    // 2. Database Configuration and Initialization
    ERP::Database::DTO::DatabaseConfig dbConfig;
    dbConfig.type = ERP::Database::DTO::DatabaseType::SQLite;
    dbConfig.database = "erp_manufacturing.db"; // SQLite database file

    try {
        ERP::Database::DatabaseInitializer dbInitializer(dbConfig);
        if (!dbInitializer.initializeDatabase()) {
            QMessageBox::critical(nullptr, "Lỗi Khởi Tạo Cơ Sở Dữ Liệu", "Không thể khởi tạo cơ sở dữ liệu. Vui lòng kiểm tra log.");
            ERP::Logger::Logger::getInstance().critical("main", "Database initialization failed. Exiting.");
            return 1;
        }
        // Initialize the ConnectionPool with the config
        ERP::Database::ConnectionPool::getInstance().initialize(dbConfig);
        ERP::Logger::Logger::getInstance().info("main", "Database connection pool initialized.");

    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Lỗi Cơ Sở Dữ Liệu", QString("Lỗi nghiêm trọng khi kết nối/khởi tạo cơ sở dữ liệu: %1").arg(e.what()));
        ERP::Logger::Logger::getInstance().critical("main", "Critical database error: " + std::string(e.what()));
        return 1;
    }

    // --- Service Layer Initialization (Order matters due to dependencies) ---
    // First, initialize DAOs
    auto userDAO = std::make_shared<ERP::User::DAOs::UserDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto sessionDAO = std::make_shared<ERP::Security::DAOs::SessionDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto roleDAO = std::make_shared<ERP::Catalog::DAOs::RoleDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto permissionDAO = std::make_shared<ERP::Catalog::DAOs::PermissionDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto categoryDAO = std::make_shared<ERP::Catalog::DAOs::CategoryDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto locationDAO = std::make_shared<ERP::Catalog::DAOs::LocationDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto unitOfMeasureDAO = std::make_shared<ERP::Catalog::DAOs::UnitOfMeasureDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto warehouseDAO = std::make_shared<ERP::Catalog::DAOs::WarehouseDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto productDAO = std::make_shared<ERP::Product::DAOs::ProductDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto productUnitConversionDAO = std::make_shared<ERP::Product::DAOs::ProductUnitConversionDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto customerDAO = std::make_shared<ERP::Customer::DAOs::CustomerDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto supplierDAO = std::make_shared<ERP::Supplier::DAOs::SupplierDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto inventoryDAO = std::make_shared<ERP::Warehouse::DAOs::InventoryDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto inventoryTransactionDAO = std::make_shared<ERP::Warehouse::DAOs::InventoryTransactionDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto inventoryCostLayerDAO = std::make_shared<ERP::Warehouse::DAOs::InventoryCostLayerDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto receiptSlipDAO = std::make_shared<ERP::Material::DAOs::ReceiptSlipDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto issueSlipDAO = std::make_shared<ERP::Material::DAOs::IssueSlipDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto materialRequestSlipDAO = std::make_shared<ERP::Material::DAOs::MaterialRequestSlipDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto materialIssueSlipDAO = std::make_shared<ERP::Material::DAOs::MaterialIssueSlipDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto salesOrderDAO = std::make_shared<ERP::Sales::DAOs::SalesOrderDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto invoiceDAO = std::make_shared<ERP::Sales::DAOs::InvoiceDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto paymentDAO = std::make_shared<ERP::Sales::DAOs::PaymentDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto shipmentDAO = std::make_shared<ERP::Sales::DAOs::ShipmentDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto quotationDAO = std::make_shared<ERP::Sales::DAOs::QuotationDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto returnDAO = std::make_shared<ERP::Sales::DAOs::ReturnDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto generalLedgerDAO = std::make_shared<ERP::Finance::DAOs::GeneralLedgerDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto accountReceivableDAO = std::make_shared<ERP::Finance::DAOs::AccountReceivableDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto arTransactionDAO = std::make_shared<ERP::Finance::DAOs::AccountReceivableTransactionDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto taxRateDAO = std::make_shared<ERP::Finance::DAOs::TaxRateDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto auditLogDAO = std::make_shared<ERP::Security::DAOs::AuditLogDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto configDAO = std::make_shared<ERP::Config::DAOs::ConfigDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto documentDAO = std::make_shared<ERP::Document::DAOs::Document::DocumentDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto deviceConfigDAO = std::make_shared<ERP::Integration::DAOs::DeviceConfigDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto apiEndpointDAO = std::make_shared<ERP::Integration::DAOs::APIEndpointDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto productionOrderDAO = std::make_shared<ERP::Manufacturing::DAOs::ProductionOrderDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto billOfMaterialDAO = std::make_shared<ERP::Manufacturing::DAOs::BillOfMaterialDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto productionLineDAO = std::make_shared<ERP::Manufacturing::DAOs::ProductionLineDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto maintenanceManagementDAO = std::make_shared<ERP::Manufacturing::DAOs::MaintenanceManagementDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto notificationDAO = std::make_shared<ERP::Notification::DAOs::NotificationDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto reportDAO = std::make_shared<ERP::Report::DAOs::ReportDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto scheduledTaskDAO = std::make_shared<ERP::Scheduler::DAOs::ScheduledTaskDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto taskExecutionLogDAO = std::make_shared<ERP::Scheduler::DAOs::TaskExecutionLogDAO>(ERP::Database::ConnectionPool::getInstancePtr());
    auto taskLogDAO = std::make_shared<ERP::TaskEngine::DAOs::TaskLogDAO>(ERP::Database::ConnectionPool::getInstancePtr());

    // Core Services / Singletons (should be initialized first as they are fundamental)
    auto auditLogService = std::make_shared<ERP::Security::Service::AuditLogService>(auditLogDAO, ERP::Database::ConnectionPool::getInstancePtr());
    auto authorizationService = std::make_shared<ERP::Security::Service::AuthorizationService>(roleDAO, permissionDAO, userDAO, ERP::Database::ConnectionPool::getInstancePtr());
    auto authenticationService = std::make_shared<ERP::Security::Service::AuthenticationService>(userDAO, sessionDAO, auditLogService, ERP::Database::ConnectionPool::getInstancePtr());
    auto taskExecutorService = std::make_shared<ERP::TaskEngine::TaskEngine>(); // TaskEngine is a singleton, get instance.

    // Temp SecurityManager for initial service injection (will be fully populated later)
    // This is a common pattern to break circular dependencies during initial setup.
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager; 
    
    // Services (grouped by module and dependency order)
    // ERP_User_Service
    auto userService = std::make_shared<ERP::User::Services::UserService>(userDAO, nullptr, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager); // RoleService depends on SecurityManager and PermissionService. So, passing nullptr for now for roleService.
    
    // ERP_Catalog_Services (depend on User, Security)
    auto roleService_impl = std::make_shared<ERP::Catalog::Services::RoleService>(roleDAO, nullptr, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager); // PermissionService depends on SecurityManager, so passing nullptr.
    auto permissionService_impl = std::make_shared<ERP::Catalog::Services::PermissionService>(permissionDAO, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    
    // Update roleService with actual permissionService (breaking potential circular if roleService needed permService directly for its init)
    roleService_impl->permissionService_ = permissionService_impl; // Access private member for setup, or use setter if public setter exists.

    auto unitOfMeasureService = std::make_shared<ERP::Catalog::Services::UnitOfMeasureService>(unitOfMeasureDAO, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto warehouseService = std::make_shared<ERP::Catalog::Services::WarehouseService>(warehouseDAO, nullptr, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager); // LocationService is a dependency, assign later
    auto locationService = std::make_shared<ERP::Catalog::Services::LocationService>(locationDAO, warehouseService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager); // WarehouseService is a dependency
    // Update warehouseService's locationService if needed (via setter, or passed in its constructor if circular)
    
    auto productService = std::make_shared<ERP::Product::Services::ProductService>(productDAO, nullptr, unitOfMeasureService, productUnitConversionDAO, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    // Update productService with actual categoryService (breaking potential circular if productService needed categoryService directly for its init)
    auto categoryService = std::make_shared<ERP::Catalog::Services::CategoryService>(categoryDAO, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    productService->categoryService_ = categoryService; // Access private member for setup.

    // ERP_Customer_Services (depend on User)
    auto customerService = std::make_shared<ERP::Customer::Services::CustomerService>(customerDAO, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    
    // ERP_Supplier_Services (depend on User)
    auto supplierService = std::make_shared<ERP::Supplier::Services::SupplierService>(supplierDAO, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);

    // ERP_Config_Services (depend on Security)
    auto configService = std::make_shared<ERP::Config::Services::ConfigService>(configDAO, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);

    // ERP_Document_Services
    auto documentService = std::make_shared<ERP::Document::Services::DocumentService>(documentDAO, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    
    // ERP_Integration_Services (depend on Security, Catalog)
    auto deviceManagerService = std::make_shared<ERP::Integration::Services::DeviceManagerService>(deviceConfigDAO, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto externalSystemService = std::make_shared<ERP::Integration::Services::ExternalSystemService>(apiEndpointDAO, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    
    // ERP_Manufacturing_Services (depend on Product, Catalog, Asset, Security)
    auto billOfMaterialService = std::make_shared<ERP::Manufacturing::Services::IBillOfMaterialService>(billOfMaterialDAO, productService, unitOfMeasureService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto productionLineService = std::make_shared<ERP::Manufacturing::Services::IProductionLineService>(productionLineDAO, locationService, nullptr, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager); // AssetManagementService dependency, assign later
    auto productionOrderService = std::make_shared<ERP::Manufacturing::Services::IProductionOrderService>(productionOrderDAO, productService, billOfMaterialService, productionLineService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto maintenanceManagementService = std::make_shared<ERP::Manufacturing::Services::IMaintenanceManagementService>(maintenanceManagementDAO, nullptr, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager); // AssetManagementService dependency, assign later

    // ERP_Warehouse_Services (depend on Product, Catalog, Sales, Security)
    auto inventoryTransactionService = std::make_shared<ERP::Warehouse::Services::InventoryTransactionService>(inventoryTransactionDAO, productService, warehouseService, locationService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto inventoryManagementService = std::make_shared<ERP::Warehouse::Services::IInventoryManagementService>(inventoryDAO, inventoryCostLayerDAO, productService, warehouseService, locationService, inventoryTransactionService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto pickingService = std::make_shared<ERP::Warehouse::Services::IPickingService>(pickingRequestDAO, pickingDetailDAO, salesOrderService, customerService, warehouseService, productService, inventoryManagementService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto stocktakeService = std::make_shared<ERP::Warehouse::Services::IStocktakeService>(stocktakeRequestDAO, stocktakeDetailDAO, inventoryManagementService, warehouseService, productService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    
    // ERP_Material_Services (depend on Product, Catalog, Warehouse, Manufacturing, Security)
    auto receiptSlipService = std::make_shared<ERP::Material::Services::IReceiptSlipService>(receiptSlipDAO, productService, warehouseService, inventoryManagementService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto materialRequestService = std::make_shared<ERP::Material::Services::IMaterialRequestService>(materialRequestSlipDAO, productService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto issueSlipService = std::make_shared<ERP::Material::Services::IIssueSlipService>(issueSlipDAO, productService, warehouseService, inventoryManagementService, materialRequestService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto materialIssueSlipService = std::make_shared<ERP::Material::Services::IMaterialIssueSlipService>(materialIssueSlipDAO, productionOrderService, productService, warehouseService, inventoryManagementService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);

    // ERP_Sales_Services (depend on Customer, Product, Catalog, Finance, Security)
    auto salesOrderService = std::make_shared<ERP::Sales::Services::ISalesOrderService>(salesOrderDAO, customerService, warehouseService, productService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto salesInvoiceService = std::make_shared<ERP::Sales::Services::IInvoiceService>(invoiceDAO, salesOrderService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto salesPaymentService = std::make_shared<ERP::Sales::Services::IPaymentService>(paymentDAO, customerService, salesInvoiceService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto salesQuotationService = std::make_shared<ERP::Sales::Services::IQuotationService>(quotationDAO, customerService, productService, unitOfMeasureService, salesOrderService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto salesShipmentService = std::make_shared<ERP::Sales::Services::IShipmentService>(shipmentDAO, salesOrderService, customerService, warehouseService, productService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto salesReturnService = std::make_shared<ERP::Sales::Services::SalesReturnService>(returnDAO, salesOrderService, customerService, warehouseService, productService, inventoryManagementService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);

    // ERP_Finance_Services (depend on Customer, Sales, Security)
    auto accountReceivableService = std::make_shared<ERP::Finance::Services::IAccountReceivableService>(accountReceivableDAO, arTransactionDAO, customerService, salesInvoiceService, salesPaymentService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto generalLedgerService = std::make_shared<ERP::Finance::Services::IGeneralLedgerService>(glDAO, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto taxService = std::make_shared<ERP::Finance::Services::ITaxService>(taxRateDAO, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);

    // ERP_Scheduler_Services (depend on Security)
    auto scheduledTaskService = std::make_shared<ERP::Scheduler::Services::IScheduledTaskService>(scheduledTaskDAO, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    auto taskExecutionLogService = std::make_shared<ERP::Scheduler::Services::ITaskExecutionLogService>(taskExecutionLogDAO, scheduledTaskService, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);

    // ERP_Notification_Services (depend on User, Security)
    auto notificationService = std::make_shared<ERP::Notification::Services::INotificationService>(notificationDAO, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);
    
    // ERP_Report_Services (depend on Security)
    auto reportService = std::make_shared<ERP::Report::Services::IReportService>(reportDAO, authorizationService, auditLogService, ERP::Database::ConnectionPool::getInstancePtr(), securityManager);


    // --- Final SecurityManager population with all initialized services ---
    // This is crucial. All services that were passed nullptr initially must now be passed their concrete instances.
    securityManager = std::make_shared<ERP::Security::SecurityManager>(
        authenticationService, authorizationService, auditLogService,
        userService,
        categoryService, // Fixed
        locationService,
        warehouseService,
        unitOfMeasureService,
        roleService_impl, // Concrete impl
        permissionService_impl, // Concrete impl
        assetManagementService,
        configService,
        customerService,
        documentService,
        accountReceivableService,
        generalLedgerService,
        taxService,
        deviceManagerService,
        externalSystemService,
        billOfMaterialService,
        maintenanceManagementService,
        productionLineService,
        productionOrderService,
        issueSlipService,
        materialIssueSlipService,
        materialRequestService,
        receiptSlipService,
        notificationService,
        productService,
        reportService,
        salesInvoiceService,
        salesPaymentService,
        salesQuotationService,
        salesOrderService,
        salesShipmentService,
        salesReturnService,
        scheduledTaskService,
        taskExecutionLogService,
        supplierService,
        taskExecutorService,
        inventoryManagementService,
        pickingService,
        stocktakeService
    );
    
    // --- Final pass to fix any remaining circular dependencies with setters ---
    // If any service's constructor *still* had to take a nullptr because its dependency was later in this main.cpp,
    // and that dependency has now been initialized, set it here.
    // Example: If a service `X` needs `Y`, and `Y` needs `X`. One of them must be initialized with `nullptr` for the other,
    // then set later using a setter.
    
    // Setters for services that initially had nullptr dependencies
    // userService->setRoleService(roleService_impl); // If userService's constructor didn't take it directly. (Already handled)
    warehouseService->setLocationService(locationService); // NEW: Explicitly set LocationService for WarehouseService
    productService->categoryService_ = categoryService; // Explicitly set CategoryService for ProductService
    productionLineService->assetManagementService_ = nullptr; // TODO: Replace nullptr with actual AssetManagementService instance if available
    maintenanceManagementService->assetManagementService_ = nullptr; // TODO: Replace nullptr with actual AssetManagementService instance if available
    pickingService->salesOrderService_ = salesOrderService; // Explicitly set for PickingService
    stocktakeService->inventoryManagementService_ = inventoryManagementService; // Explicitly set for StocktakeService
    stocktakeService->productService_ = productService; // Explicitly set for StocktakeService
    // issueSlipService->setMaterialRequestService(materialRequestService); // (Already handled)
    // materialIssueSlipService->setMaterialRequestSlipService(materialRequestService); // Add this setter if needed
    // materialIssueSlipService->setProductService(productService); // Add this setter if needed
    // materialIssueSlipService->setWarehouseService(warehouseService); // Add this setter if needed
    // materialIssueSlipService->setInventoryManagementService(inventoryManagementService); // Add this setter if needed


    ERP::TaskEngine::TaskEngine::getInstance().start();
    ERP::Logger::Logger::getInstance().info("main", "TaskEngine stopped.");

    // --- UI Initialization ---
    // Create UI widgets, passing necessary service dependencies
    // Order matters here: dependencies between widgets, and passing already initialized services.
    MainWindow w(nullptr, securityManager);

    // Catalog Module UI
    w.loadModuleWidget("Categories", new ERP::UI::Catalog::CategoryManagementWidget(w.centralWidget(), securityManager->getCategoryService(), securityManager));
    w.loadModuleWidget("Locations", new ERP::UI::Catalog::LocationManagementWidget(w.centralWidget(), securityManager->getLocationService(), securityManager->getWarehouseService(), securityManager));
    w.loadModuleWidget("UnitsOfMeasure", new ERP::UI::Catalog::UnitOfMeasureManagementWidget(w.centralWidget(), securityManager->getUnitOfMeasureService(), securityManager));
    w.loadModuleWidget("Warehouses", new ERP::UI::Catalog::WarehouseManagementWidget(w.centralWidget(), securityManager->getWarehouseService(), securityManager->getLocationService(), securityManager));
    w.loadModuleWidget("Roles", new ERP::UI::Catalog::RoleManagementWidget(w.centralWidget(), securityManager->getRoleService(), securityManager->getPermissionService(), securityManager));
    w.loadModuleWidget("Permissions", new ERP::UI::Catalog::PermissionManagementWidget(w.centralWidget(), securityManager->getPermissionService(), securityManager));

    // Product Module UI
    w.loadModuleWidget("Products", new ERP::UI::Product::ProductManagementWidget(w.centralWidget(), securityManager->getProductService(), securityManager->getCategoryService(), securityManager->getUnitOfMeasureService(), securityManager));

    // Customer Module UI
    w.loadModuleWidget("Customers", new ERP::UI::Customer::CustomerManagementWidget(w.centralWidget(), securityManager->getCustomerService(), securityManager));

    // Supplier Module UI
    w.loadModuleWidget("Suppliers", new ERP::UI::Supplier::SupplierManagementWidget(w.centralWidget(), securityManager->getSupplierService(), securityManager));

    // User Module UI
    w.loadModuleWidget("Users", new ERP::UI::User::UserManagementWidget(w.centralWidget(), securityManager->getUserService(), securityManager->getRoleService(), securityManager));

    // Sales Module UI
    w.loadModuleWidget("SalesOrders", new ERP::UI::Sales::SalesOrderManagementWidget(w.centralWidget(), securityManager->getSalesOrderService(), securityManager->getCustomerService(), securityManager->getWarehouseService(), securityManager->getProductService(), securityManager));
    w.loadModuleWidget("Invoices", new ERP::UI::Sales::InvoiceManagementWidget(w.centralWidget(), securityManager->getInvoiceService(), securityManager->getSalesOrderService(), securityManager));
    w.loadModuleWidget("Payments", new ERP::UI::Sales::PaymentManagementWidget(w.centralWidget(), securityManager->getPaymentService(), securityManager->getCustomerService(), securityManager->getInvoiceService(), securityManager));
    w.loadModuleWidget("Quotations", new ERP::UI::Sales::QuotationManagementWidget(w.centralWidget(), securityManager->getQuotationService(), securityManager->getCustomerService(), securityManager->getProductService(), securityManager->getUnitOfMeasureService(), securityManager->getSalesOrderService(), securityManager));
    w.loadModuleWidget("Shipments", new ERP::UI::Sales::ShipmentManagementWidget(w.centralWidget(), securityManager->getShipmentService(), securityManager->getSalesOrderService(), securityManager->getCustomerService(), securityManager->getProductService(), securityManager->getWarehouseService(), securityManager));
    w.loadModuleWidget("Returns", new ERP::UI::Sales::ReturnManagementWidget(w.centralWidget(), securityManager->getReturnService(), securityManager->getSalesOrderService(), securityManager->getCustomerService(), securityManager->getWarehouseService(), securityManager->getProductService(), securityManager->getInventoryManagementService(), securityManager));

    // Manufacturing Module UI
    w.loadModuleWidget("BillOfMaterials", new ERP::UI::Manufacturing::BillOfMaterialManagementWidget(w.centralWidget(), securityManager->getBillOfMaterialService(), securityManager->getProductService(), securityManager->getUnitOfMeasureService(), securityManager));
    w.loadModuleWidget("Maintenance", new ERP::UI::Manufacturing::MaintenanceManagementWidget(w.centralWidget(), securityManager->getMaintenanceManagementService(), securityManager->getAssetManagementService(), securityManager));
    w.loadModuleWidget("ProductionLines", new ERP::UI::Manufacturing::ProductionLineManagementWidget(w.centralWidget(), securityManager->getProductionLineService(), securityManager->getLocationService(), securityManager->getAssetManagementService(), securityManager));
    w.loadModuleWidget("ProductionOrders", new ERP::UI::Manufacturing::ProductionOrderManagementWidget(w.centralWidget(), securityManager->getProductionOrderService(), securityManager->getProductService(), securityManager->getBillOfMaterialService(), securityManager->getProductionLineService(), securityManager));

    // Material Module UI
    w.loadModuleWidget("ReceiptSlips", new ERP::UI::Material::ReceiptSlipManagementWidget(w.centralWidget(), securityManager->getReceiptSlipService(), securityManager->getProductService(), securityManager->getWarehouseService(), securityManager->getInventoryManagementService(), securityManager));
    w.loadModuleWidget("IssueSlips", new ERP::UI::Material::IssueSlipManagementWidget(w.centralWidget(), securityManager->getIssueSlipService(), securityManager->getProductService(), securityManager->getWarehouseService(), securityManager->getInventoryManagementService(), securityManager->getMaterialRequestService(), securityManager));
    w.loadModuleWidget("MaterialRequests", new ERP::UI::Material::MaterialRequestSlipManagementWidget(w.centralWidget(), securityManager->getMaterialRequestService(), securityManager->getProductService(), securityManager));
    w.loadModuleWidget("MaterialIssueSlips", new ERP::UI::Material::MaterialIssueSlipManagementWidget(w.centralWidget(), securityManager->getMaterialIssueSlipService(), securityManager->getProductionOrderService(), securityManager->getProductService(), securityManager->getWarehouseService(), securityManager->getInventoryManagementService(), securityManager));

    // Warehouse Module UI
    w.loadModuleWidget("Inventory", new ERP::UI::Warehouse::InventoryManagementWidget(w.centralWidget(), securityManager->getInventoryManagementService(), securityManager->getProductService(), securityManager->getWarehouseService(), securityManager));
    w.loadModuleWidget("PickingRequests", new ERP::UI::Warehouse::PickingRequestManagementWidget(w.centralWidget(), securityManager->getPickingService(), securityManager->getSalesOrderService(), securityManager->getInventoryManagementService(), securityManager));
    w.loadModuleWidget("StocktakeRequests", new ERP::UI::Warehouse::StocktakeRequestManagementWidget(w.centralWidget(), securityManager->getStocktakeService(), securityManager->getInventoryManagementService(), securityManager->getWarehouseService(), securityManager));
    w.loadModuleWidget("InventoryTransactions", new ERP::UI::Warehouse::InventoryTransactionManagementWidget(w.centralWidget(), securityManager->getInventoryTransactionService(), securityManager->getProductService(), securityManager->getWarehouseService(), securityManager->getLocationService(), securityManager));

    // Finance Module UI
    w.loadModuleWidget("AccountReceivable", new ERP::UI::Finance::AccountReceivableManagementWidget(w.centralWidget(), securityManager->getAccountReceivableService(), securityManager->getCustomerService(), securityManager->getInvoiceService(), securityManager->getPaymentService(), securityManager));
    w.loadModuleWidget("GeneralLedger", new ERP::UI::Finance::GeneralLedgerManagementWidget(w.centralWidget(), securityManager->getGeneralLedgerService(), securityManager));
    w.loadModuleWidget("TaxRates", new ERP::UI::Finance::TaxRateManagementWidget(w.centralWidget(), securityManager->getTaxService(), securityManager));
    w.loadModuleWidget("FinancialReports", new ERP::UI::Finance::FinancialReportsWidget(w.centralWidget(), securityManager->getGeneralLedgerService(), securityManager));

    // Integration Module UI
    w.loadModuleWidget("DeviceManagement", new ERP::UI::Integration::DeviceManagementWidget(w.centralWidget(), securityManager->getDeviceManagerService(), securityManager));
    w.loadModuleWidget("ExternalSystems", new ERP::UI::Integration::ExternalSystemManagementWidget(w.centralWidget(), securityManager->getExternalSystemService(), securityManager));

    // Notification Module UI
    w.loadModuleWidget("Notifications", new ERP::UI::Notification::NotificationManagementWidget(w.centralWidget(), securityManager->getNotificationService(), securityManager));

    // Report Module UI
    w.loadModuleWidget("Reports", new ERP::UI::Report::ReportManagementWidget(w.centralWidget(), securityManager->getReportService(), securityManager));

    // Scheduler Module UI
    w.loadModuleWidget("ScheduledTasks", new ERP::UI::Scheduler::ScheduledTaskManagementWidget(w.centralWidget(), securityManager->getScheduledTaskService(), securityManager));
    w.loadModuleWidget("TaskExecutionLogs", new ERP::UI::Scheduler::TaskExecutionLogManagementWidget(w.centralWidget(), securityManager->getTaskExecutionLogService(), securityManager->getScheduledTaskService(), securityManager));

    // Security Module UI (viewers/managers for audit logs, sessions, etc.)
    w.loadModuleWidget("AuditLogs", new ERP::UI::Security::AuditLogViewerWidget(w.centralWidget(), securityManager->getAuditLogService(), securityManager));
    w.loadModuleWidget("Sessions", new ERP::UI::Security::SessionManagementWidget(w.centralWidget(), securityManager->getSessionService(), securityManager));
    

    w.show();

    int execResult = a.exec();

    // --- Cleanup ---
    ERP::TaskEngine::TaskEngine::getInstance().stop();
    ERP::Logger::Logger::getInstance().info("main", "TaskEngine stopped.");
    ERP::Database::ConnectionPool::getInstance().shutdown();
    ERP::Logger::Logger::getInstance().info("main", "Database connection pool shut down.");
    ERP::Logger::Logger::getInstance().info("Application exited.");

    return execResult;
}