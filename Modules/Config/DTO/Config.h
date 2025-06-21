// Modules/Config/DTO/Config.h
#ifndef MODULES_CONFIG_DTO_CONFIG_H
#define MODULES_CONFIG_DTO_CONFIG_H
#include <string>
#include <optional>
#include <chrono>
#include <map> // For std::map<std::string, std::any> replacement of QJsonObject
#include <any> // For std::any replacement of QJsonObject

// #include <QJsonObject> // Removed
#include "DataObjects/BaseDTO.h"   // ĐÃ SỬA: Dùng tên tệp trực tiếp
#include "Modules/Common/Common.h" // For EntityStatus (Updated include path)

namespace ERP {
namespace Config {
namespace DTO {
/**
 * @brief Enum for configuration types.
 */
enum class ConfigType {
    STRING,
    INTEGER,
    BOOLEAN,
    DOUBLE,
    JSON,
    DATETIME,
    PASSWORD // Value should be encrypted
};
/**
 * @brief DTO for Config entity.
 * Represents an application configuration setting.
 */
struct ConfigDTO : public BaseDTO {
    std::string configKey;      /**< Unique key for the configuration setting (e.g., "System.Database.Host"). */
    std::string configValue;    /**< The value of the configuration setting. */
    ConfigType configType;      /**< Type of the configuration value (for parsing/validation). */
    std::optional<std::string> description; /**< Description of the setting. */
    bool isEncrypted;           /**< True if configValue is encrypted. */
    std::map<std::string, std::any> metadata; /**< Additional metadata for the config (e.g., validation rules, UI hints). */

    ConfigDTO() : BaseDTO(), configKey(""), configValue(""), configType(ConfigType::STRING), isEncrypted(false) {}
    virtual ~ConfigDTO() = default;

    // Helper to convert enum to string (or central StringUtils)
    std::string getTypeString() const {
        switch (configType) {
            case ConfigType::STRING: return "String";
            case ConfigType::INTEGER: return "Integer";
            case ConfigType::BOOLEAN: return "Boolean";
            case ConfigType::DOUBLE: return "Double";
            case ConfigType::JSON: return "JSON";
            case ConfigType::DATETIME: return "DateTime";
            case ConfigType::PASSWORD: return "Password";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Config
} // namespace ERP
#endif // MODULES_CONFIG_DTO_CONFIG_H