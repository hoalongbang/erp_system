// Modules/Security/Service/ISecurityManager.h
#ifndef MODULES_SECURITY_SERVICE_ISECURITYMANAGER_H
#define MODULES_SECURITY_SERVICE_ISECURITYMANAGER_H
#include <string>
#include <vector>
#include <memory> // For shared_ptr

// Rút gọn các include paths cho interfaces
#include "IAuthenticationService.h"
#include "IAuthorizationService.h"
#include "IAuditLogService.h"
#include "EncryptionService.h" // EncryptionService is a Singleton, so direct class reference is fine.

// Forward declarations of other service interfaces managed by SecurityManager
// Only declare interfaces here, not concrete classes.
namespace ERP { namespace User { namespace Services { class IUserService; }}}
namespace ERP { namespace Catalog { namespace Services { class ICategoryService; class ILocationService; class IUnitOfMeasureService; class IWarehouseService; class IRoleService; class IPermissionService; }}}
namespace ERP { namespace Asset { namespace Services { class IAssetManagementService; }}}
namespace ERP { namespace Config { namespace Services { class IConfigService; }}}
namespace ERP { namespace Customer { namespace Services { class ICustomerService; }}}
namespace ERP { namespace Document { namespace Services { class IDocumentService; }}}
namespace ERP { namespace Finance { namespace Services { class IAccountReceivableService; class IGeneralLedgerService; class ITaxService; }}}
namespace ERP { namespace Integration { namespace Services { class IDeviceManagerService; class IExternalSystemService; }}}
namespace ERP { namespace Manufacturing { namespace Services { class IBillOfMaterialService; class IMaintenanceManagementService; class IProductionLineService; class IProductionOrderService; }}}
namespace ERP { namespace Material { namespace Services { class IIssueSlipService; class IMaterialIssueSlipService; class IMaterialRequestService; class IReceiptSlipService; }}}
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
 * @brief ISecurityManager interface provides a facade for all security-related services.
 * This acts as a central point to access authentication, authorization, and audit logging.
 */
class ISecurityManager {
public:
    virtual ~ISecurityManager() = default;

    virtual std::shared_ptr<Service::IAuthenticationService> getAuthenticationService() const = 0;
    virtual std::shared_ptr<Service::IAuthorizationService> getAuthorizationService() const = 0;
    virtual std::shared_ptr<Service::IAuditLogService> getAuditLogService() const = 0;
    virtual Service::EncryptionService& getEncryptionService() const = 0; // EncryptionService is a Singleton

    // Helper methods to directly check permissions (delegating to AuthorizationService)
    virtual bool hasPermission(
        const std::string& userId,
        const std::vector<std::string>& userRoleIds,
        const std::string& permissionName) const = 0;
    
    // Add getters for other services, especially those that need security context
    // and are commonly injected into other services through SecurityManager.
    // These are often needed for internal service-to-service calls that still need permission checks.
    virtual std::shared_ptr<ERP::User::Services::IUserService> getUserService() const = 0;
    virtual std::shared_ptr<ERP::Catalog::Services::ICategoryService> getCategoryService() const = 0;
    virtual std::shared_ptr<ERP::Catalog::Services::ILocationService> getLocationService() const = 0;
    virtual std::shared_ptr<ERP::Catalog::Services::IUnitOfMeasureService> getUnitOfMeasureService() const = 0;
    virtual std::shared_ptr<ERP::Catalog::Services::IWarehouseService> getWarehouseService() const = 0;
    virtual std::shared_ptr<ERP::Catalog::Services::IRoleService> getRoleService() const = 0;
    virtual std::shared_ptr<ERP::Catalog::Services::IPermissionService> getPermissionService() const = 0;
    virtual std::shared_ptr<ERP::Asset::Services::IAssetManagementService> getAssetManagementService() const = 0;
    virtual std::shared_ptr<ERP::Config::Services::IConfigService> getConfigService() const = 0;
    virtual std::shared_ptr<ERP::Customer::Services::ICustomerService> getCustomerService() const = 0;
    virtual std::shared_ptr<ERP::Document::Services::IDocumentService> getDocumentService() const = 0;
    virtual std::shared_ptr<ERP::Finance::Services::IAccountReceivableService> getAccountReceivableService() const = 0;
    virtual std::shared_ptr<ERP::Finance::Services::IGeneralLedgerService> getGeneralLedgerService() const = 0;
    virtual std::shared_ptr<ERP::Finance::Services::ITaxService> getTaxService() const = 0;
    virtual std::shared_ptr<ERP::Integration::Services::IDeviceManagerService> getDeviceManagerService() const = 0;
    virtual std::shared_ptr<ERP::Integration::Services::IExternalSystemService> getExternalSystemService() const = 0;
    virtual std::shared_ptr<ERP::Manufacturing::Services::IBillOfMaterialService> getBillOfMaterialService() const = 0;
    virtual std::shared_ptr<ERP::Manufacturing::Services::IMaintenanceManagementService> getMaintenanceManagementService() const = 0;
    virtual std::shared_ptr<ERP::Manufacturing::Services::IProductionLineService> getProductionLineService() const = 0;
    virtual std::shared_ptr<ERP::Manufacturing::Services::IProductionOrderService> getProductionOrderService() const = 0;
    virtual std::shared_ptr<ERP::Material::Services::IIssueSlipService> getIssueSlipService() const = 0;
    virtual std::shared_ptr<ERP::Material::Services::IMaterialIssueSlipService> getMaterialIssueSlipService() const = 0;
    virtual std::shared_ptr<ERP::Material::Services::IMaterialRequestService> getMaterialRequestService() const = 0;
    virtual std::shared_ptr<ERP::Material::Services::IReceiptSlipService> getReceiptSlipService() const = 0;
    virtual std::shared_ptr<ERP::Notification::Services::INotificationService> getNotificationService() const = 0;
    virtual std::shared_ptr<ERP::Product::Services::IProductService> getProductService() const = 0;
    virtual std::shared_ptr<ERP::Report::Services::IReportService> getReportService() const = 0;
    virtual std::shared_ptr<ERP::Sales::Services::IInvoiceService> getInvoiceService() const = 0;
    virtual std::shared_ptr<ERP::Sales::Services::IPaymentService> getPaymentService() const = 0;
    virtual std::shared_ptr<ERP::Sales::Services::IQuotationService> getQuotationService() const = 0;
    virtual std::shared_ptr<ERP::Sales::Services::ISalesOrderService> getSalesOrderService() const = 0;
    virtual std::shared_ptr<ERP::Sales::Services::IShipmentService> getShipmentService() const = 0;
    virtual std::shared_ptr<ERP::Scheduler::Services::IScheduledTaskService> getScheduledTaskService() const = 0;
    virtual std::shared_ptr<ERP::Scheduler::Services::ITaskExecutionLogService> getTaskExecutionLogService() const = 0;
    virtual std::shared_ptr<ERP::Supplier::Services::ISupplierService> getSupplierService() const = 0;
    virtual std::shared_ptr<ERP::TaskEngine::Services::ITaskExecutorService> getTaskExecutorService() const = 0;
    virtual std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> getInventoryManagementService() const = 0;
    virtual std::shared_ptr<ERP::Warehouse::Services::IPickingService> getPickingService() const = 0;
    virtual std::shared_ptr<ERP::Warehouse::Services::IStocktakeService> getStocktakeService() const = 0;
};
} // namespace Security
} // namespace ERP
#endif // MODULES_SECURITY_SERVICE_ISECURITYMANAGER_H