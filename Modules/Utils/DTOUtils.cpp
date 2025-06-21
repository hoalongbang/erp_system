// Modules/Utils/DTOUtils.cpp
#include "DTOUtils.h" // Đã rút gọn include
#include "Logger.h"   // Đã rút gọn include
#include "ErrorHandler.h" // Đã rút gọn include
#include "Common.h"   // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include

#include <nlohmann/json.hpp> // For JSON serialization/deserialization

// Required for QJson conversions
#include <QJsonArray>
#include <QJsonDocument>
#include <QVariant>
#include <QVariantMap>

namespace ERP {
namespace Utils {

// Implementation of QJsonObjectToMap
std::map<std::string, std::any> DTOUtils::QJsonObjectToMap(const QJsonObject& jsonObject) {
    std::map<std::string, std::any> dataMap;
    for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it) {
        QString key = it.key();
        QJsonValue value = it.value();

        if (value.isBool()) {
            dataMap[key.toStdString()] = value.toBool();
        } else if (value.isDouble()) {
            dataMap[key.toStdString()] = value.toDouble();
        } else if (value.isString()) {
            dataMap[key.toStdString()] = value.toString().toStdString();
        } else if (value.isArray()) {
            QJsonArray jsonArray = value.toArray();
            std::vector<std::any> anyArray;
            for (const QJsonValueRef arrayValue : jsonArray) {
                if (arrayValue.isBool()) {
                    anyArray.push_back(arrayValue.toBool());
                } else if (arrayValue.isDouble()) {
                    anyArray.push_back(arrayValue.toDouble());
                } else if (arrayValue.isString()) {
                    anyArray.push_back(arrayValue.toString().toStdString());
                } else if (arrayValue.isObject()) {
                    anyArray.push_back(QJsonObjectToMap(arrayValue.toObject()));
                } else {
                    // Handle null or other types if necessary
                    anyArray.push_back(std::any());
                }
            }
            dataMap[key.toStdString()] = anyArray;
        } else if (value.isObject()) {
            dataMap[key.toStdString()] = QJsonObjectToMap(value.toObject());
        } else if (value.isNull()) {
            dataMap[key.toStdString()] = std::any(); // Represent null
        }
    }
    return dataMap;
}

// Implementation of mapToQJsonObject
QJsonObject DTOUtils::mapToQJsonObject(const std::map<std::string, std::any>& dataMap) {
    QJsonObject jsonObject;
    for (const auto& pair : dataMap) {
        QString key = QString::fromStdString(pair.first);
        const std::any& value = pair.second;

        if (value.type() == typeid(bool)) {
            jsonObject[key] = std::any_cast<bool>(value);
        } else if (value.type() == typeid(int)) {
            jsonObject[key] = std::any_cast<int>(value);
        } else if (value.type() == typeid(long long)) {
            jsonObject[key] = static_cast<double>(std::any_cast<long long>(value)); // Convert long long to double for QJsonValue
        } else if (value.type() == typeid(double)) {
            jsonObject[key] = std::any_cast<double>(value);
        } else if (value.type() == typeid(std::string)) {
            jsonObject[key] = QString::fromStdString(std::any_cast<std::string>(value));
        } else if (value.type() == typeid(const char*)) { // Handle C-style strings
            jsonObject[key] = QString::fromUtf8(std::any_cast<const char*>(value));
        } else if (value.type() == typeid(std::chrono::system_clock::time_point)) {
            // Convert time_point to formatted string
            jsonObject[key] = QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(std::any_cast<std::chrono::system_clock::time_point>(value), ERP::Common::DATETIME_FORMAT));
        } else if (value.type() == typeid(std::vector<std::any>)) {
            QJsonArray jsonArray;
            for (const auto& arrayValue : std::any_cast<const std::vector<std::any>&>(value)) {
                if (arrayValue.type() == typeid(bool)) {
                    jsonArray.append(std::any_cast<bool>(arrayValue));
                } else if (arrayValue.type() == typeid(int)) {
                    jsonArray.append(std::any_cast<int>(arrayValue));
                } else if (arrayValue.type() == typeid(long long)) {
                    jsonArray.append(static_cast<double>(std::any_cast<long long>(arrayValue)));
                } else if (arrayValue.type() == typeid(double)) {
                    jsonArray.append(std::any_cast<double>(arrayValue));
                } else if (arrayValue.type() == typeid(std::string)) {
                    jsonArray.append(QString::fromStdString(std::any_cast<std::string>(arrayValue)));
                } else if (arrayValue.type() == typeid(std::map<std::string, std::any>)) {
                    jsonArray.append(mapToQJsonObject(std::any_cast<std::map<std::string, std::any>>(arrayValue)));
                } else if (arrayValue.type() == typeid(std::chrono::system_clock::time_point)) {
                    jsonArray.append(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(std::any_cast<std::chrono::system_clock::time_point>(arrayValue), ERP::Common::DATETIME_FORMAT)));
                } else if (arrayValue.type() == typeid(std::any)) { // Handle empty std::any
                    jsonArray.append(QJsonValue::Null);
                }
                // Add more types as needed
            }
            jsonObject[key] = jsonArray;
        } else if (value.type() == typeid(std::map<std::string, std::any>)) {
            jsonObject[key] = mapToQJsonObject(std::any_cast<std::map<std::string, std::any>>(value));
        } else if (value.type() == typeid(std::any)) { // Handle empty std::any (e.g., std::optional<T> where T is empty)
            jsonObject[key] = QJsonValue::Null;
        } else {
            ERP::Logger::Logger::getInstance().warning("DTOUtils: mapToQJsonObject - Unsupported std::any type encountered for key: " + pair.first + ". Type: " + std::string(value.type().name()));
            // Optionally, handle as null or skip
            jsonObject[key] = QJsonValue::Null;
        }
    }
    return jsonObject;
}

// Implementation of jsonStringToMap
std::map<std::string, std::any> DTOUtils::jsonStringToMap(const std::string& jsonString) {
    if (jsonString.empty()) {
        return {};
    }
    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(jsonString).toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        ERP::Logger::Logger::getInstance().error("DTOUtils: jsonStringToMap - Failed to parse JSON string: " + jsonString);
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "DTOUtils: Failed to parse JSON string.");
        return {};
    }
    return QJsonObjectToMap(doc.object());
}

// Implementation of mapToJsonString
std::string DTOUtils::mapToJsonString(const std::map<std::string, std::any>& dataMap) {
    QJsonObject jsonObject = mapToQJsonObject(dataMap);
    QJsonDocument doc(jsonObject);
    return doc.toJson(QJsonDocument::Compact).toStdString();
}

} // namespace Utils
} // namespace ERP