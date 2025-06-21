// Modules/Database/DatabaseInitializer.cpp
#include "DatabaseInitializer.h"
#include "Common.h"      // Standard includes
#include "Logger.h"      // Standard includes
#include "ErrorHandler.h"  // Standard includes
#include "DateUtils.h"   // Standard includes
#include "Utils.h"       // Standard includes

#include <sqlite3.h>     // For SQLite specific operations (if using raw sqlite3_exec)
#include <iostream>      // For std::cout/cerr
#include <fstream>       // For std::ifstream
#include <stdexcept>     // For std::runtime_error
#include <algorithm>     // For std::transform

namespace ERP {
namespace Database {

DatabaseInitializer::DatabaseInitializer(const DTO::DatabaseConfig& config)
    : config_(config) {
    ERP::Logger::Logger::getInstance().info("DatabaseInitializer: Initializing with database: " + config_.database);
    try {
        // Direct connection for schema initialization (not from pool yet)
        if (config_.type == DTO::DatabaseType::SQLite) {
            dbConnection_ = std::make_shared<SQLiteConnection>(config_.database);
        } else {
            ERP::Logger::Logger::getInstance().critical("DatabaseInitializer", "Unsupported database type for direct initialization.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Unsupported database type for initialization.", "Kiểu cơ sở dữ liệu không được hỗ trợ.");
            throw std::runtime_error("Unsupported database type.");
        }

        if (!dbConnection_->open()) {
            ERP::Logger::Logger::getInstance().critical("DatabaseInitializer", "Failed to open database connection for initialization.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to open database connection.", "Không thể mở kết nối cơ sở dữ liệu.");
            throw std::runtime_error("Failed to open database connection for initialization.");
        }
        ERP::Logger::Logger::getInstance().info("DatabaseInitializer: Direct database connection opened successfully.");

        // Initialize DAOs directly using this connection for schema/initial data
        userDAO_ = std::make_shared<User::DAOs::UserDAO>(dbConnection_);
        roleDAO_ = std::make_shared<Catalog::DAOs::RoleDAO>(dbConnection_);
        permissionDAO_ = std::make_shared<Catalog::DAOs::PermissionDAO>(dbConnection_);
        userRoleDAO_ = std::make_shared<Security::DAOs::UserRoleDAO>(dbConnection_);
        categoryDAO_ = std::make_shared<Catalog::DAOs::CategoryDAO>(dbConnection_);
        warehouseDAO_ = std::make_shared<Catalog::DAOs::WarehouseDAO>(dbConnection_);
        locationDAO_ = std::make_shared<Catalog::DAOs::LocationDAO>(dbConnection_);
        unitOfMeasureDAO_ = std::make_shared<Catalog::DAOs::UnitOfMeasureDAO>(dbConnection_);
        productDAO_ = std::make_shared<Product::DAOs::ProductDAO>(dbConnection_);
        productUnitConversionDAO_ = std::make_shared<Product::DAOs::ProductUnitConversionDAO>(dbConnection_);
        customerDAO_ = std::make_shared<Customer::DAOs::CustomerDAO>(dbConnection_);
        supplierDAO_ = std::make_shared<Supplier::DAOs::SupplierDAO>(dbConnection_);
        inventoryDAO_ = std::make_shared<Warehouse::DAOs::InventoryDAO>(dbConnection_);
        glDAO_ = std::make_shared<Finance::DAOs::GeneralLedgerDAO>(dbConnection_);
        taxRateDAO_ = std::make_shared<Finance::DAOs::TaxRateDAO>(dbConnection_);
        arTransactionDAO_ = std::make_shared<Finance::DAOs::AccountReceivableTransactionDAO>(dbConnection_); // NEW: Initialize AR Transaction DAO
        configDAO_ = std::make_shared<Config::DAOs::ConfigDAO>(dbConnection_);
        deviceConfigDAO_ = std::make_shared<Integration::DAOs::DeviceConfigDAO>(dbConnection_);
        apiEndpointDAO_ = std::make_shared<Integration::DAOs::APIEndpointDAO>(dbConnection_);
        integrationConfigDAO_ = std::make_shared<Integration::DAOs::IntegrationConfigDAO>(dbConnection_);
        bomDAO_ = std::make_shared<Manufacturing::DAOs::BillOfMaterialDAO>(dbConnection_);
        productionLineDAO_ = std::make_shared<Manufacturing::DAOs::ProductionLineDAO>(dbConnection_);
        returnDAO_ = std::make_shared<Sales::DAOs::ReturnDAO>(dbConnection_);
        pickingRequestDAO_ = std::make_shared<Warehouse::DAOs::PickingRequestDAO>(dbConnection_);
        pickingDetailDAO_ = std::make_shared<Warehouse::DAOs::PickingDetailDAO>(dbConnection_);

    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().critical("DatabaseInitializer", "Initialization failed: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "DatabaseInitializer: Initialization failed.", "Khởi tạo cơ sở dữ liệu thất bại.");
        throw;
    }
}

bool DatabaseInitializer::initializeDatabase() {
    if (!initializeSchema()) {
        ERP::Logger::Logger::getInstance().error("DatabaseInitializer: Schema initialization failed.");
        return false;
    }
    if (!populateInitialData()) {
        ERP::Logger::Logger::getInstance().warning("DatabaseInitializer: Initial data population failed or skipped.");
        // This might not be a critical error, depending on if data is truly optional.
    }
    return true;
}

bool DatabaseInitializer::initializeSchema() {
    ERP::Logger::Logger::getInstance().info("DatabaseInitializer: Initializing database schema...");
    dbConnection_->beginTransaction();

    bool success =
        createUsersTable() &&
        createUserProfilesTable() &&
        createSessionsTable() &&
        createRolesTable() &&
        createPermissionsTable() &&
        createRolePermissionsTable() &&
        createUserRolesTable() &&
        createCategoriesTable() &&
        createWarehousesTable() &&
        createLocationsTable() &&
        createUnitOfMeasuresTable() &&
        createProductsTable() &&
        createProductUnitConversionsTable() &&
        createCustomersTable() &&
        createSuppliersTable() &&
        createInventoryTable() &&
        createInventoryTransactionsTable() &&
        createInventoryCostLayersTable() &&
        createPickingRequestsTable() &&
        createPickingDetailsTable() &&
        createStocktakeRequestsTable() &&
        createStocktakeDetailsTable() &&
        createReceiptSlipsTable() &&
        createReceiptSlipDetailsTable() &&
        createIssueSlipsTable() &&
        createIssueSlipDetailsTable() &&
        createMaterialRequestSlipsTable() &&
        createMaterialRequestSlipDetailsTable() &&
        createMaterialIssueSlipsTable() &&
        createMaterialIssueSlipDetailsTable() &&
        createSalesOrdersTable() &&
        createSalesOrderDetailsTable() &&
        createInvoicesTable() &&
        createInvoiceDetailsTable() &&
        createPaymentsTable() &&
        createQuotationsTable() &&
        createQuotationDetailsTable() &&
        createShipmentsTable() &&
        createShipmentDetailsTable() &&
        createReturnsTable() &&
        createReturnDetailsTable() &&
        createGeneralLedgerAccountsTable() &&
        createGLAccountBalancesTable() &&
        createJournalEntriesTable() &&
        createJournalEntryDetailsTable() &&
        createTaxRatesTable() &&
        createAccountReceivableTransactionsTable() && // NEW: Call new table creation function
        createAuditLogsTable() &&
        createConfigurationsTable() &&
        createDocumentsTable() &&
        createDeviceConfigsTable() &&
        createDeviceEventLogsTable() &&
        createAPIEndpointsTable() &&
        createIntegrationConfigsTable() &&
        createProductionOrdersTable() &&
        createBillOfMaterialsTable() &&
        createBillOfMaterialItemsTable() &&
        createProductionLinesTable() &&
        createMaintenanceRequestsTable() &&
        createMaintenanceActivitiesTable() &&
        createNotificationsTable() &&
        createReportRequestsTable() &&
        createReportExecutionLogsTable() &&
        createScheduledTasksTable() &&
        createTaskExecutionLogsTable() &&
        createTaskLogsTable();

    if (success) {
        dbConnection_->commitTransaction();
        ERP::Logger::Logger::getInstance().info("DatabaseInitializer: Schema initialization complete.");
    } else {
        dbConnection_->rollbackTransaction();
        ERP::Logger::Logger::getInstance().error("DatabaseInitializer: Schema initialization failed, transaction rolled back.");
    }
    return success;
}

bool DatabaseInitializer::populateInitialData() {
    ERP::Logger::Logger::getInstance().info("DatabaseInitializer: Populating initial data...");

    // Check if data already exists to prevent re-population
    if (userDAO_->count({}) > 0) {
        ERP::Logger::Logger::getInstance().info("DatabaseInitializer: Initial data already exists. Skipping population.");
        return true;
    }

    dbConnection_->beginTransaction();
    bool success = true;

    // --- Users and Roles ---
    ERP::Catalog::DTO::RoleDTO adminRole, userRole;
    adminRole.name = "Admin"; adminRole.description = "Administrator role with full access.";
    adminRole.id = ERP::Utils::generateUUID();
    userRole.name = "User"; userRole.description = "Standard user role with limited access.";
    userRole.id = ERP::Utils::generateUUID();

    success = roleDAO_->create(adminRole) && roleDAO_->create(userRole);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create roles."); dbConnection_->rollbackTransaction(); return false; }

    // --- Permissions ---
    // Minimal set for core functionality and admin access
    std::vector<ERP::Catalog::DTO::PermissionDTO> permissions = {
        {"User.CreateUser", "User", "CreateUser", "Allows creating new user accounts."},
        {"User.ViewUsers", "User", "ViewUsers", "Allows viewing user accounts."},
        {"User.UpdateUser", "User", "UpdateUser", "Allows updating user accounts."},
        {"User.DeleteUser", "User", "DeleteUser", "Allows deleting user accounts."},
        {"User.ChangeAnyPassword", "User", "ChangeAnyPassword", "Allows changing any user's password."},
        {"User.ManageRoles", "User", "ManageRoles", "Allows managing user roles."},
        {"Catalog.CreateCategory", "Catalog", "CreateCategory", "Allows creating product categories."},
        {"Catalog.ViewCategories", "Catalog", "ViewCategories", "Allows viewing product categories."},
        {"Catalog.UpdateCategory", "Catalog", "UpdateCategory", "Allows updating product categories."},
        {"Catalog.DeleteCategory", "Catalog", "DeleteCategory", "Allows deleting product categories."},
        {"Catalog.ViewWarehouses", "Catalog", "ViewWarehouses", "Allows viewing warehouses."},
        {"Catalog.CreateWarehouse", "Catalog", "CreateWarehouse", "Allows creating warehouses."},
        {"Catalog.UpdateWarehouse", "Catalog", "UpdateWarehouse", "Allows updating warehouses."},
        {"Catalog.DeleteWarehouse", "Catalog", "DeleteWarehouse", "Allows deleting warehouses."},
        {"Catalog.ViewLocations", "Catalog", "ViewLocations", "Allows viewing warehouse locations."},
        {"Catalog.CreateLocation", "Catalog", "CreateLocation", "Allows creating warehouse locations."},
        {"Catalog.UpdateLocation", "Catalog", "UpdateLocation", "Allows updating warehouse locations."},
        {"Catalog.DeleteLocation", "Catalog", "DeleteLocation", "Allows deleting warehouse locations."},
        {"Catalog.ViewUnitsOfMeasure", "Catalog", "ViewUnitsOfMeasure", "Allows viewing units of measure."},
        {"Catalog.CreateUnitOfMeasure", "Catalog", "CreateUnitOfMeasure", "Allows creating units of measure."},
        {"Catalog.UpdateUnitOfMeasure", "Catalog", "UpdateUnitOfMeasure", "Allows updating units of measure."},
        {"Catalog.DeleteUnitOfMeasure", "Catalog", "DeleteUnitOfMeasure", "Allows deleting units of measure."},
        {"Catalog.ViewRoles", "Catalog", "ViewRoles", "Allows viewing roles."},
        {"Catalog.CreateRole", "Catalog", "CreateRole", "Allows creating roles."},
        {"Catalog.UpdateRole", "Catalog", "UpdateRole", "Allows updating roles."},
        {"Catalog.DeleteRole", "Catalog", "DeleteRole", "Allows deleting roles."},
        {"Catalog.ManageRolePermissions", "Catalog", "ManageRolePermissions", "Allows assigning/removing permissions to/from roles."},
        {"Catalog.ViewPermissions", "Catalog", "ViewPermissions", "Allows viewing permissions."},
        {"Product.CreateProduct", "Product", "CreateProduct", "Allows creating products."},
        {"Product.ViewProducts", "Product", "ViewProducts", "Allows viewing products."},
        {"Product.UpdateProduct", "Product", "UpdateProduct", "Allows updating products."},
        {"Product.DeleteProduct", "Product", "DeleteProduct", "Allows deleting products."},
        {"Product.CreateProductUnitConversion", "Product", "CreateProductUnitConversion", "Allows creating product unit conversion rules."},
        {"Product.ViewProductUnitConversion", "Product", "ViewProductUnitConversion", "Allows viewing product unit conversion rules."},
        {"Product.UpdateProductUnitConversion", "Product", "UpdateProductUnitConversion", "Allows updating product unit conversion rules."},
        {"Product.DeleteProductUnitConversion", "Product", "DeleteProductUnitConversion", "Allows deleting product unit conversion rules."},
        {"Customer.CreateCustomer", "Customer", "CreateCustomer", "Allows creating customers."},
        {"Customer.ViewCustomers", "Customer", "ViewCustomers", "Allows viewing customers."},
        {"Customer.UpdateCustomer", "Customer", "UpdateCustomer", "Allows updating customers."},
        {"Customer.DeleteCustomer", "Customer", "DeleteCustomer", "Allows deleting customers."},
        {"Supplier.CreateSupplier", "Supplier", "CreateSupplier", "Allows creating suppliers."},
        {"Supplier.ViewSuppliers", "Supplier", "ViewSuppliers", "Allows viewing suppliers."},
        {"Supplier.UpdateSupplier", "Supplier", "UpdateSupplier", "Allows updating suppliers."},
        {"Supplier.DeleteSupplier", "Supplier", "DeleteSupplier", "Allows deleting suppliers."},
        {"Warehouse.RecordGoodsReceipt", "Warehouse", "RecordGoodsReceipt", "Allows recording goods receipts."},
        {"Warehouse.RecordGoodsIssue", "Warehouse", "RecordGoodsIssue", "Allows recording goods issues."},
        {"Warehouse.AdjustInventoryManual", "Warehouse", "AdjustInventoryManual", "Allows manual inventory adjustments."},
        {"Warehouse.TransferStock", "Warehouse", "TransferStock", "Allows transferring stock between locations/warehouses."},
        {"Warehouse.ViewInventory", "Warehouse", "ViewInventory", "Allows viewing current inventory levels."},
        {"Warehouse.CreatePickingRequest", "Warehouse", "CreatePickingRequest", "Allows creating picking requests."},
        {"Warehouse.ViewPickingRequests", "Warehouse", "ViewPickingRequests", "Allows viewing picking requests."},
        {"Warehouse.UpdatePickingRequest", "Warehouse", "UpdatePickingRequest", "Allows updating picking requests."},
        {"Warehouse.DeletePickingRequest", "Warehouse", "DeletePickingRequest", "Allows deleting picking requests."},
        {"Warehouse.RecordPickedQuantity", "Warehouse", "RecordPickedQuantity", "Allows recording picked quantities for picking requests."},
        {"Warehouse.CreateStocktake", "Warehouse", "CreateStocktake", "Allows creating stocktake requests."},
        {"Warehouse.ViewStocktakes", "Warehouse", "ViewStocktakes", "Allows viewing stocktake requests."},
        {"Warehouse.UpdateStocktake", "Warehouse", "UpdateStocktake", "Allows updating stocktake requests."},
        {"Warehouse.DeleteStocktake", "Warehouse", "DeleteStocktake", "Allows deleting stocktake requests."},
        {"Warehouse.RecordCountedQuantity", "Warehouse", "RecordCountedQuantity", "Allows recording counted quantities during stocktake."},
        {"Warehouse.ReconcileStocktake", "Warehouse", "ReconcileStocktake", "Allows reconciling stocktakes and posting adjustments."},
        {"Finance.CreateGLAccount", "Finance", "CreateGLAccount", "Allows creating general ledger accounts."},
        {"Finance.ViewGLAccounts", "Finance", "ViewGLAccounts", "Allows viewing general ledger accounts."},
        {"Finance.UpdateGLAccount", "Finance", "UpdateGLAccount", "Allows updating general ledger accounts."},
        {"Finance.DeleteGLAccount", "Finance", "DeleteGLAccount", "Allows deleting general ledger accounts."},
        {"Finance.CreateJournalEntry", "Finance", "CreateJournalEntry", "Allows creating journal entries."},
        {"Finance.ViewJournalEntries", "Finance", "ViewJournalEntries", "Allows viewing journal entries and details."},
        {"Finance.PostJournalEntry", "Finance", "PostJournalEntry", "Allows posting journal entries to the general ledger."},
        {"Finance.DeleteJournalEntry", "Finance", "DeleteJournalEntry", "Allows deleting journal entries."},
        {"Finance.ViewARBalance", "Finance", "ViewARBalance", "Allows viewing accounts receivable balances."},
        {"Finance.ViewARTransactions", "Finance", "ViewARTransactions", "Allows viewing accounts receivable transactions."},
        {"Finance.AdjustARBalance", "Finance", "AdjustARBalance", "Allows manual adjustments to AR balances."},
        {"Finance.CreateTaxRate", "Finance", "CreateTaxRate", "Allows creating tax rates."},
        {"Finance.ViewTaxRates", "Finance", "ViewTaxRates", "Allows viewing tax rates."},
        {"Finance.UpdateTaxRate", "Finance", "UpdateTaxRate", "Allows updating tax rates."},
        {"Finance.DeleteTaxRate", "Finance", "DeleteTaxRate", "Allows deleting tax rates."},
        {"Finance.ViewTrialBalance", "Finance", "ViewTrialBalance", "Allows viewing the Trial Balance report."},
        {"Finance.ViewBalanceSheet", "Finance", "ViewBalanceSheet", "Allows viewing the Balance Sheet report."},
        {"Finance.ViewIncomeStatement", "Finance", "ViewIncomeStatement", "Allows viewing the Income Statement report."},
        {"Finance.ViewCashFlowStatement", "Finance", "ViewCashFlowStatement", "Allows viewing the Cash Flow Statement report."},
        {"Sales.CreateSalesOrder", "Sales", "CreateSalesOrder", "Allows creating sales orders."},
        {"Sales.ViewSalesOrders", "Sales", "ViewSalesOrders", "Allows viewing sales orders."},
        {"Sales.UpdateSalesOrder", "Sales", "UpdateSalesOrder", "Allows updating sales orders."},
        {"Sales.DeleteSalesOrder", "Sales", "DeleteSalesOrder", "Allows deleting sales orders."},
        {"Sales.CreateInvoice", "Sales", "CreateInvoice", "Allows creating invoices."},
        {"Sales.ViewInvoices", "Sales", "ViewInvoices", "Allows viewing invoices."},
        {"Sales.UpdateInvoice", "Sales", "UpdateInvoice", "Allows updating invoices."},
        {"Sales.DeleteInvoice", "Sales", "DeleteInvoice", "Allows deleting invoices."},
        {"Sales.RecordPayment", "Sales", "RecordPayment", "Allows recording payments."},
        {"Sales.ViewPayments", "Sales", "ViewPayments", "Allows viewing payments."},
        {"Sales.UpdatePayment", "Sales", "UpdatePayment", "Allows updating payments."},
        {"Sales.DeletePayment", "Sales", "DeletePayment", "Allows deleting payments."},
        {"Sales.CreateQuotation", "Sales", "CreateQuotation", "Allows creating quotations."},
        {"Sales.ViewQuotations", "Sales", "ViewQuotations", "Allows viewing quotations."},
        {"Sales.UpdateQuotation", "Sales", "UpdateQuotation", "Allows updating quotations."},
        {"Sales.DeleteQuotation", "Sales", "DeleteQuotation", "Allows deleting quotations."},
        {"Sales.ConvertQuotationToSalesOrder", "Sales", "ConvertQuotationToSalesOrder", "Allows converting quotations to sales orders."},
        {"Sales.CreateShipment", "Sales", "CreateShipment", "Allows creating shipments."},
        {"Sales.ViewShipments", "Sales", "ViewShipments", "Allows viewing shipments."},
        {"Sales.UpdateShipment", "Sales", "UpdateShipment", "Allows updating shipments."},
        {"Sales.DeleteShipment", "Sales", "DeleteShipment", "Allows deleting shipments."},
        {"Sales.CreateReturn", "Sales", "CreateReturn", "Allows creating sales returns."},
        {"Sales.ViewReturns", "Sales", "ViewReturns", "Allows viewing sales returns."},
        {"Sales.UpdateReturn", "Sales", "UpdateReturn", "Allows updating sales returns."},
        {"Sales.DeleteReturn", "Sales", "DeleteReturn", "Allows deleting sales returns."},
        {"Manufacturing.CreateProductionOrder", "Manufacturing", "CreateProductionOrder", "Allows creating production orders."},
        {"Manufacturing.ViewProductionOrder", "Manufacturing", "ViewProductionOrder", "Allows viewing production orders."},
        {"Manufacturing.UpdateProductionOrder", "Manufacturing", "UpdateProductionOrder", "Allows updating production orders."},
        {"Manufacturing.DeleteProductionOrder", "Manufacturing", "DeleteProductionOrder", "Allows deleting production orders."},
        {"Manufacturing.RecordActualQuantityProduced", "Manufacturing", "RecordActualQuantityProduced", "Allows recording actual quantity produced for production orders."},
        {"Manufacturing.CreateBillOfMaterial", "Manufacturing", "CreateBillOfMaterial", "Allows creating bills of material (BOMs)."},
        {"Manufacturing.ViewBillOfMaterial", "Manufacturing", "ViewBillOfMaterial", "Allows viewing bills of material (BOMs)."},
        {"Manufacturing.UpdateBillOfMaterial", "Manufacturing", "UpdateBillOfMaterial", "Allows updating bills of material (BOMs)."},
        {"Manufacturing.DeleteBillOfMaterial", "Manufacturing", "DeleteBillOfMaterial", "Allows deleting bills of material (BOMs)."},
        {"Manufacturing.CreateProductionLine", "Manufacturing", "CreateProductionLine", "Allows creating production lines."},
        {"Manufacturing.ViewProductionLine", "Manufacturing", "ViewProductionLine", "Allows viewing production lines."},
        {"Manufacturing.UpdateProductionLine", "Manufacturing", "UpdateProductionLine", "Allows updating production lines."},
        {"Manufacturing.DeleteProductionLine", "Manufacturing", "DeleteProductionLine", "Allows deleting production lines."},
        {"Manufacturing.CreateMaintenanceRequest", "Manufacturing", "CreateMaintenanceRequest", "Allows creating maintenance requests."},
        {"Manufacturing.ViewMaintenanceManagement", "Manufacturing", "ViewMaintenanceManagement", "Allows viewing maintenance requests."},
        {"Manufacturing.UpdateMaintenanceRequest", "Manufacturing", "UpdateMaintenanceRequest", "Allows updating maintenance requests."},
        {"Manufacturing.DeleteMaintenanceRequest", "Manufacturing", "DeleteMaintenanceRequest", "Allows deleting maintenance requests."},
        {"Manufacturing.RecordMaintenanceActivity", "Manufacturing", "RecordMaintenanceActivity", "Allows recording maintenance activities."},
        {"Manufacturing.ViewMaintenanceActivities", "Manufacturing", "ViewMaintenanceActivities", "Allows viewing maintenance activities."},
        {"Material.CreateReceiptSlip", "Material", "CreateReceiptSlip", "Allows creating material receipt slips."},
        {"Material.ViewReceiptSlips", "Material", "ViewReceiptSlips", "Allows viewing material receipt slips."},
        {"Material.UpdateReceiptSlip", "Material", "UpdateReceiptSlip", "Allows updating material receipt slips."},
        {"Material.DeleteReceiptSlip", "Material", "DeleteReceiptSlip", "Allows deleting material receipt slips."},
        {"Material.RecordReceivedQuantity", "Material", "RecordReceivedQuantity", "Allows recording received quantities on material receipt slips."},
        {"Material.CreateIssueSlip", "Material", "CreateIssueSlip", "Allows creating material issue slips."},
        {"Material.ViewIssueSlips", "Material", "ViewIssueSlips", "Allows viewing material issue slips."},
        {"Material.UpdateIssueSlip", "Material", "UpdateIssueSlip", "Allows updating material issue slips."},
        {"Material.DeleteIssueSlip", "Material", "DeleteIssueSlip", "Allows deleting material issue slips."},
        {"Material.RecordIssuedQuantity", "Material", "RecordIssuedQuantity", "Allows recording issued quantities on material issue slips."},
        {"Material.CreateMaterialRequest", "Material", "CreateMaterialRequest", "Allows creating material request slips."},
        {"Material.ViewMaterialRequests", "Material", "ViewMaterialRequests", "Allows viewing material request slips."},
        {"Material.UpdateMaterialRequest", "Material", "UpdateMaterialRequest", "Allows updating material request slips."},
        {"Material.DeleteMaterialRequest", "Material", "DeleteMaterialRequest", "Allows deleting material request slips."},
        {"Material.CreateMaterialIssueSlip", "Material", "CreateMaterialIssueSlip", "Allows creating material issue slips for manufacturing."},
        {"Material.ViewMaterialIssueSlips", "Material", "ViewMaterialIssueSlips", "Allows viewing material issue slips for manufacturing."},
        {"Material.UpdateMaterialIssueSlip", "Material", "UpdateMaterialIssueSlip", "Allows updating material issue slips for manufacturing."},
        {"Material.DeleteMaterialIssueSlip", "Material", "DeleteMaterialIssueSlip", "Allows deleting material issue slips for manufacturing."},
        {"Material.RecordMaterialIssueQuantity", "Material", "RecordMaterialIssueQuantity", "Allows recording issued quantities on material issue slips for manufacturing."},
        {"Notification.CreateNotification", "Notification", "CreateNotification", "Allows creating notifications."},
        {"Notification.ViewNotifications", "Notification", "ViewNotifications", "Allows viewing notifications."},
        {"Notification.UpdateNotification", "Notification", "UpdateNotification", "Allows updating notifications."},
        {"Notification.DeleteNotification", "Notification", "DeleteNotification", "Allows deleting notifications."},
        {"Notification.MarkAsRead", "Notification", "MarkAsRead", "Allows marking notifications as read."},
        {"Report.CreateReportRequest", "Report", "CreateReportRequest", "Allows creating report requests."},
        {"Report.ViewReportRequests", "Report", "ViewReportRequests", "Allows viewing report requests."},
        {"Report.UpdateReportRequest", "Report", "UpdateReportRequest", "Allows updating report requests."},
        {"Report.DeleteReportRequest", "Report", "DeleteReportRequest", "Allows deleting report requests."},
        {"Report.RunReportNow", "Report", "RunReportNow", "Allows running reports immediately."},
        {"Report.ViewReportExecutionLogs", "Report", "ViewReportExecutionLogs", "Allows viewing report execution logs."},
        {"Scheduler.CreateScheduledTask", "Scheduler", "CreateScheduledTask", "Allows creating scheduled tasks."},
        {"Scheduler.ViewScheduledTasks", "Scheduler", "ViewScheduledTasks", "Allows viewing scheduled tasks."},
        {"Scheduler.UpdateScheduledTask", "Scheduler", "UpdateScheduledTask", "Allows updating scheduled tasks."},
        {"Scheduler.DeleteScheduledTask", "Scheduler", "DeleteScheduledTask", "Allows deleting scheduled tasks."},
        {"Scheduler.ViewTaskExecutionLogs", "Scheduler", "ViewTaskExecutionLogs", "Allows viewing task execution logs."},
        {"Scheduler.RecordTaskExecutionLog", "Scheduler", "RecordTaskExecutionLog", "Allows recording task execution logs."},
        {"Integration.RegisterDevice", "Integration", "RegisterDevice", "Allows registering new devices."},
        {"Integration.ViewDeviceConfigs", "Integration", "ViewDeviceConfigs", "Allows viewing device configurations."},
        {"Integration.UpdateDeviceConfig", "Integration", "UpdateDeviceConfig", "Allows updating device configurations."},
        {"Integration.DeleteDeviceConfig", "Integration", "DeleteDeviceConfig", "Allows deleting device configurations."},
        {"Integration.UpdateDeviceConnectionStatus", "Integration", "UpdateDeviceConnectionStatus", "Allows updating device connection status."},
        {"Integration.RecordDeviceEvent", "Integration", "RecordDeviceEvent", "Allows recording device events."},
        {"Integration.ViewDeviceEventLogs", "Integration", "ViewDeviceEventLogs", "Allows viewing device event logs."},
        {"Integration.CreateIntegrationConfig", "Integration", "CreateIntegrationConfig", "Allows creating external system integration configurations."},
        {"Integration.ViewIntegrationConfigs", "Integration", "ViewIntegrationConfigs", "Allows viewing external system integration configurations."},
        {"Integration.UpdateIntegrationConfig", "Integration", "UpdateIntegrationConfig", "Allows updating external system integration configurations."},
        {"Integration.DeleteIntegrationConfig", "Integration", "DeleteIntegrationConfig", "Allows deleting external system integration configurations."},
        {"Integration.UpdateIntegrationConfigStatus", "Integration", "UpdateIntegrationConfigStatus", "Allows updating external system integration status."},
        {"Integration.ManageAPIEndpoints", "Integration", "ManageAPIEndpoints", "Allows managing API endpoints for external systems."},
        {"Integration.SendData", "Integration", "SendData", "Allows sending test data to external systems."},
        {"Security.ViewAuditLogs", "Security", "ViewAuditLogs", "Allows viewing audit logs."},
        {"Security.ExportAuditLogs", "Security", "ExportAuditLogs", "Allows exporting audit logs."},
        {"Security.ViewSessions", "Security", "ViewSessions", "Allows viewing user sessions."},
        {"Security.DeactivateSession", "Security", "DeactivateSession", "Allows deactivating user sessions."},
        {"Security.DeleteSession", "Security", "DeleteSession", "Allows deleting user sessions."},
        {"Admin.FullAccess", "Admin", "FullAccess", "Grants full administrative access to all modules and actions."} // Catch-all for super admin
    };

    for (auto& perm : permissions) {
        perm.id = ERP::Utils::generateUUID(); // Generate UUID for each
        if (!permissionDAO_->create(perm)) {
            ERP::Logger::Logger::getInstance().error("Failed to create permission: " + perm.name);
            success = false; // Continue to create others, but mark overall as failed
        }
    }
    if (!success) { dbConnection_->rollbackTransaction(); return false; }
    
    // Assign all created permissions to the Admin role
    for (const auto& perm : permissions) {
        if (!roleDAO_->assignPermissionToRole(adminRole.id, perm.name)) {
            ERP::Logger::Logger::getInstance().error("Failed to assign permission " + perm.name + " to Admin role.");
            success = false;
        }
    }
    if (!success) { dbConnection_->rollbackTransaction(); return false; }

    // --- Admin User ---
    ERP::User::DTO::UserDTO adminUser;
    adminUser.username = "admin";
    adminUser.passwordSalt = ERP::Security::Utils::PasswordHasher::generateSalt();
    adminUser.passwordHash = ERP::Security::Utils::PasswordHasher::hashPassword("admin123", adminUser.passwordSalt);
    adminUser.email = "admin@example.com";
    adminUser.firstName = "System";
    adminUser.lastName = "Admin";
    adminUser.phoneNumber = "123456789";
    adminUser.type = ERP::User::DTO::UserType::ADMIN;
    adminUser.roleId = adminRole.id; // Assign Admin role
    adminUser.id = ERP::Utils::generateUUID();
    adminUser.status = ERP::Common::EntityStatus::ACTIVE;

    success = userDAO_->create(adminUser);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create admin user."); dbConnection_->rollbackTransaction(); return false; }

    // NEW: Assign additional roles to Admin via UserRoleDAO
    // For example, if Admin needs specific permissions from 'User' role, assign it.
    // Assuming 'Admin' role already has all permissions, this might be redundant for basic setup,
    // but demonstrates how to use UserRoleDAO.
    if (!userRoleDAO_->assignRoleToUser(adminUser.id, userRole.id)) { // Assign 'User' role to 'admin'
        ERP::Logger::Logger::getInstance().warning("Failed to assign 'User' role to admin user via UserRoleDAO. This might indicate an an issue with user_roles table or logic.");
        // Not a critical failure for initial data, but worth noting.
    }


    // --- Regular User ---
    ERP::User::DTO::UserDTO regularUser;
    regularUser.username = "user";
    regularUser.passwordSalt = ERP::Security::Utils::PasswordHasher::generateSalt();
    regularUser.passwordHash = ERP::Security::Utils::PasswordHasher::hashPassword("user123", regularUser.passwordSalt);
    regularUser.email = "user@example.com";
    regularUser.firstName = "Regular";
    regularUser.lastName = "User";
    regularUser.phoneNumber = "987654321";
    regularUser.type = ERP::User::DTO::UserType::EMPLOYEE;
    regularUser.roleId = userRole.id; // Assign User role
    regularUser.id = ERP::Utils::generateUUID();
    regularUser.status = ERP::Common::EntityStatus::ACTIVE;

    success = userDAO_->create(regularUser);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create regular user."); dbConnection_->rollbackTransaction(); return false; }

    // --- Default Configs ---
    ERP::Config::DTO::ConfigDTO defaultCurrencyConfig;
    defaultCurrencyConfig.configKey = "DEFAULT_CURRENCY";
    defaultCurrencyConfig.configValue = "VND";
    defaultCurrencyConfig.isEncrypted = false;
    defaultCurrencyConfig.description = "Default currency for financial transactions.";
    defaultCurrencyConfig.id = ERP::Utils::generateUUID();
    success = configDAO_->create(defaultCurrencyConfig);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create default currency config."); dbConnection_->rollbackTransaction(); return false; }

    // Example: Encrypted API Key config
    ERP::Config::DTO::ConfigDTO apiKeyConfig;
    apiKeyConfig.configKey = "EXTERNAL_API_KEY";
    apiKeyConfig.configValue = encryptionService_.encrypt("my_secret_api_key_123"); // Encrypt the value
    apiKeyConfig.isEncrypted = true;
    apiKeyConfig.description = "API Key for external system integration.";
    apiKeyConfig.id = ERP::Utils::generateUUID();
    success = configDAO_->create(apiKeyConfig);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create API key config."); dbConnection_->rollbackTransaction(); return false; }


    // --- Default Categories ---
    ERP::Catalog::DTO::CategoryDTO rawMaterialCat, finishedGoodCat;
    rawMaterialCat.name = "Nguyên vật liệu thô"; rawMaterialCat.description = "Vật liệu đầu vào cho sản xuất.";
    rawMaterialCat.id = ERP::Utils::generateUUID();
    finishedGoodCat.name = "Thành phẩm"; finishedGoodCat.description = "Sản phẩm cuối cùng đã hoàn thiện.";
    finishedGoodCat.id = ERP::Utils::generateUUID();

    success = categoryDAO_->create(rawMaterialCat) && categoryDAO_->create(finishedGoodCat);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create categories."); dbConnection_->rollbackTransaction(); return false; }

    // --- Default Unit of Measures ---
    ERP::Catalog::DTO::UnitOfMeasureDTO pcsUom, kgUom;
    pcsUom.name = "Cái"; pcsUom.symbol = "Pcs";
    pcsUom.id = ERP::Utils::generateUUID();
    kgUom.name = "Kilogram"; kgUom.symbol = "Kg";
    kgUom.id = ERP::Utils::generateUUID();

    success = unitOfMeasureDAO_->create(pcsUom) && unitOfMeasureDAO_->create(kgUom);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create UoMs."); dbConnection_->rollbackTransaction(); return false; }

    // --- Default Warehouse and Location ---
    ERP::Catalog::DTO::WarehouseDTO mainWarehouse;
    mainWarehouse.name = "Kho chính"; mainWarehouse.location = "TP.HCM";
    mainWarehouse.id = ERP::Utils::generateUUID();
    success = warehouseDAO_->create(mainWarehouse);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create main warehouse."); dbConnection_->rollbackTransaction(); return false; }

    ERP::Catalog::DTO::LocationDTO defaultLocation;
    defaultLocation.warehouseId = mainWarehouse.id;
    defaultLocation.name = "Khu vực chung"; defaultLocation.type = "General";
    defaultLocation.id = ERP::Utils::generateUUID();
    success = locationDAO_->create(defaultLocation);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create default location."); dbConnection_->rollbackTransaction(); return false; }

    // --- Sample Products ---
    ERP::Product::DTO::ProductDTO rawMaterialProduct, finishedGoodProduct;
    rawMaterialProduct.name = "Thép tấm 3mm"; rawMaterialProduct.productCode = "RM-STEEL-001";
    rawMaterialProduct.categoryId = rawMaterialCat.id; rawMaterialProduct.baseUnitOfMeasureId = kgUom.id;
    rawMaterialProduct.purchasePrice = 10000.0; rawMaterialProduct.purchaseCurrency = "VND";
    rawMaterialProduct.type = ERP::Product::DTO::ProductType::RAW_MATERIAL;
    rawMaterialProduct.id = ERP::Utils::generateUUID();

    finishedGoodProduct.name = "Khung xe đạp thành phẩm"; finishedGoodProduct.productCode = "FG-BIKEFRAME-001";
    finishedGoodProduct.categoryId = finishedGoodCat.id; finishedGoodProduct.baseUnitOfMeasureId = pcsUom.id;
    finishedGoodProduct.salePrice = 500000.0; finishedGoodProduct.saleCurrency = "VND";
    finishedGoodProduct.type = ERP::Product::DTO::ProductType::FINISHED_GOOD;
    finishedGoodProduct.id = ERP::Utils::generateUUID();

    success = productDAO_->create(rawMaterialProduct) && productDAO_->create(finishedGoodProduct);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create products."); dbConnection_->rollbackTransaction(); return false; }

    // --- Sample Product Unit Conversions ---
    ERP::Product::DTO::ProductUnitConversionDTO kgToGramConv, pcsToBoxConv;
    kgToGramConv.productId = rawMaterialProduct.id;
    kgToGramConv.fromUnitOfMeasureId = kgUom.id;
    // Need to ensure 'g' and 'Box' UoMs exist for this to succeed, or create them first.
    // For now, let's assume they exist or handle creation in Initializer.
    // Let's create them here for robustness
    ERP::Catalog::DTO::UnitOfMeasureDTO gramUom, boxUom;
    gramUom.name = "Gram"; gramUom.symbol = "g"; gramUom.id = ERP::Utils::generateUUID();
    boxUom.name = "Hộp"; boxUom.symbol = "Box"; boxUom.id = ERP::Utils::generateUUID();
    
    // Ensure these are created before conversions
    success = unitOfMeasureDAO_->create(gramUom) && unitOfMeasureDAO_->create(boxUom);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create additional UoMs for conversions."); dbConnection_->rollbackTransaction(); return false; }

    kgToGramConv.toUnitOfMeasureId = gramUom.id;
    kgToGramConv.conversionFactor = 1000.0; // 1 Kg = 1000g
    kgToGramConv.notes = "Standard conversion for RM-STEEL-001";
    kgToGramConv.id = ERP::Utils::generateUUID();

    pcsToBoxConv.productId = finishedGoodProduct.id;
    pcsToBoxConv.fromUnitOfMeasureId = pcsUom.id;
    pcsToBoxConv.toUnitOfMeasureId = boxUom.id;
    pcsToBoxConv.conversionFactor = 10.0; // 1 Box = 10 Pcs
    pcsToBoxConv.notes = "Standard packaging for FG-BIKEFRAME-001";
    pcsToBoxConv.id = ERP::Utils::generateUUID();

    success = productUnitConversionDAO_->create(kgToGramConv) && productUnitConversionDAO_->create(pcsToBoxConv);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create product unit conversions."); dbConnection_->rollbackTransaction(); return false; }


    // --- Sample Customer ---
    ERP::Customer::DTO::CustomerDTO sampleCustomer;
    sampleCustomer.name = "Công ty TNHH ABC"; sampleCustomer.taxId = "0312345678";
    sampleCustomer.id = ERP::Utils::generateUUID();
    success = customerDAO_->create(sampleCustomer);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create customer."); dbConnection_->rollbackTransaction(); return false; }

    // --- Sample Supplier ---
    ERP::Supplier::DTO::SupplierDTO sampleSupplier;
    sampleSupplier.name = "Công ty Cung ứng XYZ"; sampleSupplier.taxId = "0198765432";
    sampleSupplier.id = ERP::Utils::generateUUID();
    success = supplierDAO_->create(sampleSupplier);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create supplier."); dbConnection_->rollbackTransaction(); return false; }

    // --- Default GL Accounts ---
    ERP::Finance::DTO::GeneralLedgerAccountDTO cashAccount, arAccount, apAccount, revenueAccount, expenseAccount;
    cashAccount.accountNumber = "111"; cashAccount.accountName = "Tiền mặt"; cashAccount.accountType = ERP::Finance::DTO::GLAccountType::ASSET; cashAccount.normalBalance = ERP::Finance::DTO::NormalBalanceType::DEBIT;
    cashAccount.id = ERP::Utils::generateUUID();
    arAccount.accountNumber = "131"; arAccount.accountName = "Phải thu khách hàng"; arAccount.accountType = ERP::Finance::DTO::GLAccountType::ASSET; arAccount.normalBalance = ERP::Finance::DTO::NormalBalanceType::DEBIT;
    arAccount.id = ERP::Utils::generateUUID();
    apAccount.accountNumber = "331"; apAccount.accountName = "Phải trả người bán"; apAccount.accountType = ERP::Finance::DTO::GLAccountType::LIABILITY; apAccount.normalBalance = ERP::Finance::DTO::NormalBalanceType::CREDIT;
    apAccount.id = ERP::Utils::generateUUID();
    revenueAccount.accountNumber = "511"; revenueAccount.accountName = "Doanh thu bán hàng"; revenueAccount.accountType = ERP::Finance::DTO::GLAccountType::REVENUE; revenueAccount.normalBalance = ERP::Finance::DTO::NormalBalanceType::CREDIT;
    revenueAccount.id = ERP::Utils::generateUUID();
    expenseAccount.accountNumber = "641"; expenseAccount.accountName = "Chi phí bán hàng"; expenseAccount.accountType = ERP::Finance::DTO::GLAccountType::EXPENSE; expenseAccount.normalBalance = ERP::Finance::DTO::NormalBalanceType::DEBIT;
    expenseAccount.id = ERP::Utils::generateUUID();

    success = glDAO_->createGLAccount(cashAccount) && glDAO_->createGLAccount(arAccount) &&
              glDAO_->createGLAccount(apAccount) && glDAO_->createGLAccount(revenueAccount) &&
              glDAO_->createGLAccount(expenseAccount);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create GL accounts."); dbConnection_->rollbackTransaction(); return false; }

    // --- Default Tax Rate ---
    ERP::Finance::DTO::TaxRateDTO vatTax;
    vatTax.name = "VAT 10%"; vatTax.rate = 10.0;
    vatTax.effectiveDate = ERP::Utils::DateUtils::now(); // Effective from now
    vatTax.id = ERP::Utils::generateUUID();
    success = taxRateDAO_->create(vatTax);
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create default tax rate."); dbConnection_->rollbackTransaction(); return false; }


    // --- Sample Sales Order (for Return module and Picking) ---
    // This SO ID is used by sample return and sample picking request.
    ERP::Sales::DTO::SalesOrderDTO dummySalesOrder;
    dummySalesOrder.id = "some_sales_order_id_for_picking"; // This ID must exist in sales_orders table for FK
    dummySalesOrder.orderNumber = "SO-DUMMY-001";
    dummySalesOrder.customerId = sampleCustomer.id;
    dummySalesOrder.requestedByUserId = adminUser.id;
    dummySalesOrder.orderDate = ERP::Utils::DateUtils::now();
    dummySalesOrder.requiredDeliveryDate = ERP::Utils::DateUtils::now();
    dummySalesOrder.status = ERP::Sales::DTO::SalesOrderStatus::COMPLETED;
    dummySalesOrder.totalAmount = 100000.0; dummySalesOrder.netAmount = 100000.0; dummySalesOrder.amountDue = 0.0; dummySalesOrder.currency = "VND";
    dummySalesOrder.warehouseId = mainWarehouse.id;
    // Temporarily create this dummy SalesOrder if it doesn't exist to satisfy FK.
    // This is purely for DatabaseInitializer to insert sample data without complex inter-module setup here.
    if (!salesOrderDAO_->findById(dummySalesOrder.id)) {
         success = salesOrderDAO_->create(dummySalesOrder);
         if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create dummy sales order for return/picking."); dbConnection_->rollbackTransaction(); return false; }
    }


    // --- Sample Sales Return (for Return module) ---
    ERP::Sales::DTO::ReturnDTO sampleReturn;
    sampleReturn.returnNumber = "RET-2024-001";
    sampleReturn.salesOrderId = dummySalesOrder.id; // Link to the dummy sales order
    sampleReturn.customerId = sampleCustomer.id;
    sampleReturn.returnDate = ERP::Utils::DateUtils::now();
    sampleReturn.reason = "Khách hàng không hài lòng";
    sampleReturn.totalAmount = 50000.0;
    sampleReturn.status = ERP::Sales::DTO::ReturnStatus::PENDING;
    sampleReturn.warehouseId = mainWarehouse.id;
    sampleReturn.notes = "Returned 1 unit of FG-BIKEFRAME-001";
    sampleReturn.id = ERP::Utils::generateUUID();

    // Create return details
    ERP::Sales::DTO::ReturnDetailDTO returnDetail1;
    returnDetail1.productId = finishedGoodProduct.id;
    returnDetail1.quantity = 1.0;
    returnDetail1.unitOfMeasureId = pcsUom.id;
    returnDetail1.unitPrice = 50000.0; // Partial return value
    returnDetail1.refundedAmount = 0.0;
    returnDetail1.condition = "New";
    returnDetail1.notes = "Item returned in good condition.";
    returnDetail1.salesOrderDetailId = "some_sales_order_detail_id"; // Placeholder, link to existing sales order detail
    returnDetail1.id = ERP::Utils::generateUUID();
    returnDetail1.returnId = sampleReturn.id; // Link to parent
    returnDetail1.status = ERP::Common::EntityStatus::ACTIVE; // Default active

    // Add to details vector
    sampleReturn.details.push_back(returnDetail1);

    success = returnDAO_->create(sampleReturn); // This DAO::create handles only the Return header.
    // Need to manually create details after creating the header.
    if (success) {
        // Now create details separately using returnDAO_->createReturnDetail
        for (const auto& detail : sampleReturn.details) {
            if (!returnDAO_->createReturnDetail(detail)) {
                ERP::Logger::Logger::getInstance().error("Failed to create return detail.");
                success = false;
                break;
            }
        }
    }
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create sample return."); dbConnection_->rollbackTransaction(); return false; }

    // --- Sample Picking Request ---
    ERP::Warehouse::DTO::PickingRequestDTO samplePickingRequest;
    samplePickingRequest.id = ERP::Utils::generateUUID();
    samplePickingRequest.requestNumber = "PK-2024-001";
    samplePickingRequest.salesOrderId = dummySalesOrder.id; // Link to dummy sales order
    samplePickingRequest.warehouseId = mainWarehouse.id;
    samplePickingRequest.requestDate = ERP::Utils::DateUtils::now();
    samplePickingRequest.requestedByUserId = adminUser.id;
    samplePickingRequest.status = ERP::Warehouse::DTO::PickingRequestStatus::PENDING;
    samplePickingRequest.notes = "Picking request for sample sales order.";

    // Create picking details
    ERP::Warehouse::DTO::PickingDetailDTO pickingDetail1;
    pickingDetail1.id = ERP::Utils::generateUUID();
    pickingDetail1.pickingRequestId = samplePickingRequest.id;
    pickingDetail1.productId = finishedGoodProduct.id;
    pickingDetail1.warehouseId = mainWarehouse.id;
    pickingDetail1.locationId = defaultLocation.id;
    pickingDetail1.requestedQuantity = 5.0;
    pickingDetail1.pickedQuantity = 0.0;
    pickingDetail1.unitOfMeasureId = pcsUom.id;
    pickingDetail1.isPicked = false;
    pickingDetail1.notes = "Pick 5 units of finished bike frames.";
    pickingDetail1.status = ERP::Common::EntityStatus::ACTIVE;

    samplePickingRequest.details.push_back(pickingDetail1);

    success = pickingRequestDAO_->create(samplePickingRequest); // Create picking request header
    if (success) {
        // Create picking details
        for (const auto& detail : samplePickingRequest.details) {
            if (!pickingRequestDAO_->createPickingDetail(detail)) { // Use specific detail creation method
                ERP::Logger::Logger::getInstance().error("Failed to create picking detail.");
                success = false;
                break;
            }
        }
    }
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create sample picking request."); dbConnection_->rollbackTransaction(); return false; }


    // --- Sample AR Transaction (for AR module) ---
    // Note: To successfully create this, you'd ideally have a corresponding SalesOrder or Invoice in DB.
    // For now, we use a dummy customer ID and type.
    ERP::Finance::DTO::AccountReceivableTransactionDTO sampleArTxn;
    sampleArTxn.id = ERP::Utils::generateUUID();
    sampleArTxn.customerId = sampleCustomer.id;
    sampleArTxn.type = ERP::Finance::DTO::ARTransactionType::INVOICE; // Example: an invoice
    sampleArTxn.amount = 100000.0;
    sampleArTxn.currency = "VND";
    sampleArTxn.transactionDate = ERP::Utils::DateUtils::now();
    sampleArTxn.referenceDocumentId = "sample_invoice_id_001"; // Link to a dummy invoice
    sampleArTxn.referenceDocumentType = "Invoice";
    sampleArTxn.notes = "Initial sample AR transaction for customer ABC.";
    sampleArTxn.status = ERP::Common::EntityStatus::ACTIVE;

    success = arTransactionDAO_->save(sampleArTxn); // Use templated save (create)
    if (!success) { ERP::Logger::Logger::getInstance().error("Failed to create sample AR transaction."); dbConnection_->rollbackTransaction(); return false; }


    if (success) {
        dbConnection_->commitTransaction();
        ERP::Logger::Logger::getInstance().info("DatabaseInitializer: Initial data populated successfully.");
    } else {
        dbConnection_->rollbackTransaction();
        ERP::Logger::Logger::getInstance().error("DatabaseInitializer: Initial data population failed, transaction rolled back.");
    }
    return success;
}

bool DatabaseInitializer::executeSql(const std::string& sql) {
    if (!dbConnection_->execute(sql)) {
        ERP::Logger::Logger::getInstance().error("DatabaseInitializer: SQL execution failed: " + sql);
        return false;
    }
    return true;
}

// --- Table Creation Implementations ---
// (Many functions for each table, simplified for brevity here)

bool DatabaseInitializer::createUsersTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS users (
            id TEXT PRIMARY KEY,
            username TEXT NOT NULL UNIQUE,
            password_hash TEXT NOT NULL,
            password_salt TEXT NOT NULL,
            email TEXT,
            first_name TEXT,
            last_name TEXT,
            phone_number TEXT,
            type INTEGER NOT NULL,
            role_id TEXT NOT NULL,
            is_locked INTEGER DEFAULT 0,
            failed_login_attempts INTEGER DEFAULT 0,
            lock_until_time TEXT,
            last_login_time TEXT,
            last_login_ip TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (role_id) REFERENCES roles(id)
        );
    )");
}

bool DatabaseInitializer::createUserProfilesTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS user_profiles (
            id TEXT PRIMARY KEY,
            user_id TEXT NOT NULL UNIQUE,
            address TEXT,
            date_of_birth TEXT,
            gender TEXT,
            profile_picture_url TEXT,
            preferences_json TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createSessionsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS sessions (
            id TEXT PRIMARY KEY,
            user_id TEXT NOT NULL,
            token TEXT NOT NULL UNIQUE,
            expiration_time TEXT NOT NULL,
            ip_address TEXT,
            user_agent TEXT,
            device_info TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createRolesTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS roles (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            description TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT
        );
    )");
}

bool DatabaseInitializer::createPermissionsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS permissions (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            module TEXT NOT NULL,
            action TEXT NOT NULL,
            description TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT
        );
    )");
}

