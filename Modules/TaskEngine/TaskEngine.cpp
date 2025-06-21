// Modules/TaskEngine/TaskEngine.cpp
#include "TaskEngine.h" // Đã rút gọn include
#include "Logger.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include <iostream>

namespace ERP {
namespace TaskEngine {

TaskEngine& TaskEngine::getInstance() {
    static TaskEngine instance; // Guaranteed to be destroyed, instantiated on first use.
    return instance;
}

TaskEngine::TaskEngine() : running_(false) {
    ERP::Logger::Logger::getInstance().info("TaskEngine: Initialized.");
}

TaskEngine::~TaskEngine() {
    stop(); // Ensure worker thread is stopped when TaskEngine is destroyed
    ERP::Logger::Logger::getInstance().info("TaskEngine: Destroyed.");
}

void TaskEngine::start() {
    if (!running_.exchange(true)) { // Set running_ to true and check if it was false
        workerThread_ = std::thread(&TaskEngine::workerThreadLoop, this);
        ERP::Logger::Logger::getInstance().info("TaskEngine: Worker thread started.");
    } else {
        ERP::Logger::Logger::getInstance().warning("TaskEngine: Attempted to start already running worker thread.");
    }
}

void TaskEngine::stop() {
    if (running_.exchange(false)) { // Set running_ to false and check if it was true
        cv_.notify_all(); // Notify worker thread to wake up and exit
        if (workerThread_.joinable()) {
            workerThread_.join(); // Wait for the worker thread to finish
        }
        ERP::Logger::Logger::getInstance().info("TaskEngine: Worker thread stopped.");
    } else {
        ERP::Logger::Logger::getInstance().warning("TaskEngine: Attempted to stop already stopped worker thread.");
    }
}

void TaskEngine::submitTask(std::function<void()> callback, const std::string& taskId) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        immediateTaskQueue_.push({callback, taskId});
        ERP::Logger::Logger::getInstance().debug("TaskEngine: Immediate task '" + taskId + "' submitted.");
    }
    cv_.notify_one(); // Notify one waiting thread that a task is available
}

void TaskEngine::submitScheduledTask(ScheduledTaskEntry taskEntry) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        scheduledTaskQueue_.push(taskEntry);
        ERP::Logger::Logger::getInstance().debug("TaskEngine: Scheduled task '" + taskEntry.taskId + "' submitted for " + ERP::Utils::DateUtils::formatDateTime(taskEntry.nextRunTime, ERP::Common::DATETIME_FORMAT) + ".");
    }
    cv_.notify_one(); // Notify worker thread that a new scheduled task might change wakeup time
}

void TaskEngine::workerThreadLoop() {
    ERP::Logger::Logger::getInstance().info("TaskEngine: Worker thread loop started.");
    while (running_) {
        std::unique_lock<std::mutex> lock(queueMutex_);

        // Process immediate tasks first
        if (!immediateTaskQueue_.empty()) {
            auto task = immediateTaskQueue_.front();
            immediateTaskQueue_.pop();
            lock.unlock(); // Unlock while executing task to not block other submissions
            ERP::Logger::Logger::getInstance().info("TaskEngine: Executing immediate task: " + task.second);
            try {
                task.first(); // Execute the task callback
            } catch (const std::exception& e) {
                ERP::Logger::Logger::getInstance().error("TaskEngine: Exception during immediate task '" + task.second + "': " + e.what());
            }
            continue; // Go back to check for more immediate tasks or scheduled tasks
        }

        // Handle scheduled tasks
        if (!scheduledTaskQueue_.empty()) {
            ScheduledTaskEntry& nextTask = scheduledTaskQueue_.top();
            // Wait until the next scheduled task is due or until notified/shutdown
            if (std::chrono::system_clock::now() < nextTask.nextRunTime) {
                ERP::Logger::Logger::getInstance().debug("TaskEngine: Worker waiting for scheduled task '" + nextTask.taskId + "' until: " + ERP::Utils::DateUtils::formatDateTime(nextTask.nextRunTime, ERP::Common::DATETIME_FORMAT));
                cv_.wait_until(lock, nextTask.nextRunTime, [&] { return !running_ || !immediateTaskQueue_.empty() || std::chrono::system_clock::now() >= nextTask.nextRunTime; });
            }

            if (!running_) break; // Check running_ status again after waking up
            if (!immediateTaskQueue_.empty()) continue; // Prioritize immediate tasks if new ones arrived

            // If woke up because time is due or passed for nextTask
            if (std::chrono::system_clock::now() >= nextTask.nextRunTime) {
                scheduledTaskQueue_.pop(); // Remove task from queue
                lock.unlock(); // Unlock while executing task
                ERP::Logger::Logger::getInstance().info("TaskEngine: Executing scheduled task: " + nextTask.taskId);
                try {
                    nextTask.callback(); // Execute the scheduled task callback
                } catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("TaskEngine: Exception during scheduled task '" + nextTask.taskId + "': " + e.what());
                }
                continue; // Go back to check for next tasks
            }
        }
        
        // If no tasks are ready, wait for a notification or a timeout to re-check
        auto nextWakeupTime = std::chrono::system_clock::now() + std::chrono::minutes(1); // Default check-in time
        if (!scheduledTaskQueue_.empty()) {
            nextWakeupTime = scheduledTaskQueue_.top().nextRunTime;
        }

        ERP::Logger::Logger::getInstance().debug("TaskEngine: No tasks due. Worker waiting until: " + ERP::Utils::DateUtils::formatDateTime(nextWakeupTime, ERP::Common::DATETIME_FORMAT));
        cv_.wait_until(lock, nextWakeupTime, [&] { return !running_ || !immediateTaskQueue_.empty(); });

        if (!running_) break; // Check running_ status after waiting
    }
    ERP::Logger::Logger::getInstance().info("TaskEngine: Worker thread finished its loop.");
}
} // namespace TaskEngine
} // namespace ERP