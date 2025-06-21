// Modules/Report/DTO/Report.h
#ifndef MODULES_REPORT_DTO_REPORT_H
#define MODULES_REPORT_DTO_REPORT_H
#include <string>
#include <optional>
#include <chrono>
#include <vector>
#include <map>    // For std::map<string, any> replacement of QJsonObject
#include <any>    // For std::any replacement of QJsonObject
// #include <QJsonObject> // Removed
// #include <QString>     // Removed
// #include <QDateTime>   // Removed
#include "DataObjects/BaseDTO.h"
#include "Modules/Common/Common.h" // For EntityStatus
#include "Modules/Utils/Utils.h"   // For generateUUID

namespace ERP {
namespace Report {
namespace DTO {
/**
 * @brief Enum định nghĩa tần suất báo cáo.
 */
enum class ReportFrequency {
    ONCE = 0,       /**< Chạy một lần */
    HOURLY = 1,     /**< Hàng giờ */
    DAILY = 2,      /**< Hàng ngày */
    WEEKLY = 3,     /**< Hàng tuần */
    MONTHLY = 4,    /**< Hàng tháng */
    QUARTERLY = 5,  /**< Hàng quý */
    YEARLY = 6,     /**< Hàng năm */
    CUSTOM = 7      /**< Tùy chỉnh (ví dụ: biểu thức cron) */
};
/**
 * @brief Enum định nghĩa định dạng báo cáo.
 */
enum class ReportFormat {
    PDF = 0,    /**< Portable Document Format */
    EXCEL = 1,  /**< Microsoft Excel Spreadsheet */
    CSV = 2,    /**< Comma Separated Values */
    HTML = 3,   /**< HyperText Markup Language */
    JSON = 4    /**< JavaScript Object Notation */
};
/**
 * @brief Enum định nghĩa trạng thái của báo cáo đã thực thi.
 */
enum class ReportExecutionStatus {
    PENDING = 0,    /**< Đang chờ xử lý */
    IN_PROGRESS = 1,/**< Đang chạy */
    COMPLETED = 2,  /**< Đã hoàn thành */
    FAILED = 3,     /**< Thất bại */
    CANCELLED = 4   /**< Đã hủy */
};

/**
 * @brief DTO for Report Request entity.
 * Represents a request to generate a specific report with given parameters.
 */
struct ReportRequestDTO : public BaseDTO {
    std::string reportName;             /**< Tên báo cáo (ví dụ: "InventorySummaryReport") */
    std::string reportType;             /**< Loại báo cáo (chuỗi để linh hoạt hơn) */
    ReportFrequency frequency;          /**< Tần suất chạy báo cáo */
    ReportFormat format;                /**< Định dạng đầu ra */
    std::string requestedByUserId;      /**< ID người dùng yêu cầu báo cáo */
    std::chrono::system_clock::time_point requestedTime; /**< Thời gian yêu cầu */
    std::map<std::string, std::any> parameters; /**< Tham số báo cáo dạng map (thay thế QJsonObject) */
    std::optional<std::string> outputPath; /**< Đường dẫn lưu trữ đầu ra */
    std::optional<std::string> scheduleCronExpression; /**< Biểu thức Cron nếu tần suất là CUSTOM */
    std::optional<std::string> emailRecipients; /**< Danh sách email nhận báo cáo, phân tách bằng dấu phẩy */

    ReportRequestDTO() : BaseDTO(), reportName(""), reportType(""),
                         frequency(ReportFrequency::ONCE), format(ReportFormat::PDF),
                         requestedByUserId(""), requestedTime(std::chrono::system_clock::now()) {}
    virtual ~ReportRequestDTO() = default;

    // Utility methods (can be moved to StringUtils or helper for DTOs)
    std::string getFrequencyString() const {
        switch (frequency) {
            case ReportFrequency::ONCE: return "Once";
            case ReportFrequency::HOURLY: return "Hourly";
            case ReportFrequency::DAILY: return "Daily";
            case ReportFrequency::WEEKLY: return "Weekly";
            case ReportFrequency::MONTHLY: return "Monthly";
            case ReportFrequency::QUARTERLY: return "Quarterly";
            case ReportFrequency::YEARLY: return "Yearly";
            case ReportFrequency::CUSTOM: return "Custom";
            default: return "Unknown";
        }
    }
    std::string getFormatString() const {
        switch (format) {
            case ReportFormat::PDF: return "PDF";
            case ReportFormat::EXCEL: return "Excel";
            case ReportFormat::CSV: return "CSV";
            case ReportFormat::HTML: return "HTML";
            case ReportFormat::JSON: return "JSON";
            default: return "Unknown";
        }
    }
};

/**
 * @brief DTO for Report Execution Log entity.
 * Records the details of a single report generation attempt.
 */
struct ReportExecutionLogDTO : public BaseDTO {
    std::string reportRequestId;          /**< ID của yêu cầu báo cáo liên quan */
    std::chrono::system_clock::time_point executionTime; /**< Thời gian bắt đầu thực thi */
    ReportExecutionStatus status;         /**< Trạng thái của lần thực thi này */
    std::optional<std::string> executedByUserId; /**< ID người dùng thực thi (hoặc hệ thống) */
    std::optional<std::string> actualOutputPath; /**< Đường dẫn thực tế của tệp báo cáo được tạo */
    std::optional<std::string> errorMessage;     /**< Thông báo lỗi nếu thất bại */
    std::map<std::string, std::any> executionMetadata; /**< Dữ liệu bổ sung về quá trình thực thi (thay thế QJsonObject) */

    ReportExecutionLogDTO() : BaseDTO(), reportRequestId(""),
                              executionTime(std::chrono::system_clock::now()),
                              status(ReportExecutionStatus::PENDING) {}
    virtual ~ReportExecutionLogDTO() = default;

    // Utility methods
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
#endif // MODULES_REPORT_DTO_REPORT_H