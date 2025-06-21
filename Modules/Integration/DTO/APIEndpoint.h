// Modules/Integration/DTO/APIEndpoint.h
#ifndef MODULES_INTEGRATION_DTO_APIENDPOINT_H
#define MODULES_INTEGRATION_DTO_APIENDPOINT_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point
#include <map>          // For metadata_json

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên lớp cơ sở

namespace ERP {
namespace Integration {
namespace DTO {

/**
 * @brief Enum defining common HTTP methods for API endpoints.
 */
enum class HTTPMethod {
    GET = 0,    /**< GET method for retrieving data. */
    POST = 1,   /**< POST method for creating new data. */
    PUT = 2,    /**< PUT method for updating existing data (full replacement). */
    DELETE = 3, /**< DELETE method for removing data. */
    PATCH = 4,  /**< PATCH method for partial updates. */
    UNKNOWN = 99/**< Unknown or unsupported method. */
};

/**
 * @brief DTO for API Endpoint entity.
 * Represents a specific API endpoint for integration with an external system.
 */
struct APIEndpointDTO : public BaseDTO {
    std::string integrationConfigId;    /**< ID của cấu hình tích hợp mà endpoint này thuộc về. */
    std::string endpointCode;           /**< Mã định danh duy nhất cho endpoint (ví dụ: "CREATE_SALES_ORDER", "GET_INVENTORY"). */
    HTTPMethod method;                  /**< Phương thức HTTP của endpoint (GET, POST, PUT, DELETE). */
    std::string url;                    /**< URL đầy đủ hoặc tương đối của endpoint. */
    std::optional<std::string> description; /**< Mô tả endpoint (tùy chọn). */
    std::optional<std::string> requestSchema; /**< JSON schema cho request body (tùy chọn). */
    std::optional<std::string> responseSchema;/**< JSON schema cho response body (tùy chọn). */
    std::map<std::string, std::any> metadata; /**< Metadata bổ sung (ví dụ: yêu cầu xác thực, tiêu đề tùy chỉnh). */

    // Default constructor
    APIEndpointDTO() = default;

    // Virtual destructor for proper polymorphic cleanup
    virtual ~APIEndpointDTO() = default;

    /**
     * @brief Converts an HTTPMethod enum value to its string representation.
     * @return The string representation of the HTTP method.
     */
    std::string getMethodString() const {
        switch (method) {
            case HTTPMethod::GET: return "GET";
            case HTTPMethod::POST: return "POST";
            case HTTPMethod::PUT: return "PUT";
            case HTTPMethod::DELETE: return "DELETE";
            case HTTPMethod::PATCH: return "PATCH";
            case HTTPMethod::UNKNOWN: return "UNKNOWN";
            default: return "UNKNOWN";
        }
    }
};
} // namespace DTO
} // namespace Integration
} // namespace ERP
#endif // MODULES_INTEGRATION_DTO_APIENDPOINT_H