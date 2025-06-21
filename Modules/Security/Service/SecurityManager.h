// Modules/Security/Service/SecurityManager.h
#ifndef MODULES_SECURITY_SERVICE_SECURITYMANAGER_H
#define MODULES_SECURITY_SERVICE_SECURITYMANAGER_H
#include <string>
#include <vector>
#include <memory> // For shared_ptr

#include "ISecurityManager.h"       // Đã rút gọn include
#include "AuthenticationService.h"  // Đã rút gọn include
#include "AuthorizationService.h"   // Đã rút gọn include
#include "AuditLogService.h"        // Đã rút gọn include
#include "EncryptionService.h"      // Đã rút gọn include

// Forward declarations of all services
// (These are the actual implementations, not just interfaces, so they can be stored as shared_ptr)
namespace ERP { namespace User { namespace Services { class IUserService; }}}
namespace ERP { namespace Catalog { namespace Services { class ICategoryService; class ILocationService; class IUnitOfMeasureService; class IWarehouseService; class IRoleService; class IPermissionService; }}}
namespace ERP { namespace Asset { namespace Services { class IAssetManagementService; }}}
namespace ERP { namespace Config { namespace Services { class IConfigService; }}}
namespace ERP { namespace Customer { namespace Services { class ICustomerService; }}}
namespace ERP { namespace Document { namespace Services { class IDocumentService; }}}
namespace ERP { namespace Finance { namespace Services { class IAccountReceivableService; class IGeneralLedgerService; class ITaxService; }}}
namespace ERP { namespace Integration { namespace Services { class IDeviceManagerService; class IExternalSystemService; }}}
namespace ERP { namespace Manufacturing { namespace Services { class IBillOfMaterialService; class IMaintenanceManagementService; class IProductionLineService; class IProductionOrderService; }}}
namespace ERP { namespace Material { namespace Services { class IIssueSlipService; class IMaterialIssueSlipService; class IMaterialRequestSlipService; class IReceiptSlipService; }}}
namespace ERP { namespace Notification { namespace Services { class INotificationService; }}}
namespace ERP { namespace Product { namespace Services { class IProductService; }}}
namespace ERP { namespace Report { namespace Services { class IReportService; }}}
namespace ERP { namespace Sales { namespace Services { class IInvoiceService; class IPaymentService; class IQuotationService; class ISalesOrderService; class IShipmentService; }}}
namespace ERP { namespace Scheduler { namespace Services { class IScheduledTaskService; class ITaskExecutionLogService; }}}
namespace ERP { namespace Supplier { namespace Services { class ISupplierService; }}}
namespace ERP { namespace TaskEngine { namespace Services { class ITaskExecutorService; }}}
namespace ERP { namespace Warehouse { namespace Services { class IInventoryManagementService; class IPickingService; class IStocktakeService; }}}


namespace ERP {
namespace Security {
/**
 * @brief Default implementation of ISecurityManager.
 * This class acts as a facade for all security-related services (Authentication, Authorization, Audit Logging)
 * and provides access to other core services that need to interact with the security context.
 */
class SecurityManager : public ISecurityManager {
public:
    /**
     * @brief Constructor for SecurityManager.
     * Takes shared pointers to all core services it manages or provides access to.
     * @param authenticationService Shared pointer to AuthenticationService.
     * @param authorizationService Shared pointer to AuthorizationService.
     * @param auditLogService Shared pointer to AuditLogService.
     * @param userService Shared pointer to IUserService.
     * // ... (and all other services as shared_ptr to their interfaces)
     */
    SecurityManager(
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
        std::shared_ptr<ERP::Integration::Services::IDeviceManagerService> deviceManagerService,
        std::shared_ptr<ERP::Integration::Services::IExternalSystemService> externalSystemService,
        std::shared_ptr<ERP::Manufacturing::Services::IBillOfMaterialService> billOfMaterialService,
        std::shared_ptr<ERP::Manufacturing::Services::IMaintenanceManagementService> maintenanceManagementService,
        std::shared_ptr<ERP::Manufacturing::Services::IProductionLineService> productionLineService,
        std::shared_ptr<ERP::Manufacturing::Services::IProductionOrderService> productionOrderService,
        std::shared_ptr<ERP::Material::Services::IIssueSlipService> issueSlipService,
        std::shared_ptr<ERP::Material::Services::IMaterialIssueSlipService> materialIssueSlipService,
        std::shared_ptr<ERP::Material::Services::IMaterialRequestSlipService> materialRequestSlipService,
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
        std::shared_ptr<ERP::TaskEngine::Services::ITaskExecutorService> taskExecutorService,
        std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService,
        std::shared_ptr<ERP::Warehouse::Services::IPickingService> pickingService,
        std::shared_ptr<ERP::Warehouse::Services::IStocktakeService> stocktakeService
    );


