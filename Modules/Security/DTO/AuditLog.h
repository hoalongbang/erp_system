// Modules/Security/DTO/AuditLog.h
#ifndef MODULES_SECURITY_DTO_AUDITLOG_H
#define MODULES_SECURITY_DTO_AUDITLOG_H
#include <string>
#include <optional>
#include <chrono>
#include <map> // For std::map<string, any> replacement of QJsonObject
#include <any> // For std::any replacement of QJsonObject
// #include <QJsonObject> // Removed
// #include <QString>     // Removed
// #include <QDateTime>   // Removed
#include "DataObjects/BaseDTO.h"
#include "Modules/Common/Common.h" // For EntityStatus, LogSeverity
#include "Modules/Utils/Utils.h"   // For generateUUID

namespace ERP {
namespace Security {
namespace DTO {

/**
 * @brief Enum định nghĩa loại hành động kiểm toán.
 */
enum class AuditActionType {
    LOGIN = 0,               /**< Đăng nhập thành công */
    LOGIN_FAILED = 1,        /**< Đăng nhập thất bại */
    LOGOUT = 2,              /**< Đăng xuất */
    CREATE = 3,              /**< Tạo bản ghi mới */
    UPDATE = 4,              /**< Cập nhật bản ghi */
    DELETE = 5,              /**< Xóa bản ghi */
    VIEW = 6,                /**< Xem bản ghi (đặc biệt là dữ liệu nhạy cảm) */
    PASSWORD_CHANGE = 7,     /**< Thay đổi mật khẩu */
    PERMISSION_CHANGE = 8,   /**< Thay đổi quyền hạn */
    CONFIGURATION_CHANGE = 9,/**< Thay đổi cấu hình hệ thống */
    FILE_UPLOAD = 10,        /**< Tải lên tệp */
    FILE_DOWNLOAD = 11,      /**< Tải xuống tệp */
    PROCESS_START = 12,      /**< Bắt đầu một quy trình nghiệp vụ */
    PROCESS_END = 13,        /**< Kết thúc một quy trình nghiệp vụ */
    ERROR = 14,              /**< Lỗi hệ thống */
    WARNING = 15,            /**< Cảnh báo hệ thống */
    IMPERSONATION = 16,      /**< Mạo danh người dùng khác */
    DATA_EXPORT = 17,        /**< Xuất dữ liệu */
    DATA_IMPORT = 18,        /**< Nhập dữ liệu */
    SCHEDULED_TASK = 19,     /**< Tác vụ được lên lịch */
    EQUIPMENT_CALIBRATION = 20, /**< Hiệu chuẩn thiết bị */
    CUSTOM = 99              /**< Loại hành động tùy chỉnh */
};

/**
 * @brief DTO for Audit Log entity.
 * Represents a record of significant activities or changes in the system.
 */
struct AuditLogDTO : public BaseDTO {
    std::string userId;          /**< ID người dùng thực hiện hành động */
    std::string userName;        /**< Tên người dùng thực hiện hành động */
    std::optional<std::string> sessionId; /**< ID phiên đăng nhập (nếu có) */
    AuditActionType actionType;  /**< Loại hành động kiểm toán */
    ERP::Common::LogSeverity severity; /**< Mức độ nghiêm trọng của log */
    std::string module;          /**< Module liên quan (ví dụ: "Sales", "Inventory") */
    std::string subModule;       /**< Module con/Tính năng (ví dụ: "SalesOrder", "StockAdjustment") */
    std::optional<std::string> entityId;    /**< ID của thực thể bị ảnh hưởng */
    std::optional<std::string> entityType;  /**< Loại thực thể bị ảnh hưởng (ví dụ: "Product", "User") */
    std::optional<std::string> entityName;  /**< Tên của thực thể bị ảnh hưởng */
    std::optional<std::string> ipAddress;   /**< Địa chỉ IP của client */
    std::optional<std::string> userAgent;   /**< User-Agent của trình duyệt/ứng dụng */
    std::optional<std::string> workstationId; /**< ID máy trạm */

    std::optional<std::string> productionLineId; /**< ID dây chuyền sản xuất (nếu có) */
    std::optional<std::string> shiftId;          /**< ID ca làm việc (nếu có) */
    std::optional<std::string> batchNumber;      /**< Số lô sản xuất (nếu có) */
    std::optional<std::string> partNumber;       /**< Mã sản phẩm/chi tiết (nếu có) */

    std::map<std::string, std::any> beforeData; /**< Dữ liệu của thực thể trước khi thay đổi (thay thế QJsonObject) */
    std::map<std::string, std::any> afterData;  /**< Dữ liệu của thực thể sau khi thay đổi (thay thế QJsonObject) */
    std::optional<std::string> changeReason;     /**< Lý do thay đổi */
    std::map<std::string, std::any> metadata;   /**< Dữ liệu bổ sung dạng map (thay thế QJsonObject) */
    std::optional<std::string> comments;         /**< Bình luận thêm */
    std::optional<std::string> approvalId;       /**< ID phê duyệt liên quan (nếu có) */
    bool isCompliant;           /**< True nếu hành động tuân thủ chính sách/quy định, false nếu không */
    std::optional<std::string> complianceNote;   /**< Ghi chú về việc tuân thủ */

    AuditLogDTO() : BaseDTO(), userId(""), userName(""), actionType(AuditActionType::CUSTOM),
                    severity(ERP::Common::LogSeverity::INFO), module(""), subModule(""), isCompliant(true) {}
    virtual ~AuditLogDTO() = default;

    // Utility methods
    std::string getActionTypeString() const {
        switch (actionType) {
            case AuditActionType::LOGIN: return "Login";
            case AuditActionType::LOGIN_FAILED: return "Login Failed";
            case AuditActionType::LOGOUT: return "Logout";
            case AuditActionType::CREATE: return "Create";
            case AuditActionType::UPDATE: return "Update";
            case AuditActionType::DELETE: return "Delete";
            case AuditActionType::VIEW: return "View";
            case AuditActionType::PASSWORD_CHANGE: return "Password Change";
            case AuditActionType::PERMISSION_CHANGE: return "Permission Change";
            case AuditActionType::CONFIGURATION_CHANGE: return "Configuration Change";
            case AuditActionType::FILE_UPLOAD: return "File Upload";
            case AuditActionType::FILE_DOWNLOAD: return "File Download";
            case AuditActionType::PROCESS_START: return "Process Start";
            case AuditActionType::PROCESS_END: return "Process End";
            case AuditActionType::ERROR: return "Error";
            case AuditActionType::WARNING: return "Warning";
            case AuditActionType::IMPERSONATION: return "Impersonation";
            case AuditActionType::DATA_EXPORT: return "Data Export";
            case AuditActionType::DATA_IMPORT: return "Data Import";
            case AuditActionType::SCHEDULED_TASK: return "Scheduled Task";
            case AuditActionType::EQUIPMENT_CALIBRATION: return "Equipment Calibration";
            case AuditActionType::CUSTOM: return "Custom";
            default: return "Unknown";
        }
    }
};

} // namespace DTO
} // namespace Security
} // namespace ERP
#endif // MODULES_SECURITY_DTO_AUDITLOG_H