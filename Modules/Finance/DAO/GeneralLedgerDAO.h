// Modules/Finance/DAO/GeneralLedgerDAO.h
#ifndef MODULES_FINANCE_DAO_GENERALLEDGERDAO_H
#define MODULES_FINANCE_DAO_GENERALLEDGERDAO_H
#include "DAOBase/DAOBase.h" // Include the templated DAOBase
#include "Modules/Finance/DTO/GeneralLedgerAccount.h" // For DTOs
#include "Modules/Finance/DTO/GLAccountBalance.h" // For DTOs
#include "Modules/Finance/DTO/JournalEntry.h" // For DTOs
#include "Modules/Finance/DTO/JournalEntryDetail.h" // For DTOs
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "Modules/Utils/DTOUtils.h" // For DTO conversion helpers
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <any>
#include <optional>

namespace ERP {
namespace Finance {
namespace DAOs {

// GeneralLedgerDAO will handle multiple DTOs.
// Similar to AccountReceivableDAO, it will inherit for GLAccountBalance,
// and have specific methods for JournalEntry and JournalEntryDetail.
// Optimal would be separate DAOs, but adhering to original file structure.

class GeneralLedgerDAO : public ERP::DAOBase::DAOBase<ERP::Finance::DTO::GLAccountBalanceDTO> {
public:
    explicit GeneralLedgerDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~GeneralLedgerDAO() override = default;

    // Override toMap and fromMap for GLAccountBalanceDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Finance::DTO::GLAccountBalanceDTO& dto) const override;
    ERP::Finance::DTO::GLAccountBalanceDTO fromMap(const std::map<std::string, std::any>& data) const override;

public:
    // Specific methods for GeneralLedgerAccountDTO
    bool createGLAccount(const ERP::Finance::DTO::GeneralLedgerAccountDTO& account);
    std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> getGLAccountById(const std::string& id);
    std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> getGLAccountByNumber(const std::string& accountNumber);
    std::vector<ERP::Finance::DTO::GeneralLedgerAccountDTO> getGLAccounts(const std::map<std::string, std::any>& filter = {});
    bool updateGLAccount(const ERP::Finance::DTO::GeneralLedgerAccountDTO& account);
    bool removeGLAccount(const std::string& id);
    int countGLAccounts(const std::map<std::string, std::any>& filter = {});


    // Specific methods for JournalEntryDTO
    bool createJournalEntry(const ERP::Finance::DTO::JournalEntryDTO& entry);
    std::optional<ERP::Finance::DTO::JournalEntryDTO> getJournalEntryById(const std::string& id);
    std::vector<ERP::Finance::DTO::JournalEntryDTO> getJournalEntries(const std::map<std::string, std::any>& filter = {});
    bool updateJournalEntry(const ERP::Finance::DTO::JournalEntryDTO& entry);
    bool removeJournalEntry(const std::string& id);

    // Specific methods for JournalEntryDetailDTO
    bool createJournalEntryDetail(const ERP::Finance::DTO::JournalEntryDetailDTO& detail);
    std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> getJournalEntryDetailsByEntryId(const std::string& journalEntryId);
    bool updateJournalEntryDetail(const ERP::Finance::DTO::JournalEntryDetailDTO& detail);
    bool removeJournalEntryDetail(const std::string& id);

    // Helpers for GeneralLedgerAccountDTO conversion
    static std::map<std::string, std::any> toMap(const ERP::Finance::DTO::GeneralLedgerAccountDTO& dto);
    static ERP::Finance::DTO::GeneralLedgerAccountDTO fromMap(const std::map<std::string, std::any>& data);

    // Helpers for JournalEntryDTO conversion
    static std::map<std::string, std::any> toMap(const ERP::Finance::DTO::JournalEntryDTO& dto);
    static ERP::Finance::DTO::JournalEntryDTO fromMap(const std::map<std::string, std::any>& data);

    // Helpers for JournalEntryDetailDTO conversion
    static std::map<std::string, std::any> toMap(const ERP::Finance::DTO::JournalEntryDetailDTO& dto);
    static ERP::Finance::DTO::JournalEntryDetailDTO fromMap(const std::map<std::string, std::any>& data);

private:
    std::string glAccountsTableName_ = "general_ledger_accounts";
    std::string glBalancesTableName_ = "gl_account_balances";
    std::string journalEntriesTableName_ = "journal_entries";
    std::string journalEntryDetailsTableName_ = "journal_entry_details";
};

} // namespace DAOs
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_DAO_GENERALLEDGERDAO_H