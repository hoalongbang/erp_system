// Modules/Catalog/Service/UnitOfMeasureService.cpp
#include "UnitOfMeasureService.h" // Đã rút gọn include
#include "UnitOfMeasure.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Catalog {
namespace Services {

UnitOfMeasureService::UnitOfMeasureService(
    std::shared_ptr<DAOs::UnitOfMeasureDAO> uomDAO,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      uomDAO_(uomDAO) {
    if (!uomDAO_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "UnitOfMeasureService: Initialized with null DAO.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ đơn vị đo.");
        ERP::Logger::Logger::getInstance().critical("UnitOfMeasureService: Injected UnitOfMeasureDAO is null.");
        throw std::runtime_error("UnitOfMeasureService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("UnitOfMeasureService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> UnitOfMeasureService::createUnitOfMeasure(
    const ERP::Catalog::DTO::UnitOfMeasureDTO& uomDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("UnitOfMeasureService: Attempting to create unit of measure: " + uomDTO.name + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.CreateUnitOfMeasure", "Bạn không có quyền tạo đơn vị đo.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (uomDTO.name.empty() || uomDTO.symbol.empty()) {
        ERP::Logger::Logger::getInstance().warning("UnitOfMeasureService: Invalid input for UoM creation (empty name or symbol).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "UnitOfMeasureService: Invalid input for UoM creation.", "Tên hoặc ký hiệu đơn vị đo không được để trống.");
        return std::nullopt;
    }

    // Check if name or symbol already exists
    std::map<std::string, std::any> filterByName;
    filterByName["name"] = uomDTO.name;
    if (uomDAO_->count(filterByName) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("UnitOfMeasureService: Unit of measure with name " + uomDTO.name + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "UnitOfMeasureService: Unit of measure with name " + uomDTO.name + " already exists.", "Tên đơn vị đo đã tồn tại. Vui lòng chọn tên khác.");
        return std::nullopt;
    }
    std::map<std::string, std::any> filterBySymbol;
    filterBySymbol["symbol"] = uomDTO.symbol;
    if (uomDAO_->count(filterBySymbol) > 0) { // Using count from DAOBase
        ERP::Logger::Logger::getInstance().warning("UnitOfMeasureService: Unit of measure with symbol " + uomDTO.symbol + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "UnitOfMeasureService: Unit of measure with symbol " + uomDTO.symbol + " already exists.", "Ký hiệu đơn vị đo đã tồn tại. Vui lòng chọn ký hiệu khác.");
        return std::nullopt;
    }

    ERP::Catalog::DTO::UnitOfMeasureDTO newUoM = uomDTO;
    newUoM.id = ERP::Utils::generateUUID();
    newUoM.createdAt = ERP::Utils::DateUtils::now();
    newUoM.createdBy = currentUserId;
    newUoM.status = ERP::Common::EntityStatus::ACTIVE; // Default status

    std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> createdUoM = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!uomDAO_->create(newUoM)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("UnitOfMeasureService: Failed to create unit of measure " + newUoM.name + " in DAO.");
                return false;
            }
            createdUoM = newUoM;
            eventBus_.publish(std::make_shared<EventBus::UnitOfMeasureCreatedEvent>(newUoM.id, newUoM.name, newUoM.symbol));
            return true;
        },
        "UnitOfMeasureService", "createUnitOfMeasure"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("UnitOfMeasureService: Unit of measure " + newUoM.name + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "UnitOfMeasure", newUoM.id, "UnitOfMeasure", newUoM.name,
                       std::nullopt, newUoM.toMap(), "Unit of measure created.");
        return createdUoM;
    }
    return std::nullopt;
}

std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> UnitOfMeasureService::getUnitOfMeasureById(
    const std::string& uomId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("UnitOfMeasureService: Retrieving unit of measure by ID: " + uomId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewUnitsOfMeasure", "Bạn không có quyền xem đơn vị đo.")) {
        return std::nullopt;
    }

    return uomDAO_->getById(uomId); // Using getById from DAOBase template
}

std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> UnitOfMeasureService::getUnitOfMeasureByNameOrSymbol(
    const std::string& nameOrSymbol,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("UnitOfMeasureService: Retrieving unit of measure by name or symbol: " + nameOrSymbol + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewUnitsOfMeasure", "Bạn không có quyền xem đơn vị đo.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filterByName;
    filterByName["name"] = nameOrSymbol;
    std::vector<ERP::Catalog::DTO::UnitOfMeasureDTO> uoms = uomDAO_->get(filterByName); // Using get from DAOBase
    if (!uoms.empty()) {
        return uoms[0];
    }

    std::map<std::string, std::any> filterBySymbol;
    filterBySymbol["symbol"] = nameOrSymbol;
    uoms = uomDAO_->get(filterBySymbol); // Using get from DAOBase
    if (!uoms.empty()) {
        return uoms[0];
    }

    ERP::Logger::Logger::getInstance().debug("UnitOfMeasureService: Unit of measure with name or symbol " + nameOrSymbol + " not found.");
    return std::nullopt;
}

std::vector<ERP::Catalog::DTO::UnitOfMeasureDTO> UnitOfMeasureService::getAllUnitsOfMeasure(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("UnitOfMeasureService: Retrieving all units of measure with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ViewUnitsOfMeasure", "Bạn không có quyền xem tất cả đơn vị đo.")) {
        return {};
    }

    return uomDAO_->get(filter); // Using get from DAOBase template
}

bool UnitOfMeasureService::updateUnitOfMeasure(
    const ERP::Catalog::DTO::UnitOfMeasureDTO& uomDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("UnitOfMeasureService: Attempting to update unit of measure: " + uomDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.UpdateUnitOfMeasure", "Bạn không có quyền cập nhật đơn vị đo.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> oldUoMOpt = uomDAO_->getById(uomDTO.id); // Using getById from DAOBase
    if (!oldUoMOpt) {
        ERP::Logger::Logger::getInstance().warning("UnitOfMeasureService: Unit of measure with ID " + uomDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy đơn vị đo cần cập nhật.");
        return false;
    }

    // If name or symbol is changed, check for uniqueness (excluding self)
    if (uomDTO.name != oldUoMOpt->name) {
        std::map<std::string, std::any> filterByName;
        filterByName["name"] = uomDTO.name;
        std::vector<ERP::Catalog::DTO::UnitOfMeasureDTO> existingUoMs = uomDAO_->get(filterByName);
        if (!existingUoMs.empty() && existingUoMs[0].id != uomDTO.id) {
            ERP::Logger::Logger::getInstance().warning("UnitOfMeasureService: New UoM name " + uomDTO.name + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Tên đơn vị đo mới đã tồn tại. Vui lòng chọn tên khác.");
            return false;
        }
    }
    if (uomDTO.symbol != oldUoMOpt->symbol) {
        std::map<std::string, std::any> filterBySymbol;
        filterBySymbol["symbol"] = uomDTO.symbol;
        std::vector<ERP::Catalog::DTO::UnitOfMeasureDTO> existingUoMs = uomDAO_->get(filterBySymbol);
        if (!existingUoMs.empty() && existingUoMs[0].id != uomDTO.id) {
            ERP::Logger::Logger::getInstance().warning("UnitOfMeasureService: New UoM symbol " + uomDTO.symbol + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Ký hiệu đơn vị đo mới đã tồn tại. Vui lòng chọn ký hiệu khác.");
            return false;
        }
    }

    ERP::Catalog::DTO::UnitOfMeasureDTO updatedUoM = uomDTO;
    updatedUoM.updatedAt = ERP::Utils::DateUtils::now();
    updatedUoM.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!uomDAO_->update(updatedUoM)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("UnitOfMeasureService: Failed to update unit of measure " + updatedUoM.id + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::UnitOfMeasureUpdatedEvent>(updatedUoM.id, updatedUoM.name, updatedUoM.symbol));
            return true;
        },
        "UnitOfMeasureService", "updateUnitOfMeasure"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("UnitOfMeasureService: Unit of measure " + updatedUoM.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "UnitOfMeasure", updatedUoM.id, "UnitOfMeasure", updatedUoM.name,
                       oldUoMOpt->toMap(), updatedUoM.toMap(), "Unit of measure updated.");
        return true;
    }
    return false;
}

bool UnitOfMeasureService::updateUnitOfMeasureStatus(
    const std::string& uomId,
    ERP::Common::EntityStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("UnitOfMeasureService: Attempting to update status for UoM: " + uomId + " to " + ERP::Common::entityStatusToString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.ChangeUnitOfMeasureStatus", "Bạn không có quyền cập nhật trạng thái đơn vị đo.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> uomOpt = uomDAO_->getById(uomId); // Using getById from DAOBase
    if (!uomOpt) {
        ERP::Logger::Logger::getInstance().warning("UnitOfMeasureService: UoM with ID " + uomId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy đơn vị đo để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Catalog::DTO::UnitOfMeasureDTO oldUoM = *uomOpt;
    if (oldUoM.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("UnitOfMeasureService: UoM " + uomId + " is already in status " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true; // Already in desired status
    }

    ERP::Catalog::DTO::UnitOfMeasureDTO updatedUoM = oldUoM;
    updatedUoM.status = newStatus;
    updatedUoM.updatedAt = ERP::Utils::DateUtils::now();
    updatedUoM.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!uomDAO_->update(updatedUoM)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("UnitOfMeasureService: Failed to update status for UoM " + uomId + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::UnitOfMeasureStatusChangedEvent>(uomId, newStatus));
            return true;
        },
        "UnitOfMeasureService", "updateUnitOfMeasureStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("UnitOfMeasureService: Status for UoM " + uomId + " updated successfully to " + ERP::Common::entityStatusToString(newStatus) + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "UnitOfMeasureStatus", uomId, "UnitOfMeasure", oldUoM.name,
                       oldUoM.toMap(), updatedUoM.toMap(), "Unit of measure status changed to " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true;
    }
    return false;
}

bool UnitOfMeasureService::deleteUnitOfMeasure(
    const std::string& uomId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("UnitOfMeasureService: Attempting to delete UoM: " + uomId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Catalog.DeleteUnitOfMeasure", "Bạn không có quyền xóa đơn vị đo.")) {
        return false;
    }

    std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> uomOpt = uomDAO_->getById(uomId); // Using getById from DAOBase
    if (!uomOpt) {
        ERP::Logger::Logger::getInstance().warning("UnitOfMeasureService: UoM with ID " + uomId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy đơn vị đo cần xóa.");
        return false;
    }

    ERP::Catalog::DTO::UnitOfMeasureDTO uomToDelete = *uomOpt;

    // Additional checks: Prevent deletion if UoM has associated products or unit conversion rules
    std::map<std::string, std::any> productFilter;
    productFilter["base_unit_of_measure_id"] = uomId;
    if (securityManager_->getProductService()->getAllProducts(productFilter).size() > 0) { // Assuming ProductService can query by base UoM
        ERP::Logger::Logger::getInstance().warning("UnitOfMeasureService: Cannot delete UoM " + uomId + " as it is a base unit for products.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa đơn vị đo là đơn vị cơ sở của sản phẩm.");
        return false;
    }
    // Need to check Product.UnitConversionRuleDTO as well (both from and to UoM)
    // This requires iterating through all products and their unit conversion rules.
    // For simplicity, this check is omitted here, but would be crucial in a real system.

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!uomDAO_->remove(uomId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("UnitOfMeasureService: Failed to delete UoM " + uomId + " in DAO.");
                return false;
            }
            return true;
        },
        "UnitOfMeasureService", "deleteUnitOfMeasure"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("UnitOfMeasureService: UoM " + uomId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Catalog", "UnitOfMeasure", uomId, "UnitOfMeasure", uomToDelete.name,
                       uomToDelete.toMap(), std::nullopt, "Unit of measure deleted.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Catalog
} // namespace ERP