bool DatabaseInitializer::createRolePermissionsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS role_permissions (
            role_id TEXT NOT NULL,
            permission_id TEXT NOT NULL,
            PRIMARY KEY (role_id, permission_id),
            FOREIGN KEY (role_id) REFERENCES roles(id),
            FOREIGN KEY (permission_id) REFERENCES permissions(id)
        );
    )");
}

bool DatabaseInitializer::createUserRolesTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS user_roles (
            user_id TEXT NOT NULL,
            role_id TEXT NOT NULL,
            PRIMARY KEY (user_id, role_id),
            FOREIGN KEY (user_id) REFERENCES users(id),
            FOREIGN KEY (role_id) REFERENCES roles(id)
        );
    )");
}

bool DatabaseInitializer::createCategoriesTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS categories (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            description TEXT,
            parent_category_id TEXT,
            sort_order INTEGER DEFAULT 0,
            is_active INTEGER DEFAULT 1,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (parent_category_id) REFERENCES categories(id)
        );
    )");
}

bool DatabaseInitializer::createWarehousesTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS warehouses (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            location TEXT,
            contact_person TEXT,
            contact_phone TEXT,
            email TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT
        );
    )");
}

bool DatabaseInitializer::createLocationsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS locations (
            id TEXT PRIMARY KEY,
            warehouse_id TEXT NOT NULL,
            name TEXT NOT NULL,
            type TEXT,
            capacity REAL,
            unit_of_capacity TEXT,
            barcode TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (warehouse_id) REFERENCES warehouses(id)
        );
    )");
}

