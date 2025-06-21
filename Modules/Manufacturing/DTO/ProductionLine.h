// Modules/Manufacturing/DTO/ProductionLine.h
#ifndef MODULES_MANUFACTURING_DTO_PRODUCTIONLINE_H
#define MODULES_MANUFACTURING_DTO_PRODUCTIONLINE_H
#include <string>
#include <optional>
#include <chrono>
#include <vector> // For associated Asset IDs
#include <map>    // For std::map<std::string, std::any> replacement of QJsonObject
#include <any>    // For std::any replacement of QJsonObject
// #include <QJsonObject> // Removed
// #include <QString>     // Removed
#include "DataObjects/BaseDTO.h"
#include "Modules/Common/Common.h" // For EntityStatus
#include "Modules/Utils/Utils.h"   // For generateUUID

namespace ERP {
namespace Manufacturing {
namespace DTO {
/**
 * @brief Enum định nghĩa trạng thái của dây chuyền sản xuất.
 */
enum class ProductionLineStatus {
    OPERATIONAL = 0, /**< Đang hoạt động */
    MAINTENANCE = 1, /**< Đang bảo trì */
    IDLE = 2,        /**< Rảnh rỗi */
    SHUTDOWN = 3     /**< Đã dừng hoạt động */
};
/**
 * @brief DTO for Production Line entity.
 * Represents a physical or logical production line in the manufacturing plant.
 */
struct ProductionLineDTO : public BaseDTO {
    std::string lineName;       /**< Tên dây chuyền sản xuất */
    std::optional<std::string> description; /**< Mô tả dây chuyền */
    ProductionLineStatus status;/**< Trạng thái hiện tại */
    std::string locationId;     /**< ID vị trí vật lý (liên kết với Catalog/Location) */
    std::vector<std::string> associatedAssetIds; /**< IDs của các tài sản (máy móc) trên dây chuyền này */
    std::map<std::string, std::any> configuration; /**< Cấu hình dây chuyền dạng map (ví dụ: công suất tối đa, tốc độ) */
    std::map<std::string, std::any> metadata;      /**< Dữ liệu bổ sung dạng map */

    ProductionLineDTO() : BaseDTO(), lineName(""), status(ProductionLineStatus::OPERATIONAL), locationId("") {}
    virtual ~ProductionLineDTO() = default;

    // Utility methods (can be moved to StringUtils or helper for DTOs)
    std::string getStatusString() const {
        switch (status) {
            case ProductionLineStatus::OPERATIONAL: return "Operational";
            case ProductionLineStatus::MAINTENANCE: return "Maintenance";
            case ProductionLineStatus::IDLE: return "Idle";
            case ProductionLineStatus::SHUTDOWN: return "Shutdown";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_DTO_PRODUCTIONLINE_H