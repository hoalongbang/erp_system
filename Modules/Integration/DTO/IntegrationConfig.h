// Modules/Integration/DTO/IntegrationConfig.h
#ifndef MODULES_INTEGRATION_DTO_INTEGRATIONCONFIG_H
#define MODULES_INTEGRATION_DTO_INTEGRATIONCONFIG_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point
#include <map>          // For metadata_json
#include <any>          // For std::any in map

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên lớp cơ sở

namespace ERP {
namespace Integration {
namespace DTO {

/**
 * @brief Enum defining types of external systems for integration.
 */
enum class IntegrationType {
    ERP = 0,            /**< Hệ thống hoạch định nguồn lực doanh nghiệp khác. */
    CRM = 1,            /**< Hệ thống quản lý quan hệ khách hàng. */
    WMS = 2,            /**< Hệ thống quản lý kho. */
    E_COMMERCE = 3,     /**< Nền tảng thương mại điện tử (ví dụ: Shopify, Magento). */
    PAYMENT_GATEWAY = 4,/**< Cổng thanh toán (ví dụ: Stripe, PayPal). */
    SHIPPING_CARRIER = 5, /**< Hãng vận chuyển (ví dụ: FedEx, UPS). */
    MANUFACTURING = 6,  /**< Hệ thống sản xuất (MES, SCADA). */
    OTHER = 99,         /**< Loại tích hợp khác. */
    UNKNOWN = 100       /**< Loại tích hợp không xác định. */
};

/**
 * @brief DTO for Integration Configuration entity.
 * Represents the configuration settings for integrating with an external system.
 */
struct IntegrationConfigDTO : public BaseDTO {
    std::string systemName;         /**< Tên hiển thị của hệ thống bên ngoài. */
    std::string systemCode;         /**< Mã định danh duy nhất cho hệ thống (ví dụ: "SAP_ERP", "SHOPIFY_STORE"). */
    IntegrationType type;           /**< Loại hệ thống tích hợp (ERP, CRM, WMS, v.v.). */
    std::optional<std::string> baseUrl; /**< Base URL cho API của hệ thống bên ngoài (tùy chọn). */
    std::optional<std::string> username; /**< Tên người dùng để xác thực API (tùy chọn). */
    std::optional<std::string> password; /**< Mật khẩu/token để xác thực API (tùy chọn, có thể được mã hóa). */
    bool isEncrypted = false;       /**< Cờ chỉ báo liệu thông tin xác thực có được mã hóa không. */
    std::map<std::string, std::any> metadata; /**< Metadata bổ sung (ví dụ: khóa API, chứng chỉ). */

    // Default constructor to initialize BaseDTO and specific fields
    IntegrationConfigDTO() : BaseDTO() {}

    // Virtual destructor for proper polymorphic cleanup
    virtual ~IntegrationConfigDTO() = default;

    /**
     * @brief Converts an IntegrationType enum value to its string representation.
     * @return The string representation of the integration type.
     */
    std::string getTypeString() const {
        switch (type) {
            case IntegrationType::ERP: return "ERP";
            case IntegrationType::CRM: return "CRM";
            case IntegrationType::WMS: return "WMS";
            case IntegrationType::E_COMMERCE: return "E-commerce";
            case IntegrationType::PAYMENT_GATEWAY: return "Payment Gateway";
            case IntegrationType::SHIPPING_CARRIER: return "Shipping Carrier";
            case IntegrationType::MANUFACTURING: return "Manufacturing";
            case IntegrationType::OTHER: return "Other";
            case IntegrationType::UNKNOWN: return "Unknown";
            default: return "Unknown";
        }
    }
};
} // namespace DTO
} // namespace Integration
} // namespace ERP
#endif // MODULES_INTEGRATION_DTO_INTEGRATIONCONFIG_H