bool DatabaseInitializer::createUnitOfMeasuresTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS unit_of_measures (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            symbol TEXT NOT NULL UNIQUE,
            description TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT
        );
    )");
}

bool DatabaseInitializer::createProductsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS products (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            product_code TEXT NOT NULL UNIQUE,
            category_id TEXT NOT NULL,
            base_unit_of_measure_id TEXT NOT NULL,
            description TEXT,
            purchase_price REAL,
            purchase_currency TEXT,
            sale_price REAL,
            sale_currency TEXT,
            image_url TEXT,
            weight REAL,
            weight_unit TEXT,
            type INTEGER NOT NULL,
            manufacturer TEXT,
            supplier_id TEXT,
            barcode TEXT,
            attributes_json TEXT,
            pricing_rules_json TEXT,
            unit_conversion_rules_json TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (category_id) REFERENCES categories(id),
            FOREIGN KEY (base_unit_of_measure_id) REFERENCES unit_of_measures(id),
            FOREIGN KEY (supplier_id) REFERENCES suppliers(id)
        );
    )");
}

bool DatabaseInitializer::createProductUnitConversionsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS product_unit_conversions (
            id TEXT PRIMARY KEY,
            product_id TEXT NOT NULL,
            from_unit_of_measure_id TEXT NOT NULL,
            to_unit_of_measure_id TEXT NOT NULL,
            conversion_factor REAL NOT NULL,
            notes TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            UNIQUE(product_id, from_unit_of_measure_id, to_unit_of_measure_id),
            FOREIGN KEY (product_id) REFERENCES products(id),
            FOREIGN KEY (from_unit_of_measure_id) REFERENCES unit_of_measures(id),
            FOREIGN KEY (to_unit_of_measure_id) REFERENCES unit_of_measures(id)
        );
    )");
}