    ~SecurityManager() override = default;

    std::shared_ptr<Service::IAuthenticationService> getAuthenticationService() const override;
    std::shared_ptr<Service::IAuthorizationService> getAuthorizationService() const override;
    std::shared_ptr<Service::IAuditLogService> getAuditLogService() const override;
    Service::EncryptionService& getEncryptionService() const override;

    bool hasPermission(
        const std::string& userId,
        const std::vector<std::string>& userRoleIds,
        const std::string& permissionName) const override;
    
    // Getters for other services
    std::shared_ptr<ERP::User::Services::IUserService> getUserService() const override;
    std::shared_ptr<ERP::Catalog::Services::ICategoryService> getCategoryService() const override;
    std::shared_ptr<ERP::Catalog::Services::ILocationService> getLocationService() const override;
    std::shared_ptr<ERP::Catalog::Services::IUnitOfMeasureService> getUnitOfMeasureService() const override;
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> getWarehouseService() const override;
    std::shared_ptr<ERP::Catalog::Services::IRoleService> getRoleService() const override;
    std::shared_ptr<ERP::Catalog::Services::IPermissionService> getPermissionService() const override;
    std::shared_ptr<ERP::Asset::Services::IAssetManagementService> getAssetManagementService() const override;
    std::shared_ptr<ERP::Config::Services::IConfigService> getConfigService() const override;
    std::shared_ptr<ERP::Customer::Services::ICustomerService> getCustomerService() const override;
    std::shared_ptr<ERP::Document::Services::IDocumentService> getDocumentService() const override;
    std::shared_ptr<ERP::Finance::Services::IAccountReceivableService> getAccountReceivableService() const override;
    std::shared_ptr<ERP::Finance::Services::IGeneralLedgerService> getGeneralLedgerService() const override;
    std::shared_ptr<ERP::Finance::Services::ITaxService> getTaxService() const override;
    std::shared_ptr<ERP::Integration::Services::IDeviceManagerService> getDeviceManagerService() const override;
    std::shared_ptr<ERP::Integration::Services::IExternalSystemService> getExternalSystemService() const override;
    std::shared_ptr<ERP::Manufacturing::Services::IBillOfMaterialService> getBillOfMaterialService() const override;
    std::shared_ptr<ERP::Manufacturing::Services::IMaintenanceManagementService> getMaintenanceManagementService() const override;
    std::shared_ptr<ERP::Manufacturing::Services::IProductionLineService> getProductionLineService() const override;
    std::shared_ptr<ERP::Manufacturing::Services::IProductionOrderService> getProductionOrderService() const override;
    std::shared_ptr<ERP::Material::Services::IIssueSlipService> getIssueSlipService() const override;
    std::shared_ptr<ERP::Material::Services::IMaterialIssueSlipService> getMaterialIssueSlipService() const override;
    std::shared_ptr<ERP::Material::Services::IMaterialRequestSlipService> getMaterialRequestSlipService() const override;
    std::shared_ptr<ERP::Material::Services::IReceiptSlipService> getReceiptSlipService() const override;
    std::shared_ptr<ERP::Notification::Services::INotificationService> getNotificationService() const override;
    std::shared_ptr<ERP::Product::Services::IProductService> getProductService() const override;
    std::shared_ptr<ERP::Report::Services::IReportService> getReportService() const override;
    std::shared_ptr<ERP::Sales::Services::IInvoiceService> getInvoiceService() const override;
    std::shared_ptr<ERP::Sales::Services::IPaymentService> getPaymentService() const override;
    std::shared_ptr<ERP::Sales::Services::IQuotationService> getQuotationService() const override;
    std::shared_ptr<ERP::Sales::Services::ISalesOrderService> getSalesOrderService() const override;
    std::shared_ptr<ERP::Sales::Services::IShipmentService> getShipmentService() const override;
    std::shared_ptr<ERP::Scheduler::Services::IScheduledTaskService> getScheduledTaskService() const override;
    std::shared_ptr<ERP::Scheduler::Services::ITaskExecutionLogService> getTaskExecutionLogService() const override;
    std::shared_ptr<ERP::Supplier::Services::ISupplierService> getSupplierService() const override;
    std::shared_ptr<ERP::TaskEngine::Services::ITaskExecutorService> getTaskExecutorService() const override;
    std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> getInventoryManagementService() const override;
    std::shared_ptr<ERP::Warehouse::Services::IPickingService> getPickingService() const override;
    std::shared_ptr<ERP::Warehouse::Services::IStocktakeService> getStocktakeService() const override;


private:
    std::shared_ptr<Service::IAuthenticationService> authenticationService_;
    std::shared_ptr<Service::IAuthorizationService> authorizationService_;
    std::shared_ptr<Service::IAuditLogService> auditLogService_;

