// Modules/Asset/DTO/Asset.h
#ifndef MODULES_ASSET_DTO_ASSET_H
#define MODULES_ASSET_DTO_ASSET_H
#include <string>
#include <optional>
#include <chrono>
#include <map> // For std::map<std::string, std::any> replacement of QJsonObject
#include <any> // For std::any replacement of QJsonObject
// #include <QJsonObject> // Removed
// #include <QString>     // Removed
// #include <QDateTime>   // Removed
#include "DataObjects/BaseDTO.h"
#include "Modules/Common/Common.h" // For EntityStatus
#include "Modules/Utils/Utils.h" // For generateUUID
// No longer need Q_DECLARE_METATYPE here if AssetDTO only uses standard C++ types
namespace ERP {
namespace Asset {
namespace DTO {
/**
 * @brief Enum định nghĩa loại tài sản.
 */
enum class AssetType {
    EQUIPMENT = 0,      /**< Thiết bị sản xuất (máy móc, dây chuyền) */
    VEHICLE = 1,        /**< Phương tiện vận tải (xe tải, xe nâng) */
    BUILDING = 2,       /**< Tòa nhà/Cơ sở vật chất */
    TOOL = 3,           /**< Công cụ/Dụng cụ */
    FURNITURE = 4,      /**< Nội thất/Trang thiết bị văn phòng */
    IT_HARDWARE = 5,    /**< Phần cứng CNTT (máy tính, máy chủ) */
    INFRASTRUCTURE = 6, /**< Hạ tầng (điện, nước, mạng) */
    OTHER = 7           /**< Loại khác */
};
/**
 * @brief Enum định nghĩa trạng thái của tài sản.
 */
enum class AssetState {
    ACTIVE = 0,         /**< Đang hoạt động */
    IN_MAINTENANCE = 1, /**< Đang bảo trì */
    BREAKDOWN = 2,      /**< Hỏng hóc */
    IDLE = 3,           /**< Rảnh rỗi/Không sử dụng */
    RETIRED = 4,        /**< Đã ngừng sử dụng/Thanh lý */
    CALIBRATION = 5,    /**< Đang hiệu chuẩn */
    TRANSFER = 6        /**< Đang luân chuyển */
};
/**
 * @brief Enum định nghĩa tình trạng vật lý của tài sản.
 */
enum class AssetCondition {
    EXCELLENT = 0, /**< Tuyệt vời */
    GOOD = 1,      /**< Tốt */
    FAIR = 2,      /**< Trung bình */
    POOR = 3,      /**< Kém */
    DAMAGED = 4    /**< Hư hại */
};
/**
 * @brief DTO for Asset entity.
 * Represents a physical asset (machine, equipment, vehicle, etc.) in the ERP system.
 */
struct AssetDTO : public BaseDTO {
    std::string assetName;      /**< Tên tài sản */
    std::string assetCode;      /**< Mã tài sản (duy nhất) */
    AssetType type;             /**< Loại tài sản */
    AssetState state;           /**< Trạng thái hiện tại của tài sản */
    std::optional<std::string> description; /**< Mô tả chi tiết */

    std::string serialNumber;   /**< Số serial */
    std::string manufacturer;   /**< Nhà sản xuất */
    std::string model;          /**< Model */
    std::optional<std::chrono::system_clock::time_point> purchaseDate; /**< Ngày mua */
    double purchaseCost;        /**< Chi phí mua ban đầu */
    std::optional<std::string> currency; /**< Đơn vị tiền tệ */
    std::optional<std::chrono::system_clock::time_point> installationDate; /**< Ngày lắp đặt */
    std::optional<std::chrono::system_clock::time_point> warrantyEndDate;  /**< Ngày hết hạn bảo hành */

    // Vị trí và thuộc tính sản xuất
    std::optional<std::string> locationId;     /**< ID vị trí vật lý (liên kết với Catalog/Location) */
    std::optional<std::string> productionLineId; /**< ID dây chuyền sản xuất (nếu là thiết bị sản xuất) */

    std::map<std::string, std::any> configuration; /**< Cấu hình kỹ thuật dạng map<string, any> (ví dụ: công suất, thông số kỹ thuật) */
    std::map<std::string, std::any> metadata;      /**< Dữ liệu bổ sung dạng map<string, any> (ví dụ: hình ảnh, tài liệu đính kèm) */

    bool isActive = true;                      /**< MỚI: Trạng thái kích hoạt (hoạt động/không hoạt động) */
    AssetCondition condition = AssetCondition::GOOD; /**< MỚI: Tình trạng vật lý */
    double currentValue = 0.0;                 /**< MỚI: Giá trị hiện tại (giá trị sổ sách) */

    std::string registeredByUserId;     /**< ID người dùng đăng ký tài sản */
    std::chrono::system_clock::time_point registeredAt; /**< Thời gian đăng ký */

    // Constructors
    AssetDTO() : BaseDTO(), assetName(""), assetCode(ERP::Utils::generateUUID()), serialNumber(""), manufacturer(""), model(""),
                 type(AssetType::EQUIPMENT), state(AssetState::ACTIVE),
                 purchaseCost(0.0), currency("VND"), registeredAt(std::chrono::system_clock::now()) {}

    virtual ~AssetDTO() = default;

    // Utility methods (can be moved to StringUtils or helper for DTOs)
    std::string getTypeString() const;
    std::string getStateString() const;
    std::string getConditionString() const; // NEW
};
} // namespace DTO
} // namespace Asset
} // namespace ERP
#endif // MODULES_ASSET_DTO_ASSET_H