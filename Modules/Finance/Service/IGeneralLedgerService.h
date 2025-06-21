// Modules/Finance/Service/IGeneralLedgerService.h
#ifndef MODULES_FINANCE_SERVICE_IGENERALLEDGERSERVICE_H
#define MODULES_FINANCE_SERVICE_IGENERALLEDGERSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "GeneralLedgerAccount.h" // DTO
#include "GLAccountBalance.h"     // DTO
#include "JournalEntry.h"         // DTO
#include "JournalEntryDetail.h"   // DTO
#include "Common.h"               // Enum Common
#include "BaseService.h"          // Base Service

namespace ERP {
namespace Finance {
namespace Services {

/**
 * @brief IGeneralLedgerService interface defines operations for managing the general ledger.
 */
class IGeneralLedgerService {
public:
    virtual ~IGeneralLedgerService() = default;
    /**
     * @brief Creates a new general ledger account.
     * @param glAccountDTO DTO containing new GL account information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional GeneralLedgerAccountDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> createGLAccount(
        const ERP::Finance::DTO::GeneralLedgerAccountDTO& glAccountDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves GL account information by ID.
     * @param glAccountId ID of the GL account to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional GeneralLedgerAccountDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> getGLAccountById(
        const std::string& glAccountId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves GL account information by account number.
     * @param accountNumber Account number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional GeneralLedgerAccountDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> getGLAccountByNumber(
        const std::string& accountNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all GL accounts or accounts matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching GeneralLedgerAccountDTOs.
     */
    virtual std::vector<ERP::Finance::DTO::GeneralLedgerAccountDTO> getAllGLAccounts(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates GL account information.
     * @param glAccountDTO DTO containing updated GL account information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateGLAccount(
        const ERP::Finance::DTO::GeneralLedgerAccountDTO& glAccountDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a GL account.
     * @param glAccountId ID of the GL account.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateGLAccountStatus(
        const std::string& glAccountId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a GL account record by ID (soft delete).
     * @param glAccountId ID of the GL account to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteGLAccount(
        const std::string& glAccountId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Creates a new journal entry.
     * @param journalEntryDTO DTO containing new journal entry information.
     * @param journalEntryDetails Vector of JournalEntryDetailDTOs for the entry.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional JournalEntryDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Finance::DTO::JournalEntryDTO> createJournalEntry(
        const ERP::Finance::DTO::JournalEntryDTO& journalEntryDTO,
        const std::vector<ERP::Finance::DTO::JournalEntryDetailDTO>& journalEntryDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Posts a journal entry to the general ledger, updating GL account balances.
     * @param journalEntryId ID of the journal entry to post.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if posting is successful, false otherwise.
     */
    virtual bool postJournalEntry(
        const std::string& journalEntryId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves all journal entries.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching JournalEntryDTOs.
     */
    virtual std::vector<ERP::Finance::DTO::JournalEntryDTO> getAllJournalEntries(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all journal entry details for a specific journal entry.
     * @param journalEntryId ID of the journal entry.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching JournalEntryDetailDTOs.
     */
    virtual std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> getJournalEntryDetails(
        const std::string& journalEntryId,
        const std::vector<std::string>& userRoleIds = {}) = 0;

    // NEW: Financial Report Generation Methods
    /**
     * @brief Generates a Trial Balance report.
     * @param startDate The start date for the report period.
     * @param endDate The end date for the report period.
     * @param userRoleIds Roles of the user performing the operation.
     * @return A map where keys are account names/numbers and values are their net balances.
     */
    virtual std::map<std::string, double> generateTrialBalance(
        const std::chrono::system_clock::time_point& startDate,
        const std::chrono::system_clock::time_point& endDate,
        const std::vector<std::string>& userRoleIds) = 0;

    /**
     * @brief Generates a Balance Sheet report as of a specific date.
     * @param asOfDate The date for which the balance sheet is generated.
     * @param userRoleIds Roles of the user performing the operation.
     * @return A map where keys are balance sheet items (e.g., "Cash", "Accounts Receivable") and values are their amounts.
     */
    virtual std::map<std::string, double> generateBalanceSheet(
        const std::chrono::system_clock::time_point& asOfDate,
        const std::vector<std::string>& userRoleIds) = 0;

    /**
     * @brief Generates an Income Statement report for a specific period.
     * @param startDate The start date for the report period.
     * @param endDate The end date for the report period.
     * @param userRoleIds Roles of the user performing the operation.
     * @return A map where keys are income statement items (e.g., "Revenue", "Cost of Goods Sold") and values are their amounts.
     */
    virtual std::map<std::string, double> generateIncomeStatement(
        const std::chrono::system_clock::time_point& startDate,
        const std::chrono::system_clock::time_point& endDate,
        const std::vector<std::string>& userRoleIds) = 0;

    /**
     * @brief Generates a Cash Flow Statement report for a specific period.
     * @param startDate The start date for the report period.
     * @param endDate The end date for the report period.
     * @param userRoleIds Roles of the user performing the operation.
     * @return A map where keys are cash flow items (e.g., "Cash from Operating Activities") and values are their amounts.
     */
    virtual std::map<std::string, double> generateCashFlowStatement(
        const std::chrono::system_clock::time_point& startDate,
        const std::chrono::system_clock::time_point& endDate,
        const std::vector<std::string>& userRoleIds) = 0;
};

} // namespace Services
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_SERVICE_IGENERALLEDGERSERVICE_H