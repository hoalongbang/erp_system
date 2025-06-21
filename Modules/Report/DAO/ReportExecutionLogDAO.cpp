// Modules/Report/DAO/ReportExecutionLogDAO.cpp
#include "ReportExecutionLogDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions

namespace ERP {
namespace Report {
namespace DAOs {

ReportExecutionLogDAO::ReportExecutionLogDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Report::DTO::ReportExecutionLogDTO>(connectionPool, "report_execution_logs") {
    ERP::Logger::Logger::getInstance().info("ReportExecutionLogDAO: Initialized.");
}

std::map<std::string, std::any> ReportExecutionLogDAO::toMap(const ERP::Report::DTO::ReportExecutionLogDTO& log) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(log); // BaseDTO fields

    data["report_request_id"] = log.reportRequestId;
    data["execution_time"] = ERP::Utils::DateUtils::formatDateTime(log.executionTime, ERP::Common::DATETIME_FORMAT);
    data["status"] = static_cast<int>(log.status);
    ERP::DAOHelpers::putOptionalString(data, "executed_by_user_id", log.executedByUserId);
    ERP::DAOHelpers::putOptionalString(data, "actual_output_path", log.actualOutputPath);
    ERP::DAOHelpers::putOptionalString(data, "error_message", log.errorMessage);
    data["execution_metadata_json"] = ERP::Utils::DTOUtils::mapToJsonString(log.executionMetadata); // Convert map to JSON string
    ERP::DAOHelpers::putOptionalString(data, "log_output", log.logOutput);

    return data;
}

ERP::Report::DTO::ReportExecutionLogDTO ReportExecutionLogDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Report::DTO::ReportExecutionLogDTO log;
    ERP::Utils::DTOUtils::fromMap(data, log); // BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "report_request_id", log.reportRequestId);
        ERP::DAOHelpers::getPlainTimeValue(data, "execution_time", log.executionTime);
        
        int statusInt;
        ERP::DAOHelpers::getPlainValue(data, "status", statusInt);
        log.status = static_cast<ERP::Report::DTO::ReportExecutionStatus>(statusInt);

        ERP::DAOHelpers::getOptionalStringValue(data, "executed_by_user_id", log.executedByUserId);
        ERP::DAOHelpers::getOptionalStringValue(data, "actual_output_path", log.actualOutputPath);
        ERP::DAOHelpers::getOptionalStringValue(data, "error_message", log.errorMessage);
        
        // Convert JSON string to map
        std::string executionMetadataJsonString;
        ERP::DAOHelpers::getPlainValue(data, "execution_metadata_json", executionMetadataJsonString);
        log.executionMetadata = ERP::Utils::DTOUtils::jsonStringToMap(executionMetadataJsonString);

        ERP::DAOHelpers::getOptionalStringValue(data, "log_output", log.logOutput);

    } catch (const std::bad_any_cast& e) {
        ERP::Logger::Logger::getInstance().error("ReportExecutionLogDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ReportExecutionLogDAO: Data type mismatch in fromMap: " + std::string(e.what()));
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("ReportExecutionLogDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ReportExecutionLogDAO: Unexpected error in fromMap: " + std::string(e.what()));
    }
    return log;
}

bool ReportExecutionLogDAO::save(const ERP::Report::DTO::ReportExecutionLogDTO& log) {
    return create(log);
}

std::optional<ERP::Report::DTO::ReportExecutionLogDTO> ReportExecutionLogDAO::findById(const std::string& id) {
    return getById(id);
}

bool ReportExecutionLogDAO::update(const ERP::Report::DTO::ReportExecutionLogDTO& log) {
    return DAOBase<ERP::Report::DTO::ReportExecutionLogDTO>::update(log);
}

bool ReportExecutionLogDAO::remove(const std::string& id) {
    return DAOBase<ERP::Report::DTO::ReportExecutionLogDTO>::remove(id);
}

std::vector<ERP::Report::DTO::ReportExecutionLogDTO> ReportExecutionLogDAO::findAll() {
    return DAOBase<ERP::Report::DTO::ReportExecutionLogDTO>::findAll();
}

std::vector<ERP::Report::DTO::ReportExecutionLogDTO> ReportExecutionLogDAO::getReportExecutionLogsByRequestId(const std::string& reportRequestId) {
    std::map<std::string, std::any> filters;
    filters["report_request_id"] = reportRequestId;
    return getReportExecutionLogs(filters);
}

std::vector<ERP::Report::DTO::ReportExecutionLogDTO> ReportExecutionLogDAO::getReportExecutionLogs(const std::map<std::string, std::any>& filters) {
    std::vector<std::map<std::string, std::any>> results = executeQuery(tableName_, filters);
    std::vector<ERP::Report::DTO::ReportExecutionLogDTO> logs;
    for (const auto& row : results) {
        logs.push_back(fromMap(row));
    }
    return logs;
}

int ReportExecutionLogDAO::countReportExecutionLogs(const std::map<std::string, std::any>& filters) {
    return count(filters); // Use templated count
}

bool ReportExecutionLogDAO::removeReportExecutionLogsByRequestId(const std::string& reportRequestId) {
    std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
    if (!conn) {
        ERP::Logger::Logger::getInstance().error("ReportExecutionLogDAO::removeReportExecutionLogsByRequestId: Failed to get database connection.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
        return false;
    }
    
    std::string sql = "DELETE FROM " + tableName_ + " WHERE report_request_id = :report_request_id;";
    std::map<std::string, std::any> params;
    params["report_request_id"] = reportRequestId;

    bool success = conn->execute(sql, params);
    if (!success) {
        ERP::Logger::Logger::getInstance().error("ReportExecutionLogDAO::removeReportExecutionLogsByRequestId: Failed to remove report execution logs for report_request_id " + reportRequestId + ". Error: " + conn->getLastError());
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove report execution logs.", "Không thể xóa nhật ký thực thi báo cáo.");
    }
    connectionPool_->releaseConnection(conn);
    return success;
}

} // namespace DAOs
} // namespace Report
} // namespace ERP