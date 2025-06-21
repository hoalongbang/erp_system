// Modules/Security/Service/SecurityManager.cpp
#include "SecurityManager.h" // Standard includes
#include "Logger.h"
#include "ErrorHandler.h"
// Removed Qt includes as they are no longer needed here
// #include <QJsonObject>
// #include <QJsonDocument>

// Includes for all other service interfaces/implementations that SecurityManager provides
// (These are typically included in main.cpp during setup, but SecurityManager itself
//  needs the interfaces for its members)
#include "UserService.h"
#include "CategoryService.h"
#include "LocationService.h"
#include "UnitOfMeasureService.h"
#include "WarehouseService.h"
#include "RoleService.h"
#include "PermissionService.h"
#include "AssetManagementService.h"
#include "ConfigService.h"
#include "CustomerService.h"
#include "DocumentService.h"
#include "AccountReceivableService.h"
#include "GeneralLedgerService.h"
#include "TaxService.h"
#include "DeviceManagerService.h"
#include "ExternalSystemService.h"
#include "BillOfMaterialService.h"
#include "MaintenanceManagementService.h"
#include "ProductionLineService.h"
#include "ProductionOrderService.h"
#include "IssueSlipService.h"
#include "MaterialIssueSlipService.h"
#include "MaterialRequestService.h"
#include "ReceiptSlipService.h"
#include "NotificationService.h"
#include "ProductService.h"
#include "ReportService.h"
#include "SalesInvoiceService.h"
#include "SalesPaymentService.h"
#include "SalesQuotationService.h"
#include "SalesOrderService.h"
#include "SalesShipmentService.h"
#include "ScheduledTaskService.h"
#include "TaskExecutionLogService.h"
#include "SupplierService.h"
#include "TaskEngine.h" // For TaskExecutorService (singleton)
#include "InventoryManagementService.h"
#include "PickingService.h"
#include "StocktakeService.h"


