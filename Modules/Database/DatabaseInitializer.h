// Modules/Database/DatabaseInitializer.h
#ifndef MODULES_DATABASE_DATABASEINITIALIZER_H
#define MODULES_DATABASE_DATABASEINITIALIZER_H

#include <string>
#include <memory> // For std::shared_ptr

// Rút gọn include paths
#include "DatabaseConfig.h"     // DTO for database configuration
#include "DBConnection.h"       // Base DB connection interface
#include "SQLiteConnection.h"   // SQLite implementation
#include "Logger.h"             // For logging
#include "ErrorHandler.h"       // For error handling
#include "Common.h"             // For ErrorCode, DATETIME_FORMAT

// Includes for creating DTOs (used for sample data or default records)
#include "User.h"
#include "Role.h"
#include "Permission.h"
#include "Category.h"
#include "Warehouse.h"
#include "Location.h"
#include "UnitOfMeasure.h"
#include "Product.h"
#include "ProductUnitConversion.h" // For ProductUnitConversion DTO
#include "Customer.h"
#include "Supplier.h"
#include "Inventory.h" // For initial inventory records
#include "GeneralLedgerAccount.h" // For initial GL accounts
#include "TaxRate.h" // For initial tax rates
#include "AccountReceivableTransaction.h" // NEW: For AccountReceivableTransaction DTO
#include "Config.h" // For initial config settings
#include "DeviceConfig.h" // For initial device configs
#include "IntegrationConfig.h" // For initial integration configs
#include "BillOfMaterial.h" // For initial BOMs
#include "ProductionLine.h" // For initial production lines
#include "Return.h" // For Return DTO (includes ReturnDetail)
#include "PickingRequest.h" // For PickingRequest DTO (includes PickingDetail)
#include "PickingDetail.h" // Included by PickingRequest.h but good to be explicit for DAO init

// Include DAO headers for creating sample data
#include "UserDAO.h"
#include "RoleDAO.h"
#include "PermissionDAO.h"
#include "UserRoleDAO.h" // For initial data population
#include "CategoryDAO.h"
#include "WarehouseDAO.h"
#include "LocationDAO.h"
#include "UnitOfMeasureDAO.h"
#include "ProductDAO.h"
#include "ProductUnitConversionDAO.h" // For ProductUnitConversionDAO
#include "CustomerDAO.h"
#include "SupplierDAO.h"
#include "InventoryDAO.h"
#include "GeneralLedgerDAO.h"
#include "TaxRateDAO.h"
#include "AccountReceivableTransactionDAO.h" // NEW: For AccountReceivableTransactionDAO
#include "ConfigDAO.h"
#include "DeviceConfigDAO.h"
#include "APIEndpointDAO.h" // For IntegrationConfig with API endpoints
#include "IntegrationConfigDAO.h"
#include "BillOfMaterialDAO.h"
#include "ProductionLineDAO.h"
#include "ReturnDAO.h" // For ReturnDAO
#include "PickingRequestDAO.h" // For PickingRequestDAO
#include "PickingDetailDAO.h" // For PickingDetailDAO


// Include security services for password hashing/encryption
#include "PasswordHasher.h"
#include "EncryptionService.h"
#include "IAuthorizationService.h" // For permission service if creating default roles/perms

namespace ERP {
namespace Database {

/**
 * @brief The DatabaseInitializer class handles the creation and initialization
 * of the application's database schema and default data.
 */
class DatabaseInitializer {
public:
    /**
     * @brief Constructor for DatabaseInitializer.
     * @param config The database configuration.
     */
    explicit DatabaseInitializer(const DTO::DatabaseConfig& config);

    /**
     * @brief Initializes the database schema.
     * This method creates all necessary tables if they do not already exist.
     * @return True if initialization is successful, false otherwise.
     */
    bool initializeSchema();

    /**
     * @brief Populates the database with initial/sample data.
     * This should typically run only once on a fresh database.
     * @return True if data population is successful, false otherwise.
     */
    bool populateInitialData();

    /**
     * @brief Combines schema initialization and initial data population.
     * @return True if both steps are successful, false otherwise.
     */
    bool initializeDatabase();

private:
    DTO::DatabaseConfig config_;
    std::shared_ptr<DBConnection> dbConnection_; // Direct connection for initialization
    ERP::Security::Service::EncryptionService& encryptionService_ = ERP::Security::Service::EncryptionService::getInstance();

    // Helper method to execute SQL directly (without pool/DAO abstraction)
    bool executeSql(const std::string& sql);

