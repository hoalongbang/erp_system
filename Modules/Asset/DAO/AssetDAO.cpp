// Modules/Asset/DAO/AssetDAO.cpp
#include "Modules/Asset/DAO/AssetDAO.h"
#include "Modules/Utils/DTOUtils.h"    // For common DTO to map conversions
#include "Modules/Utils/DateUtils.h"   // For date/time formatting
#include "Modules/Utils/StringUtils.h" // For enum to string conversion

// #include "Modules/Security/Service/EncryptionService.h" // Encryption handled at service layer if needed
#include <nlohmann/json.hpp> // For JSON serialization/deserialization of std::map<string, any>
#include <typeinfo>          // Required for std::bad_any_cast

namespace ERP {
namespace Asset {
namespace DAOs {

// Use nlohmann json namespace
using json = nlohmann::json;

AssetDAO::AssetDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Asset::DTO::AssetDTO>(connectionPool, "assets") { // Pass table name to base constructor
}

std::map<std::string, std::any> AssetDAO::toMap(const ERP::Asset::DTO::AssetDTO& asset) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(asset); // Use DTOUtils for BaseDTO fields

    data["asset_name"] = asset.assetName;
    data["asset_code"] = asset.assetCode;
    data["type"] = static_cast<int>(asset.type);
    data["state"] = static_cast<int>(asset.state);
    ERP::DAOHelpers::putOptionalString(data, "description", asset.description);

    data["serial_number"] = asset.serialNumber;
    data["manufacturer"] = asset.manufacturer;
    data["model"] = asset.model;
    ERP::DAOHelpers::putOptionalTime(data, "purchase_date", asset.purchaseDate);
    data["purchase_cost"] = asset.purchaseCost;
    ERP::DAOHelpers::putOptionalString(data, "currency", asset.currency);
    ERP::DAOHelpers::putOptionalTime(data, "installation_date", asset.installationDate);
    ERP::DAOHelpers::putOptionalTime(data, "warranty_end_date", asset.warrantyEndDate);

    ERP::DAOHelpers::putOptionalString(data, "location_id", asset.locationId);
    ERP::DAOHelpers::putOptionalString(data, "production_line_id", asset.productionLineId);

    // Serialize std::map<string, any> to JSON string for configuration and metadata
    try {
        if (!asset.configuration.empty()) {
            json configJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(asset.configuration));
            data["configuration_json"] = configJson.dump();
        } else {
            data["configuration_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("AssetDAO: toMap - Error serializing configuration: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "AssetDAO: Error serializing configuration.");
        data["configuration_json"] = "";
    }

    try {
        if (!asset.metadata.empty()) {
            json metadataJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(asset.metadata));
            data["metadata_json"] = metadataJson.dump();
        } else {
            data["metadata_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("AssetDAO: toMap - Error serializing metadata: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "AssetDAO: Error serializing metadata.");
        data["metadata_json"] = "";
    }

    data["is_active"] = asset.isActive;
    data["condition"] = static_cast<int>(asset.condition);
    data["current_value"] = asset.currentValue;

    data["registered_by_user_id"] = asset.registeredByUserId;
    data["registered_at"] = ERP::Utils::DateUtils::formatDateTime(asset.registeredAt, ERP::Common::DATETIME_FORMAT);

    return data;
}

ERP::Asset::DTO::AssetDTO AssetDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Asset::DTO::AssetDTO asset;
    ERP::Utils::DTOUtils::fromMap(data, asset); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "asset_name", asset.assetName);
        ERP::DAOHelpers::getPlainValue(data, "asset_code", asset.assetCode);
        
        int typeInt;
        if (ERP::DAOHelpers::getPlainValue(data, "type", typeInt)) asset.type = static_cast<ERP::Asset::DTO::AssetType>(typeInt);
        
        int stateInt;
        if (ERP::DAOHelpers::getPlainValue(data, "state", stateInt)) asset.state = static_cast<ERP::Asset::DTO::AssetState>(stateInt);
        
        ERP::DAOHelpers::getOptionalStringValue(data, "description", asset.description);
        ERP::DAOHelpers::getPlainValue(data, "serial_number", asset.serialNumber);
        ERP::DAOHelpers::getPlainValue(data, "manufacturer", asset.manufacturer);
        ERP::DAOHelpers::getPlainValue(data, "model", asset.model);
        ERP::DAOHelpers::getOptionalTimeValue(data, "purchase_date", asset.purchaseDate);
        ERP::DAOHelpers::getPlainValue(data, "purchase_cost", asset.purchaseCost);
        ERP::DAOHelpers::getOptionalStringValue(data, "currency", asset.currency);
        ERP::DAOHelpers::getOptionalTimeValue(data, "installation_date", asset.installationDate);
        ERP::DAOHelpers::getOptionalTimeValue(data, "warranty_end_date", asset.warrantyEndDate);
        ERP::DAOHelpers::getOptionalStringValue(data, "location_id", asset.locationId);
        ERP::DAOHelpers::getOptionalStringValue(data, "production_line_id", asset.productionLineId);

        // Deserialize JSON string to std::map<string, any> for configuration and metadata
        if (data.count("configuration_json") && data.at("configuration_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("configuration_json"));
            if (!jsonStr.empty()) {
                asset.configuration = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
        if (data.count("metadata_json") && data.at("metadata_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("metadata_json"));
            if (!jsonStr.empty()) {
                asset.metadata = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }

        ERP::DAOHelpers::getPlainValue(data, "is_active", asset.isActive);
        int conditionInt;
        if (ERP::DAOHelpers::getPlainValue(data, "condition", conditionInt)) asset.condition = static_cast<ERP::Asset::DTO::AssetCondition>(conditionInt);
        ERP::DAOHelpers::getPlainValue(data, "current_value", asset.currentValue);

        ERP::DAOHelpers::getPlainValue(data, "registered_by_user_id", asset.registeredByUserId);
        ERP::DAOHelpers::getPlainTimeValue(data, "registered_at", asset.registeredAt);

    } catch (const std::bad_any_cast& e) {
        ERP::Logger::Logger::getInstance().error("AssetDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Failed to cast data for AssetDTO: " + std::string(e.what()));
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("AssetDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Unexpected error in fromMap for AssetDTO: " + std::string(e.what()));
    }
    return asset;
}

} // namespace DAOs
} // namespace Asset
} // namespace ERP