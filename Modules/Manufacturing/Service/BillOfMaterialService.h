// Modules/Manufacturing/Service/BillOfMaterialService.h
#ifndef MODULES_MANUFACTURING_SERVICE_BILLOFMATERIALSERVICE_H
#define MODULES_MANUFACTURING_SERVICE_BILLOFMATERIALSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"        // NEW: Kế thừa từ BaseService
#include "BillOfMaterial.h"     // Đã rút gọn include
#include "BillOfMaterialItem.h" // Đã rút gọn include
#include "BillOfMaterialDAO.h"  // Đã rút gọn include
#include "ProductService.h"     // For Product validation
#include "UnitOfMeasureService.h" // For UnitOfMeasure validation
#include "ISecurityManager.h"   // Đã rút gọn include
#include "EventBus.h"           // Đã rút gọn include
#include "Logger.h"             // Đã rút gọn include
#include "ErrorHandler.h"       // Đã rút gọn include
#include "Common.h"             // Đã rút gọn include
#include "Utils.h"              // Đã rút gọn include
#include "DateUtils.h"          // Đã rút gọn include

namespace ERP {
namespace Manufacturing {
namespace Services {

// Forward declare if ProductService/UnitOfMeasureService are only used via pointer/reference
// class IProductService;
// class IUnitOfMeasureService;

/**
 * @brief IBillOfMaterialService interface defines operations for managing Bills of Material (BOMs).
 */
class IBillOfMaterialService {
public:
    virtual ~IBillOfMaterialService() = default;
    /**
     * @brief Creates a new Bill of Material (BOM).
     * @param bomDTO DTO containing new BOM information.
     * @param bomItems Vector of BillOfMaterialItemDTOs for the BOM.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional BillOfMaterialDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> createBillOfMaterial(
        const ERP::Manufacturing::DTO::BillOfMaterialDTO& bomDTO,
        const std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO>& bomItems,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves BOM information by ID.
     * @param bomId ID of the BOM to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional BillOfMaterialDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> getBillOfMaterialById(
        const std::string& bomId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves BOM information by BOM name or product ID.
     * @param bomNameOrProductId Name of the BOM or product ID to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional BillOfMaterialDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> getBillOfMaterialByNameOrProductId(
        const std::string& bomNameOrProductId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all BOMs or BOMs matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching BillOfMaterialDTOs.
     */
    virtual std::vector<ERP::Manufacturing::DTO::BillOfMaterialDTO> getAllBillOfMaterials(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates BOM information.
     * @param bomDTO DTO containing updated BOM information (must have ID).
     * @param bomItems Vector of BillOfMaterialItemDTOs for the BOM (full replacement).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateBillOfMaterial(
        const ERP::Manufacturing::DTO::BillOfMaterialDTO& bomDTO,
        const std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO>& bomItems,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a BOM.
     * @param bomId ID of the BOM.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateBillOfMaterialStatus(
        const std::string& bomId,
        ERP::Manufacturing::DTO::BillOfMaterialStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a BOM record by ID (soft delete).
     * @param bomId ID of the BOM to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteBillOfMaterial(
        const std::string& bomId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves items for a specific Bill of Material.
     * @param bomId ID of the BOM.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of BillOfMaterialItemDTOs.
     */
    virtual std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO> getBillOfMaterialItems(
        const std::string& bomId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
};
/**
 * @brief Default implementation of IBillOfMaterialService.
 * This class uses BillOfMaterialDAO, and other services for validation.
 */
class BillOfMaterialService : public IBillOfMaterialService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for BillOfMaterialService.
     * @param bomDAO Shared pointer to BillOfMaterialDAO.
     * @param productService Shared pointer to IProductService.
     * @param unitOfMeasureService Shared pointer to IUnitOfMeasureService.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    BillOfMaterialService(std::shared_ptr<DAOs::BillOfMaterialDAO> bomDAO,
                          std::shared_ptr<ERP::Product::Services::IProductService> productService,
                          std::shared_ptr<ERP::Catalog::Services::IUnitOfMeasureService> unitOfMeasureService,
                          std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                          std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                          std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                          std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> createBillOfMaterial(
        const ERP::Manufacturing::DTO::BillOfMaterialDTO& bomDTO,
        const std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO>& bomItems,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> getBillOfMaterialById(
        const std::string& bomId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> getBillOfMaterialByNameOrProductId(
        const std::string& bomNameOrProductId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Manufacturing::DTO::BillOfMaterialDTO> getAllBillOfMaterials(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateBillOfMaterial(
        const ERP::Manufacturing::DTO::BillOfMaterialDTO& bomDTO,
        const std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO>& bomItems,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateBillOfMaterialStatus(
        const std::string& bomId,
        ERP::Manufacturing::DTO::BillOfMaterialStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteBillOfMaterial(
        const std::string& bomId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO> getBillOfMaterialItems(
        const std::string& bomId,
        const std::vector<std::string>& userRoleIds = {}) override;

private:
    std::shared_ptr<DAOs::BillOfMaterialDAO> bomDAO_;
    std::shared_ptr<ERP::Product::Services::IProductService> productService_;
    std::shared_ptr<ERP::Catalog::Services::IUnitOfMeasureService> unitOfMeasureService_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_
};
} // namespace Services
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_SERVICE_BILLOFMATERIALSERVICE_H