    // Helper for creating DAO instances within initializer context
    // This is a direct instantiation as services and full security manager might not be ready yet.
    std::shared_ptr<User::DAOs::UserDAO> userDAO_;
    std::shared_ptr<Catalog::DAOs::RoleDAO> roleDAO_;
    std::shared_ptr<Catalog::DAOs::PermissionDAO> permissionDAO_;
    std::shared_ptr<Security::DAOs::UserRoleDAO> userRoleDAO_;
    std::shared_ptr<Catalog::DAOs::CategoryDAO> categoryDAO_;
    std::shared_ptr<Catalog::DAOs::WarehouseDAO> warehouseDAO_;
    std::shared_ptr<Catalog::DAOs::LocationDAO> locationDAO_;
    std::shared_ptr<Catalog::DAOs::UnitOfMeasureDAO> unitOfMeasureDAO_;
    std::shared_ptr<Product::DAOs::ProductDAO> productDAO_;
    std::shared_ptr<Product::DAOs::ProductUnitConversionDAO> productUnitConversionDAO_;
    std::shared_ptr<Customer::DAOs::CustomerDAO> customerDAO_;
    std::shared_ptr<Supplier::DAOs::SupplierDAO> supplierDAO_;
    std::shared_ptr<Warehouse::DAOs::InventoryDAO> inventoryDAO_;
    std::shared_ptr<Finance::DAOs::GeneralLedgerDAO> glDAO_;
    std::shared_ptr<Finance::DAOs::TaxRateDAO> taxRateDAO_;
    std::shared_ptr<Finance::DAOs::AccountReceivableTransactionDAO> arTransactionDAO_; // NEW: AccountReceivableTransactionDAO
    std::shared_ptr<Config::DAOs::ConfigDAO> configDAO_;
    std::shared_ptr<Integration::DAOs::DeviceConfigDAO> deviceConfigDAO_;
    std::shared_ptr<Integration::DAOs::APIEndpointDAO> apiEndpointDAO_;
    std::shared_ptr<Integration::DAOs::IntegrationConfigDAO> integrationConfigDAO_;
    std::shared_ptr<Manufacturing::DAOs::BillOfMaterialDAO> bomDAO_;
    std::shared_ptr<Manufacturing::DAOs::ProductionLineDAO> productionLineDAO_;
    std::shared_ptr<Sales::DAOs::ReturnDAO> returnDAO_;
    std::shared_ptr<Warehouse::DAOs::PickingRequestDAO> pickingRequestDAO_;
    std::shared_ptr<Warehouse::DAOs::PickingDetailDAO> pickingDetailDAO_;


    // Private helper for creating tables
    bool createUsersTable();
    bool createUserProfilesTable();
    bool createSessionsTable();
    bool createRolesTable();
    bool createPermissionsTable();
    bool createRolePermissionsTable();
    bool createUserRolesTable();
    bool createCategoriesTable();
    bool createWarehousesTable();
    bool createLocationsTable();
    bool createUnitOfMeasuresTable();
    bool createProductsTable();
    bool createProductUnitConversionsTable();
    bool createCustomersTable();
    bool createSuppliersTable();
    bool createInventoryTable();
    bool createInventoryTransactionsTable();
    bool createInventoryCostLayersTable();
    bool createPickingRequestsTable();
    bool createPickingDetailsTable();
    bool createStocktakeRequestsTable();
    bool createStocktakeDetailsTable();
    bool createReceiptSlipsTable();
    bool createReceiptSlipDetailsTable();
    bool createIssueSlipsTable();
    bool createIssueSlipDetailsTable();
    bool createMaterialRequestSlipsTable();
    bool createMaterialRequestSlipDetailsTable();
    bool createMaterialIssueSlipsTable();
    bool createMaterialIssueSlipDetailsTable();
    bool createSalesOrdersTable();
    bool createSalesOrderDetailsTable();
    bool createInvoicesTable();
    bool createInvoiceDetailsTable();
    bool createPaymentsTable();
    bool createQuotationsTable();
    bool createQuotationDetailsTable();
    bool createShipmentsTable();
    bool createShipmentDetailsTable();
    bool createReturnsTable();
    bool createReturnDetailsTable();
    bool createGeneralLedgerAccountsTable();
    bool createGLAccountBalancesTable();
    bool createJournalEntriesTable();
    bool createJournalEntryDetailsTable();
    bool createTaxRatesTable();
    bool createAccountReceivableTransactionsTable(); // NEW: Declare new table creation function
    bool createAuditLogsTable();
    bool createConfigurationsTable();
    bool createDocumentsTable();
    bool createDeviceConfigsTable();
    bool createDeviceEventLogsTable();
    bool createAPIEndpointsTable();
    bool createIntegrationConfigsTable();
    bool createProductionOrdersTable();
    bool createBillOfMaterialsTable();
    bool createBillOfMaterialItemsTable();
    bool createProductionLinesTable();
    bool createMaintenanceRequestsTable();
    bool createMaintenanceActivitiesTable();
    bool createNotificationsTable();
    bool createReportRequestsTable();
    bool createReportExecutionLogsTable();
    bool createScheduledTasksTable();
    bool createTaskExecutionLogsTable();
    bool createTaskLogsTable();
};

} // namespace Database
} // namespace ERP

#endif // MODULES_DATABASE_DATABASEINITIALIZER_H