bool DatabaseInitializer::createCustomersTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS customers (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            tax_id TEXT,
            notes TEXT,
            default_payment_terms TEXT,
            credit_limit REAL,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT
            -- Additional fields like contact_persons_json, addresses_json if stored as JSON
        );
    )");
}

bool DatabaseInitializer::createSuppliersTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS suppliers (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            tax_id TEXT,
            notes TEXT,
            default_payment_terms TEXT,
            default_delivery_terms TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT
            -- Additional fields like contact_persons_json, addresses_json if stored as JSON
        );
    )");
}

bool DatabaseInitializer::createInventoryTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS inventory (
            id TEXT PRIMARY KEY,
            product_id TEXT NOT NULL,
            warehouse_id TEXT NOT NULL,
            location_id TEXT NOT NULL,
            quantity REAL DEFAULT 0.0,
            reserved_quantity REAL DEFAULT 0.0,
            available_quantity REAL DEFAULT 0.0,
            unit_cost REAL,
            lot_number TEXT,
            serial_number TEXT,
            manufacture_date TEXT,
            expiration_date TEXT,
            reorder_level REAL,
            reorder_quantity REAL,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            UNIQUE(product_id, warehouse_id, location_id),
            FOREIGN KEY (product_id) REFERENCES products(id),
            FOREIGN KEY (warehouse_id) REFERENCES warehouses(id),
            FOREIGN KEY (location_id) REFERENCES locations(id)
        );
    )");
}

