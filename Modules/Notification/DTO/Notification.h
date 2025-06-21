// Modules/Notification/DTO/Notification.h
#ifndef MODULES_NOTIFICATION_DTO_NOTIFICATION_H
#define MODULES_NOTIFICATION_DTO_NOTIFICATION_H
#include <string>
#include <optional>
#include <chrono>
#include <vector>
#include <memory>
#include <map> // For std::map<string, any> replacement of QJsonObject
#include <any> // For std::any replacement of QJsonObject
// #include <QJsonObject> // Removed
// #include <QDateTime>   // Removed
// #include <QPixmap>     // Removed
// #include <QIcon>       // Removed
#include "DataObjects/BaseDTO.h"
#include "Modules/Common/Common.h"
#include "Modules/Utils/Utils.h"   // For generateUUID
namespace ERP {
namespace Notification {
namespace DTO {
/**
 * @brief Enum defining the priority level of notification.
 */
enum class NotificationPriority {
    LOW = 0,        /**< Ưu tiên thấp */
    NORMAL = 1,     /**< Ưu tiên bình thường */
    HIGH = 2,       /**< Ưu tiên cao */
    URGENT = 3      /**< Ưu tiên khẩn cấp */
};
/**
 * @brief Enum defining the type of notification.
 */
enum class NotificationType {
    INFO = 0,       /**< Thông tin chung */
    WARNING = 1,    /**< Cảnh báo */
    ERROR = 2,      /**< Lỗi */
    SUCCESS = 3,    /**< Thành công */
    ALERT = 4,      /**< Cảnh báo nghiêm trọng */
    SYSTEM = 5      /**< Thông báo hệ thống */
};
/**
 * @brief DTO for Notification entity.
 * Represents a single notification sent within the system.
 */
struct NotificationDTO : public BaseDTO {
    std::string userId;             /**< ID người dùng nhận thông báo */
    std::string title;              /**< Tiêu đề thông báo */
    std::string message;            /**< Nội dung thông báo */
    NotificationType type;          /**< Loại thông báo (Info, Warning, Error, etc.) */
    NotificationPriority priority;  /**< Mức độ ưu tiên */
    std::chrono::system_clock::time_point sentTime; /**< Thời gian gửi */
    bool isRead;                    /**< Đã đọc chưa */
    std::optional<std::string> senderId; /**< ID người gửi (nếu là người dùng) hoặc hệ thống gửi */
    std::optional<std::string> relatedEntityId;   /**< ID của thực thể nghiệp vụ liên quan */
    std::optional<std::string> relatedEntityType; /**< Loại thực thể nghiệp vụ liên quan */
    std::map<std::string, std::any> customData; /**< Dữ liệu tùy chỉnh dạng map (thay thế QJsonObject) */
    std::map<std::string, std::any> metadata;   /**< Dữ liệu bổ sung dạng map (thay thế QJsonObject) */
    // std::optional<QPixmap> image;   // Removed QPixmap
    // std::optional<QIcon> icon;      // Removed QIcon

    NotificationDTO() : BaseDTO(), userId(""), title(""), message(""),
                        type(NotificationType::INFO), priority(NotificationPriority::NORMAL),
                        sentTime(std::chrono::system_clock::now()), isRead(false) {}
    virtual ~NotificationDTO() = default;

    // Utility methods (can be moved to StringUtils or helper for DTOs)
    std::string getTypeString() const {
        switch (type) {
            case NotificationType::INFO: return "Info";
            case NotificationType::WARNING: return "Warning";
            case NotificationType::ERROR: return "Error";
            case NotificationType::SUCCESS: return "Success";
            case NotificationType::ALERT: return "Alert";
            case NotificationType::SYSTEM: return "System";
            default: return "Unknown";
        }
    }
    std::string getPriorityString() const {
        switch (priority) {
            case NotificationPriority::LOW: return "Low";
            case NotificationPriority::NORMAL: return "Normal";
            case NotificationPriority::HIGH: return "High";
            case NotificationPriority::URGENT: return "Urgent";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Notification
} // namespace ERP
#endif // MODULES_NOTIFICATION_DTO_NOTIFICATION_H