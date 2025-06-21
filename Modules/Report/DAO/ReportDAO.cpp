// Modules/Report/DAO/ReportDAO.cpp
#include "Modules/Report/DAO/ReportDAO.h"
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
namespace Report {
namespace DAOs {

using json = nlohmann::json;

ReportDAO::ReportDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Report::DTO::ReportRequestDTO>(connectionPool, "report_requests") { // Pass table name for ReportRequestDTO
    Logger::Logger::getInstance().info("ReportDAO: Initialized.");
}

// toMap for ReportRequestDTO
std::map<std::string, std::any> ReportDAO::toMap(const ERP::Report::DTO::ReportRequestDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["report_name"] = dto.reportName;
    data["report_type"] = dto.reportType;
    data["frequency"] = static_cast<int>(dto.frequency);
    data["format"] = static_cast<int>(dto.format);
    data["requested_by_user_id"] = dto.requestedByUserId;
    data["requested_time"] = ERP::Utils::DateUtils::formatDateTime(dto.requestedTime, ERP::Common::DATETIME_FORMAT);
    
    // Serialize std::map<string, any> to JSON string for parameters
    try {
        if (!dto.parameters.empty()) {
            json paramsJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.parameters));
            data["parameters_json"] = paramsJson.dump();
        } else {
            data["parameters_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("ReportDAO: toMap - Error serializing parameters: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ReportDAO: Error serializing parameters.");
        data["parameters_json"] = "";
    }

    ERP::DAOHelpers::putOptionalString(data, "output_path", dto.outputPath);
    ERP::DAOHelpers::putOptionalString(data, "schedule_cron_expression", dto.scheduleCronExpression);
    ERP::DAOHelpers::putOptionalString(data, "email_recipients", dto.emailRecipients);

    return data;
}

// fromMap for ReportRequestDTO
ERP::Report::DTO::ReportRequestDTO ReportDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Report::DTO::ReportRequestDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "report_name", dto.reportName);
        ERP::DAOHelpers::getPlainValue(data, "report_type", dto.reportType);
        
        int frequencyInt;
        if (ERP::DAOHelpers::getPlainValue(data, "frequency", frequencyInt)) dto.frequency = static_cast<ERP::Report::DTO::ReportFrequency>(frequencyInt);
        
        int formatInt;
        if (ERP::DAOHelpers::getPlainValue(data, "format", formatInt)) dto.format = static_cast<ERP::Report::DTO::ReportFormat>(formatInt);
        
        ERP::DAOHelpers::getPlainValue(data, "requested_by_user_id", dto.requestedByUserId);
        ERP::DAOHelpers::getPlainTimeValue(data, "requested_time", dto.requestedTime);