bool DatabaseInitializer::createInventoryTransactionsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS inventory_transactions (
            id TEXT PRIMARY KEY,
            product_id TEXT NOT NULL,
            warehouse_id TEXT NOT NULL,
            location_id TEXT NOT NULL,
            type INTEGER NOT NULL,
            quantity REAL NOT NULL,
            unit_cost REAL,
            transaction_date TEXT NOT NULL,
            lot_number TEXT,
            serial_number TEXT,
            manufacture_date TEXT,
            expiration_date TEXT,
            reference_document_id TEXT,
            reference_document_type TEXT,
            notes TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (product_id) REFERENCES products(id),
            FOREIGN KEY (warehouse_id) REFERENCES warehouses(id),
            FOREIGN KEY (location_id) REFERENCES locations(id)
        );
    )");
}

bool DatabaseInitializer::createInventoryCostLayersTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS inventory_cost_layers (
            id TEXT PRIMARY KEY,
            product_id TEXT NOT NULL,
            warehouse_id TEXT NOT NULL,
            location_id TEXT NOT NULL,
            receipt_date TEXT NOT NULL,
            quantity REAL NOT NULL,
            unit_cost REAL NOT NULL,
            remaining_quantity REAL NOT NULL,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (product_id) REFERENCES products(id),
            FOREIGN KEY (warehouse_id) REFERENCES warehouses(id),
            FOREIGN KEY (location_id) REFERENCES locations(id)
        );
    )");
}

