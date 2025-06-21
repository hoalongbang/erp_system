// Modules/Finance/DTO/TaxRate.h
#ifndef MODULES_FINANCE_DTO_TAXRATE_H
#define MODULES_FINANCE_DTO_TAXRATE_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên lớp cơ sở

namespace ERP {
namespace Finance {
namespace DTO {
/**
 * @brief DTO for Tax Rate entity.
 * Represents a tax rate applied to transactions (e.g., VAT, sales tax).
 */
struct TaxRateDTO : public BaseDTO {
    std::string name;                               /**< Tên thuế suất (ví dụ: "VAT 10%"). */
    double rate;                                    /**< Tỷ lệ thuế (ví dụ: 0.10 cho 10%). */
    std::optional<std::string> description;         /**< Mô tả thuế suất (tùy chọn). */
    std::chrono::system_clock::time_point effectiveDate; /**< Ngày bắt đầu hiệu lực của thuế suất. */
    std::optional<std::chrono::system_clock::time_point> expirationDate; /**< Ngày hết hiệu lực của thuế suất (tùy chọn). */

    // Default constructor
    TaxRateDTO() = default;

    // Virtual destructor for proper polymorphic cleanup
    virtual ~TaxRateDTO() = default;
};
} // namespace DTO
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_DTO_TAXRATE_H