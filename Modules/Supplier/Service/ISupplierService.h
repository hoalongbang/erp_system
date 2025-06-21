// Modules/Supplier/Service/ISupplierService.h
#ifndef MODULES_SUPPLIER_SERVICE_ISUPPLIERSERVICE_H
#define MODULES_SUPPLIER_SERVICE_ISUPPLIERSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>

// Rút gọn các include paths
#include "Supplier.h"      // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

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

} // namespace Services
} // namespace Supplier
} // namespace ERP
#endif // MODULES_SUPPLIER_SERVICE_ISUPPLIERSERVICE_H