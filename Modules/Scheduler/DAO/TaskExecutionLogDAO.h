// Modules/Scheduler/DAO/TaskExecutionLogDAO.h
#ifndef MODULES_SCHEDULER_DAO_TASKEXECUTIONLOGDAO_H
#define MODULES_SCHEDULER_DAO_TASKEXECUTIONLOGDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"            // Base DAO template
#include "TaskExecutionLog.h"   // TaskExecutionLog DTO

namespace ERP {
    namespace Scheduler {
        namespace DAOs {
            /**
             * @brief DAO class for TaskExecutionLog entity.
             * Handles database operations for TaskExecutionLogDTO.
             */
            class TaskExecutionLogDAO : public ERP::DAOBase::DAOBase<ERP::Scheduler::DTO::TaskExecutionLogDTO> {
            public:
                TaskExecutionLogDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~TaskExecutionLogDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Scheduler::DTO::TaskExecutionLogDTO& log) override;
                std::optional<ERP::Scheduler::DTO::TaskExecutionLogDTO> findById(const std::string& id) override;
                bool update(const ERP::Scheduler::DTO::TaskExecutionLogDTO& log) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Scheduler::DTO::TaskExecutionLogDTO> findAll() override;

                // Specific methods for TaskExecutionLog
                std::vector<ERP::Scheduler::DTO::TaskExecutionLogDTO> getTaskExecutionLogsByScheduledTaskId(const std::string& scheduledTaskId);
                std::vector<ERP::Scheduler::DTO::TaskExecutionLogDTO> getTaskExecutionLogs(const std::map<std::string, std::any>& filters);
                int countTaskExecutionLogs(const std::map<std::string, std::any>& filters);
                bool removeTaskExecutionLogsByScheduledTaskId(const std::string& scheduledTaskId);

            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Scheduler::DTO::TaskExecutionLogDTO& log) const override;
                ERP::Scheduler::DTO::TaskExecutionLogDTO fromMap(const std::map<std::string, std::any>& data) const override;

            private:
                std::string tableName_ = "task_execution_logs";
            };
        } // namespace DAOs
    } // namespace Scheduler
} // namespace ERP
#endif // MODULES_SCHEDULER_DAO_TASKEXECUTIONLOGDAO_H