// Modules/Finance/Service/GeneralLedgerService.h
#ifndef MODULES_FINANCE_SERVICE_GENERALLEDGERSERVICE_H
#define MODULES_FINANCE_SERVICE_GENERALLEDGERSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"        // NEW: Kế thừa từ BaseService
#include "GeneralLedgerAccount.h" // Đã rút gọn include
#include "GLAccountBalance.h" // Đã rút gọn include
#include "JournalEntry.h" // Đã rút gọn include
#include "JournalEntryDetail.h" // Đã rút gọn include
#include "GeneralLedgerDAO.h"   // Đã rút gọn include
#include "ISecurityManager.h"   // Đã rút gọn include
#include "EventBus.h"           // Đã rút gọn include
#include "Logger.h"             // Đã rút gọn include
#include "ErrorHandler.h"       // Đã rút gọn include
#include "Common.h"             // Đã rút gọn include
#include "Utils.h"              // Đã rút gọn include
#include "DateUtils.h"          // Đã rút gọn include

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

/**
 * @brief Default implementation of IGeneralLedgerService.
 * This class uses GeneralLedgerDAO and ISecurityManager.
 */
class GeneralLedgerService : public IGeneralLedgerService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for GeneralLedgerService.
     * @param glDAO Shared pointer to GeneralLedgerDAO.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    GeneralLedgerService(std::shared_ptr<DAOs::GeneralLedgerDAO> glDAO,
                         std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                         std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                         std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                         std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> createGLAccount(
        const ERP::Finance::DTO::GeneralLedgerAccountDTO& glAccountDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> getGLAccountById(
        const std::string& glAccountId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> getGLAccountByNumber(
        const std::string& accountNumber,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Finance::DTO::GeneralLedgerAccountDTO> getAllGLAccounts(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateGLAccount(
        const ERP::Finance::DTO::GeneralLedgerAccountDTO& glAccountDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateGLAccountStatus(
        const std::string& glAccountId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteGLAccount(
        const std::string& glAccountId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Finance::DTO::JournalEntryDTO> createJournalEntry(
        const ERP::Finance::DTO::JournalEntryDTO& journalEntryDTO,
        const std::vector<ERP::Finance::DTO::JournalEntryDetailDTO>& journalEntryDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool postJournalEntry(
        const std::string& journalEntryId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::vector<ERP::Finance::DTO::JournalEntryDTO> getAllJournalEntries(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> getJournalEntryDetails(
        const std::string& journalEntryId,
        const std::vector<std::string>& userRoleIds = {}) override;

    // NEW: Financial Report Generation Implementations
    std::map<std::string, double> generateTrialBalance(
        const std::chrono::system_clock::time_point& startDate,
        const std::chrono::system_clock::time_point& endDate,
        const std::vector<std::string>& userRoleIds) override;
    std::map<std::string, double> generateBalanceSheet(
        const std::chrono::system_clock::time_point& asOfDate,
        const std::vector<std::string>& userRoleIds) override;
    std::map<std::string, double> generateIncomeStatement(
        const std::chrono::system_clock::time_point& startDate,
        const std::chrono::system_clock::time_point& endDate,
        const std::vector<std::string>& userRoleIds) override;
    std::map<std::string, double> generateCashFlowStatement(
        const std::chrono::system_clock::time_point& startDate,
        const std::chrono::system_clock::time_point& endDate,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::GeneralLedgerDAO> glDAO_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // Helper functions
    bool updateGLAccountBalance(const std::string& glAccountId, double debitAmount, double creditAmount, ERP::Database::DBConnection& db_conn);
    std::map<std::string, double> calculateAccountBalances(
        const std::chrono::system_clock::time_point& startDate,
        const std::chrono::system_clock::time_point& endDate,
        bool includeOpeningBalances, // If true, sums all transactions up to end date from beginning
        const std::vector<std::string>& userRoleIds
    );

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_SERVICE_GENERALLEDGERSERVICE_H