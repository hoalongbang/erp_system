// Modules/Manufacturing/Service/ProductionLineService.cpp
#include "ProductionLineService.h" // Standard includes
#include "ProductionLine.h"         // ProductionLine DTO
#include "Location.h"               // Location DTO
#include "Asset.h"                  // Asset DTO
#include "Event.h"                  // Event DTO
#include "ConnectionPool.h"         // ConnectionPool
#include "DBConnection.h"           // DBConnection
#include "Common.h"                 // Common Enums/Constants
#include "Utils.h"                  // Utility functions
#include "DateUtils.h"              // Date utility functions
#include "AutoRelease.h"            // RAII helper
#include "ISecurityManager.h"       // Security Manager interface
#include "UserService.h"            // User Service (for audit logging)
#include "LocationService.h"        // LocationService (for location validation)
#include "AssetManagementService.h" // AssetManagementService (for asset validation)
#include "DTOUtils.h"               // For JSON conversions (for associated_asset_ids_json)

#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed

namespace ERP {
namespace Manufacturing {
namespace Services {

ProductionLineService::ProductionLineService(
    std::shared_ptr<DAOs::ProductionLineDAO> productionLineDAO,
    std::shared_ptr<ERP::Catalog::Services::ILocationService> locationService, // Dependency
    std::shared_ptr<ERP::Asset::Services::IAssetManagementService> assetManagementService, // Dependency, can be nullptr
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Initialize BaseService
      productionLineDAO_(productionLineDAO),
      locationService_(locationService), // Initialize specific dependencies
      assetManagementService_(assetManagementService) { // Initialize specific dependencies
    
    if (!productionLineDAO_ || !locationService_ || !securityManager_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "ProductionLineService: Initialized with null DAO or dependent services (LocationService, SecurityManager).", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ dây chuyền sản xuất.");
        ERP::Logger::Logger::getInstance().critical("ProductionLineService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("ProductionLineService: Null dependencies.");
    }
    // AssetManagementService is optional during early initialization, can be null.
    ERP::Logger::Logger::getInstance().info("ProductionLineService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> ProductionLineService::createProductionLine(
    const ERP::Manufacturing::DTO::ProductionLineDTO& productionLineDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ProductionLineService: Attempting to create production line: " + productionLineDTO.lineName + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.CreateProductionLine", "Bạn không có quyền tạo dây chuyền sản xuất.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (productionLineDTO.lineName.empty() || productionLineDTO.locationId.empty() || productionLineDTO.status == ERP::Manufacturing::DTO::ProductionLineStatus::UNKNOWN) {
        ERP::Logger::Logger::getInstance().warning("ProductionLineService: Invalid input for production line creation (empty name, location ID, or unknown status).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ProductionLineService: Invalid input for creation.", "Thông tin dây chuyền sản xuất không đầy đủ.");
        return std::nullopt;
    }

    // Check if line name already exists
    std::map<std::string, std::any> filterByName;
    filterByName["line_name"] = productionLineDTO.lineName;
    if (productionLineDAO_->countProductionLines(filterByName) > 0) { // Specific DAO method
        ERP::Logger::Logger::getInstance().warning("ProductionLineService: Production line with name " + productionLineDTO.lineName + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ProductionLineService: Production line with name " + productionLineDTO.lineName + " already exists.", "Tên dây chuyền sản xuất đã tồn tại. Vui lòng chọn tên khác.");
        return std::nullopt;
    }

    // Validate Location existence
    std::optional<ERP::Catalog::DTO::LocationDTO> location = locationService_->getLocationById(productionLineDTO.locationId, userRoleIds);
    if (!location || location->status != ERP::Common::EntityStatus::ACTIVE) {
        ERP::Logger::Logger::getInstance().warning("ProductionLineService: Invalid Location ID provided or location is not active: " + productionLineDTO.locationId);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID địa điểm không hợp lệ hoặc địa điểm không hoạt động.");
        return std::nullopt;
    }

    // Validate associated assets if provided (and assetManagementService is available)
    if (assetManagementService_ && !productionLineDTO.associatedAssetIds.empty()) {
        for (const auto& assetId : productionLineDTO.associatedAssetIds) {
            std::optional<ERP::Asset::DTO::AssetDTO> asset = assetManagementService_->getAssetById(assetId, userRoleIds);
            if (!asset || asset->status != ERP::Common::EntityStatus::ACTIVE) {
                ERP::Logger::Logger::getInstance().warning("ProductionLineService: Associated Asset " + assetId + " not found or not active.");
                ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Tài sản liên kết không tồn tại hoặc không hoạt động.");
                return std::nullopt;
            }
        }
    } else if (!assetManagementService_ && !productionLineDTO.associatedAssetIds.empty()) {
        ERP::Logger::Logger::getInstance().warning("ProductionLineService: Assets provided, but AssetManagementService is null. Cannot validate assets.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "AssetManagementService is not available.", "Dịch vụ quản lý tài sản không khả dụng.");
        return std::nullopt;
    }

    ERP::Manufacturing::DTO::ProductionLineDTO newLine = productionLineDTO;
    newLine.id = ERP::Utils::generateUUID();
    newLine.createdAt = ERP::Utils::DateUtils::now();
    newLine.createdBy = currentUserId;

    std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> createdLine = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!productionLineDAO_->create(newLine)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("ProductionLineService: Failed to create production line in DAO.");
                return false;
            }
            createdLine = newLine;
            // Optionally, publish event
            eventBus_.publish(std::make_shared<EventBus::ProductionLineCreatedEvent>(newLine.id, newLine.lineName));
            return true;
        },
        "ProductionLineService", "createProductionLine"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ProductionLineService: Production line " + newLine.lineName + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "ProductionLine", newLine.id, "ProductionLine", newLine.lineName,
                       std::nullopt, newLine.toMap(), "Production line created.");
        return createdLine;
    }
    return std::nullopt;
}

std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> ProductionLineService::getProductionLineById(
    const std::string& lineId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("ProductionLineService: Retrieving production line by ID: " + lineId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.ViewProductionLine", "Bạn không có quyền xem dây chuyền sản xuất.")) {
        return std::nullopt;
    }

    return productionLineDAO_->getProductionLineById(lineId); // Specific DAO method
}

std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> ProductionLineService::getProductionLineByName(
    const std::string& lineName,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("ProductionLineService: Retrieving production line by name: " + lineName + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.ViewProductionLine", "Bạn không có quyền xem dây chuyền sản xuất.")) {
        return std::nullopt;
    }

    return productionLineDAO_->getProductionLineByName(lineName); // Specific DAO method
}

std::vector<ERP::Manufacturing::DTO::ProductionLineDTO> ProductionLineService::getAllProductionLines(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ProductionLineService: Retrieving all production lines with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.ViewProductionLine", "Bạn không có quyền xem tất cả dây chuyền sản xuất.")) {
        return {};
    }

