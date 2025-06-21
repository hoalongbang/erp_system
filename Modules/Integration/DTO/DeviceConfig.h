// Modules/Integration/DTO/DeviceConfig.h
#ifndef MODULES_INTEGRATION_DTO_DEVICECONFIG_H
#define MODULES_INTEGRATION_DTO_DEVICECONFIG_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên lớp cơ sở

namespace ERP {
namespace Integration {
namespace DTO {

/**
 * @brief Enum defining types of devices that can be integrated.
 */
enum class DeviceType {
    BARCODE_SCANNER = 0,    /**< Máy quét mã vạch. */
    WEIGHING_SCALE = 1,     /**< Cân điện tử. */
    RFID_READER = 2,        /**< Đầu đọc RFID. */
    PRINTER = 3,            /**< Máy in (nhãn, hóa đơn). */
    SENSOR = 4,             /**< Cảm biến (nhiệt độ, độ ẩm, áp suất). */
    OTHER = 99,             /**< Loại thiết bị khác. */
    UNKNOWN = 100           /**< Loại thiết bị không xác định. */
};

/**
 * @brief Enum defining the connection status of a device.
 */
enum class ConnectionStatus {
    CONNECTED = 0,  /**< Thiết bị đang kết nối và hoạt động. */
    DISCONNECTED = 1, /**< Thiết bị đã ngắt kết nối. */
    ERROR = 2       /**< Có lỗi trong quá trình kết nối/vận hành. */
};

/**
 * @brief DTO for Device Configuration entity.
 * Represents the configuration settings for a physical device integrated with the system.
 */
struct DeviceConfigDTO : public BaseDTO {
    std::string deviceName;             /**< Tên hiển thị của thiết bị. */
    std::string deviceIdentifier;       /**< Mã định danh duy nhất của thiết bị (ví dụ: Serial Number). */
    DeviceType type;                    /**< Loại thiết bị (máy quét, cân, v.v.). */
    std::optional<std::string> connectionString; /**< Chuỗi kết nối/địa chỉ (ví dụ: COM1, IP:Port) (tùy chọn). */
    std::optional<std::string> ipAddress;/**< Địa chỉ IP của thiết bị (nếu có) (tùy chọn). */
    ConnectionStatus connectionStatus;  /**< Trạng thái kết nối hiện tại của thiết bị. */
    std::optional<std::string> locationId; /**< ID của vị trí vật lý của thiết bị trong kho/nhà máy (tùy chọn). */
    std::optional<std::string> notes;   /**< Ghi chú về thiết bị (tùy chọn). */
    bool isCritical = false;            /**< Cờ chỉ báo thiết bị có quan trọng đối với hoạt động không. */

    // Default constructor to initialize BaseDTO and specific fields
    DeviceConfigDTO() : BaseDTO(), connectionStatus(ConnectionStatus::DISCONNECTED) {}

    // Virtual destructor for proper polymorphic cleanup
    virtual ~DeviceConfigDTO() = default;

    /**
     * @brief Converts a DeviceType enum value to its string representation.
     * @return The string representation of the device type.
     */
    std::string getTypeString() const {
        switch (type) {
            case DeviceType::BARCODE_SCANNER: return "Barcode Scanner";
            case DeviceType::WEIGHING_SCALE: return "Weighing Scale";
            case DeviceType::RFID_READER: return "RFID Reader";
            case DeviceType::PRINTER: return "Printer";
            case DeviceType::SENSOR: return "Sensor";
            case DeviceType::OTHER: return "Other";
            case DeviceType::UNKNOWN: return "Unknown";
            default: return "Unknown";
        }
    }

    /**
     * @brief Converts a ConnectionStatus enum value to its string representation.
     * @return The string representation of the connection status.
     */
    std::string getConnectionStatusString() const {
        switch (connectionStatus) {
            case ConnectionStatus::CONNECTED: return "Connected";
            case ConnectionStatus::DISCONNECTED: return "Disconnected";
            case ConnectionStatus::ERROR: return "Error";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Integration
} // namespace ERP
#endif // MODULES_INTEGRATION_DTO_DEVICECONFIG_H