bool DatabaseInitializer::createPickingRequestsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS picking_requests (
            id TEXT PRIMARY KEY,
            request_number TEXT NOT NULL UNIQUE,
            sales_order_id TEXT NOT NULL,
            warehouse_id TEXT NOT NULL,
            request_date TEXT NOT NULL,
            requested_by_user_id TEXT NOT NULL,
            assigned_to_user_id TEXT,
            status INTEGER NOT NULL,
            pick_start_time TEXT,
            pick_end_time TEXT,
            notes TEXT,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (sales_order_id) REFERENCES sales_orders(id),
            FOREIGN KEY (warehouse_id) REFERENCES warehouses(id),
            FOREIGN KEY (requested_by_user_id) REFERENCES users(id),
            FOREIGN KEY (assigned_to_user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createPickingDetailsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS picking_details (
            id TEXT PRIMARY KEY,
            picking_request_id TEXT NOT NULL,
            product_id TEXT NOT NULL,
            warehouse_id TEXT NOT NULL,
            location_id TEXT NOT NULL,
            requested_quantity REAL NOT NULL,
            picked_quantity REAL DEFAULT 0.0,
            unit_of_measure_id TEXT NOT NULL,
            lot_number TEXT,
            serial_number TEXT,
            is_picked INTEGER DEFAULT 0,
            notes TEXT,
            sales_order_detail_id TEXT,
            inventory_transaction_id TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (picking_request_id) REFERENCES picking_requests(id),
            FOREIGN KEY (product_id) REFERENCES products(id),
            FOREIGN KEY (warehouse_id) REFERENCES warehouses(id),
            FOREIGN KEY (location_id) REFERENCES locations(id),
            FOREIGN KEY (unit_of_measure_id) REFERENCES unit_of_measures(id),
            FOREIGN KEY (sales_order_detail_id) REFERENCES sales_order_details(id),
            FOREIGN KEY (inventory_transaction_id) REFERENCES inventory_transactions(id)
        );
    )");
}

bool DatabaseInitializer::createStocktakeRequestsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS stocktake_requests (
            id TEXT PRIMARY KEY,
            warehouse_id TEXT NOT NULL,
            location_id TEXT,
            requested_by_user_id TEXT NOT NULL,
            counted_by_user_id TEXT,
            count_date TEXT NOT NULL,
            status INTEGER NOT NULL,
            notes TEXT,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (warehouse_id) REFERENCES warehouses(id),
            FOREIGN KEY (location_id) REFERENCES locations(id),
            FOREIGN KEY (requested_by_user_id) REFERENCES users(id),
            FOREIGN KEY (counted_by_user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createStocktakeDetailsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS stocktake_details (
            id TEXT PRIMARY KEY,
            stocktake_request_id TEXT NOT NULL,
            product_id TEXT NOT NULL,
            warehouse_id TEXT NOT NULL,
            location_id TEXT NOT NULL,
            system_quantity REAL NOT NULL,
            counted_quantity REAL DEFAULT 0.0,
            difference REAL DEFAULT 0.0,
            lot_number TEXT,
            serial_number TEXT,
            notes TEXT,
            adjustment_transaction_id TEXT, -- Link to inventory_transactions if adjustment is made
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (stocktake_request_id) REFERENCES stocktake_requests(id),
            FOREIGN KEY (product_id) REFERENCES products(id),
            FOREIGN KEY (warehouse_id) REFERENCES warehouses(id),
            FOREIGN KEY (location_id) REFERENCES locations(id),
            FOREIGN KEY (adjustment_transaction_id) REFERENCES inventory_transactions(id)
        );
    )");
}

bool DatabaseInitializer::createReceiptSlipsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS receipt_slips (
            id TEXT PRIMARY KEY,
            receipt_number TEXT NOT NULL UNIQUE,
            warehouse_id TEXT NOT NULL,
            received_by_user_id TEXT NOT NULL,
            receipt_date TEXT NOT NULL,
            status INTEGER NOT NULL,
            reference_document_id TEXT,
            reference_document_type TEXT,
            notes TEXT,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (warehouse_id) REFERENCES warehouses(id),
            FOREIGN KEY (received_by_user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createReceiptSlipDetailsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS receipt_slip_details (
            id TEXT PRIMARY KEY,
            receipt_slip_id TEXT NOT NULL,
            product_id TEXT NOT NULL,
            location_id TEXT NOT NULL,
            expected_quantity REAL NOT NULL,
            received_quantity REAL DEFAULT 0.0,
            unit_cost REAL,
            lot_number TEXT,
            serial_number TEXT,
            manufacture_date TEXT,
            expiration_date TEXT,
            notes TEXT,
            is_fully_received INTEGER DEFAULT 0,
            inventory_transaction_id TEXT, -- Link to inventory_transactions
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (receipt_slip_id) REFERENCES receipt_slips(id),
            FOREIGN KEY (product_id) REFERENCES products(id),
            FOREIGN KEY (location_id) REFERENCES locations(id),
            FOREIGN KEY (inventory_transaction_id) REFERENCES inventory_transactions(id)
        );
    )");
}

bool DatabaseInitializer::createIssueSlipsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS issue_slips (
            id TEXT PRIMARY KEY,
            issue_number TEXT NOT NULL UNIQUE,
            warehouse_id TEXT NOT NULL,
            issued_by_user_id TEXT NOT NULL,
            issue_date TEXT NOT NULL,
            material_request_slip_id TEXT, -- Optional link to material request
            status INTEGER NOT NULL,
            reference_document_id TEXT,
            reference_document_type TEXT,
            notes TEXT,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (warehouse_id) REFERENCES warehouses(id),
            FOREIGN KEY (issued_by_user_id) REFERENCES users(id),
            FOREIGN KEY (material_request_slip_id) REFERENCES material_request_slips(id)
        );
    )");
}

bool DatabaseInitializer::createIssueSlipDetailsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS issue_slip_details (
            id TEXT PRIMARY KEY,
            issue_slip_id TEXT NOT NULL,
            product_id TEXT NOT NULL,
            location_id TEXT NOT NULL,
            requested_quantity REAL NOT NULL,
            issued_quantity REAL DEFAULT 0.0,
            lot_number TEXT,
            serial_number TEXT,
            notes TEXT,
            is_fully_issued INTEGER DEFAULT 0,
            inventory_transaction_id TEXT, -- Link to inventory_transactions
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (issue_slip_id) REFERENCES issue_slips(id),
            FOREIGN KEY (product_id) REFERENCES products(id),
            FOREIGN KEY (location_id) REFERENCES locations(id),
            FOREIGN KEY (inventory_transaction_id) REFERENCES inventory_transactions(id)
        );
    )");
}

bool DatabaseInitializer::createMaterialRequestSlipsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS material_request_slips (
            id TEXT PRIMARY KEY,
            request_number TEXT NOT NULL UNIQUE,
            requesting_department TEXT NOT NULL,
            requested_by_user_id TEXT NOT NULL,
            request_date TEXT NOT NULL,
            approved_by_user_id TEXT,
            approval_date TEXT,
            status INTEGER NOT NULL,
            notes TEXT,
            reference_document_id TEXT,
            reference_document_type TEXT,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (requested_by_user_id) REFERENCES users(id),
            FOREIGN KEY (approved_by_user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createMaterialRequestSlipDetailsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS material_request_slip_details (
            id TEXT PRIMARY KEY,
            material_request_slip_id TEXT NOT NULL,
            product_id TEXT NOT NULL,
            requested_quantity REAL NOT NULL,
            issued_quantity REAL DEFAULT 0.0,
            notes TEXT,
            is_fully_issued INTEGER DEFAULT 0,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (material_request_slip_id) REFERENCES material_request_slips(id),
            FOREIGN KEY (product_id) REFERENCES products(id)
        );
    )");
}

bool DatabaseInitializer::createMaterialIssueSlipsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS material_issue_slips (
            id TEXT PRIMARY KEY,
            issue_number TEXT NOT NULL UNIQUE,
            production_order_id TEXT NOT NULL,
            warehouse_id TEXT NOT NULL,
            issued_by_user_id TEXT NOT NULL,
            issue_date TEXT NOT NULL,
            status INTEGER NOT NULL,
            notes TEXT,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (production_order_id) REFERENCES production_orders(id),
            FOREIGN KEY (warehouse_id) REFERENCES warehouses(id),
            FOREIGN KEY (issued_by_user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createMaterialIssueSlipDetailsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS material_issue_slip_details (
            id TEXT PRIMARY KEY,
            material_issue_slip_id TEXT NOT NULL,
            product_id TEXT NOT NULL,
            location_id TEXT, -- Added for consistency in issue details
            requested_quantity REAL DEFAULT 0.0, -- Added for consistency
            issued_quantity REAL NOT NULL,
            lot_number TEXT,
            serial_number TEXT,
            notes TEXT,
            is_fully_issued INTEGER DEFAULT 0, -- Added for consistency
            inventory_transaction_id TEXT, -- Link to inventory_transactions
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (material_issue_slip_id) REFERENCES material_issue_slips(id),
            FOREIGN KEY (product_id) REFERENCES products(id),
            FOREIGN KEY (inventory_transaction_id) REFERENCES inventory_transactions(id)
        );
    )");
}


bool DatabaseInitializer::createSalesOrdersTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS sales_orders (
            id TEXT PRIMARY KEY,
            order_number TEXT NOT NULL UNIQUE,
            customer_id TEXT NOT NULL,
            requested_by_user_id TEXT NOT NULL,
            approved_by_user_id TEXT,
            order_date TEXT NOT NULL,
            required_delivery_date TEXT,
            status INTEGER NOT NULL,
            total_amount REAL,
            total_discount REAL,
            total_tax REAL,
            net_amount REAL,
            amount_paid REAL DEFAULT 0.0,
            amount_due REAL,
            currency TEXT NOT NULL,
            payment_terms TEXT,
            delivery_address TEXT,
            notes TEXT,
            warehouse_id TEXT NOT NULL,
            quotation_id TEXT, -- Link to quotation
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (customer_id) REFERENCES customers(id),
            FOREIGN KEY (requested_by_user_id) REFERENCES users(id),
            FOREIGN KEY (approved_by_user_id) REFERENCES users(id),
            FOREIGN KEY (warehouse_id) REFERENCES warehouses(id),
            FOREIGN KEY (quotation_id) REFERENCES quotations(id)
        );
    )");
}