namespace ERP {
namespace Security {

SecurityManager::SecurityManager(
    std::shared_ptr<Service::IAuthenticationService> authenticationService,
    std::shared_ptr<Service::IAuthorizationService> authorizationService,
    std::shared_ptr<Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::User::Services::IUserService> userService,
    std::shared_ptr<ERP::Catalog::Services::ICategoryService> categoryService,
    std::shared_ptr<ERP::Catalog::Services::ILocationService> locationService,
    std::shared_ptr<ERP::Catalog::Services::IUnitOfMeasureService> unitOfMeasureService,
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
    std::shared_ptr<ERP::Catalog::Services::IRoleService> roleService,
    std::shared_ptr<ERP::Catalog::Services::IPermissionService> permissionService,
    std::shared_ptr<ERP::Asset::Services::IAssetManagementService> assetManagementService,
    std::shared_ptr<ERP::Config::Services::IConfigService> configService,
    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService,
    std::shared_ptr<ERP::Document::Services::IDocumentService> documentService,
    std::shared_ptr<ERP::Finance::Services::IAccountReceivableService> accountReceivableService,
    std::shared_ptr<ERP::Finance::Services::IGeneralLedgerService> generalLedgerService,
    std::shared_ptr<ERP::Finance::Services::ITaxService> taxService,
    std::shared_ptr<ERP::Integration::Services::IDeviceManagerService> deviceManagerService, // New
    std::shared_ptr<ERP::Integration::Services::IExternalSystemService> externalSystemService, // New
    std::shared_ptr<ERP::Manufacturing::Services::IBillOfMaterialService> billOfMaterialService,
    std::shared_ptr<ERP::Manufacturing::Services::IMaintenanceManagementService> maintenanceManagementService,
    std::shared_ptr<ERP::Manufacturing::Services::IProductionLineService> productionLineService,
    std::shared_ptr<ERP::Manufacturing::Services::IProductionOrderService> productionOrderService,
    std::shared_ptr<ERP::Material::Services::IIssueSlipService> issueSlipService,
    std::shared_ptr<ERP::Material::Services::IMaterialIssueSlipService> materialIssueSlipService,
    std::shared_ptr<ERP::Material::Services::IMaterialRequestService> materialRequestService, // Corrected type
    std::shared_ptr<ERP::Material::Services::IReceiptSlipService> receiptSlipService,
    std::shared_ptr<ERP::Notification::Services::INotificationService> notificationService,
    std::shared_ptr<ERP::Product::Services::IProductService> productService,
    std::shared_ptr<ERP::Report::Services::IReportService> reportService,
    std::shared_ptr<ERP::Sales::Services::IInvoiceService> invoiceService,
    std::shared_ptr<ERP::Sales::Services::IPaymentService> paymentService,
    std::shared_ptr<ERP::Sales::Services::IQuotationService> quotationService,
    std::shared_ptr<ERP::Sales::Services::ISalesOrderService> salesOrderService,
    std::shared_ptr<ERP::Sales::Services::IShipmentService> shipmentService,
    std::shared_ptr<ERP::Scheduler::Services::IScheduledTaskService> scheduledTaskService,
    std::shared_ptr<ERP::Scheduler::Services::ITaskExecutionLogService> taskExecutionLogService,
    std::shared_ptr<ERP::Supplier::Services::ISupplierService> supplierService,
    std::shared_ptr<ERP::TaskEngine::Services::ITaskExecutorService> taskExecutorService, // Singleton
    std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService,
    std::shared_ptr<ERP::Warehouse::Services::IPickingService> pickingService,
    std::shared_ptr<ERP::Warehouse::Services::IStocktakeService> stocktakeService
    ) :
    authenticationService_(authenticationService),
    authorizationService_(authorizationService),
    auditLogService_(auditLogService),
    userService_(userService),
    categoryService_(categoryService),
    locationService_(locationService),
    unitOfMeasureService_(unitOfMeasureService),
    warehouseService_(warehouseService),
    roleService_(roleService),
    permissionService_(permissionService),
    assetManagementService_(assetManagementService),
    configService_(configService),
    customerService_(customerService),
    documentService_(documentService),
    accountReceivableService_(accountReceivableService),
    generalLedgerService_(generalLedgerService),
    taxService_(taxService),
    deviceManagerService_(deviceManagerService), // New
    externalSystemService_(externalSystemService), // New
    billOfMaterialService_(billOfMaterialService),
    maintenanceManagementService_(maintenanceManagementService),
    productionLineService_(productionLineService),
    productionOrderService_(productionOrderService),
    issueSlipService_(issueSlipService),
    materialIssueSlipService_(materialIssueSlipService),
    materialRequestService_(materialRequestService), // Corrected assignment
    receiptSlipService_(receiptSlipService),
    notificationService_(notificationService),
    productService_(productService),
    reportService_(reportService),
    invoiceService_(invoiceService),
    paymentService_(paymentService),
    quotationService_(quotationService),
    salesOrderService_(salesOrderService),
    shipmentService_(shipmentService),
    scheduledTaskService_(scheduledTaskService),
    taskExecutionLogService_(taskExecutionLogService),
    supplierService_(supplierService),
    taskExecutorService_(taskExecutorService),
    inventoryManagementService_(inventoryManagementService),
    pickingService_(pickingService),
    stocktakeService_(stocktakeService)
{
    // Basic null checks for the most critical services
    if (!authenticationService_ || !authorizationService_ || !auditLogService_) {
        ERP::Logger::Logger::getInstance().critical("SecurityManager", "One or more core security services are null.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "SecurityManager: Null core security services.", "Lỗi hệ thống: Dịch vụ bảo mật không khả dụng.");
        throw std::runtime_error("SecurityManager: Null core security services.");
    }
    ERP::Logger::Logger::getInstance().info("SecurityManager: Initialized. Security services ready.");
}

std::shared_ptr<Service::IAuthenticationService> SecurityManager::getAuthenticationService() const {
    return authenticationService_;
}

std::shared_ptr<Service::IAuthorizationService> SecurityManager::getAuthorizationService() const {
    return authorizationService_;
}

std::shared_ptr<Service::IAuditLogService> SecurityManager::getAuditLogService() const {
    return auditLogService_;
}

Service::EncryptionService& SecurityManager::getEncryptionService() const {
    return Service::EncryptionService::getInstance(); // EncryptionService is a singleton
}

bool SecurityManager::hasPermission(
    const std::string& userId,
    const std::vector<std::string>& userRoleIds,
    const std::string& permissionName) const {
    if (!authorizationService_) {
        ERP::Logger::Logger::getInstance().critical("SecurityManager", "AuthorizationService is null. Cannot perform permission check.");
        return false;
    }
    // Record audit log for permission check (optional, can be very verbose)
    // recordAuditLog(userId, "N/A", "N/A", ERP::Security::DTO::AuditActionType::VIEW, ERP::Common::LogSeverity::DEBUG,
    //                "Security", "PermissionCheck", std::nullopt, "Permission", permissionName,
    //                std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
    //                std::nullopt, std::nullopt, "Checking permission: " + permissionName, {}, std::nullopt, std::nullopt,
    //                authorizationService_->hasPermission(userId, userRoleIds, permissionName), "Permission check result.");

    return authorizationService_->hasPermission(userId, userRoleIds, permissionName);
}

// Implementations of all other getters
std::shared_ptr<ERP::User::Services::IUserService> SecurityManager::getUserService() const { return userService_; }
std::shared_ptr<ERP::Catalog::Services::ICategoryService> SecurityManager::getCategoryService() const { return categoryService_; }
std::shared_ptr<ERP::Catalog::Services::ILocationService> SecurityManager::getLocationService() const { return locationService_; }
std::shared_ptr<ERP::Catalog::Services::IUnitOfMeasureService> SecurityManager::getUnitOfMeasureService() const { return unitOfMeasureService_; }
std::shared_ptr<ERP::Catalog::Services::IWarehouseService> SecurityManager::getWarehouseService() const { return warehouseService_; }
std::shared_ptr<ERP::Catalog::Services::IRoleService> SecurityManager::getRoleService() const { return roleService_; }
std::shared_ptr<ERP::Catalog::Services::IPermissionService> SecurityManager::getPermissionService() const { return permissionService_; }
std::shared_ptr<ERP::Asset::Services::IAssetManagementService> SecurityManager::getAssetManagementService() const { return assetManagementService_; }
std::shared_ptr<ERP::Config::Services::IConfigService> SecurityManager::getConfigService() const { return configService_; }
std::shared_ptr<ERP::Customer::Services::ICustomerService> SecurityManager::getCustomerService() const { return customerService_; }
std::shared_ptr<ERP::Document::Services::IDocumentService> SecurityManager::getDocumentService() const { return documentService_; }
std::shared_ptr<ERP::Finance::Services::IAccountReceivableService> SecurityManager::getAccountReceivableService() const { return accountReceivableService_; }
std::shared_ptr<ERP::Finance::Services::IGeneralLedgerService> SecurityManager::getGeneralLedgerService() const { return generalLedgerService_; }
std::shared_ptr<ERP::Finance::Services::ITaxService> SecurityManager::getTaxService() const { return taxService_; }
std::shared_ptr<ERP::Integration::Services::IDeviceManagerService> SecurityManager::getDeviceManagerService() const { return deviceManagerService_; }
std::shared_ptr<ERP::Integration::Services::IExternalSystemService> SecurityManager::getExternalSystemService() const { return externalSystemService_; }
std::shared_ptr<ERP::Manufacturing::Services::IBillOfMaterialService> SecurityManager::getBillOfMaterialService() const { return billOfMaterialService_; }
std::shared_ptr<ERP::Manufacturing::Services::IMaintenanceManagementService> SecurityManager::getMaintenanceManagementService() const { return maintenanceManagementService_; }
std::shared_ptr<ERP::Manufacturing::Services::IProductionLineService> SecurityManager::getProductionLineService() const { return productionLineService_; }
std::shared_ptr<ERP::Manufacturing::Services::IProductionOrderService> SecurityManager::getProductionOrderService() const { return productionOrderService_; }
std::shared_ptr<ERP::Material::Services::IIssueSlipService> SecurityManager::getIssueSlipService() const { return issueSlipService_; }
std::shared_ptr<ERP::Material::Services::IMaterialIssueSlipService> SecurityManager::getMaterialIssueSlipService() const { return materialIssueSlipService_; }
std::shared_ptr<ERP::Material::Services::IMaterialRequestService> SecurityManager::getMaterialRequestService() const { return materialRequestService_; }
std::shared_ptr<ERP::Material::Services::IReceiptSlipService> SecurityManager::getReceiptSlipService() const { return receiptSlipService_; }
std::shared_ptr<ERP::Notification::Services::INotificationService> SecurityManager::getNotificationService() const { return notificationService_; }
std::shared_ptr<ERP::Product::Services::IProductService> SecurityManager::getProductService() const { return productService_; }
std::shared_ptr<ERP::Report::Services::IReportService> SecurityManager::getReportService() const { return reportService_; }
std::shared_ptr<ERP::Sales::Services::IInvoiceService> SecurityManager::getInvoiceService() const { return invoiceService_; }
std::shared_ptr<ERP::Sales::Services::IPaymentService> SecurityManager::getPaymentService() const { return paymentService_; }
std::shared_ptr<ERP::Sales::Services::IQuotationService> SecurityManager::getQuotationService() const { return quotationService_; }
std::shared_ptr<ERP::Sales::Services::ISalesOrderService> SecurityManager::getSalesOrderService() const { return salesOrderService_; }
std::shared_ptr<ERP::Sales::Services::IShipmentService> SecurityManager::getShipmentService() const { return shipmentService_; }
std::shared_ptr<ERP::Scheduler::Services::IScheduledTaskService> SecurityManager::getScheduledTaskService() const { return scheduledTaskService_; }
std::shared_ptr<ERP::Scheduler::Services::ITaskExecutionLogService> SecurityManager::getTaskExecutionLogService() const { return taskExecutionLogService_; }
std::shared_ptr<ERP::Supplier::Services::ISupplierService> SecurityManager::getSupplierService() const { return supplierService_; }
std::shared_ptr<ERP::TaskEngine::Services::ITaskExecutorService> SecurityManager::getTaskExecutorService() const { return taskExecutorService_; }
std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> SecurityManager::getInventoryManagementService() const { return inventoryManagementService_; }
std::shared_ptr<ERP::Warehouse::Services::IPickingService> SecurityManager::getPickingService() const { return pickingService_; }
std::shared_ptr<ERP::Warehouse::Services::IStocktakeService> SecurityManager::getStocktakeService() const { return stocktakeService_; }

} // namespace Security
} // namespace ERP