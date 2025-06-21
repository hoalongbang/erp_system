// Modules/Asset/Service/AssetManagementService.cpp
#include "AssetManagementService.h" // Đã rút gọn include
#include "Asset.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "LocationService.h" // Đã rút gọn include
#include "ProductionLineService.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

// Temporarily include for QJsonObject, to be removed after auditlogservice fully refactored
#include <QJsonObject>
#include <QJsonDocument>

namespace ERP {
namespace Asset {
namespace Services {

AssetManagementService::AssetManagementService(
    std::shared_ptr<DAOs::AssetDAO> assetDAO,
    std::shared_ptr<ERP::Catalog::Services::ILocationService> locationService,
    std::shared_ptr<ERP::Manufacturing::Services::IProductionLineService> productionLineService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      assetDAO_(assetDAO), locationService_(locationService), productionLineService_(productionLineService) {
    if (!assetDAO_ || !locationService_ || !productionLineService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "AssetManagementService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ quản lý tài sản.");
        ERP::Logger::Logger::getInstance().critical("AssetManagementService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("AssetManagementService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("AssetManagementService: Initialized.");
}

// Old checkUserPermission and getUserName/getCurrentSessionId/logAudit removed as they are now in BaseService

// Helper for asset code existence check
bool AssetManagementService::assetCodeExists(const std::string& assetCode, const std::optional<std::string>& excludeAssetId) {
    std::map<std::string, std::any> filter;
    filter["asset_code"] = assetCode;
    std::vector<ERP::Asset::DTO::AssetDTO> results = assetDAO_->get(filter); // Using get from DAOBase
    if (!results.empty()) {
        if (excludeAssetId && results[0].id == *excludeAssetId) {
            return false; // Found itself, so not a conflict
        }
        return true;
    }
    return false;
}

// Helper for asset in use check (placeholder)
bool AssetManagementService::isAssetInUse(const std::string& assetId, const std::vector<std::string>& userRoleIds) {
    // Placeholder. In a real system, you'd check other modules:
    // - Is it assigned to an active Work Order? (via IProductionOrderService or IManufacturingService)
    // - Is it part of an active Production Line configuration? (via IProductionLineService)
    // - Is it currently undergoing maintenance that prevents deletion? (via IMaintenanceManagementService)
    // For demo, always return false
    return false;
}

std::optional<ERP::Asset::DTO::AssetDTO> AssetManagementService::createAsset(
    const ERP::Asset::DTO::AssetDTO& assetDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("AssetManagementService: Attempting to create asset: " + assetDTO.assetName + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Asset.CreateAsset", "Bạn không có quyền tạo tài sản.")) {
        // Audit log permission denied explicitly
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::HIGH,
                       "Asset", "Asset", std::nullopt, "Asset", assetDTO.assetName,
                       std::nullopt, std::nullopt, "Asset creation failed: Unauthorized.", {},
                       std::nullopt, std::nullopt, false, "Unauthorized access.");
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (assetDTO.assetName.empty() || assetDTO.assetCode.empty()) {
        ERP::Logger::Logger::getInstance().warning("AssetManagementService: Invalid input for asset creation (empty name or code).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "AssetManagementService: Invalid input for asset creation.", "Thông tin tài sản không đầy đủ.");
        return std::nullopt;
    }

    // Check if asset code or serial number already exists
    if (assetCodeExists(assetDTO.assetCode, std::nullopt)) {
        ERP::Logger::Logger::getInstance().warning("AssetManagementService: Asset with code " + assetDTO.assetCode + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DuplicateEntry, "Mã tài sản đã tồn tại.");
        return std::nullopt;
    }
    if (assetDTO.serialNumber != "" && assetDAO_->count({{"serial_number", assetDTO.serialNumber}}) > 0) {
        ERP::Logger::Logger::getInstance().warning("AssetManagementService: Asset with serial number " + assetDTO.serialNumber + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DuplicateEntry, "Số serial tài sản đã tồn tại.");
        return std::nullopt;
    }

    // Validate location existence if specified
    if (assetDTO.locationId && (!locationService_->getLocationById(*assetDTO.locationId, userRoleIds))) {
        ERP::Logger::Logger::getInstance().warning("AssetManagementService: Location " + *assetDTO.locationId + " not found for asset creation.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Vị trí không tồn tại cho tài sản.");
        return std::nullopt;
    }
    // Validate production line existence if specified
    if (assetDTO.productionLineId && (!productionLineService_->getProductionLineById(*assetDTO.productionLineId, userRoleIds))) {
        ERP::Logger::Logger::getInstance().warning("AssetManagementService: Production line " + *assetDTO.productionLineId + " not found for asset creation.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Dây chuyền sản xuất không tồn tại cho tài sản.");
        return std::nullopt;
    }

    ERP::Asset::DTO::AssetDTO newAsset = assetDTO;
    newAsset.id = ERP::Utils::generateUUID();
    newAsset.registeredAt = ERP::Utils::DateUtils::now();
    newAsset.registeredByUserId = currentUserId;
    newAsset.createdAt = newAsset.registeredAt;
    newAsset.createdBy = currentUserId;
    newAsset.status = ERP::Common::EntityStatus::ACTIVE; // Default to active

    std::optional<ERP::Asset::DTO::AssetDTO> createdAsset = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Note: DAO methods already acquire/release connections internally now.
            // The transaction context ensures atomicity across multiple DAO calls if needed.
            if (!assetDAO_->create(newAsset)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("AssetManagementService: Failed to create asset " + newAsset.assetName + " in DAO.");
                return false;
            }
            createdAsset = newAsset;
            // Optionally, publish event for new asset creation
            // eventBus_.publish(std::make_shared<EventBus::AssetCreatedEvent>(newAsset));
            return true;
        },
        "AssetManagementService", "createAsset"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("AssetManagementService: Asset " + newAsset.assetName + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Asset", "Asset", newAsset.id, "Asset", newAsset.assetName,
                       std::nullopt, newAsset.toMap(), "Asset created.");
        return createdAsset;
    }
    return std::nullopt;
}

std::optional<ERP::Asset::DTO::AssetDTO> AssetManagementService::getAssetById(
    const std::string& assetId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("AssetManagementService: Retrieving asset by ID: " + assetId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Asset.ViewAsset", "Bạn không có quyền xem tài sản.")) {
        return std::nullopt;
    }

    return assetDAO_->getById(assetId); // Using getById from DAOBase template
}

std::optional<ERP::Asset::DTO::AssetDTO> AssetManagementService::getAssetByCode(
    const std::string& assetCode,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("AssetManagementService: Retrieving asset by code: " + assetCode + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Asset.ViewAsset", "Bạn không có quyền xem tài sản.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["asset_code"] = assetCode;
    std::vector<ERP::Asset::DTO::AssetDTO> assets = assetDAO_->get(filter); // Using get from DAOBase template
    if (!assets.empty()) {
        return assets[0];
    }
    ERP::Logger::Logger::getInstance().debug("AssetManagementService: Asset with code " + assetCode + " not found.");
    return std::nullopt;
}

std::vector<ERP::Asset::DTO::AssetDTO> AssetManagementService::getAllAssets(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("AssetManagementService: Retrieving all assets with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Asset.ViewAsset", "Bạn không có quyền xem tất cả tài sản.")) {
        return {};
    }

    return assetDAO_->get(filter); // Using get from DAOBase template
}

bool AssetManagementService::updateAsset(
    const ERP::Asset::DTO::AssetDTO& assetDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("AssetManagementService: Attempting to update asset: " + assetDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Asset.UpdateAsset", "Bạn không có quyền cập nhật tài sản.")) {
        return false;
    }

    std::optional<ERP::Asset::DTO::AssetDTO> oldAssetOpt = assetDAO_->getById(assetDTO.id); // Using getById from DAOBase
    if (!oldAssetOpt) {
        ERP::Logger::Logger::getInstance().warning("AssetManagementService: Asset with ID " + assetDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy tài sản cần cập nhật.");
        return false;
    }

    // If asset code or serial number conflicts with existing ones (excluding self)
    if (assetCodeExists(assetDTO.assetCode, assetDTO.id)) {
        ERP::Logger::Logger::getInstance().warning("AssetManagementService: Asset code " + assetDTO.assetCode + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DuplicateEntry, "Mã tài sản đã tồn tại.");
        return false;
    }
    if (assetDTO.serialNumber != "" && assetDAO_->count({{"serial_number", assetDTO.serialNumber}, {"id_ne", assetDTO.id}}) > 0) { // Assuming DAO supports "not equal" filter
        ERP::Logger::Logger::getInstance().warning("AssetManagementService: Serial number " + assetDTO.serialNumber + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DuplicateEntry, "Số serial tài sản đã tồn tại.");
        return false;
    }

    // Validate location existence if specified or changed
    if (assetDTO.locationId != oldAssetOpt->locationId) { // Only check if changed
        if (assetDTO.locationId && (!locationService_->getLocationById(*assetDTO.locationId, userRoleIds))) {
            ERP::Logger::Logger::getInstance().warning("AssetManagementService: Location " + *assetDTO.locationId + " not found for asset update.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Vị trí không tồn tại cho tài sản.");
            return false;
        }
    }
    // Validate production line existence if specified or changed
    if (assetDTO.productionLineId != oldAssetOpt->productionLineId) { // Only check if changed
        if (assetDTO.productionLineId && (!productionLineService_->getProductionLineById(*assetDTO.productionLineId, userRoleIds))) {
            ERP::Logger::Logger::getInstance().warning("AssetManagementService: Production line " + *assetDTO.productionLineId + " not found for asset update.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Dây chuyền sản xuất không tồn tại cho tài sản.");
            return false;
        }
    }

    ERP::Asset::DTO::AssetDTO updatedAsset = assetDTO;
    updatedAsset.updatedAt = ERP::Utils::DateUtils::now();
    updatedAsset.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!assetDAO_->update(updatedAsset)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("AssetManagementService: Failed to update asset " + updatedAsset.id + " in DAO.");
                return false;
            }
            // Optionally, publish event for asset update
            // eventBus_.publish(std::make_shared<EventBus::AssetUpdatedEvent>(updatedAsset));
            return true;
        },
        "AssetManagementService", "updateAsset"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("AssetManagementService: Asset " + updatedAsset.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Asset", "Asset", updatedAsset.id, "Asset", updatedAsset.assetName,
                       oldAssetOpt->toMap(), updatedAsset.toMap(), "Asset updated.");
        return true;
    }
    return false;
}

bool AssetManagementService::updateAssetState(
    const std::string& assetId,
    ERP::Asset::DTO::AssetState newState,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds,
    const std::optional<std::string>& reason) {
    ERP::Logger::Logger::getInstance().info("AssetManagementService: Attempting to update state for asset: " + assetId + " to " + ERP::Asset::DTO::AssetDTO().getStateString(newState) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Asset.UpdateAssetState", "Bạn không có quyền cập nhật trạng thái tài sản.")) {
        return false;
    }

    std::optional<ERP::Asset::DTO::AssetDTO> assetOpt = assetDAO_->getById(assetId); // Using getById from DAOBase
    if (!assetOpt) {
        ERP::Logger::Logger::getInstance().warning("AssetManagementService: Asset with ID " + assetId + " not found for state update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy tài sản để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Asset::DTO::AssetDTO oldAsset = *assetOpt;
    if (oldAsset.state == newState) {
        ERP::Logger::Logger::getInstance().info("AssetManagementService: Asset " + assetId + " is already in state " + oldAsset.getStateString() + ".");
        return true; // Already in desired state
    }

    ERP::Asset::DTO::AssetDTO updatedAsset = oldAsset;
    updatedAsset.state = newState;
    updatedAsset.updatedAt = ERP::Utils::DateUtils::now();
    updatedAsset.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!assetDAO_->update(updatedAsset)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("AssetManagementService: Failed to update state for asset " + assetId + " in DAO.");
                return false;
            }
            // Optionally, publish event for state change
            // eventBus_.publish(std::make_shared<EventBus::AssetStateChangedEvent>(assetId, newState));
            return true;
        },
        "AssetManagementService", "updateAssetState"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("AssetManagementService: State for asset " + assetId + " updated successfully to " + updatedAsset.getStateString() + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Asset", "AssetState", assetId, "Asset", oldAsset.assetName,
                       oldAsset.toMap(), updatedAsset.toMap(), "Asset state changed to " + updatedAsset.getStateString() + ". Reason: " + reason.value_or("N/A") + ".");
        return true;
    }
    return false;
}

bool AssetManagementService::deleteAsset(
    const std::string& assetId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("AssetManagementService: Attempting to delete asset: " + assetId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Asset.DeleteAsset", "Bạn không có quyền xóa tài sản.")) {
        return false;
    }

    std::optional<ERP::Asset::DTO::AssetDTO> assetOpt = assetDAO_->getById(assetId); // Using getById from DAOBase
    if (!assetOpt) {
        ERP::Logger::Logger::getInstance().warning("AssetManagementService: Asset with ID " + assetId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy tài sản cần xóa.");
        return false;
    }

    ERP::Asset::DTO::AssetDTO assetToDelete = *assetOpt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Check if asset is currently in use (e.g., assigned to a work order, production line)
            if (isAssetInUse(assetId, userRoleIds)) { // Placeholder call
                ERP::Logger::Logger::getInstance().warning("AssetManagementService: Cannot delete asset " + assetId + " as it is currently in use.");
                ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa tài sản đang được sử dụng. Vui lòng gỡ bỏ nó trước.");
                return false;
            }
            if (!assetDAO_->remove(assetId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("AssetManagementService: Failed to delete asset " + assetId + " in DAO.");
                return false;
            }
            return true;
        },
        "AssetManagementService", "deleteAsset"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("AssetManagementService: Asset " + assetId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Asset", "Asset", assetId, "Asset", assetToDelete.assetName,
                       assetToDelete.toMap(), std::nullopt, "Asset deleted.");
        return true;
    }
    return false;
}

std::vector<ERP::Asset::DTO::AssetDTO> AssetManagementService::getAssetsRequiringCalibration(
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("AssetManagementService: Retrieving assets requiring calibration.");

    if (!checkPermission(currentUserId, userRoleIds, "Asset.ViewAssetCalibrationNeeds", "Bạn không có quyền xem tài sản cần hiệu chuẩn.")) {
        return {};
    }
    
    // This query would typically involve checking a "last_calibration_date" and "calibration_frequency"
    // or a dedicated "asset_calibration_schedule" table.
    // For simplicity, simulate by returning some active assets
    std::map<std::string, std::any> activeAssetsFilter;
    activeAssetsFilter["state"] = static_cast<int>(ERP::Asset::DTO::AssetState::ACTIVE);
    std::vector<ERP::Asset::DTO::AssetDTO> allActiveAssets = assetDAO_->get(activeAssetsFilter); // Using get from DAOBase
    std::vector<ERP::Asset::DTO::AssetDTO> calibrationNeededAssets;

    // Simulate some assets needing calibration (e.g., equipment assets with codes starting with "EQ-")
    for (const auto& asset : allActiveAssets) {
        if (asset.type == ERP::Asset::DTO::AssetType::EQUIPMENT && asset.assetCode.rfind("EQ-", 0) == 0) { // Check prefix
            // Add more complex logic here, e.g., if (asset.lastCalibrationDate + asset.calibrationFrequency < now)
            calibrationNeededAssets.push_back(asset);
        }
    }
    return calibrationNeededAssets;
}

bool AssetManagementService::recordAssetCalibration(
    const std::string& assetId,
    const std::chrono::system_clock::time_point& calibrationDate,
    const std::string& calibratedByUserId,
    const std::vector<std::string>& userRoleIds,
    const std::optional<std::chrono::system_clock::time_point>& nextCalibrationDate) {
    ERP::Logger::Logger::getInstance().info("AssetManagementService: Attempting to record calibration for asset: " + assetId + " by " + calibratedByUserId + " on " + ERP::Utils::DateUtils::formatDateTime(calibrationDate, ERP::Common::DATETIME_FORMAT) + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Asset.RecordAssetCalibration", "Bạn không có quyền ghi nhận hiệu chuẩn tài sản.")) {
        return false;
    }

    std::optional<ERP::Asset::DTO::AssetDTO> assetOpt = assetDAO_->getById(assetId); // Using getById from DAOBase
    if (!assetOpt) {
        ERP::Logger::Logger::getInstance().warning("AssetManagementService: Asset with ID " + assetId + " not found for calibration.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy tài sản để hiệu chuẩn.");
        return false;
    }
    
    ERP::Asset::DTO::AssetDTO oldAsset = *assetOpt;
    
    // For audit logging, convert map to QJsonObject for now, to be removed later
    QJsonObject beforeDataJson = ERP::Utils::DTOUtils::mapToQJsonObject(oldAsset.toMap());

    ERP::Asset::DTO::AssetDTO updatedAsset = oldAsset;
    updatedAsset.updatedAt = ERP::Utils::DateUtils::now();
    updatedAsset.updatedBy = calibratedByUserId;
    
    // Update asset state (e.g., from CALIBRATION to ACTIVE) if needed
    if (updatedAsset.state == ERP::Asset::DTO::AssetState::CALIBRATION) {
        // This internal call should use BaseService's checkPermission for consistency.
        // It's a bit circular here as it calls back to itself.
        // Best practice: make updateAssetState handle permissions for its own action only.
        updateAssetState(assetId, ERP::Asset::DTO::AssetState::ACTIVE, calibratedByUserId, userRoleIds, "Completed calibration");
    }

    // Update metadata for calibration record
    // Note: Asset.h now uses std::map<string, any> for metadata
    std::map<std::string, std::any> calibrationMetadata = updatedAsset.metadata;
    calibrationMetadata["last_calibration_date"] = ERP::Utils::DateUtils::formatDateTime(calibrationDate, ERP::Common::DATETIME_FORMAT);
    calibrationMetadata["calibrated_by_user_id"] = calibratedByUserId;
    if (nextCalibrationDate) {
        calibrationMetadata["next_calibration_date"] = ERP::Utils::DateUtils::formatDateTime(*nextCalibrationDate, ERP::Common::DATETIME_FORMAT);
    }
    updatedAsset.metadata = calibrationMetadata; // Update the asset's metadata

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!assetDAO_->update(updatedAsset)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("AssetManagementService: Failed to record calibration for asset " + assetId + " in DAO.");
                return false;
            }
            // Optionally, publish event for calibration
            // eventBus_.publish(std::make_shared<EventBus::AssetCalibratedEvent>(assetId, calibrationDate));
            return true;
        },
        "AssetManagementService", "recordAssetCalibration"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("AssetManagementService: Asset calibration recorded successfully for asset: " + assetId);
        // For audit logging, convert map to QJsonObject for now, to be removed later
        QJsonObject afterDataJson = ERP::Utils::DTOUtils::mapToQJsonObject(updatedAsset.toMap());
        QJsonObject metadataLogJson = ERP::Utils::DTOUtils::mapToQJsonObject(calibrationMetadata);

        recordAuditLog(calibratedByUserId, securityManager_->getUserService()->getUserName(calibratedByUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::EQUIPMENT_CALIBRATION, ERP::Common::LogSeverity::INFO,
                       "Asset", "Calibration", assetId, "Asset", oldAsset.assetName,
                       beforeDataJson, afterDataJson, "Asset calibrated.", metadataLogJson, std::nullopt, std::nullopt, true, std::nullopt);
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Asset
} // namespace ERP