bool DatabaseInitializer::createSalesOrderDetailsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS sales_order_details (
            id TEXT PRIMARY KEY,
            sales_order_id TEXT NOT NULL,
            product_id TEXT NOT NULL,
            quantity REAL NOT NULL,
            unit_price REAL NOT NULL,
            discount REAL DEFAULT 0.0,
            discount_type INTEGER, -- 0: Fixed, 1: Percentage
            tax_rate REAL DEFAULT 0.0,
            line_total REAL NOT NULL,
            delivered_quantity REAL DEFAULT 0.0,
            invoiced_quantity REAL DEFAULT 0.0,
            is_fully_delivered INTEGER DEFAULT 0,
            is_fully_invoiced INTEGER DEFAULT 0,
            notes TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (sales_order_id) REFERENCES sales_orders(id),
            FOREIGN KEY (product_id) REFERENCES products(id)
        );
    )");
}

bool DatabaseInitializer::createInvoicesTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS invoices (
            id TEXT PRIMARY KEY,
            invoice_number TEXT NOT NULL UNIQUE,
            customer_id TEXT NOT NULL,
            sales_order_id TEXT NOT NULL,
            type INTEGER NOT NULL, -- 0: Sales Invoice, 1: Proforma, 2: Credit Note, 3: Debit Note
            invoice_date TEXT NOT NULL,
            due_date TEXT NOT NULL,
            status INTEGER NOT NULL, -- 0: Draft, 1: Issued, 2: Paid, 3: Partially Paid, 4: Cancelled, 5: Overdue
            total_amount REAL NOT NULL,
            total_discount REAL,
            total_tax REAL,
            net_amount REAL NOT NULL,
            amount_paid REAL DEFAULT 0.0,
            amount_due REAL NOT NULL,
            currency TEXT NOT NULL,
            notes TEXT,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (customer_id) REFERENCES customers(id),
            FOREIGN KEY (sales_order_id) REFERENCES sales_orders(id)
        );
    )");
}

bool DatabaseInitializer::createInvoiceDetailsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS invoice_details (
            id TEXT PRIMARY KEY,
            invoice_id TEXT NOT NULL,
            product_id TEXT NOT NULL,
            quantity REAL NOT NULL,
            unit_price REAL NOT NULL,
            discount REAL DEFAULT 0.0,
            discount_type INTEGER, -- 0: Fixed, 1: Percentage
            tax_rate REAL DEFAULT 0.0,
            line_total REAL NOT NULL,
            notes TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (invoice_id) REFERENCES invoices(id),
            FOREIGN KEY (product_id) REFERENCES products(id)
        );
    )");
}

bool DatabaseInitializer::createPaymentsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS payments (
            id TEXT PRIMARY KEY,
            payment_number TEXT NOT NULL UNIQUE,
            customer_id TEXT NOT NULL,
            invoice_id TEXT NOT NULL,
            amount REAL NOT NULL,
            currency TEXT NOT NULL,
            payment_date TEXT NOT NULL,
            method INTEGER NOT NULL, -- 0: Cash, 1: Bank Transfer, 2: Credit Card, etc.
            transaction_id TEXT, -- e.g., bank transaction ID, credit card approval code
            status INTEGER NOT NULL, -- 0: Pending, 1: Completed, 2: Failed, 3: Refunded, 4: Cancelled
            notes TEXT,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (customer_id) REFERENCES customers(id),
            FOREIGN KEY (invoice_id) REFERENCES invoices(id)
        );
    )");
}

bool DatabaseInitializer::createQuotationsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS quotations (
            id TEXT PRIMARY KEY,
            quotation_number TEXT NOT NULL UNIQUE,
            customer_id TEXT NOT NULL,
            requested_by_user_id TEXT NOT NULL,
            quotation_date TEXT NOT NULL,
            valid_until_date TEXT NOT NULL,
            status INTEGER NOT NULL, -- 0: Draft, 1: Sent, 2: Accepted, 3: Rejected, 4: Expired, 5: Cancelled
            total_amount REAL,
            total_discount REAL,
            total_tax REAL,
            net_amount REAL,
            currency TEXT NOT NULL,
            payment_terms TEXT,
            delivery_terms TEXT,
            notes TEXT,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (customer_id) REFERENCES customers(id),
            FOREIGN KEY (requested_by_user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createQuotationDetailsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS quotation_details (
            id TEXT PRIMARY KEY,
            quotation_id TEXT NOT NULL,
            product_id TEXT NOT NULL,
            quantity REAL NOT NULL,
            unit_price REAL NOT NULL,
            discount REAL DEFAULT 0.0,
            discount_type INTEGER, -- 0: Fixed, 1: Percentage
            tax_rate REAL DEFAULT 0.0,
            line_total REAL NOT NULL,
            notes TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (quotation_id) REFERENCES quotations(id),
            FOREIGN KEY (product_id) REFERENCES products(id)
        );
    )");
}

bool DatabaseInitializer::createShipmentsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS shipments (
            id TEXT PRIMARY KEY,
            shipment_number TEXT NOT NULL UNIQUE,
            sales_order_id TEXT NOT NULL,
            customer_id TEXT NOT NULL,
            shipped_by_user_id TEXT NOT NULL,
            shipment_date TEXT NOT NULL,
            delivery_date TEXT,
            type INTEGER NOT NULL, -- 0: Sales Delivery, 1: Sample Delivery, 2: Return Shipment
            status INTEGER NOT NULL, -- 0: Pending, 1: Packed, 2: Shipped, 3: Delivered, 4: Cancelled, 5: Returned
            carrier_name TEXT,
            tracking_number TEXT,
            delivery_address TEXT,
            notes TEXT,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (sales_order_id) REFERENCES sales_orders(id),
            FOREIGN KEY (customer_id) REFERENCES customers(id),
            FOREIGN KEY (shipped_by_user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createShipmentDetailsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS shipment_details (
            id TEXT PRIMARY KEY,
            shipment_id TEXT NOT NULL,
            product_id TEXT NOT NULL,
            warehouse_id TEXT NOT NULL,
            location_id TEXT NOT NULL,
            quantity REAL NOT NULL,
            lot_number TEXT,
            serial_number TEXT,
            notes TEXT,
            sales_order_item_id TEXT, -- Link to sales_order_details if applicable
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (shipment_id) REFERENCES shipments(id),
            FOREIGN KEY (product_id) REFERENCES products(id),
            FOREIGN KEY (warehouse_id) REFERENCES warehouses(id),
            FOREIGN KEY (location_id) REFERENCES locations(id)
        );
    )");
}

bool DatabaseInitializer::createReturnsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS returns (
            id TEXT PRIMARY KEY,
            return_number TEXT NOT NULL UNIQUE,
            sales_order_id TEXT NOT NULL,
            customer_id TEXT NOT NULL,
            return_date TEXT NOT NULL,
            reason TEXT,
            total_amount REAL NOT NULL,
            status INTEGER NOT NULL,
            warehouse_id TEXT,
            notes TEXT,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (sales_order_id) REFERENCES sales_orders(id),
            FOREIGN KEY (customer_id) REFERENCES customers(id),
            FOREIGN KEY (warehouse_id) REFERENCES warehouses(id)
        );
    )");
}

bool DatabaseInitializer::createReturnDetailsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS return_details (
            id TEXT PRIMARY KEY,
            return_id TEXT NOT NULL,
            product_id TEXT NOT NULL,
            quantity REAL NOT NULL,
            unit_of_measure_id TEXT NOT NULL,
            unit_price REAL NOT NULL,
            refunded_amount REAL DEFAULT 0.0,
            condition TEXT,
            notes TEXT,
            sales_order_detail_id TEXT,
            inventory_transaction_id TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (return_id) REFERENCES returns(id),
            FOREIGN KEY (product_id) REFERENCES products(id),
            FOREIGN KEY (unit_of_measure_id) REFERENCES unit_of_measures(id),
            FOREIGN KEY (sales_order_detail_id) REFERENCES sales_order_details(id),
            FOREIGN KEY (inventory_transaction_id) REFERENCES inventory_transactions(id)
        );
    )");
}


bool DatabaseInitializer::createGeneralLedgerAccountsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS general_ledger_accounts (
            id TEXT PRIMARY KEY,
            account_number TEXT NOT NULL UNIQUE,
            account_name TEXT NOT NULL,
            account_type INTEGER NOT NULL, -- 0: Asset, 1: Liability, 2: Equity, 3: Revenue, 4: Expense, etc.
            normal_balance INTEGER NOT NULL, -- 0: Debit, 1: Credit
            parent_account_id TEXT,
            description TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (parent_account_id) REFERENCES general_ledger_accounts(id)
        );
    )");
}

bool DatabaseInitializer::createGLAccountBalancesTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS gl_account_balances (
            id TEXT PRIMARY KEY,
            gl_account_id TEXT NOT NULL UNIQUE,
            current_debit_balance REAL DEFAULT 0.0,
            current_credit_balance REAL DEFAULT 0.0,
            currency TEXT NOT NULL,
            last_posted_date TEXT NOT NULL,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (gl_account_id) REFERENCES general_ledger_accounts(id)
        );
    )");
}

bool DatabaseInitializer::createJournalEntriesTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS journal_entries (
            id TEXT PRIMARY KEY,
            journal_number TEXT NOT NULL UNIQUE,
            description TEXT NOT NULL,
            entry_date TEXT NOT NULL,
            posting_date TEXT,
            reference TEXT,
            total_debit REAL NOT NULL,
            total_credit REAL NOT NULL,
            posted_by_user_id TEXT,
            is_posted INTEGER DEFAULT 0,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (posted_by_user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createJournalEntryDetailsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS journal_entry_details (
            id TEXT PRIMARY KEY,
            journal_entry_id TEXT NOT NULL,
            gl_account_id TEXT NOT NULL,
            debit_amount REAL DEFAULT 0.0,
            credit_amount REAL DEFAULT 0.0,
            notes TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (journal_entry_id) REFERENCES journal_entries(id),
            FOREIGN KEY (gl_account_id) REFERENCES general_ledger_accounts(id)
        );
    )");
}

bool DatabaseInitializer::createTaxRatesTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS tax_rates (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            rate REAL NOT NULL,
            description TEXT,
            effective_date TEXT NOT NULL,
            expiration_date TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT
        );
    )");
}

bool DatabaseInitializer::createAccountReceivableTransactionsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS accounts_receivable_transactions (
            id TEXT PRIMARY KEY,
            customer_id TEXT NOT NULL,
            type INTEGER NOT NULL, -- 0: Invoice, 1: Payment, 2: Adjustment, etc.
            amount REAL NOT NULL,
            currency TEXT NOT NULL,
            transaction_date TEXT NOT NULL,
            reference_document_id TEXT,
            reference_document_type TEXT,
            notes TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (customer_id) REFERENCES customers(id)
        );
    )");
}

bool DatabaseInitializer::createAuditLogsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS audit_logs (
            id TEXT PRIMARY KEY,
            user_id TEXT NOT NULL,
            user_name TEXT,
            session_id TEXT,
            action_type INTEGER NOT NULL,
            severity INTEGER NOT NULL,
            module TEXT NOT NULL,
            sub_module TEXT,
            entity_id TEXT,
            entity_type TEXT,
            entity_name TEXT,
            ip_address TEXT,
            user_agent TEXT,
            workstation_id TEXT,
            production_line_id TEXT,
            shift_id TEXT,
            batch_number TEXT,
            part_number TEXT,
            before_data_json TEXT, -- Stored as JSON string
            after_data_json TEXT,  -- Stored as JSON string
            change_reason TEXT,
            metadata_json TEXT,    -- Stored as JSON string
            comments TEXT,
            approval_id TEXT,
            is_compliant INTEGER DEFAULT 1,
            compliance_note TEXT,
            created_at TEXT NOT NULL,
            created_by TEXT
            -- No updated_at/by as notifications are usually immutable after creation
        );
    )");
}

