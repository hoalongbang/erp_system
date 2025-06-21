// Modules/Catalog/Service/ILocationService.h
#ifndef MODULES_CATALOG_SERVICE_ILOCATIONSERVICE_H
#define MODULES_CATALOG_SERVICE_ILOCATIONSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>

// Rút gọn các include paths
#include "Location.h"      // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

namespace ERP {
namespace Catalog {
namespace Services {

/**
 * @brief ILocationService interface defines operations for managing warehouse locations.
 */
class ILocationService {
public:
    virtual ~ILocationService() = default;
    /**
     * @brief Creates a new warehouse location.
     * @param locationDTO DTO containing new location information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional LocationDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::LocationDTO> createLocation(
        const ERP::Catalog::DTO::LocationDTO& locationDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves location information by ID.
     * @param locationId ID of the location to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional LocationDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::LocationDTO> getLocationById(
        const std::string& locationId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves location information by name and warehouse ID.
     * @param locationName Name of the location to retrieve.
     * @param warehouseId ID of the warehouse the location belongs to.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional LocationDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Catalog::DTO::LocationDTO> getLocationByNameAndWarehouse(
        const std::string& locationName,
        const std::string& warehouseId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all locations or locations matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching LocationDTOs.
     */
    virtual std::vector<ERP::Catalog::DTO::LocationDTO> getAllLocations(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all locations within a specific warehouse.
     * @param warehouseId ID of the warehouse.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching LocationDTOs.
     */
    virtual std::vector<ERP::Catalog::DTO::LocationDTO> getLocationsByWarehouse(
        const std::string& warehouseId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates location information.
     * @param locationDTO DTO containing updated location information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateLocation(
        const ERP::Catalog::DTO::LocationDTO& locationDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a location.
     * @param locationId ID of the location.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateLocationStatus(
        const std::string& locationId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a location record by ID (soft delete).
     * @param locationId ID of the location to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteLocation(
        const std::string& locationId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};

} // namespace Services
} // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_SERVICE_ILOCATIONSERVICE_H