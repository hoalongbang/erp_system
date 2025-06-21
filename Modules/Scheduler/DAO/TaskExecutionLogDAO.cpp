// Modules/Scheduler/DAO/TaskExecutionLogDAO.cpp
#include "TaskExecutionLogDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions

namespace ERP {
    namespace Scheduler {
        namespace DAOs {

            TaskExecutionLogDAO::TaskExecutionLogDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Scheduler::DTO::TaskExecutionLogDTO>(connectionPool, "task_execution_logs") {
                ERP::Logger::Logger::getInstance().info("TaskExecutionLogDAO: Initialized.");
            }

            std::map<std::string, std::any> TaskExecutionLogDAO::toMap(const ERP::Scheduler::DTO::TaskExecutionLogDTO& log) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(log); // BaseDTO fields

                data["scheduled_task_id"] = log.scheduledTaskId;
                data["start_time"] = ERP::Utils::DateUtils::formatDateTime(log.startTime, ERP::Common::DATETIME_FORMAT);
                ERP::DAOHelpers::putOptionalTime(data, "end_time", log.endTime);
                data["status"] = static_cast<int>(log.status);
                ERP::DAOHelpers::putOptionalString(data, "executed_by_user_id", log.executedByUserId);
                ERP::DAOHelpers::putOptionalString(data, "log_output", log.logOutput);
                ERP::DAOHelpers::putOptionalString(data, "error_message", log.errorMessage);
                data["execution_context_json"] = ERP::Utils::DTOUtils::mapToJsonString(log.executionContext); // Convert map to JSON string

                return data;
            }

            ERP::Scheduler::DTO::TaskExecutionLogDTO TaskExecutionLogDAO::fromMap(const std::map<std::string, std::any>& data) const {
                ERP::Scheduler::DTO::TaskExecutionLogDTO log;
                ERP::Utils::DTOUtils::fromMap(data, log); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "scheduled_task_id", log.scheduledTaskId);
                    ERP::DAOHelpers::getPlainTimeValue(data, "start_time", log.startTime);
                    ERP::DAOHelpers::getOptionalTimeValue(data, "end_time", log.endTime);

                    int statusInt;
                    ERP::DAOHelpers::getPlainValue(data, "status", statusInt);
                    log.status = static_cast<ERP::Scheduler::DTO::TaskExecutionStatus>(statusInt);

                    ERP::DAOHelpers::getOptionalStringValue(data, "executed_by_user_id", log.executedByUserId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "log_output", log.logOutput);
                    ERP::DAOHelpers::getOptionalStringValue(data, "error_message", log.errorMessage);

                    // Convert JSON string to map
                    std::string executionContextJsonString;
                    ERP::DAOHelpers::getPlainValue(data, "execution_context_json", executionContextJsonString);
                    log.executionContext = ERP::Utils::DTOUtils::jsonStringToMap(executionContextJsonString);

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("TaskExecutionLogDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "TaskExecutionLogDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("TaskExecutionLogDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "TaskExecutionLogDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return log;
            }

            bool TaskExecutionLogDAO::save(const ERP::Scheduler::DTO::TaskExecutionLogDTO& log) {
                return create(log);
            }

            std::optional<ERP::Scheduler::DTO::TaskExecutionLogDTO> TaskExecutionLogDAO::findById(const std::string& id) {
                return getById(id);
            }

            bool TaskExecutionLogDAO::update(const ERP::Scheduler::DTO::TaskExecutionLogDTO& log) {
                return DAOBase<ERP::Scheduler::DTO::TaskExecutionLogDTO>::update(log);
            }

            bool TaskExecutionLogDAO::remove(const std::string& id) {
                return DAOBase<ERP::Scheduler::DTO::TaskExecutionLogDTO>::remove(id);
            }

            std::vector<ERP::Scheduler::DTO::TaskExecutionLogDTO> TaskExecutionLogDAO::findAll() {
                return DAOBase<ERP::Scheduler::DTO::TaskExecutionLogDTO>::findAll();
            }

            std::vector<ERP::Scheduler::DTO::TaskExecutionLogDTO> TaskExecutionLogDAO::getTaskExecutionLogsByScheduledTaskId(const std::string& scheduledTaskId) {
                std::map<std::string, std::any> filters;
                filters["scheduled_task_id"] = scheduledTaskId;
                return getTaskExecutionLogs(filters);
            }

            std::vector<ERP::Scheduler::DTO::TaskExecutionLogDTO> TaskExecutionLogDAO::getTaskExecutionLogs(const std::map<std::string, std::any>& filters) {
                std::vector<std::map<std::string, std::any>> results = executeQuery(tableName_, filters);
                std::vector<ERP::Scheduler::DTO::TaskExecutionLogDTO> logs;
                for (const auto& row : results) {
                    logs.push_back(fromMap(row));
                }
                return logs;
            }

            int TaskExecutionLogDAO::countTaskExecutionLogs(const std::map<std::string, std::any>& filters) {
                return count(filters); // Use templated count
            }

            bool TaskExecutionLogDAO::removeTaskExecutionLogsByScheduledTaskId(const std::string& scheduledTaskId) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("TaskExecutionLogDAO::removeTaskExecutionLogsByScheduledTaskId: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + tableName_ + " WHERE scheduled_task_id = :scheduled_task_id;";
                std::map<std::string, std::any> params;
                params["scheduled_task_id"] = scheduledTaskId;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("TaskExecutionLogDAO::removeTaskExecutionLogsByScheduledTaskId: Failed to remove task execution logs for scheduled_task_id " + scheduledTaskId + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove task execution logs.", "Không thể xóa nhật ký thực thi tác vụ.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

        } // namespace DAOs
    } // namespace Scheduler
} // namespace ERP