    return productionLineDAO_->getProductionLines(filter); // Specific DAO method
}

bool ProductionLineService::updateProductionLine(
    const ERP::Manufacturing::DTO::ProductionLineDTO& productionLineDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ProductionLineService: Attempting to update production line: " + productionLineDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.UpdateProductionLine", "Bạn không có quyền cập nhật dây chuyền sản xuất.")) {
        return false;
    }

    std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> oldLineOpt = productionLineDAO_->getProductionLineById(productionLineDTO.id); // Specific DAO method
    if (!oldLineOpt) {
        ERP::Logger::Logger::getInstance().warning("ProductionLineService: Production line with ID " + productionLineDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy dây chuyền sản xuất cần cập nhật.");
        return false;
    }

    // If line name is changed, check for uniqueness
    if (productionLineDTO.lineName != oldLineOpt->lineName) {
        std::map<std::string, std::any> filterByName;
        filterByName["line_name"] = productionLineDTO.lineName;
        if (productionLineDAO_->countProductionLines(filterByName) > 0) { // Specific DAO method
            ERP::Logger::Logger::getInstance().warning("ProductionLineService: New line name " + productionLineDTO.lineName + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ProductionLineService: New line name " + productionLineDTO.lineName + " already exists.", "Tên dây chuyền sản xuất mới đã tồn tại. Vui lòng chọn tên khác.");
            return false;
        }
    }

    // Validate Location existence
    if (productionLineDTO.locationId != oldLineOpt->locationId) { // Only check if changed
        std::optional<ERP::Catalog::DTO::LocationDTO> location = locationService_->getLocationById(productionLineDTO.locationId, userRoleIds);
        if (!location || location->status != ERP::Common::EntityStatus::ACTIVE) {
            ERP::Logger::Logger::getInstance().warning("ProductionLineService: Invalid Location ID provided for update or location is not active: " + productionLineDTO.locationId);
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID địa điểm không hợp lệ hoặc địa điểm không hoạt động.");
            return false;
        }
    }

    // Validate associated assets (if assetManagementService is available)
    if (assetManagementService_ && !productionLineDTO.associatedAssetIds.empty()) {
        for (const auto& assetId : productionLineDTO.associatedAssetIds) {
            std::optional<ERP::Asset::DTO::AssetDTO> asset = assetManagementService_->getAssetById(assetId, userRoleIds);
            if (!asset || asset->status != ERP::Common::EntityStatus::ACTIVE) {
                ERP::Logger::Logger::getInstance().warning("ProductionLineService: Associated Asset " + assetId + " not found or not active for update.");
                ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Tài sản liên kết không tồn tại hoặc không hoạt động.");
                return std::nullopt;
            }
        }
    } else if (!assetManagementService_ && !productionLineDTO.associatedAssetIds.empty()) {
        ERP::Logger::Logger::getInstance().warning("ProductionLineService: Assets provided, but AssetManagementService is null. Cannot validate assets for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "AssetManagementService is not available.", "Dịch vụ quản lý tài sản không khả dụng.");
        return std::nullopt;
    }


    ERP::Manufacturing::DTO::ProductionLineDTO updatedLine = productionLineDTO;
    updatedLine.updatedAt = ERP::Utils::DateUtils::now();
    updatedLine.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!productionLineDAO_->update(updatedLine)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("ProductionLineService: Failed to update production line " + updatedLine.id + " in DAO.");
                return false;
            }
            // Optionally, publish event
            eventBus_.publish(std::make_shared<EventBus::ProductionLineUpdatedEvent>(updatedLine.id, updatedLine.lineName));
            return true;
        },
        "ProductionLineService", "updateProductionLine"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ProductionLineService: Production line " + updatedLine.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "ProductionLine", updatedLine.id, "ProductionLine", updatedLine.lineName,
                       oldLineOpt->toMap(), updatedLine.toMap(), "Production line updated.");
        return true;
    }
    return false;
}

