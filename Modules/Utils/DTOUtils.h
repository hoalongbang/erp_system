// Modules/Utils/DTOUtils.h
#ifndef MODULES_UTILS_DTOUTILS_H
#define MODULES_UTILS_DTOUTILS_H
#include <string>       // For std::string
#include <map>          // For std::map
#include <any>          // For std::any
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point
#include <vector>       // For std::vector (e.g., of nested DTOs)

// Include DAOHelpers for existing utility functions
#include "DAOHelpers.h"
#include "Common.h" // For DATETIME_FORMAT
#include "DateUtils.h" // For formatDateTime, parseDateTime
#include "Logger.h" // For logging errors
#include "ErrorHandler.h" // For error handling

// Include Qt headers if QJsonObject conversions are provided here
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QVariant>
#include <QVariantMap>
#include <QDateTime>

namespace ERP {
namespace Utils {
/**
 * @brief DTOUtils class provides utility functions for converting between
 * DTO objects and generic data structures (like std::map<std::string, std::any>),
 * and for handling common DTO field operations.
 * Also includes helpers for converting to/from QJsonObject for Qt interoperability.
 */
class DTOUtils {
public:
    /**
     * @brief Populates the BaseDTO fields from a map<string, any> data source.
     * @tparam T The DTO type, must inherit from BaseDTO.
     * @param data The source map containing data.
     * @param dto The DTO object to populate.
     */
    template<typename T, typename = std::enable_if_t<std::is_base_of<ERP::DataObjects::BaseDTO, T>::value>>
    static void fromMap(const std::map<std::string, std::any>& data, T& dto) {
        try {
            ERP::DAOHelpers::getPlainValue(data, "id", dto.id);
            int statusInt;
            if (ERP::DAOHelpers::getPlainValue(data, "status", statusInt)) {
                dto.status = static_cast<ERP::Common::EntityStatus>(statusInt);
            } else {
                dto.status = ERP::Common::EntityStatus::UNKNOWN;
            }
            ERP::DAOHelpers::getPlainTimeValue(data, "created_at", dto.createdAt);
            ERP::DAOHelpers::getOptionalTimeValue(data, "updated_at", dto.updatedAt);
            ERP::DAOHelpers::getOptionalStringValue(data, "created_by", dto.createdBy);
            ERP::DAOHelpers::getOptionalStringValue(data, "updated_by", dto.updatedBy);
        } catch (const std::bad_any_cast& e) {
            ERP::Logger::Logger::getInstance().error("DTOUtils: fromMap - Data type mismatch in BaseDTO fields: " + std::string(e.what()));
            ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "DTOUtils: Data type mismatch in BaseDTO fields.");
        } catch (const std::exception& e) {
            ERP::Logger::Logger::getInstance().error("DTOUtils: fromMap - Unexpected error populating BaseDTO fields: " + std::string(e.what()));
            ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "DTOUtils: Unexpected error populating BaseDTO fields.");
        }
    }

    /**
     * @brief Converts BaseDTO fields of a DTO object into a data map.
     * @tparam T The DTO type, must inherit from BaseDTO.
     * @param dto The DTO object to convert.
     * @return A map containing BaseDTO fields.
     */
    template<typename T, typename = std::enable_if_t<std::is_base_of<ERP::DataObjects::BaseDTO, T>::value>>
    static std::map<std::string, std::any> toMap(const T& dto) {
        std::map<std::string, std::any> data;
        data["id"] = dto.id;
        data["status"] = static_cast<int>(dto.status);
        data["created_at"] = ERP::Utils::DateUtils::formatDateTime(dto.createdAt, ERP::Common::DATETIME_FORMAT);
        ERP::DAOHelpers::putOptionalTime(data, "updated_at", dto.updatedAt);
        ERP::DAOHelpers::putOptionalString(data, "created_by", dto.createdBy);
        ERP::DAOHelpers::putOptionalString(data, "updated_by", dto.updatedBy);
        return data;
    }

    // --- Conversion to/from QJsonObject (for UI Layer or specific integrations) ---

    /**
     * @brief Converts a QJsonObject to std::map<std::string, std::any>.
     * Handles basic types (string, int, double, bool), QJsonArray, QJsonObject.
     * QJsonArray and nested QJsonObject are converted recursively.
     * @param jsonObject The QJsonObject to convert.
     * @return The converted std::map<std::string, std::any>.
     */
    static std::map<std::string, std::any> QJsonObjectToMap(const QJsonObject& jsonObject);

    /**
     * @brief Converts a std::map<std::string, std::any> to QJsonObject.
     * Handles basic types, std::vector<std::any>, std::map<std::string, std::any>.
     * std::vector<std::any> and nested std::map<std::string, std::any> are converted recursively.
     * @param dataMap The std::map<std::string, std::any> to convert.
     * @return The converted QJsonObject.
     */
    static QJsonObject mapToQJsonObject(const std::map<std::string, std::any>& dataMap);

    /**
     * @brief Converts a JSON string to std::map<std::string, std::any>.
     * Internally uses QJsonDocument to parse the string.
     * @param jsonString The JSON string to convert.
     * @return The converted std::map<std::string, std::any>. Returns empty map on parse error.
     */
    static std::map<std::string, std::any> jsonStringToMap(const std::string& jsonString);

    /**
     * @brief Converts a std::map<std::string, std::any> to a JSON string.
     * Internally uses QJsonDocument to convert the QJsonObject representation to string.
     * @param dataMap The std::map<std::string, std::any> to convert.
     * @return The JSON string representation. Returns empty string on error.
     */
    static std::string mapToJsonString(const std::map<std::string, std::any>& dataMap);
};

} // namespace Utils
} // namespace ERP
#endif // MODULES_UTILS_DTOUTILS_H