// Modules/Report/DTO/ReportExecutionLog.h
#ifndef MODULES_REPORT_DTO_REPORTEXECUTIONLOG_H
#define MODULES_REPORT_DTO_REPORTEXECUTIONLOG_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point
#include <map>          // For execution_metadata_json
#include <any>          // For std::any in map

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên lớp cơ sở

namespace ERP {
namespace Report {
namespace DTO {

/**
 * @brief Enum defining the status of a report execution.
 */
enum class ReportExecutionStatus {
    PENDING = 0,    /**< Báo cáo đang chờ được thực thi. */
    IN_PROGRESS = 1,/**< Báo cáo đang được thực thi. */
    COMPLETED = 2,  /**< Báo cáo đã thực thi thành công. */
    FAILED = 3,     /**< Báo cáo thực thi thất bại. */
    CANCELLED = 4   /**< Báo cáo đã bị hủy. */
};

/**
 * @brief DTO for Report Execution Log entity.
 * Records each instance of a report being generated (scheduled or manual).
 */
struct ReportExecutionLogDTO : public BaseDTO {
    std::string reportRequestId;                /**< ID của yêu cầu báo cáo cha. */
    std::chrono::system_clock::time_point executionTime; /**< Thời gian báo cáo được thực thi. */
    ReportExecutionStatus status;               /**< Trạng thái của lần thực thi này (SUCCESS, FAILED, RUNNING, etc.). */
    std::optional<std::string> executedByUserId; /**< ID người dùng đã thực thi báo cáo (có thể là system) (tùy chọn). */
    std::optional<std::string> actualOutputPath; /**< Đường dẫn thực tế đến tệp báo cáo được tạo (tùy chọn). */
    std::optional<std::string> errorMessage;   /**< Thông báo lỗi nếu thực thi thất bại (tùy chọn). */
    std::map<std::string, std::any> executionMetadata; /**< Metadata về lần thực thi này (ví dụ: các tham số được sử dụng). */
    std::optional<std::string> logOutput;      /**< Đầu ra log chi tiết của quá trình thực thi (tùy chọn). */

    // Default constructor
    ReportExecutionLogDTO() : BaseDTO() {}

    // Virtual destructor for proper polymorphic cleanup
    virtual ~ReportExecutionLogDTO() = default;

    /**
     * @brief Converts a ReportExecutionStatus enum value to its string representation.
     * @return The string representation of the execution status.
     */
    std::string getStatusString() const {
        switch (status) {
            case ReportExecutionStatus::PENDING: return "Pending";
            case ReportExecutionStatus::IN_PROGRESS: return "In Progress";
            case ReportExecutionStatus::COMPLETED: return "Completed";
            case ReportExecutionStatus::FAILED: return "Failed";
            case ReportExecutionStatus::CANCELLED: return "Cancelled";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Report
} // namespace ERP
#endif // MODULES_REPORT_DTO_REPORTEXECUTIONLOG_H