    // Shared Pointers to all other services (as interfaces)
    std::shared_ptr<ERP::User::Services::IUserService> userService_;
    std::shared_ptr<ERP::Catalog::Services::ICategoryService> categoryService_;
    std::shared_ptr<ERP::Catalog::Services::ILocationService> locationService_;
    std::shared_ptr<ERP::Catalog::Services::IUnitOfMeasureService> unitOfMeasureService_;
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService_;
    std::shared_ptr<ERP::Catalog::Services::IRoleService> roleService_;
    std::shared_ptr<ERP::Catalog::Services::IPermissionService> permissionService_;
    std::shared_ptr<ERP::Asset::Services::IAssetManagementService> assetManagementService_;
    std::shared_ptr<ERP::Config::Services::IConfigService> configService_;
    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService_;
    std::shared_ptr<ERP::Document::Services::IDocumentService> documentService_;
    std::shared_ptr<ERP::Finance::Services::IAccountReceivableService> accountReceivableService_;
    std::shared_ptr<ERP::Finance::Services::IGeneralLedgerService> generalLedgerService_;
    std::shared_ptr<ERP::Finance::Services::ITaxService> taxService_;
    std::shared_ptr<ERP::Integration::Services::IDeviceManagerService> deviceManagerService_;
    std::shared_ptr<ERP::Integration::Services::IExternalSystemService> externalSystemService_;
    std::shared_ptr<ERP::Manufacturing::Services::IBillOfMaterialService> billOfMaterialService_;
    std::shared_ptr<ERP::Manufacturing::Services::IMaintenanceManagementService> maintenanceManagementService_;
    std::shared_ptr<ERP::Manufacturing::Services::IProductionLineService> productionLineService_;
    std::shared_ptr<ERP::Manufacturing::Services::IProductionOrderService> productionOrderService_;
    std::shared_ptr<ERP::Material::Services::IIssueSlipService> issueSlipService_;
    std::shared_ptr<ERP::Material::Services::IMaterialIssueSlipService> materialIssueSlipService_;
    std::shared_ptr<ERP::Material::Services::IMaterialRequestSlipService> materialRequestSlipService_;
    std::shared_ptr<ERP::Material::Services::IReceiptSlipService> receiptSlipService_;
    std::shared_ptr<ERP::Notification::Services::INotificationService> notificationService_;
    std::shared_ptr<ERP::Product::Services::IProductService> productService_;
    std::shared_ptr<ERP::Report::Services::IReportService> reportService_;
    std::shared_ptr<ERP::Sales::Services::IInvoiceService> invoiceService_;
    std::shared_ptr<ERP::Sales::Services::IPaymentService> paymentService_;
    std::shared_ptr<ERP::Sales::Services::IQuotationService> quotationService_;
    std::shared_ptr<ERP::Sales::Services::ISalesOrderService> salesOrderService_;
    std::shared_ptr<ERP::Sales::Services::IShipmentService> shipmentService_;
    std::shared_ptr<ERP::Scheduler::Services::IScheduledTaskService> scheduledTaskService_;
    std::shared_ptr<ERP::Scheduler::Services::ITaskExecutionLogService> taskExecutionLogService_;
    std::shared_ptr<ERP::Supplier::Services::ISupplierService> supplierService_;
    std::shared_ptr<ERP::TaskEngine::Services::ITaskExecutorService> taskExecutorService_;
    std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService_;
    std::shared_ptr<ERP::Warehouse::Services::IPickingService> pickingService_;
    std::shared_ptr<ERP::Warehouse::Services::IStocktakeService> stocktakeService_;
};
} // namespace Security
} // namespace ERP
#endif // MODULES_SECURITY_SERVICE_SECURITYMANAGER_H