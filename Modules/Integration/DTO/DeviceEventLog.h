// Modules/Integration/DTO/DeviceEventLog.h
#ifndef MODULES_INTEGRATION_DTO_DEVICEVENTLOG_H
#define MODULES_INTEGRATION_DTO_DEVICEVENTLOG_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point
#include <map>          // For event_data_json
#include <any>          // For std::any in map

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên lớp cơ sở

namespace ERP {
namespace Integration {
namespace DTO {

/**
 * @brief Enum defining types of device events.
 */
enum class DeviceEventType {
    CONNECTION_ESTABLISHED = 0, /**< Kết nối được thiết lập. */
    CONNECTION_LOST = 1,        /**< Mất kết nối. */
    CONNECTION_FAILED = 2,      /**< Kết nối thất bại. */
    DATA_RECEIVED = 3,          /**< Dữ liệu nhận được từ thiết bị. */
    COMMAND_SENT = 4,           /**< Lệnh đã gửi đến thiết bị. */
    ERROR = 5,                  /**< Thiết bị báo lỗi. */
    WARNING = 6,                /**< Thiết bị báo cảnh báo. */
    OTHER = 99                  /**< Loại sự kiện khác. */
};

/**
 * @brief DTO for Device Event Log entity.
 * Records significant events related to integrated devices.
 */
struct DeviceEventLogDTO : public BaseDTO {
    std::string deviceId;                   /**< ID của thiết bị liên quan đến sự kiện. */
    DeviceEventType eventType;              /**< Loại sự kiện (kết nối, dữ liệu, lỗi, v.v.). */
    std::chrono::system_clock::time_point eventTime; /**< Thời gian sự kiện xảy ra. */
    std::string eventDescription;           /**< Mô tả ngắn gọn về sự kiện. */
    std::map<std::string, std::any> eventData; /**< Dữ liệu chi tiết của sự kiện (dạng JSON string). */
    std::optional<std::string> notes;       /**< Ghi chú bổ sung về sự kiện (tùy chọn). */

    // Default constructor to initialize BaseDTO and specific fields
    DeviceEventLogDTO() : BaseDTO() {}

    // Virtual destructor for proper polymorphic cleanup
    virtual ~DeviceEventLogDTO() = default;

    /**
     * @brief Converts a DeviceEventType enum value to its string representation.
     * @return The string representation of the event type.
     */
    std::string getEventTypeString() const {
        switch (eventType) {
            case DeviceEventType::CONNECTION_ESTABLISHED: return "Connection Established";
            case DeviceEventType::CONNECTION_LOST: return "Connection Lost";
            case DeviceEventType::CONNECTION_FAILED: return "Connection Failed";
            case DeviceEventType::DATA_RECEIVED: return "Data Received";
            case DeviceEventType::COMMAND_SENT: return "Command Sent";
            case DeviceEventType::ERROR: return "Error";
            case DeviceEventType::WARNING: return "Warning";
            case DeviceEventType::OTHER: return "Other";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Integration
} // namespace ERP
#endif // MODULES_INTEGRATION_DTO_DEVICEVENTLOG_H