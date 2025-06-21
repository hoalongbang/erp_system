// Modules/Manufacturing/DAO/ProductionLineDAO.cpp
#include "Modules/Manufacturing/DAO/ProductionLineDAO.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "DAOHelpers.h"
#include "Modules/Utils/DTOUtils.h" // For common DTO to map conversions
#include <nlohmann/json.hpp> // For JSON serialization/deserialization of std::map<string, any>
#include <sstream>
#include <stdexcept>
#include <typeinfo> // For std::bad_any_cast

namespace ERP {
namespace Manufacturing {
namespace DAOs {

using json = nlohmann::json;

ProductionLineDAO::ProductionLineDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Manufacturing::DTO::ProductionLineDTO>(connectionPool, "production_lines") { // Pass table name for ProductionLineDTO
    Logger::Logger::getInstance().info("ProductionLineDAO: Initialized.");
}

// toMap for ProductionLineDTO
std::map<std::string, std::any> ProductionLineDAO::toMap(const ERP::Manufacturing::DTO::ProductionLineDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["line_name"] = dto.lineName;
    ERP::DAOHelpers::putOptionalString(data, "description", dto.description);
    data["status"] = static_cast<int>(dto.status);
    data["location_id"] = dto.locationId;

    // Serialize vector of strings (associatedAssetIds) to JSON array string
    try {
        json assetIdsJson = json::array();
        for (const auto& id : dto.associatedAssetIds) {
            assetIdsJson.push_back(id);
        }
        data["associated_asset_ids_json"] = assetIdsJson.dump();
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("ProductionLineDAO: toMap - Error serializing associated_asset_ids: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ProductionLineDAO: Error serializing associated assets.");
        data["associated_asset_ids_json"] = "";
    }

    // Serialize std::map<string, any> to JSON string for configuration and metadata
    try {
        if (!dto.configuration.empty()) {
            json configJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.configuration));
            data["configuration_json"] = configJson.dump();
        } else {
            data["configuration_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("ProductionLineDAO: toMap - Error serializing configuration: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ProductionLineDAO: Error serializing configuration.");
        data["configuration_json"] = "";
    }

    try {
        if (!dto.metadata.empty()) {
            json metadataJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.metadata));
            data["metadata_json"] = metadataJson.dump();
        } else {
            data["metadata_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("ProductionLineDAO: toMap - Error serializing metadata: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ProductionLineDAO: Error serializing metadata.");
        data["metadata_json"] = "";
    }
    return data;
}

// fromMap for ProductionLineDTO
ERP::Manufacturing::DTO::ProductionLineDTO ProductionLineDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Manufacturing::DTO::ProductionLineDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "line_name", dto.lineName);
        ERP::DAOHelpers::getOptionalStringValue(data, "description", dto.description);
        
        int statusInt;
        if (ERP::DAOHelpers::getPlainValue(data, "status", statusInt)) dto.status = static_cast<ERP::Manufacturing::DTO::ProductionLineStatus>(statusInt);
        
        ERP::DAOHelpers::getPlainValue(data, "location_id", dto.locationId);

        // Deserialize JSON string to vector of strings (associatedAssetIds)
        if (data.count("associated_asset_ids_json") && data.at("associated_asset_ids_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("associated_asset_ids_json"));
            if (!jsonStr.empty()) {
                try {
                    json assetIdsJson = json::parse(jsonStr);
                    if (assetIdsJson.is_array()) {
                        for (const auto& element : assetIdsJson) {
                            if (element.is_string()) {
                                dto.associatedAssetIds.push_back(element.get<std::string>());
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("ProductionLineDAO: fromMap - Error deserializing associated_asset_ids: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ProductionLineDAO: Error deserializing associated assets.");
                }
            }
        }

        // Deserialize JSON string to std::map<string, any> for configuration and metadata
        if (data.count("configuration_json") && data.at("configuration_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("configuration_json"));
            if (!jsonStr.empty()) {
                dto.configuration = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
        if (data.count("metadata_json") && data.at("metadata_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("metadata_json"));
            if (!jsonStr.empty()) {
                dto.metadata = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("ProductionLineDAO: fromMap - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ProductionLineDAO: Data type mismatch in fromMap.");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("ProductionLineDAO: fromMap - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ProductionLineDAO: Unexpected error in fromMap.");
    }
    return dto;
}

} // namespace DAOs
} // namespace Manufacturing
} // namespace ERP