bool ProductionLineService::updateProductionLineStatus(
    const std::string& lineId,
    ERP::Manufacturing::DTO::ProductionLineStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ProductionLineService: Attempting to update status for production line: " + lineId + " to " + ERP::Manufacturing::DTO::ProductionLineDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.UpdateProductionLineStatus", "Bạn không có quyền cập nhật trạng thái dây chuyền sản xuất.")) {
        return false;
    }

    std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> lineOpt = productionLineDAO_->getProductionLineById(lineId); // Specific DAO method
    if (!lineOpt) {
        ERP::Logger::Logger::getInstance().warning("ProductionLineService: Production line with ID " + lineId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy dây chuyền sản xuất để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Manufacturing::DTO::ProductionLineDTO oldLine = *lineOpt;
    if (oldLine.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("ProductionLineService: Production line " + lineId + " is already in status " + ERP::Manufacturing::DTO::ProductionLineDTO().getStatusString(newStatus) + ".");
        return true; // Already in desired status
    }

    // Add state transition validation logic here
    // For example: Cannot change from SHUTDOWN to OPERATIONAL directly without maintenance check.

    ERP::Manufacturing::DTO::ProductionLineDTO updatedLine = oldLine;
    updatedLine.status = newStatus;
    updatedLine.updatedAt = ERP::Utils::DateUtils::now();
    updatedLine.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!productionLineDAO_->update(updatedLine)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("ProductionLineService: Failed to update status for production line " + lineId + " in DAO.");
                return false;
            }
            // Optionally, publish event
            eventBus_.publish(std::make_shared<EventBus::ProductionLineStatusChangedEvent>(lineId, newStatus));
            return true;
        },
        "ProductionLineService", "updateProductionLineStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ProductionLineService: Status for production line " + lineId + " updated successfully to " + updatedLine.getStatusString() + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "ProductionLineStatus", lineId, "ProductionLine", oldLine.lineName,
                       oldLine.toMap(), updatedLine.toMap(), "Production line status changed to " + updatedLine.getStatusString() + ".");
        return true;
    }
    return false;
}

bool ProductionLineService::deleteProductionLine(
    const std::string& lineId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ProductionLineService: Attempting to delete production line: " + lineId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.DeleteProductionLine", "Bạn không có quyền xóa dây chuyền sản xuất.")) {
        return false;
    }

    std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> lineOpt = productionLineDAO_->getProductionLineById(lineId); // Specific DAO method
    if (!lineOpt) {
        ERP::Logger::Logger::getInstance().warning("ProductionLineService: Production line with ID " + lineId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy dây chuyền sản xuất cần xóa.");
        return false;
    }

    ERP::Manufacturing::DTO::ProductionLineDTO lineToDelete = *lineOpt;

    // Additional checks: Prevent deletion if there are active production orders or associated assets
    std::map<std::string, std::any> productionOrderFilter;
    productionOrderFilter["production_line_id"] = lineId;
    if (securityManager_->getProductionOrderService()->countProductionOrders(productionOrderFilter) > 0) { // Check for associated production orders
        ERP::Logger::Logger::getInstance().warning("ProductionLineService: Cannot delete production line " + lineId + " as it has associated production orders.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa dây chuyền sản xuất có lệnh sản xuất liên quan.");
        return false;
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Remove associated assets link (if stored as separate relationship, not JSON list)
            // If stored as JSON list in DTO, it's removed on update, not on delete.

            if (!productionLineDAO_->remove(lineId)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("ProductionLineService: Failed to delete production line " + lineId + " in DAO.");
                return false;
            }
            return true;
        },
        "ProductionLineService", "deleteProductionLine"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ProductionLineService: Production line " + lineId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "ProductionLine", lineId, "ProductionLine", lineToDelete.lineName,
                       lineToDelete.toMap(), std::nullopt, "Production line deleted.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Manufacturing
} // namespace ERP