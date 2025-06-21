// Modules/Sales/Service/IShipmentService.h
#ifndef MODULES_SALES_SERVICE_ISHIPMENTSERVICE_H
#define MODULES_SALES_SERVICE_ISHIPMENTSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "Shipment.h"    // DTO
#include "Common.h"      // Enum Common
#include "BaseService.h" // Base Service

namespace ERP {
namespace Sales {
namespace Services {

/**
 * @brief IShipmentService interface defines operations for managing sales shipments.
 */
class IShipmentService {
public:
    virtual ~IShipmentService() = default;
    /**
     * @brief Creates a new sales shipment.
     * @param shipmentDTO DTO containing new shipment information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ShipmentDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::ShipmentDTO> createShipment(
        const ERP::Sales::DTO::ShipmentDTO& shipmentDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves shipment information by ID.
     * @param shipmentId ID of the shipment to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ShipmentDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::ShipmentDTO> getShipmentById(
        const std::string& shipmentId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves shipment information by shipment number.
     * @param shipmentNumber Shipment number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ShipmentDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::ShipmentDTO> getShipmentByNumber(
        const std::string& shipmentNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all shipments or shipments matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching ShipmentDTOs.
     */
    virtual std::vector<ERP::Sales::DTO::ShipmentDTO> getAllShipments(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates shipment information.
     * @param shipmentDTO DTO containing updated shipment information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateShipment(
        const ERP::Sales::DTO::ShipmentDTO& shipmentDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a shipment.
     * @param shipmentId ID of the shipment.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateShipmentStatus(
        const std::string& shipmentId,
        ERP::Sales::DTO::ShipmentStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a shipment record by ID (soft delete).
     * @param shipmentId ID of the shipment to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteShipment(
        const std::string& shipmentId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};

} // namespace Services
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_SERVICE_ISHIPMENTSERVICE_H