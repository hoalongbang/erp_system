// Modules/TaskEngine/DAO/TaskLogDAO.cpp
#include "Modules/TaskEngine/DAO/TaskLogDAO.h"
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
namespace TaskEngine {
namespace DAOs {

using json = nlohmann::json;

TaskLogDAO::TaskLogDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::TaskEngine::DTO::TaskLogDTO>(connectionPool, "task_logs") { // Pass table name for TaskLogDTO
    Logger::Logger::getInstance().info("TaskLogDAO: Initialized.");
}

// toMap for TaskLogDTO
std::map<std::string, std::any> TaskLogDAO::toMap(const ERP::TaskEngine::DTO::TaskLogDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["task_id"] = dto.taskId;
    data["execution_time"] = ERP::Utils::DateUtils::formatDateTime(dto.executionTime, ERP::Common::DATETIME_FORMAT);
    data["status"] = static_cast<int>(dto.status);
    ERP::DAOHelpers::putOptionalString(data, "output", dto.output);
    ERP::DAOHelpers::putOptionalString(data, "error_message", dto.errorMessage);
    data["duration_seconds"] = dto.durationSeconds;

    // Serialize std::map<string, any> to JSON string for context
    try {
        if (!dto.context.empty()) {
            json contextJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.context));
            data["context_json"] = contextJson.dump();
        } else {
            data["context_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("TaskLogDAO: toMap - Error serializing context: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "TaskLogDAO: Error serializing context.");
        data["context_json"] = "";
    }
    return data;
}

// fromMap for TaskLogDTO
ERP::TaskEngine::DTO::TaskLogDTO TaskLogDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::TaskEngine::DTO::TaskLogDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "task_id", dto.taskId);
        ERP::DAOHelpers::getPlainTimeValue(data, "execution_time", dto.executionTime);
        
        int statusInt;
        if (ERP::DAOHelpers::getPlainValue(data, "status", statusInt)) dto.status = static_cast<ERP::TaskEngine::DTO::TaskStatus>(statusInt);
        
        ERP::DAOHelpers::getOptionalStringValue(data, "output", dto.output);
        ERP::DAOHelpers::getOptionalStringValue(data, "error_message", dto.errorMessage);
        ERP::DAOHelpers::getPlainValue(data, "duration_seconds", dto.durationSeconds);

        // Deserialize JSON string to std::map<string, any> for context
        if (data.count("context_json") && data.at("context_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("context_json"));
            if (!jsonStr.empty()) {
                dto.context = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("TaskLogDAO: fromMap - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "TaskLogDAO: Data type mismatch in fromMap.");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("TaskLogDAO: fromMap - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "TaskLogDAO: Unexpected error in fromMap.");
    }
    return dto;
}

} // namespace DAOs
} // namespace TaskEngine
} // namespace ERP