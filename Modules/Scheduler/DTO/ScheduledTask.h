// Modules/Scheduler/DTO/ScheduledTask.h
#ifndef MODULES_SCHEDULER_DTO_SCHEDULEDTASK_H
#define MODULES_SCHEDULER_DTO_SCHEDULEDTASK_H
#include <string>
#include <optional>
#include <chrono>
#include <map>    // For std::map<string, any>
#include <any>    // For std::any
// #include <QJsonObject> // Removed
// #include <QString>     // Removed
// #include <QDateTime>   // Removed
#include "DataObjects/BaseDTO.h"   // ĐÃ SỬA: Đã bỏ đường dẫn tương đối, dùng tên tệp trực tiếp
#include "Modules/Common/Common.h"    // ĐÃ SỬA: Đã bỏ đường dẫn tương đối, dùng tên tệp trực tiếp
#include "Modules/Utils/Utils.h"     // ĐÃ SỬA: Đã bỏ đường dẫn tương đối, dùng tên tệp trực tiếp
namespace ERP {
    namespace Scheduler {
        namespace DTO {
            /**
             * @brief Enum định nghĩa tần suất lập lịch.
             */
            enum class ScheduleFrequency {
                ONCE = 0,       /**< Chạy một lần */
                HOURLY = 1,     /**< Hàng giờ */
                DAILY = 2,      /**< Hàng ngày */
                WEEKLY = 3,     /**< Hàng tuần */
                MONTHLY = 4,    /**< Hàng tháng */
                YEARLY = 5,     /**< Hàng năm */
                CUSTOM_CRON = 6 /**< Biểu thức Cron tùy chỉnh */
            };
            /**
             * @brief Enum định nghĩa trạng thái của một tác vụ được lên lịch.
             */
            enum class ScheduledTaskStatus {
                ACTIVE = 0,     /**< Đang hoạt động, sẽ được chạy theo lịch */
                INACTIVE = 1,   /**< Không hoạt động, sẽ không được chạy */
                SUSPENDED = 2,  /**< Tạm dừng, có thể kích hoạt lại sau */
                COMPLETED = 3,  /**< Đã hoàn thành (chỉ cho tác vụ chạy một lần) */
                FAILED = 4      /**< Thất bại, cần kiểm tra */
            };
            /**
             * @brief DTO for Scheduled Task entity.
             */
            struct ScheduledTaskDTO : public BaseDTO {
                std::string taskName;           /**< Tên của tác vụ */
                std::string taskType;           /**< Loại tác vụ (ví dụ: "ReportGeneration", "DataCleanup") */
                ScheduleFrequency frequency;    /**< Tần suất lập lịch */
                std::optional<std::string> cronExpression; /**< Biểu thức Cron nếu tần suất là CUSTOM_CRON */
                std::chrono::system_clock::time_point nextRunTime; /**< Thời gian chạy tiếp theo */
                std::optional<std::chrono::system_clock::time_point> lastRunTime; /**< Thời gian chạy cuối cùng */
                ScheduledTaskStatus status;     /**< Trạng thái của tác vụ */
                std::optional<std::string> assignedToUserId; /**< Người dùng chịu trách nhiệm (nếu có) */
                std::optional<std::string> lastErrorMessage; /**< Thông báo lỗi từ lần chạy cuối cùng */
                std::map<std::string, std::any> parameters; /**< Tham số tác vụ dạng map (thay thế QJsonObject) */
                std::optional<std::chrono::system_clock::time_point> startDate; /**< Ngày bắt đầu hiệu lực của lịch */
                std::optional<std::chrono::system_clock::time_point> endDate;   /**< Ngày kết thúc hiệu lực của lịch */

                ScheduledTaskDTO() : BaseDTO(), taskName(""), taskType(""),
                                     frequency(ScheduleFrequency::ONCE),
                                     nextRunTime(std::chrono::system_clock::now()),
                                     status(ScheduledTaskStatus::ACTIVE) {}
                virtual ~ScheduledTaskDTO() = default;

                // Utility methods (can be moved to StringUtils or helper for DTOs)
                std::string getFrequencyString() const {
                    switch (frequency) {
                        case ScheduleFrequency::ONCE: return "Once";
                        case ScheduleFrequency::HOURLY: return "Hourly";
                        case ScheduleFrequency::DAILY: return "Daily";
                        case ScheduleFrequency::WEEKLY: return "Weekly";
                        case ScheduleFrequency::MONTHLY: return "Monthly";
                        case ScheduleFrequency::YEARLY: return "Yearly";
                        case ScheduleFrequency::CUSTOM_CRON: return "Custom (Cron)";
                        default: return "Unknown";
                    }
                }
                std::string getStatusString() const {
                    switch (status) {
                        case ScheduledTaskStatus::ACTIVE: return "Active";
                        case ScheduledTaskStatus::INACTIVE: return "Inactive";
                        case ScheduledTaskStatus::SUSPENDED: return "Suspended";
                        case ScheduledTaskStatus::COMPLETED: return "Completed";
                        case ScheduledTaskStatus::FAILED: return "Failed";
                        default: return "Unknown";
                    }
                }
            };
        } // namespace DTO
    } // namespace Scheduler
} // namespace ERP
#endif // MODULES_SCHEDULER_DTO_SCHEDULEDTASK_H