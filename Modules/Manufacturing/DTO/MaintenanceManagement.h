// Modules/Manufacturing/DTO/MaintenanceManagement.h
#ifndef MODULES_MANUFACTURING_DTO_MAINTENANCEMANAGEMENT_H
#define MODULES_MANUFACTURING_DTO_MAINTENANCEMANAGEMENT_H
#include <string>
#include <optional>
#include <chrono>
#include <map>    // For std::map<std::string, std::any> replacement of QJsonObject
#include <any>    // For std::any replacement of QJsonObject
#include <vector> // For MaintenanceActivityDTO if nested

// #include <QJsonObject> // Removed
// #include <QString>     // Removed
// #include <QDateTime>   // Removed
#include "DataObjects/BaseDTO.h"
#include "Modules/Common/Common.h" // For EntityStatus
#include "Modules/Utils/Utils.h"   // For generateUUID

namespace ERP {
namespace Manufacturing {
namespace DTO {

/**
 * @brief Enum định nghĩa loại yêu cầu bảo trì.
 */
enum class MaintenanceRequestType {
    PREVENTIVE = 0, /**< Bảo trì phòng ngừa */
    CORRECTIVE = 1, /**< Bảo trì khắc phục */
    PREDICTIVE = 2, /**< Bảo trì dự đoán */
    INSPECTION = 3  /**< Kiểm tra */
};

/**
 * @brief Enum định nghĩa mức độ ưu tiên của yêu cầu bảo trì.
 */
enum class MaintenancePriority {
    LOW = 0,    /**< Thấp */
    NORMAL = 1, /**< Bình thường */
    HIGH = 2,   /**< Cao */
    URGENT = 3  /**< Khẩn cấp */
};

/**
 * @brief Enum định nghĩa trạng thái của yêu cầu bảo trì.
 */
enum class MaintenanceRequestStatus {
    PENDING = 0,    /**< Đang chờ */
    SCHEDULED = 1,  /**< Đã lên lịch */
    IN_PROGRESS = 2,/**< Đang thực hiện */
    COMPLETED = 3,  /**< Đã hoàn thành */
    CANCELLED = 4,  /**< Đã hủy */
    REJECTED = 5    /**< Đã từ chối */
};

/**
 * @brief DTO for Maintenance Request entity.
 * Represents a request for maintenance on an asset.
 */
struct MaintenanceRequestDTO : public BaseDTO {
    std::string assetId;        /**< ID của tài sản cần bảo trì */
    MaintenanceRequestType requestType; /**< Loại yêu cầu (phòng ngừa, khắc phục, v.v.) */
    MaintenancePriority priority; /**< Mức độ ưu tiên */
    MaintenanceRequestStatus status; /**< Trạng thái hiện tại của yêu cầu */
    std::optional<std::string> description; /**< Mô tả chi tiết vấn đề hoặc công việc */
    std::string requestedByUserId; /**< ID người dùng yêu cầu bảo trì */
    std::chrono::system_clock::time_point requestedDate; /**< Ngày yêu cầu */
    std::optional<std::chrono::system_clock::time_point> scheduledDate; /**< Ngày dự kiến thực hiện */
    std::optional<std::string> assignedToUserId; /**< ID người dùng/kỹ thuật viên được giao */
    std::optional<std::string> failureReason; /**< Lý do hỏng hóc (nếu là khắc phục) */
    std::map<std::string, std::any> metadata; /**< Dữ liệu bổ sung dạng map */

    MaintenanceRequestDTO() : BaseDTO(), assetId(""), requestType(MaintenanceRequestType::CORRECTIVE),
                              priority(MaintenancePriority::NORMAL), status(MaintenanceRequestStatus::PENDING),
                              requestedByUserId(""), requestedDate(std::chrono::system_clock::now()) {}
    virtual ~MaintenanceRequestDTO() = default;

    // Utility methods (can be moved to StringUtils or helper for DTOs)
    std::string getTypeString() const {
        switch (requestType) {
            case MaintenanceRequestType::PREVENTIVE: return "Preventive";
            case MaintenanceRequestType::CORRECTIVE: return "Corrective";
            case MaintenanceRequestType::PREDICTIVE: return "Predictive";
            case MaintenanceRequestType::INSPECTION: return "Inspection";
            default: return "Unknown";
        }
    }
    std::string getPriorityString() const {
        switch (priority) {
            case MaintenancePriority::LOW: return "Low";
            case MaintenancePriority::NORMAL: return "Normal";
            case MaintenancePriority::HIGH: return "High";
            case MaintenancePriority::URGENT: return "Urgent";
            default: return "Unknown";
        }
    }
    std::string getStatusString() const {
        switch (status) {
            case MaintenanceRequestStatus::PENDING: return "Pending";
            case MaintenanceRequestStatus::SCHEDULED: return "Scheduled";
            case MaintenanceRequestStatus::IN_PROGRESS: return "In Progress";
            case MaintenanceRequestStatus::COMPLETED: return "Completed";
            case MaintenanceRequestStatus::CANCELLED: return "Cancelled";
            case MaintenanceRequestStatus::REJECTED: return "Rejected";
            default: return "Unknown";
        }
    }
};

/**
 * @brief DTO for Maintenance Activity entity.
 * Represents a record of work performed as part of a maintenance request.
 */
struct MaintenanceActivityDTO : public BaseDTO {
    std::string maintenanceRequestId; /**< ID của yêu cầu bảo trì liên quan */
    std::string activityDescription;  /**< Mô tả công việc đã thực hiện */
    std::chrono::system_clock::time_point activityDate; /**< Ngày thực hiện hoạt động */
    std::string performedByUserId;    /**< ID người dùng/kỹ thuật viên thực hiện */
    double durationHours;             /**< Thời lượng hoạt động tính bằng giờ */
    std::optional<double> cost;       /**< Chi phí liên quan đến hoạt động */
    std::optional<std::string> costCurrency; /**< Đơn vị tiền tệ chi phí */
    std::optional<std::string> partsUsed;    /**< Các bộ phận đã sử dụng (mô tả hoặc ID) */
    std::map<std::string, std::any> metadata; /**< Dữ liệu bổ sung dạng map */

    MaintenanceActivityDTO() : BaseDTO(), maintenanceRequestId(""), activityDescription(""),
                               activityDate(std::chrono::system_clock::now()), performedByUserId(""),
                               durationHours(0.0) {}
    virtual ~MaintenanceActivityDTO() = default;
};

} // namespace DTO
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_DTO_MAINTENANCEMANAGEMENT_H