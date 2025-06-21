// Modules/Scheduler/DTO/TaskExecutionLog.h
#ifndef MODULES_SCHEDULER_DTO_TASKEXECUTIONLOG_H
#define MODULES_SCHEDULER_DTO_TASKEXECUTIONLOG_H
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
    namespace Scheduler {
        namespace DTO {
            /**
             * @brief Enum định nghĩa trạng thái thực thi của một tác vụ.
             */
            enum class TaskExecutionStatus {
                SUCCESS = 0,    /**< Thành công */
                FAILED = 1,     /**< Thất bại */
                RUNNING = 2,    /**< Đang chạy */
                SKIPPED = 3     /**< Bỏ qua (ví dụ: do tác vụ trước đó chưa hoàn thành) */
            };
            /**
             * @brief DTO for Task Execution Log entity.
             * Records the details of each run of a scheduled task.
             */
            struct TaskExecutionLogDTO : public BaseDTO {
                std::string scheduledTaskId;    /**< ID của tác vụ được lên lịch liên quan */
                std::chrono::system_clock::time_point startTime; /**< Thời gian bắt đầu thực thi */
                std::optional<std::chrono::system_clock::time_point> endTime; /**< Thời gian kết thúc thực thi */
                TaskExecutionStatus status;     /**< Trạng thái thực thi */
                std::optional<std::string> executedByUserId; /**< ID người dùng thực thi (hoặc hệ thống) */
                std::optional<std::string> logOutput; /**< Đầu ra log của tác vụ */
                std::optional<std::string> errorMessage; /**< Thông báo lỗi nếu thất bại */
                std::map<std::string, std::any> executionContext; /**< Ngữ cảnh thực thi (tham số đầu vào, biến môi trường, ...) dạng map (thay thế QJsonObject) */

                TaskExecutionLogDTO() : BaseDTO(), scheduledTaskId(""),
                                        startTime(std::chrono::system_clock::now()),
                                        status(TaskExecutionStatus::RUNNING) {}
                virtual ~TaskExecutionLogDTO() = default;

                // Utility methods
                std::string getStatusString() const {
                    switch (status) {
                        case TaskExecutionStatus::SUCCESS: return "Success";
                        case TaskExecutionStatus::FAILED: return "Failed";
                        case TaskExecutionStatus::RUNNING: return "Running";
                        case TaskExecutionStatus::SKIPPED: return "Skipped";
                        default: return "Unknown";
                    }
                }
            };
        } // namespace DTO
    } // namespace Scheduler
} // namespace ERP
#endif // MODULES_SCHEDULER_DTO_TASKEXECUTIONLOG_H