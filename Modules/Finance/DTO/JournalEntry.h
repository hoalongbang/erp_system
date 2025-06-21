// Modules/Finance/DTO/JournalEntry.h
#ifndef MODULES_FINANCE_DTO_JOURNALENTRY_H
#define MODULES_FINANCE_DTO_JOURNALENTRY_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point
#include <vector>       // For nested DTOs (JournalEntryDetailDTO) if used in single DTO structure

// Rút gọn include paths
#include "BaseDTO.h"    // Base DTO
#include "Common.h"     // Common enums (like EntityStatus)

using ERP::DataObjects::BaseDTO; // ✅ Rút gọn tên lớp cơ sở

namespace ERP {
namespace Finance {
namespace DTO {
/**
 * @brief DTO for Journal Entry entity.
 * Represents a single journal entry, which is a record of a financial transaction.
 * A journal entry typically has multiple debit and credit lines (details).
 */
struct JournalEntryDTO : public BaseDTO {
    std::string journalNumber;                  /**< Số bút toán duy nhất. */
    std::string description;                    /**< Mô tả bút toán. */
    std::chrono::system_clock::time_point entryDate; /**< Ngày bút toán được ghi nhận. */
    std::optional<std::chrono::system_clock::time_point> postingDate; /**< Ngày bút toán được hạch toán vào sổ cái (tùy chọn). */
    std::optional<std::string> reference;       /**< Số tham chiếu (ví dụ: số hóa đơn, số phiếu thanh toán) (tùy chọn). */
    double totalDebit = 0.0;                    /**< Tổng số tiền Nợ của bút toán. */
    double totalCredit = 0.0;                   /**< Tổng số tiền Có của bút toán. */
    std::optional<std::string> postedByUserId;  /**< ID người dùng đã hạch toán bút toán (tùy chọn). */
    bool isPosted = false;                      /**< Cờ chỉ báo bút toán đã được hạch toán vào sổ cái chưa. */

    // Note: JournalEntryDetails are typically managed via a separate collection/DAO,
    // not directly embedded in the main DTO for persistence, but loaded when needed.
    // std::vector<JournalEntryDetailDTO> details; // Example if loaded for runtime use

    // Default constructor to initialize BaseDTO and specific fields
    JournalEntryDTO() : BaseDTO() {}

    // Virtual destructor for proper polymorphic cleanup
    virtual ~JournalEntryDTO() = default;
};
} // namespace DTO
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_DTO_JOURNALENTRY_H