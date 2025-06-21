// Modules/TaskEngine/Service/ITaskExecutorService.h
#ifndef MODULES_TASKENGINE_SERVICE_ITASKEXECUTORSERVICE_H
#define MODULES_TASKENGINE_SERVICE_ITASKEXECUTORSERVICE_H
#include <string>       // For std::string
#include <functional>   // For std::function
#include <memory>       // For shared_ptr

// Rút gọn include paths
// No specific DTOs or DAOs directly included here, as this is a general executor interface.

namespace ERP {
    namespace TaskEngine {
        namespace Services {

            /**
             * @brief ITaskExecutorService interface defines operations for executing tasks.
             * This can be used by other services to submit background tasks.
             */
            class ITaskExecutorService {
            public:
                virtual ~ITaskExecutorService() = default;
                /**
                 * @brief Submits a task for asynchronous execution.
                 * @param callback A callable object (e.g., lambda) representing the task logic.
                 * It should take no arguments and return void.
                 * @param taskId A unique ID for the task, used for logging or tracking.
                 */
                virtual void submitTask(std::function<void()> callback, const std::string& taskId) = 0;

                // Future: Add method for submitting scheduled tasks if this interface needs to manage them directly
                // virtual void submitScheduledTask(const ERP::Scheduler::DTO::ScheduledTaskDTO& taskDTO) = 0;
            };

        } // namespace Services
    } // namespace TaskEngine
} // namespace ERP
#endif // MODULES_TASKENGINE_SERVICE_ITASKEXECUTORSERVICE_H