bool DatabaseInitializer::createConfigurationsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS configurations (
            id TEXT PRIMARY KEY,
            config_key TEXT NOT NULL UNIQUE,
            config_value TEXT,
            description TEXT,
            is_encrypted INTEGER DEFAULT 0,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT
        );
    )");
}

bool DatabaseInitializer::createDocumentsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS documents (
            id TEXT PRIMARY KEY,
            document_type INTEGER NOT NULL,
            file_name TEXT NOT NULL,
            file_path TEXT NOT NULL,
            file_size INTEGER,
            mime_type TEXT,
            uploaded_by_user_id TEXT NOT NULL,
            upload_date TEXT NOT NULL,
            notes TEXT,
            related_entity_id TEXT,
            related_entity_type TEXT,
            metadata_json TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (uploaded_by_user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createDeviceConfigsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS device_configs (
            id TEXT PRIMARY KEY,
            device_name TEXT NOT NULL,
            device_identifier TEXT NOT NULL UNIQUE,
            type INTEGER NOT NULL,
            connection_string TEXT,
            ip_address TEXT,
            connection_status INTEGER NOT NULL,
            location_id TEXT,
            notes TEXT,
            is_critical INTEGER DEFAULT 0,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (location_id) REFERENCES locations(id)
        );
    )");
}

bool DatabaseInitializer::createDeviceEventLogsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS device_event_logs (
            id TEXT PRIMARY KEY,
            device_id TEXT NOT NULL,
            event_type INTEGER NOT NULL,
            event_time TEXT NOT NULL,
            event_description TEXT NOT NULL,
            event_data_json TEXT,
            notes TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (device_id) REFERENCES device_configs(id)
        );
    )");
}

bool DatabaseInitializer::createAPIEndpointsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS api_endpoints (
            id TEXT PRIMARY KEY,
            integration_config_id TEXT NOT NULL,
            endpoint_code TEXT NOT NULL,
            method INTEGER NOT NULL, -- 0: GET, 1: POST, etc.
            url TEXT NOT NULL,
            description TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            UNIQUE(integration_config_id, endpoint_code),
            FOREIGN KEY (integration_config_id) REFERENCES integration_configs(id)
        );
    )");
}

bool DatabaseInitializer::createIntegrationConfigsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS integration_configs (
            id TEXT PRIMARY KEY,
            system_name TEXT NOT NULL,
            system_code TEXT NOT NULL UNIQUE,
            type INTEGER NOT NULL,
            base_url TEXT,
            username TEXT,
            password TEXT,
            is_encrypted INTEGER DEFAULT 0,
            metadata_json TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT
        );
    )");
}

bool DatabaseInitializer::createProductionOrdersTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS production_orders (
            id TEXT PRIMARY KEY,
            order_number TEXT NOT NULL UNIQUE,
            product_id TEXT NOT NULL,
            planned_quantity REAL NOT NULL,
            unit_of_measure_id TEXT NOT NULL,
            bom_id TEXT, -- Link to BillOfMaterial
            production_line_id TEXT, -- Link to ProductionLine
            status INTEGER NOT NULL,
            planned_start_date TEXT NOT NULL,
            planned_end_date TEXT NOT NULL,
            actual_start_date TEXT,
            actual_end_date TEXT,
            actual_quantity_produced REAL DEFAULT 0.0,
            notes TEXT,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (product_id) REFERENCES products(id),
            FOREIGN KEY (unit_of_measure_id) REFERENCES unit_of_measures(id),
            FOREIGN KEY (bom_id) REFERENCES bill_of_materials(id),
            FOREIGN KEY (production_line_id) REFERENCES production_lines(id)
        );
    )");
}

bool DatabaseInitializer::createBillOfMaterialsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS bill_of_materials (
            id TEXT PRIMARY KEY,
            bom_name TEXT NOT NULL,
            product_id TEXT NOT NULL UNIQUE, -- Finished good product
            description TEXT,
            base_quantity REAL NOT NULL,
            base_quantity_unit_id TEXT NOT NULL,
            version INTEGER DEFAULT 1,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (product_id) REFERENCES products(id),
            FOREIGN KEY (base_quantity_unit_id) REFERENCES unit_of_measures(id)
        );
    )");
}

bool DatabaseInitializer::createBillOfMaterialItemsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS bill_of_material_items (
            id TEXT PRIMARY KEY,
            bom_id TEXT NOT NULL,
            product_id TEXT NOT NULL, -- Component product
            quantity REAL NOT NULL,
            unit_of_measure_id TEXT NOT NULL,
            notes TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (bom_id) REFERENCES bill_of_materials(id),
            FOREIGN KEY (product_id) REFERENCES products(id),
            FOREIGN KEY (unit_of_measure_id) REFERENCES unit_of_measures(id)
        );
    )");
}

bool DatabaseInitializer::createProductionLinesTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS production_lines (
            id TEXT PRIMARY KEY,
            line_name TEXT NOT NULL UNIQUE,
            description TEXT,
            location_id TEXT NOT NULL, -- Physical location of the line
            status INTEGER NOT NULL,
            associated_asset_ids_json TEXT, -- List of asset IDs (machines) in JSON string
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (location_id) REFERENCES locations(id)
        );
    )");
}

bool DatabaseInitializer::createMaintenanceRequestsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS maintenance_requests (
            id TEXT PRIMARY KEY,
            asset_id TEXT NOT NULL,
            request_type INTEGER NOT NULL, -- e.g., Preventive, Corrective, Predictive
            priority INTEGER NOT NULL,     -- e.g., Low, Normal, High, Urgent
            status INTEGER NOT NULL,       -- e.g., Pending, Scheduled, In Progress, Completed, Cancelled
            description TEXT,
            requested_by_user_id TEXT NOT NULL,
            requested_date TEXT NOT NULL,
            scheduled_date TEXT,
            assigned_to_user_id TEXT,
            failure_reason TEXT,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (asset_id) REFERENCES assets(id),
            FOREIGN KEY (requested_by_user_id) REFERENCES users(id),
            FOREIGN KEY (assigned_to_user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createMaintenanceActivitiesTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS maintenance_activities (
            id TEXT PRIMARY KEY,
            maintenance_request_id TEXT NOT NULL,
            activity_description TEXT NOT NULL,
            activity_date TEXT NOT NULL,
            performed_by_user_id TEXT NOT NULL,
            duration_hours REAL,
            cost REAL,
            cost_currency TEXT,
            parts_used TEXT,
            notes TEXT,
            status INTEGER NOT NULL,
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (maintenance_request_id) REFERENCES maintenance_requests(id),
            FOREIGN KEY (performed_by_user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createNotificationsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS notifications (
            id TEXT PRIMARY KEY,
            user_id TEXT NOT NULL, -- Recipient user
            title TEXT NOT NULL,
            message TEXT NOT NULL,
            type INTEGER NOT NULL, -- e.g., INFO, WARNING, ERROR, SUCCESS
            priority INTEGER NOT NULL, -- e.g., LOW, NORMAL, HIGH, URGENT
            sent_time TEXT NOT NULL,
            sender_id TEXT, -- User who sent the notification (can be system)
            related_entity_id TEXT, -- e.g., ID of a Sales Order, Production Order
            related_entity_type TEXT, -- e.g., "SalesOrder", "ProductionOrder"
            is_read INTEGER DEFAULT 0,
            is_public INTEGER DEFAULT 0, -- If true, visible to all users with permission
            metadata_json TEXT, -- Additional data for the notification
            status INTEGER NOT NULL, -- ACTIVE, INACTIVE, DELETED
            created_at TEXT NOT NULL,
            created_by TEXT
            -- No updated_at/by as notifications are usually immutable after creation
        );
    )");
}

bool DatabaseInitializer::createReportRequestsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS report_requests (
            id TEXT PRIMARY KEY,
            report_name TEXT NOT NULL,
            report_type TEXT NOT NULL, -- e.g., "SalesSummary", "InventoryValuation"
            frequency INTEGER NOT NULL, -- e.g., ONCE, DAILY, WEEKLY, MONTHLY
            format INTEGER NOT NULL,    -- e.g., PDF, EXCEL, CSV
            parameters_json TEXT,       -- JSON string for report parameters
            requested_by_user_id TEXT NOT NULL,
            requested_time TEXT NOT NULL,
            output_path TEXT,           -- Where report file will be saved
            schedule_cron_expression TEXT, -- For custom cron schedules
            email_recipients TEXT,      -- Comma-separated emails for delivery
            status INTEGER NOT NULL,    -- PENDING, IN_PROGRESS, COMPLETED, FAILED, CANCELLED
            metadata_json TEXT,         -- General metadata (e.g., last run status, last error message)
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (requested_by_user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createReportExecutionLogsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS report_execution_logs (
            id TEXT PRIMARY KEY,
            report_request_id TEXT NOT NULL,
            execution_time TEXT NOT NULL,
            status INTEGER NOT NULL, -- e.g., SUCCESS, FAILED, RUNNING
            executed_by_user_id TEXT,
            actual_output_path TEXT,
            error_message TEXT,
            execution_metadata_json TEXT, -- Parameters used during this execution
            log_output TEXT, -- Console output or detailed log
            created_at TEXT NOT NULL,
            created_by TEXT,
            FOREIGN KEY (report_request_id) REFERENCES report_requests(id),
            FOREIGN KEY (executed_by_user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createScheduledTasksTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS scheduled_tasks (
            id TEXT PRIMARY KEY,
            task_name TEXT NOT NULL UNIQUE,
            task_type TEXT NOT NULL, -- e.g., "ReportGeneration", "DataSync", "Backup"
            frequency INTEGER NOT NULL, -- ONCE, HOURLY, DAILY, etc.
            cron_expression TEXT, -- For custom cron schedules
            next_run_time TEXT NOT NULL,
            last_run_time TEXT,
            last_error_message TEXT,
            status INTEGER NOT NULL, -- ACTIVE, INACTIVE, SUSPENDED, COMPLETED, FAILED
            assigned_to_user_id TEXT,
            parameters_json TEXT, -- JSON string for task-specific parameters
            start_date TEXT, -- Start date for recurring tasks
            end_date TEXT,   -- End date for recurring tasks
            created_at TEXT NOT NULL,
            created_by TEXT,
            updated_at TEXT,
            updated_by TEXT,
            FOREIGN KEY (assigned_to_user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createTaskExecutionLogsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS task_execution_logs (
            id TEXT PRIMARY KEY,
            scheduled_task_id TEXT NOT NULL,
            start_time TEXT NOT NULL,
            end_time TEXT,
            status INTEGER NOT NULL, -- e.g., SUCCESS, FAILED, RUNNING, SKIPPED
            executed_by_user_id TEXT,
            log_output TEXT, -- Console output or detailed log
            error_message TEXT,
            execution_context_json TEXT, -- Parameters used for this specific run
            created_at TEXT NOT NULL,
            created_by TEXT,
            FOREIGN KEY (scheduled_task_id) REFERENCES scheduled_tasks(id),
            FOREIGN KEY (executed_by_user_id) REFERENCES users(id)
        );
    )");
}

bool DatabaseInitializer::createTaskLogsTable() {
    return executeSql(R"(
        CREATE TABLE IF NOT EXISTS task_logs (
            id TEXT PRIMARY KEY,
            task_id TEXT NOT NULL,
            log_time TEXT NOT NULL,
            log_level INTEGER NOT NULL, -- DEBUG, INFO, WARNING, ERROR, CRITICAL
            message TEXT NOT NULL,
            details TEXT,
            created_at TEXT NOT NULL,
            created_by TEXT
        );
    )");
}


} // namespace Database
} // namespace ERP