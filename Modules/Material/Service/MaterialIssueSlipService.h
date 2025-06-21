// Modules/Material/Service/IMaterialIssueSlipService.h
#ifndef MODULES_MATERIAL_SERVICE_IMATERIALISSUESLIPSERVICE_H
#define MODULES_MATERIAL_SERVICE_IMATERIALISSUESLIPSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point
#include <memory> // For shared_ptr

// Rút gọn các include paths
#include "BaseService.h"           // Base Service
#include "MaterialIssueSlip.h"     // DTO
#include "MaterialIssueSlipDetail.h" // DTO

// Forward declarations for services that are dependencies but might cause circular includes
namespace ERP { namespace Manufacturing { namespace Services { class IProductionOrderService; }}}
namespace ERP { namespace Product { namespace Services { class IProductService; }}}
namespace ERP { namespace Catalog { namespace Services { class IWarehouseService; }}}
namespace ERP { namespace Warehouse { namespace Services { class IInventoryManagementService; }}}

namespace ERP {
namespace Material {
namespace Services {

/**
 * @brief IMaterialIssueSlipService interface defines operations for managing material issue slips for manufacturing.
 */
class IMaterialIssueSlipService {
public:
    virtual ~IMaterialIssueSlipService() = default;
    /**
     * @brief Creates a new material issue slip for manufacturing.
     * @param materialIssueSlipDTO DTO containing new slip information.
     * @param materialIssueSlipDetails Vector of MaterialIssueSlipDetailDTOs.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional MaterialIssueSlipDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> createMaterialIssueSlip(
        const ERP::Material::DTO::MaterialIssueSlipDTO& materialIssueSlipDTO,
        const std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO>& materialIssueSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves material issue slip information by ID.
     * @param issueSlipId ID of the slip to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional MaterialIssueSlipDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> getMaterialIssueSlipById(
        const std::string& issueSlipId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all material issue slips or slips matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching MaterialIssueSlipDTOs.
     */
    virtual std::vector<ERP::Material::DTO::MaterialIssueSlipDTO> getAllMaterialIssueSlips(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves material issue slips by production order ID.
     * @param productionOrderId ID of the production order.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching MaterialIssueSlipDTOs.
     */
    virtual std::vector<ERP::Material::DTO::MaterialIssueSlipDTO> getMaterialIssueSlipsByProductionOrderId(
        const std::string& productionOrderId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates material issue slip information.
     * @param materialIssueSlipDTO DTO containing updated slip information (must have ID).
     * @param materialIssueSlipDetails Vector of MaterialIssueSlipDetailDTOs (full replacement).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateMaterialIssueSlip(
        const ERP::Material::DTO::MaterialIssueSlipDTO& materialIssueSlipDTO,
        const std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO>& materialIssueSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a material issue slip.
     * @param issueSlipId ID of the slip.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateMaterialIssueSlipStatus(
        const std::string& issueSlipId,
        ERP::Material::DTO::MaterialIssueSlipStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a material issue slip record by ID (soft delete).
     * @param issueSlipId ID of the slip to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteMaterialIssueSlip(
        const std::string& issueSlipId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves a specific material issue slip detail by ID.
     * @param detailId ID of the detail.
     * @param currentUserId ID of the user making the request.
     * @param userRoleIds Roles of the user making the request.
     * @return An optional MaterialIssueSlipDetailDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::MaterialIssueSlipDetailDTO> getMaterialIssueSlipDetailById(
        const std::string& detailId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves all details for a specific material issue slip.
     * @param issueSlipId ID of the material issue slip.
     * @param currentUserId ID of the user making the request.
     * @param userRoleIds Roles of the user making the request.
     * @return Vector of MaterialIssueSlipDetailDTOs.
     */
    virtual std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> getMaterialIssueSlipDetails(
        const std::string& issueSlipId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Records actual issued quantity for a specific material issue slip detail.
     * This also creates an inventory transaction.
     * @param detailId ID of the detail.
     * @param issuedQuantity Actual quantity issued.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if successful, false otherwise.
     */
    virtual bool recordIssuedQuantity(
        const std::string& detailId,
        double issuedQuantity,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};

/**
 * @brief Default implementation of IMaterialIssueSlipService.
 * This class uses MaterialIssueSlipDAO and ISecurityManager.
 */
class MaterialIssueSlipService : public IMaterialIssueSlipService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for MaterialIssueSlipService.
     * @param materialIssueSlipDAO Shared pointer to MaterialIssueSlipDAO.
     * @param productionOrderService Shared pointer to IProductionOrderService (dependency).
     * @param productService Shared pointer to IProductService (dependency).
     * @param warehouseService Shared pointer to IWarehouseService (dependency).
     * @param inventoryManagementService Shared pointer to IInventoryManagementService (dependency).
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    MaterialIssueSlipService(std::shared_ptr<DAOs::MaterialIssueSlipDAO> materialIssueSlipDAO,
                             std::shared_ptr<ERP::Manufacturing::Services::IProductionOrderService> productionOrderService,
                             std::shared_ptr<ERP::Product::Services::IProductService> productService,
                             std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
                             std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService,
                             std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                             std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                             std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                             std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> createMaterialIssueSlip(
        const ERP::Material::DTO::MaterialIssueSlipDTO& materialIssueSlipDTO,
        const std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO>& materialIssueSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Material::DTO::MaterialIssueSlipDTO> getMaterialIssueSlipById(
        const std::string& issueSlipId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Material::DTO::MaterialIssueSlipDTO> getAllMaterialIssueSlips(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Material::DTO::MaterialIssueSlipDTO> getMaterialIssueSlipsByProductionOrderId(
        const std::string& productionOrderId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateMaterialIssueSlip(
        const ERP::Material::DTO::MaterialIssueSlipDTO& materialIssueSlipDTO,
        const std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO>& materialIssueSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateMaterialIssueSlipStatus(
        const std::string& issueSlipId,
        ERP::Material::DTO::MaterialIssueSlipStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteMaterialIssueSlip(
        const std::string& issueSlipId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Material::DTO::MaterialIssueSlipDetailDTO> getMaterialIssueSlipDetailById(
        const std::string& detailId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> getMaterialIssueSlipDetails(
        const std::string& issueSlipId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool recordIssuedQuantity(
        const std::string& detailId,
        double issuedQuantity,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::MaterialIssueSlipDAO> materialIssueSlipDAO_;
    std::shared_ptr<ERP::Manufacturing::Services::IProductionOrderService> productionOrderService_;
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
#endif // MODULES_MATERIAL_SERVICE_IMATERIALISSUESLIPSERVICE_H