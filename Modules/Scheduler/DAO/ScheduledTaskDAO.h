// Modules/Scheduler/DAO/ScheduledTaskDAO.h
#ifndef MODULES_SCHEDULER_DAO_SCHEDULEDTASKDAO_H
#define MODULES_SCHEDULER_DAO_SCHEDULEDTASKDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"            // Base DAO template
#include "ScheduledTask.h"      // ScheduledTask DTO

namespace ERP {
    namespace Scheduler {
        namespace DAOs {
            /**
             * @brief DAO class for ScheduledTask entity.
             * Handles database operations for ScheduledTaskDTO.
             */
            class ScheduledTaskDAO : public ERP::DAOBase::DAOBase<ERP::Scheduler::DTO::ScheduledTaskDTO> {
            public:
                ScheduledTaskDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~ScheduledTaskDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Scheduler::DTO::ScheduledTaskDTO& task) override;
                std::optional<ERP::Scheduler::DTO::ScheduledTaskDTO> findById(const std::string& id) override;
                bool update(const ERP::Scheduler::DTO::ScheduledTaskDTO& task) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Scheduler::DTO::ScheduledTaskDTO> findAll() override;

                // Specific methods for ScheduledTask
                std::vector<ERP::Scheduler::DTO::ScheduledTaskDTO> getScheduledTasks(const std::map<std::string, std::any>& filters);
                int countScheduledTasks(const std::map<std::string, std::any>& filters);

            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Scheduler::DTO::ScheduledTaskDTO& task) const override;
                ERP::Scheduler::DTO::ScheduledTaskDTO fromMap(const std::map<std::string, std::any>& data) const override;

            private:
                std::string tableName_ = "scheduled_tasks";
            };
        } // namespace DAOs
    } // namespace Scheduler
} // namespace ERP
#endif // MODULES_SCHEDULER_DAO_SCHEDULEDTASKDAO_H