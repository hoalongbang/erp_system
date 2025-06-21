// Modules/TaskEngine/ITask.h
#ifndef MODULES_TASKENGINE_ITASK_H
#define MODULES_TASKENGINE_ITASK_H
#include <string>
#include <optional>
namespace ERP {
namespace TaskEngine {
/**
 * @brief Interface for a generic task to be executed by the TaskEngine.
 * All concrete tasks must implement this interface.
 */
class ITask {
public:
    virtual ~ITask() = default;
    /**
     * @brief Executes the task logic.
     * @return True if the task execution was successful, false otherwise.
     */
    virtual bool execute() = 0;
    /**
     * @brief Gets an error message if the last execution failed.
     * @return An optional string containing the error message.
     */
    virtual std::optional<std::string> getErrorMessage() const = 0;
};
} // namespace TaskEngine
} // namespace ERP
#endif // MODULES_TASKENGINE_ITASK_H