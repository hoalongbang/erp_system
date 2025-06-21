// Modules/Asset/Service/IAssetManagementService.h
#ifndef MODULES_ASSET_SERVICE_IASSETMANAGEMENTSERVICE_H
#define MODULES_ASSET_SERVICE_IASSETMANAGEMENTSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <chrono> // For std::chrono::system_clock::time_point
#include <map>    // For std::map<std::string, std::any>

// Rút gọn các include paths
#include "Asset.h"         // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

namespace ERP {
namespace Asset {
namespace Services {

/**
 * @brief IAssetManagementService interface defines operations for managing assets.
 */
class IAssetManagementService {
public:
    virtual ~IAssetManagementService() = default;
    /**
     * @brief Creates a new asset.
     * @param assetDTO DTO containing new asset information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional AssetDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Asset::DTO::AssetDTO> createAsset(
        const ERP::Asset::DTO::AssetDTO& assetDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves asset information by ID.
     * @param assetId ID of the asset to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional AssetDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Asset::DTO::AssetDTO> getAssetById(
        const std::string& assetId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves asset information by asset code.
     * @param assetCode Asset code to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional AssetDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Asset::DTO::AssetDTO> getAssetByCode(
        const std::string& assetCode,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all assets or assets matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching AssetDTOs.
     */
    virtual std::vector<ERP::Asset::DTO::AssetDTO> getAllAssets(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates asset information.
     * @param assetDTO DTO containing updated asset information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateAsset(
        const ERP::Asset::DTO::AssetDTO& assetDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the state of an asset.
     * @param assetId ID of the asset.
     * @param newState New state.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @param reason Optional reason for the state change.
     * @return true if state update is successful, false otherwise.
     */
    virtual bool updateAssetState(
        const std::string& assetId,
        ERP::Asset::DTO::AssetState newState,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds,
        const std::optional<std::string>& reason = std::nullopt) = 0;
    /**
     * @brief Deletes an asset record by ID (soft delete).
     * @param assetId ID of the asset to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteAsset(
        const std::string& assetId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves assets that require calibration.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of AssetDTOs requiring calibration.
     */
    virtual std::vector<ERP::Asset::DTO::AssetDTO> getAssetsRequiringCalibration(
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Records an asset calibration event.
     * @param assetId ID of the asset.
     * @param calibrationDate Date of calibration.
     * @param calibratedByUserId User who performed calibration.
     * @param userRoleIds Roles of the user performing the operation.
     * @param nextCalibrationDate Optional: next scheduled calibration date.
     * @return true if successful, false otherwise.
     */
    virtual bool recordAssetCalibration(
        const std::string& assetId,
        const std::chrono::system_clock::time_point& calibrationDate,
        const std::string& calibratedByUserId,
        const std::vector<std::string>& userRoleIds,
        const std::optional<std::chrono::system_clock::time_point>& nextCalibrationDate = std::nullopt) = 0;
};

} // namespace Services
} // namespace Asset
} // namespace ERP
#endif // MODULES_ASSET_SERVICE_IASSETMANAGEMENTSERVICE_H