        // Deserialize JSON string to std::map<string, any> for parameters
        if (data.count("parameters_json") && data.at("parameters_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("parameters_json"));
            if (!jsonStr.empty()) {
                dto.parameters = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
        
        ERP::DAOHelpers::getOptionalStringValue(data, "output_path", dto.outputPath);
        ERP::DAOHelpers::getOptionalStringValue(data, "schedule_cron_expression", dto.scheduleCronExpression);
        ERP::DAOHelpers::getOptionalStringValue(data, "email_recipients", dto.emailRecipients);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("ReportDAO: fromMap (Request) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ReportDAO: Data type mismatch in fromMap (Request).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("ReportDAO: fromMap (Request) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ReportDAO: Unexpected error in fromMap (Request).");
    }
    return dto;
}

// --- ReportExecutionLogDTO specific methods and helpers ---
std::map<std::string, std::any> ReportDAO::toMap(const ERP::Report::DTO::ReportExecutionLogDTO& dto) {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["report_request_id"] = dto.reportRequestId;
    data["execution_time"] = ERP::Utils::DateUtils::formatDateTime(dto.executionTime, ERP::Common::DATETIME_FORMAT);
    data["status"] = static_cast<int>(dto.status);
    ERP::DAOHelpers::putOptionalString(data, "executed_by_user_id", dto.executedByUserId);
    ERP::DAOHelpers::putOptionalString(data, "actual_output_path", dto.actualOutputPath);
    ERP::DAOHelpers::putOptionalString(data, "error_message", dto.errorMessage);

    // Serialize std::map<string, any> to JSON string for execution_metadata
    try {
        if (!dto.executionMetadata.empty()) {
            json metadataJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.executionMetadata));
            data["execution_metadata_json"] = metadataJson.dump();
        } else {
            data["execution_metadata_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("ReportDAO: toMap (Log) - Error serializing execution_metadata: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ReportDAO: Error serializing log metadata.");
        data["execution_metadata_json"] = "";
    }
    return data;
}

ERP::Report::DTO::ReportExecutionLogDTO ReportDAO::fromMap(const std::map<std::string, std::any>& data) {
    ERP::Report::DTO::ReportExecutionLogDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "report_request_id", dto.reportRequestId);
        ERP::DAOHelpers::getPlainTimeValue(data, "execution_time", dto.executionTime);
        
        int statusInt;
        if (ERP::DAOHelpers::getPlainValue(data, "status", statusInt)) dto.status = static_cast<ERP::Report::DTO::ReportExecutionStatus>(statusInt);
        
        ERP::DAOHelpers::getOptionalStringValue(data, "executed_by_user_id", dto.executedByUserId);
        ERP::DAOHelpers::getOptionalStringValue(data, "actual_output_path", dto.actualOutputPath);
        ERP::DAOHelpers::getOptionalStringValue(data, "error_message", dto.errorMessage);

        // Deserialize JSON string to std::map<string, any> for execution_metadata
        if (data.count("execution_metadata_json") && data.at("execution_metadata_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("execution_metadata_json"));
            if (!jsonStr.empty()) {
                dto.executionMetadata = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("ReportDAO: fromMap (Log) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ReportDAO: Data type mismatch in fromMap (Log).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("ReportDAO: fromMap (Log) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ReportDAO: Unexpected error in fromMap (Log).");
    }
    return dto;
}

bool ReportDAO::createReportExecutionLog(const ERP::Report::DTO::ReportExecutionLogDTO& log) {
    ERP::Logger::Logger::getInstance().info("ReportDAO: Attempting to create new report execution log.");
    std::map<std::string, std::any> data = toMap(log);
    
    std::string columns;
    std::string placeholders;
    std::map<std::string, std::any> params;
    bool first = true;

    for (const auto& pair : data) {
        if (!first) { columns += ", "; placeholders += ", "; }
        columns += pair.first;
        placeholders += "?";
        params[pair.first] = pair.second;
        first = false;
    }

    std::string sql = "INSERT INTO " + reportExecutionLogsTableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "ReportDAO", "createReportExecutionLog", sql, params
    );
}

std::optional<ERP::Report::DTO::ReportExecutionLogDTO> ReportDAO::getReportExecutionLogById(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("ReportDAO: Attempting to get report execution log by ID: " + id);
    std::string sql = "SELECT * FROM " + reportExecutionLogsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "ReportDAO", "getReportExecutionLogById", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front());
    }
    return std::nullopt;
}

std::vector<ERP::Report::DTO::ReportExecutionLogDTO> ReportDAO::getReportExecutionLogsByRequestId(const std::string& requestId) {
    ERP::Logger::Logger::getInstance().info("ReportDAO: Retrieving report execution logs for request ID: " + requestId);
    std::string sql = "SELECT * FROM " + reportExecutionLogsTableName_ + " WHERE report_request_id = ?;";
    std::map<std::string, std::any> params;
    params["report_request_id"] = requestId;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "ReportDAO", "getReportExecutionLogsByRequestId", sql, params
    );

    std::vector<ERP::Report::DTO::ReportExecutionLogDTO> resultsDto;
    for (const auto& rowMap : resultsMap) {
        resultsDto.push_back(fromMap(rowMap));
    }
    return resultsDto;
}

bool ReportDAO::updateReportExecutionLog(const ERP::Report::DTO::ReportExecutionLogDTO& log) {
    ERP::Logger::Logger::getInstance().info("ReportDAO: Attempting to update report execution log with ID: " + log.id);
    std::map<std::string, std::any> data = toMap(log);
    if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
        ERP::Logger::Logger::getInstance().warning("ReportDAO: Update log called with empty data or missing ID.");
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ReportDAO: Update log called with empty data or missing ID.");
        return false;
    }

    std::string setClause;
    std::map<std::string, std::any> params;
    bool firstSet = true;

    for (const auto& pair : data) {
        if (pair.first == "id") continue;
        if (!firstSet) setClause += ", ";
        setClause += pair.first + " = ?";
        params[pair.first] = pair.second;
        firstSet = false;
    }

    std::string sql = "UPDATE " + reportExecutionLogsTableName_ + " SET " + setClause + " WHERE id = ?;";
    params["id_filter"] = log.id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "ReportDAO", "updateReportExecutionLog", sql, params
    );
}

bool ReportDAO::removeReportExecutionLog(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("ReportDAO: Attempting to remove report execution log with ID: " + id);
    std::string sql = "DELETE FROM " + reportExecutionLogsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "ReportDAO", "removeReportExecutionLog", sql, params
    );
}

bool ReportDAO::removeReportExecutionLogsByRequestId(const std::string& requestId) {
    ERP::Logger::Logger::getInstance().info("ReportDAO: Attempting to remove all logs for report request ID: " + requestId);
    std::string sql = "DELETE FROM " + reportExecutionLogsTableName_ + " WHERE report_request_id = ?;";
    std::map<std::string, std::any> params;
    params["report_request_id"] = requestId;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "ReportDAO", "removeReportExecutionLogsByRequestId", sql, params
    );
}

} // namespace DAOs
} // namespace Report
} // namespace ERP