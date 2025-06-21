// Modules/TaskEngine/DTO/TaskLog.h
#ifndef MODULES_TASKENGINE_DTO_TASKLOG_H
#define MODULES_TASKENGINE_DTO_TASKLOG_H
#include <string>
#include <optional>
#include <chrono>
#include <map>    // For std::map<string, any>
#include <any>    // For std::any
// #include <QJsonObject> // Removed
// #include <QString>     // Removed
// #include <QDateTime>   // Removed
#include "DataObjects/BaseDTO.h"   // Updated include path
#include "Modules/Common/Common.h"    // For EntityStatus (Updated include path)
#include "Modules/Utils/Utils.h"     // For UUID generation
namespace ERP {
    namespace TaskEngine {
        namespace DTO {
            /**
             * @brief Enum định nghĩa trạng thái thực thi của một tác vụ trong TaskEngine.
             */
            enum class TaskStatus {
                IDLE = 0,       /**< Không làm gì */
                RUNNING = 1,    /**< Đang chạy */
                COMPLETED = 2,  /**< Đã hoàn thành */
                FAILED = 3      /**< Thất bại */
            };
            /**
             * @brief DTO for Task Log entity.
             * Records the output and status of an executed task.
             */
            struct TaskLogDTO : public BaseDTO {
                std::string taskId;                 /**< ID của tác vụ đã thực thi */
                std::chrono::system_clock::time_point executionTime; /**< Thời gian bắt đầu thực thi */
                TaskStatus status;                  /**< Trạng thái của tác vụ */
                std::optional<std::string> output;  /**< Đầu ra của tác vụ */
                std::optional<std::string> errorMessage; /**< Thông báo lỗi nếu thất bại */
                double durationSeconds;             /**< Thời gian thực thi (giây) */
                std::map<std::string, std::any> context; /**< Ngữ cảnh/tham số của tác vụ dạng map (thay thế QJsonObject) */

                TaskLogDTO() : BaseDTO(), taskId(""),
                               executionTime(std::chrono::system_clock::now()),
                               status(TaskStatus::IDLE), durationSeconds(0.0) {}
                virtual ~TaskLogDTO() = default;

                // Utility methods
                std::string getStatusString() const {
                    switch (status) {
                        case TaskStatus::IDLE: return "Idle";
                        case TaskStatus::RUNNING: return "Running";
                        case TaskStatus::COMPLETED: return "Completed";
                        case TaskStatus::FAILED: return "Failed";
                        default: return "Unknown";
                    }
                }
            };
        } // namespace DTO
    } // namespace TaskEngine
} // namespace ERP
#endif // MODULES_TASKENGINE_DTO_TASKLOG_H