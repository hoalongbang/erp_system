// Modules/Material/Service/MaterialRequestService.h
#ifndef MODULES_MATERIAL_SERVICE_MATERIALREQUESTSERVICE_H
#define MODULES_MATERIAL_SERVICE_MATERIALREQUESTSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"        // NEW: Kế thừa từ BaseService
#include "MaterialRequestSlip.h"     // Đã rút gọn include
#include "MaterialRequestSlipDetail.h" // Đã rút gọn include
#include "MaterialRequestSlipDAO.h"  // Đã rút gọn include
#include "ProductService.h"     // For Product validation
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

// Forward declare if ProductService is only used via pointer/reference
// class IProductService;

/**
 * @brief IMaterialRequestService interface defines operations for managing material request slips.
 */
class IMaterialRequestService {
public:
    virtual ~IMaterialRequestService() = default;
    /**
     * @brief Creates a new material request slip.
     * @param requestSlipDTO DTO containing new request information.
     * @param requestSlipDetails Vector of MaterialRequestSlipDetailDTOs for the slip.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional MaterialRequestSlipDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> createMaterialRequestSlip(
        const ERP::Material::DTO::MaterialRequestSlipDTO& requestSlipDTO,
        const std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO>& requestSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves material request slip information by ID.
     * @param requestSlipId ID of the request to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional MaterialRequestSlipDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> getMaterialRequestSlipById(
        const std::string& requestSlipId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves material request slip information by request number.
     * @param requestNumber Request number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional MaterialRequestSlipDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> getMaterialRequestSlipByNumber(
        const std::string& requestNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all material request slips or slips matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching MaterialRequestSlipDTOs.
     */
    virtual std::vector<ERP::Material::DTO::MaterialRequestSlipDTO> getAllMaterialRequestSlips(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates material request slip information.
     * @param requestSlipDTO DTO containing updated request information (must have ID).
     * @param requestSlipDetails Vector of MaterialRequestSlipDetailDTOs for the slip (full replacement).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateMaterialRequestSlip(
        const ERP::Material::DTO::MaterialRequestSlipDTO& requestSlipDTO,
        const std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO>& requestSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a material request slip.
     * @param requestSlipId ID of the request.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateMaterialRequestSlipStatus(
        const std::string& requestSlipId,
        ERP::Material::DTO::MaterialRequestSlipStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a material request slip record by ID (soft delete).
     * @param requestSlipId ID of the request to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteMaterialRequestSlip(
        const std::string& requestSlipId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves all details for a specific material request slip.
     * @param requestSlipId ID of the material request slip.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of MaterialRequestSlipDetailDTOs.
     */
    virtual std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> getMaterialRequestSlipDetails(
        const std::string& requestSlipId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves a specific material request slip detail by ID.
     * @param detailId ID of the detail.
     * @param userRoleIds Roles of the user making the request.
     * @return An optional MaterialRequestSlipDetailDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Material::DTO::MaterialRequestSlipDetailDTO> getMaterialRequestSlipDetailById(
        const std::string& detailId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
};
/**
 * @brief Default implementation of IMaterialRequestService.
 * This class uses MaterialRequestSlipDAO and other services for validation.
 */
class MaterialRequestService : public IMaterialRequestService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for MaterialRequestService.
     * @param materialRequestSlipDAO Shared pointer to MaterialRequestSlipDAO.
     * @param productService Shared pointer to IProductService.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    MaterialRequestService(std::shared_ptr<DAOs::MaterialRequestSlipDAO> materialRequestSlipDAO,
                           std::shared_ptr<ERP::Product::Services::IProductService> productService,
                           std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                           std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                           std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                           std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> createMaterialRequestSlip(
        const ERP::Material::DTO::MaterialRequestSlipDTO& requestSlipDTO,
        const std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO>& requestSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> getMaterialRequestSlipById(
        const std::string& requestSlipId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> getMaterialRequestSlipByNumber(
        const std::string& requestNumber,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Material::DTO::MaterialRequestSlipDTO> getAllMaterialRequestSlips(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateMaterialRequestSlip(
        const ERP::Material::DTO::MaterialRequestSlipDTO& requestSlipDTO,
        const std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO>& requestSlipDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateMaterialRequestSlipStatus(
        const std::string& requestSlipId,
        ERP::Material::DTO::MaterialRequestSlipStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteMaterialRequestSlip(
        const std::string& requestSlipId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> getMaterialRequestSlipDetails(
        const std::string& requestSlipId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Material::DTO::MaterialRequestSlipDetailDTO> getMaterialRequestSlipDetailById(
        const std::string& detailId,
        const std::vector<std::string>& userRoleIds = {}) override;

private:
    std::shared_ptr<DAOs::MaterialRequestSlipDAO> materialRequestSlipDAO_;
    std::shared_ptr<ERP::Product::Services::IProductService> productService_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_SERVICE_MATERIALREQUESTSERVICE_H