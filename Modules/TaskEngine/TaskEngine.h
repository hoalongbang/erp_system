// Modules/TaskEngine/TaskEngine.h
#ifndef MODULES_TASKENGINE_TASKENGINE_H
#define MODULES_TASKENGINE_TASKENGINE_H
#include "ITask.h"        // Đã rút gọn include
#include "Logger.h"       // Đã rút gọn include
#include <string>
#include <vector>
#include <queue>        // For std::priority_queue (if scheduling tasks by time)
#include <chrono>       // For std::chrono::system_clock::time_point
#include <thread>       // For std::thread
#include <mutex>        // For std::mutex
#include <condition_variable> // For std::condition_variable
#include <atomic>       // For std::atomic_bool
#include <functional>   // For std::function
#include <memory>       // For std::shared_ptr

namespace ERP {
namespace TaskEngine {
/**
 * @brief ITaskExecutorService interface defines operations for executing tasks.
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
    
    /**
     * @brief Submits a scheduled task for execution at or after its nextRunTime.
     * The TaskEngine will manage dispatching these tasks.
     * @param taskDTO The ScheduledTaskDTO to execute.
     */
    // virtual void submitScheduledTask(const ERP::Scheduler::DTO::ScheduledTaskDTO& taskDTO) = 0;
};


/**
 * @brief The TaskEngine class is responsible for managing and executing background tasks.
 * It uses a worker thread and a queue to process tasks asynchronously.
 */
class TaskEngine : public ITaskExecutorService {
public:
    /**
     * @brief Gets the singleton instance of the TaskEngine.
     * @return Reference to the TaskEngine instance.
     */
    static TaskEngine& getInstance();

    // Deleted copy constructor and assignment operator to enforce singleton
    TaskEngine(const TaskEngine&) = delete;
    TaskEngine& operator=(const TaskEngine&) = delete;

    /**
     * @brief Starts the TaskEngine's worker thread.
     */
    void start();

    /**
     * @brief Stops the TaskEngine, gracefully shutting down the worker thread.
     */
    void stop();

    /**
     * @brief Submits a task for asynchronous execution.
     * @param callback A callable object (e.g., lambda) representing the task logic.
     * @param taskId A unique ID for the task, used for logging or tracking.
     */
    void submitTask(std::function<void()> callback, const std::string& taskId) override;

    // Structure to hold tasks in the priority queue (for scheduled tasks)
    struct ScheduledTaskEntry {
        std::chrono::system_clock::time_point nextRunTime;
        std::string taskId;
        std::function<void()> callback;

        bool operator>(const ScheduledTaskEntry& other) const {
            return nextRunTime > other.nextRunTime;
        }
    };

    /**
     * @brief Submits a task to be executed at a specific time.
     * @param taskEntry The ScheduledTaskEntry containing the task details.
     */
    void submitScheduledTask(ScheduledTaskEntry taskEntry);


private:
    TaskEngine(); // Private constructor for singleton
    ~TaskEngine(); // Private destructor for singleton

    void workerThreadLoop(); // The main loop for the worker thread

    std::atomic<bool> running_;
    std::thread workerThread_;
    std::mutex queueMutex_;
    std::condition_variable cv_;

    // Queue for immediate tasks
    std::queue<std::pair<std::function<void()>, std::string>> immediateTaskQueue_;

    // Priority queue for scheduled tasks (ordered by nextRunTime)
    std::priority_queue<ScheduledTaskEntry, std::vector<ScheduledTaskEntry>, std::greater<ScheduledTaskEntry>> scheduledTaskQueue_;
};
} // namespace TaskEngine
} // namespace ERP
#endif // MODULES_TASKENGINE_TASKENGINE_H