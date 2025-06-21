// Modules/Document/DAO/DocumentDAO.cpp
#include "Modules/Document/DAO/DocumentDAO.h"
#include "Modules/Database/ConnectionPool.h" // For database connection
#include "Logger/Logger.h"
#include "Modules/ErrorHandling/ErrorHandler.h"
#include "Modules/Common/Common.h"
#include "Modules/Utils/DateUtils.h"
#include "Modules/Utils/StringUtils.h" // For enum conversions
#include "Modules/Utils/DTOUtils.h"    // For common DTO to map conversions

#include <sstream>
#include <stdexcept>
#include <nlohmann/json.hpp> // For JSON serialization/deserialization of std::map<string, any>
// #include <QJsonDocument> // Removed

namespace ERP {
namespace Document {
namespace DAOs {

// Use nlohmann json namespace
using json = nlohmann::json;

DocumentDAO::DocumentDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Document::DTO::DocumentDTO>(connectionPool, "documents") { // Pass table name to base constructor
    Logger::Logger::getInstance().info("DocumentDAO: Initialized.");
}

std::map<std::string, std::any> DocumentDAO::toMap(const ERP::Document::DTO::DocumentDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Use DTOUtils for BaseDTO fields

    data["file_name"] = dto.fileName;
    data["internal_file_path"] = dto.internalFilePath;
    data["mime_type"] = dto.mimeType;
    data["file_size"] = dto.fileSize; // long long maps directly to std::any
    data["type"] = static_cast<int>(dto.type);
    ERP::DAOHelpers::putOptionalString(data, "description", dto.description);

    ERP::DAOHelpers::putOptionalString(data, "related_entity_id", dto.relatedEntityId);
    ERP::DAOHelpers::putOptionalString(data, "related_entity_type", dto.relatedEntityType);

    data["uploaded_by_user_id"] = dto.uploadedByUserId;
    data["upload_time"] = ERP::Utils::DateUtils::formatDateTime(dto.uploadTime, ERP::Common::DATETIME_FORMAT);

    // Serialize std::map<string, any> to JSON string for metadata
    try {
        if (!dto.metadata.empty()) {
            json metadataJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.metadata));
            data["metadata_json"] = metadataJson.dump();
        } else {
            data["metadata_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("DocumentDAO: toMap - Error serializing metadata: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "DocumentDAO: Error serializing metadata.");
        data["metadata_json"] = "";
    }

    data["is_public"] = dto.isPublic;
    ERP::DAOHelpers::putOptionalString(data, "storage_location", dto.storageLocation);

    return data;
}

ERP::Document::DTO::DocumentDTO DocumentDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Document::DTO::DocumentDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "file_name", dto.fileName);
        ERP::DAOHelpers::getPlainValue(data, "internal_file_path", dto.internalFilePath);
        ERP::DAOHelpers::getPlainValue(data, "mime_type", dto.mimeType);
        
        long long fileSizeLong; // SQLite might return int, convert to long long
        if (data.count("file_size") && data.at("file_size").type() == typeid(long long)) {
            dto.fileSize = std::any_cast<long long>(data.at("file_size"));
        } else if (data.count("file_size") && data.at("file_size").type() == typeid(int)) {
            dto.fileSize = static_cast<long long>(std::any_cast<int>(data.at("file_size")));
        }

        int typeInt;
        if (ERP::DAOHelpers::getPlainValue(data, "type", typeInt)) dto.type = static_cast<ERP::Document::DTO::DocumentType>(typeInt);
        
        ERP::DAOHelpers::getOptionalStringValue(data, "description", dto.description);
        ERP::DAOHelpers::getOptionalStringValue(data, "related_entity_id", dto.relatedEntityId);
        ERP::DAOHelpers::getOptionalStringValue(data, "related_entity_type", dto.relatedEntityType);
        ERP::DAOHelpers::getPlainValue(data, "uploaded_by_user_id", dto.uploadedByUserId);
        ERP::DAOHelpers::getPlainTimeValue(data, "upload_time", dto.uploadTime);

        // Deserialize JSON string to std::map<string, any> for metadata
        if (data.count("metadata_json") && data.at("metadata_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("metadata_json"));
            if (!jsonStr.empty()) {
                dto.metadata = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
        
        ERP::DAOHelpers::getPlainValue(data, "is_public", dto.isPublic);
        ERP::DAOHelpers::getOptionalStringValue(data, "storage_location", dto.storageLocation);

    } catch (const std::bad_any_cast& e) {
        ERP::Logger::Logger::getInstance().error("DocumentDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "Failed to cast data for DocumentDTO: " + std::string(e.what()));
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("DocumentDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "Unexpected error in fromMap for DocumentDTO: " + std::string(e.what()));
    }
    return dto;
}

} // namespace DAOs
} // namespace Document
} // namespace ERP