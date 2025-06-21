// Modules/Scheduler/DAO/ScheduledTaskDAO.cpp
#include "ScheduledTaskDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions

namespace ERP {
    namespace Scheduler {
        namespace DAOs {

            ScheduledTaskDAO::ScheduledTaskDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Scheduler::DTO::ScheduledTaskDTO>(connectionPool, "scheduled_tasks") {
                ERP::Logger::Logger::getInstance().info("ScheduledTaskDAO: Initialized.");
            }

            std::map<std::string, std::any> ScheduledTaskDAO::toMap(const ERP::Scheduler::DTO::ScheduledTaskDTO& task) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(task); // BaseDTO fields

                data["task_name"] = task.taskName;
                data["task_type"] = task.taskType;
                data["frequency"] = static_cast<int>(task.frequency);
                ERP::DAOHelpers::putOptionalString(data, "cron_expression", task.cronExpression);
                data["next_run_time"] = ERP::Utils::DateUtils::formatDateTime(task.nextRunTime, ERP::Common::DATETIME_FORMAT);
                ERP::DAOHelpers::putOptionalTime(data, "last_run_time", task.lastRunTime);
                ERP::DAOHelpers::putOptionalString(data, "last_error_message", task.lastErrorMessage);
                data["status"] = static_cast<int>(task.status);
                ERP::DAOHelpers::putOptionalString(data, "assigned_to_user_id", task.assignedToUserId);
                data["parameters_json"] = ERP::Utils::DTOUtils::mapToJsonString(task.parameters); // Convert map to JSON string
                ERP::DAOHelpers::putOptionalTime(data, "start_date", task.startDate);
                ERP::DAOHelpers::putOptionalTime(data, "end_date", task.endDate);

                return data;
            }

            ERP::Scheduler::DTO::ScheduledTaskDTO FromMap(const std::map<std::string, std::any>& data) {
                ERP::Scheduler::DTO::ScheduledTaskDTO task;
                ERP::Utils::DTOUtils::fromMap(data, task); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "task_name", task.taskName);
                    ERP::DAOHelpers::getPlainValue(data, "task_type", task.taskType);

                    int frequencyInt;
                    ERP::DAOHelpers::getPlainValue(data, "frequency", frequencyInt);
                    task.frequency = static_cast<ERP::Scheduler::DTO::ScheduleFrequency>(frequencyInt);

                    ERP::DAOHelpers::getOptionalStringValue(data, "cron_expression", task.cronExpression);
                    ERP::DAOHelpers::getPlainTimeValue(data, "next_run_time", task.nextRunTime);
                    ERP::DAOHelpers::getOptionalTimeValue(data, "last_run_time", task.lastRunTime);
                    ERP::DAOHelpers::getOptionalStringValue(data, "last_error_message", task.lastErrorMessage);

                    int statusInt;
                    ERP::DAOHelpers::getPlainValue(data, "status", statusInt);
                    task.status = static_cast<ERP::Scheduler::DTO::ScheduledTaskStatus>(statusInt);

                    ERP::DAOHelpers::getOptionalStringValue(data, "assigned_to_user_id", task.assignedToUserId);

                    // Convert JSON string to map
                    std::string parametersJsonString;
                    ERP::DAOHelpers::getPlainValue(data, "parameters_json", parametersJsonString);
                    task.parameters = ERP::Utils::DTOUtils::jsonStringToMap(parametersJsonString);

                    ERP::DAOHelpers::getOptionalTimeValue(data, "start_date", task.startDate);
                    ERP::DAOHelpers::getOptionalTimeValue(data, "end_date", task.endDate);

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("ScheduledTaskDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ScheduledTaskDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("ScheduledTaskDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ScheduledTaskDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return task;
            }

            bool ScheduledTaskDAO::save(const ERP::Scheduler::DTO::ScheduledTaskDTO& task) {
                return create(task);
            }

            std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> ScheduledTaskDAO::findById(const std::string& id) {
                return getById(id);
            }

            bool ScheduledTaskDAO::update(const ERP::Scheduler::DTO::ScheduledTaskDTO& task) {
                return DAOBase<ERP::Scheduler::DTO::ScheduledTaskDTO>::update(task);
            }

            bool ScheduledTaskDAO::remove(const std::string& id) {
                return DAOBase<ERP::Scheduler::DTO::ScheduledTaskDTO>::remove(id);
            }

            std::vector<ERP::Scheduler::DTO::ScheduledTaskDTO> ScheduledTaskDAO::findAll() {
                return DAOBase<ERP::Scheduler::DTO::ScheduledTaskDTO>::findAll();
            }

            std::vector<ERP::Scheduler::DTO::ScheduledTaskDTO> ScheduledTaskDAO::getScheduledTasks(const std::map<std::string, std::any>& filters) {
                return get(filters); // Use templated get
            }

            int ScheduledTaskDAO::countScheduledTasks(const std::map<std::string, std::any>& filters) {
                return count(filters); // Use templated count
            }

        } // namespace DAOs
    } // namespace Scheduler
} // namespace ERP