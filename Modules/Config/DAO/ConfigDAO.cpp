// Modules/Config/DAO/ConfigDAO.cpp
#include "Modules/Config/DAO/ConfigDAO.h"
#include "Modules/Utils/DTOUtils.h"    // For common DTO to map conversions
#include "Modules/Utils/DateUtils.h"   // For date/time formatting
#include <nlohmann/json.hpp> // For JSON serialization/deserialization of std::map<string, any>
#include <typeinfo>          // Required for std::bad_any_cast

namespace ERP {
namespace Config {
namespace DAOs {

// Use nlohmann json namespace
using json = nlohmann::json;

ConfigDAO::ConfigDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Config::DTO::ConfigDTO>(connectionPool, "configurations") { // Pass table name to base constructor
}

std::map<std::string, std::any> ConfigDAO::toMap(const ERP::Config::DTO::ConfigDTO& config) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(config); // Use DTOUtils for BaseDTO fields

    data["config_key"] = config.configKey;
    data["config_value"] = config.configValue;
    data["config_type"] = static_cast<int>(config.configType);
    ERP::DAOHelpers::putOptionalString(data, "description", config.description);
    data["is_encrypted"] = config.isEncrypted;

    // Serialize std::map<string, any> to JSON string for metadata
    try {
        if (!config.metadata.empty()) {
            json metadataJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(config.metadata));
            data["metadata_json"] = metadataJson.dump();
        } else {
            data["metadata_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("ConfigDAO: toMap - Error serializing metadata: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ConfigDAO: Error serializing metadata.");
        data["metadata_json"] = "";
    }
    return data;
}

ERP::Config::DTO::ConfigDTO ConfigDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Config::DTO::ConfigDTO config;
    ERP::Utils::DTOUtils::fromMap(data, config); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "config_key", config.configKey);
        ERP::DAOHelpers::getPlainValue(data, "config_value", config.configValue);
        
        int typeInt;
        if (ERP::DAOHelpers::getPlainValue(data, "config_type", typeInt)) config.configType = static_cast<ERP::Config::DTO::ConfigType>(typeInt);
        
        ERP::DAOHelpers::getOptionalStringValue(data, "description", config.description);
        ERP::DAOHelpers::getPlainValue(data, "is_encrypted", config.isEncrypted);

        // Deserialize JSON string to std::map<string, any> for metadata
        if (data.count("metadata_json") && data.at("metadata_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("metadata_json"));
            if (!jsonStr.empty()) {
                config.metadata = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
    } catch (const std::bad_any_cast& e) {
        ERP::Logger::Logger::getInstance().error("ConfigDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Failed to cast data for ConfigDTO: " + std::string(e.what()));
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("ConfigDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Unexpected error in fromMap for ConfigDTO: " + std::string(e.what()));
    }
    return config;
}

} // namespace DAOs
} // namespace Config
} // namespace ERP