// Modules/Finance/DTO/JournalEntryDetail.h
#ifndef MODULES_FINANCE_DTO_JOURNALENTRYDETAIL_H
#define MODULES_FINANCE_DTO_JOURNALENTRYDETAIL_H
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
 * @brief DTO for Journal Entry Detail entity.
 * Represents a single line item (debit or credit) within a Journal Entry.
 */
struct JournalEntryDetailDTO : public BaseDTO {
    std::string journalEntryId;         /**< ID của bút toán nhật ký cha. */
    std::string glAccountId;            /**< ID của tài khoản GL bị ảnh hưởng bởi chi tiết này. */
    double debitAmount = 0.0;           /**< Số tiền Nợ của chi tiết. */
    double creditAmount = 0.0;          /**< Số tiền Có của chi tiết. */
    std::optional<std::string> notes;   /**< Ghi chú cho chi tiết bút toán (tùy chọn). */

    // Default constructor to initialize BaseDTO and specific fields
    JournalEntryDetailDTO() : BaseDTO() {}

    // Virtual destructor for proper polymorphic cleanup
    virtual ~JournalEntryDetailDTO() = default;
};
} // namespace DTO
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_DTO_JOURNALENTRYDETAIL_H