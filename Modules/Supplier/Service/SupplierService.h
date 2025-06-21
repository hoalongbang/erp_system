// Modules/Supplier/Service/SupplierService.h
#ifndef MODULES_SUPPLIER_SERVICE_SUPPLIERSERVICE_H
#define MODULES_SUPPLIER_SERVICE_SUPPLIERSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"      // NEW: Kế thừa từ BaseService
#include "Supplier.h"         // Đã rút gọn include
#include "SupplierDAO.h"      // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "EventBus.h"         // Đã rút gọn include
#include "Logger.h"           // Đã rút gọn include
#include "ErrorHandler.h"     // Đã rút gọn include
#include "Common.h"           // Đã rút gọn include
#include "Utils.h"            // Đã rút gọn include
#include "DateUtils.h"        // Đã rút gọn include

namespace ERP {
namespace Supplier {
namespace Services {

/**
 * @brief ISupplierService interface defines operations for managing supplier accounts.
 */
class ISupplierService {
public:
    virtual ~ISupplierService() = default;
    /**
     * @brief Creates a new supplier.
     * @param supplierDTO DTO containing new supplier information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional SupplierDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Supplier::DTO::SupplierDTO> createSupplier(
        const ERP::Supplier::DTO::SupplierDTO& supplierDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves supplier information by ID.
     * @param supplierId ID of the supplier to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional SupplierDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Supplier::DTO::SupplierDTO> getSupplierById(
        const std::string& supplierId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves supplier information by name.
     * @param supplierName Name of the supplier to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional SupplierDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Supplier::DTO::SupplierDTO> getSupplierByName(
        const std::string& supplierName,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all suppliers or suppliers matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching SupplierDTOs.
     */
    virtual std::vector<ERP::Supplier::DTO::SupplierDTO> getAllSuppliers(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates supplier information.
     * @param supplierDTO DTO containing updated supplier information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateSupplier(
        const ERP::Supplier::DTO::SupplierDTO& supplierDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a supplier.
     * @param supplierId ID of the supplier.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateSupplierStatus(
        const std::string& supplierId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a supplier record by ID (soft delete).
     * @param supplierId ID of the supplier to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteSupplier(
        const std::string& supplierId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of ISupplierService.
 * This class uses SupplierDAO and ISecurityManager.
 */
class SupplierService : public ISupplierService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for SupplierService.
     * @param supplierDAO Shared pointer to SupplierDAO.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    SupplierService(std::shared_ptr<DAOs::SupplierDAO> supplierDAO,
                    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                    std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Supplier::DTO::SupplierDTO> createSupplier(
        const ERP::Supplier::DTO::SupplierDTO& supplierDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Supplier::DTO::SupplierDTO> getSupplierById(
        const std::string& supplierId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Supplier::DTO::SupplierDTO> getSupplierByName(
        const std::string& supplierName,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Supplier::DTO::SupplierDTO> getAllSuppliers(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateSupplier(
        const ERP::Supplier::DTO::SupplierDTO& supplierDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateSupplierStatus(
        const std::string& supplierId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteSupplier(
        const std::string& supplierId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::SupplierDAO> supplierDAO_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Supplier
} // namespace ERP
#endif // MODULES_SUPPLIER_SERVICE_SUPPLIERSERVICE_H