// Modules/Manufacturing/Service/IProductionOrderService.h
#ifndef MODULES_MANUFACTURING_SERVICE_IPRODUCTIONORDERSERVICE_H
#define MODULES_MANUFACTURING_SERVICE_IPRODUCTIONORDERSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "ProductionOrder.h" // DTO
#include "Common.h"          // Enum Common
#include "BaseService.h"     // Base Service

namespace ERP {
namespace Manufacturing {
namespace Services {

/**
 * @brief IProductionOrderService interface defines operations for managing production orders.
 */
class IProductionOrderService {
public:
    virtual ~IProductionOrderService() = default;
    /**
     * @brief Creates a new production order.
     * @param productionOrderDTO DTO containing new order information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ProductionOrderDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> createProductionOrder(
        const ERP::Manufacturing::DTO::ProductionOrderDTO& productionOrderDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves production order information by ID.
     * @param orderId ID of the production order to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ProductionOrderDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> getProductionOrderById(
        const std::string& orderId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves production order information by order number.
     * @param orderNumber Order number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ProductionOrderDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> getProductionOrderByNumber(
        const std::string& orderNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all production orders or orders matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching ProductionOrderDTOs.
     */
    virtual std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> getAllProductionOrders(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves production orders by status.
     * @param status The status to filter by.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching ProductionOrderDTOs.
     */
    virtual std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> getProductionOrdersByStatus(
        ERP::Manufacturing::DTO::ProductionOrderStatus status,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves production orders by production line.
     * @param productionLineId The ID of the production line.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching ProductionOrderDTOs.
     */
    virtual std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> getProductionOrdersByProductionLine(
        const std::string& productionLineId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates production order information.
     * @param productionOrderDTO DTO containing updated order information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateProductionOrder(
        const ERP::Manufacturing::DTO::ProductionOrderDTO& productionOrderDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a production order.
     * @param orderId ID of the order.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateProductionOrderStatus(
        const std::string& orderId,
        ERP::Manufacturing::DTO::ProductionOrderStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a production order record by ID (soft delete).
     * @param orderId ID of the order to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteProductionOrder(
        const std::string& orderId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Records actual quantity produced for a production order.
     * @param orderId ID of the production order.
     * @param actualQuantityProduced Actual quantity produced.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if successful, false otherwise.
     */
    virtual bool recordActualQuantityProduced(
        const std::string& orderId,
        double actualQuantityProduced,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};

} // namespace Services
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_SERVICE_IPRODUCTIONORDERSERVICE_H