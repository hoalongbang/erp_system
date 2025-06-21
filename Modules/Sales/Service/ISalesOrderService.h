// Modules/Sales/Service/ISalesOrderService.h
#ifndef MODULES_SALES_SERVICE_ISALESORDERSERVICE_H
#define MODULES_SALES_SERVICE_ISALESORDERSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "SalesOrder.h"         // DTO
#include "SalesOrderDetail.h"   // DTO
#include "Common.h"             // Enum Common
#include "BaseService.h"        // Base Service

namespace ERP {
namespace Sales {
namespace Services {

/**
 * @brief ISalesOrderService interface defines operations for managing sales orders.
 */
class ISalesOrderService {
public:
    virtual ~ISalesOrderService() = default;
    /**
     * @brief Creates a new sales order.
     * @param salesOrderDTO DTO containing new sales order information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional SalesOrderDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::SalesOrderDTO> createSalesOrder(
        const ERP::Sales::DTO::SalesOrderDTO& salesOrderDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves sales order information by ID.
     * @param salesOrderId ID of the sales order to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional SalesOrderDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::SalesOrderDTO> getSalesOrderById(
        const std::string& salesOrderId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves sales order information by order number.
     * @param orderNumber Order number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional SalesOrderDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::SalesOrderDTO> getSalesOrderByNumber(
        const std::string& orderNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all sales orders or sales orders matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching SalesOrderDTOs.
     */
    virtual std::vector<ERP::Sales::DTO::SalesOrderDTO> getAllSalesOrders(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates sales order information.
     * @param salesOrderDTO DTO containing updated sales order information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateSalesOrder(
        const ERP::Sales::DTO::SalesOrderDTO& salesOrderDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a sales order.
     * @param salesOrderId ID of the sales order.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateSalesOrderStatus(
        const std::string& salesOrderId,
        ERP::Sales::DTO::SalesOrderStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a sales order record by ID.
     * @param salesOrderId ID of the sales order to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteSalesOrder(
        const std::string& salesOrderId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};

} // namespace Services
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_SERVICE_ISALESORDERSERVICE_H