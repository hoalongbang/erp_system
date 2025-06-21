// Modules/Material/Service/ReceiptSlipService.h
#ifndef MODULES_MATERIAL_SERVICE_RECEIPTSLIPSERVICE_H
#define MODULES_MATERIAL_SERVICE_RECEIPTSLIPSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"        // NEW: Kế thừa từ BaseService
#include "ReceiptSlip.h"        // Đã rút gọn include
#include "ReceiptSlipDetail.h"  // Đã rút gọn include
#include "ReceiptSlipDAO.h"     // Đã rút gọn include
#include "ProductService.h"     // For Product validation
#include "WarehouseService.h"   // For Warehouse/Location validation
#include "InventoryManagementService.h" // For Inventory operations
#include "ISecurityManager.h"   // Đã rút gọn include
#include "EventBus.h"           // Đã rút gọn include
#include "Logger.h"             // Đã rút gọn include
#include "ErrorHandler.h"       // Đã rút gọn include
#include "Common.h"             // Đã rút gọn include
#include "Utils.h"              // Đã rút gọn include
#include "DateUtils.h"          // Đã rút gọn include

namespace ERP {
namespace Material {
namespace Services {

// Forward declare if other services are only used via pointer/reference
// class IProductService;
// class IWarehouseService;
// class IInventoryManagementService;

/**
 * @brief IReceiptSlipService interface defines operations for managing material receipt slips (phiếu nhập kho).
 */
class IReceiptSlipService {
public:
    virtual ~IReceiptSlipService() = default;
    /**
     * @brief Creates a new material receipt slip.
     * @param receiptSlipDTO DTO containing new receipt information.
     * @param receiptSlipDetails Vector of ReceiptSlipDetailDTOs for the slip.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ReceiptSlipDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::ReceiptSlipDTO> createReceiptSlip(
        const ERP::Material::DTO::ReceiptSlipDTO& receiptSlipDTO,
        const std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO>& receiptSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves receipt slip information by ID.
     * @param receiptSlipId ID of the receipt slip to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ReceiptSlipDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::ReceiptSlipDTO> getReceiptSlipById(
        const std::string& receiptSlipId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves receipt slip information by receipt number.
     * @param receiptNumber Receipt number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional ReceiptSlipDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::ReceiptSlipDTO> getReceiptSlipByNumber(
        const std::string& receiptNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all receipt slips or slips matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching ReceiptSlipDTOs.
     */
    virtual std::vector<ERP::Material::DTO::ReceiptSlipDTO> getAllReceiptSlips(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates receipt slip information.
     * @param receiptSlipDTO DTO containing updated receipt information (must have ID).
     * @param receiptSlipDetails Vector of ReceiptSlipDetailDTOs for the slip (full replacement).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateReceiptSlip(
        const ERP::Material::DTO::ReceiptSlipDTO& receiptSlipDTO,
        const std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO>& receiptSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a receipt slip.
     * @param receiptSlipId ID of the receipt slip.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateReceiptSlipStatus(
        const std::string& receiptSlipId,
        ERP::Material::DTO::ReceiptSlipStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a receipt slip record by ID (soft delete).
     * @param receiptSlipId ID of the receipt slip to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteReceiptSlip(
        const std::string& receiptSlipId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Records actual received quantity for a specific receipt slip detail.
     * This also creates an inventory transaction and cost layer.
     * @param detailId ID of the receipt slip detail.
     * @param receivedQuantity Actual quantity received.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if successful, false otherwise.
     */
    virtual bool recordReceivedQuantity(
        const std::string& detailId,
        double receivedQuantity,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves all details for a specific receipt slip.
     * @param receiptSlipId ID of the receipt slip.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of ReceiptSlipDetailDTOs.
     */
    virtual std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> getReceiptSlipDetails(
        const std::string& receiptSlipId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
};
/**
 * @brief Default implementation of IReceiptSlipService.
 * This class uses ReceiptSlipDAO, and other services for validation and inventory operations.
 */
class ReceiptSlipService : public IReceiptSlipService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for ReceiptSlipService.
     * @param receiptSlipDAO Shared pointer to ReceiptSlipDAO.
     * @param productService Shared pointer to IProductService.
     * @param warehouseService Shared pointer to IWarehouseService.
     * @param inventoryManagementService Shared pointer to IInventoryManagementService.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    ReceiptSlipService(std::shared_ptr<DAOs::ReceiptSlipDAO> receiptSlipDAO,
                       std::shared_ptr<ERP::Product::Services::IProductService> productService,
                       std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
                       std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService,
                       std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                       std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                       std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                       std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Material::DTO::ReceiptSlipDTO> createReceiptSlip(
        const ERP::Material::DTO::ReceiptSlipDTO& receiptSlipDTO,
        const std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO>& receiptSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Material::DTO::ReceiptSlipDTO> getReceiptSlipById(
        const std::string& receiptSlipId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Material::DTO::ReceiptSlipDTO> getReceiptSlipByNumber(
        const std::string& receiptNumber,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Material::DTO::ReceiptSlipDTO> getAllReceiptSlips(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateReceiptSlip(
        const ERP::Material::DTO::ReceiptSlipDTO& receiptSlipDTO,
        const std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO>& receiptSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateReceiptSlipStatus(
        const std::string& receiptSlipId,
        ERP::Material::DTO::ReceiptSlipStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteReceiptSlip(
        const std::string& receiptSlipId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool recordReceivedQuantity(
        const std::string& detailId,
        double receivedQuantity,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> getReceiptSlipDetails(
        const std::string& receiptSlipId,
        const std::vector<std::string>& userRoleIds = {}) override;

private:
    std::shared_ptr<DAOs::ReceiptSlipDAO> receiptSlipDAO_;
    std::shared_ptr<ERP::Product::Services::IProductService> productService_;
    std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService_;
    std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_SERVICE_RECEIPTSLIPSERVICE_H