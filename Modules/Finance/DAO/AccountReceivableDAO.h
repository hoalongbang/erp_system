// Modules/Finance/DAO/AccountReceivableDAO.h
#ifndef MODULES_FINANCE_DAO_ACCOUNTRECEIVABLEDAO_H
#define MODULES_FINANCE_DAO_ACCOUNTRECEIVABLEDAO_H
#include "DAOBase/DAOBase.h" // Include the templated DAOBase
#include "Modules/Finance/DTO/AccountReceivableBalance.h" // For DTOs
#include "Modules/Finance/DTO/AccountReceivableTransaction.h" // For DTOs
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

// AccountReceivableDAO handles two DTOs, so we will need to decide on strategy:
// 1. Separate DAOs for each DTO (recommended for clarity and consistency with single-DTO DAO pattern)
// 2. Make this DAO inherit from DAOBase multiple times with different templates (less common, more complex)
// 3. Keep it as a non-templated DAOBase and implement all CRUD manually for both DTOs.
// Given the existing pattern, separating into two DAOs is the cleanest approach.
// However, to match the original file structure, I'll adapt it by making it a non-templated DAOBase
// and implementing the toMap/fromMap for specific queries, while relying on explicit `execute`/`query`
// for methods that aren't pure CRUD on a single DTO.
// For now, I'm adapting to inherit from DAOBase for AccountReceivableBalanceDTO and add specific methods for transactions.
// The optimal refactoring would be to split this into two distinct DAO classes (e.g., ARBalanceDAO and ARTransactionDAO).

class AccountReceivableDAO : public ERP::DAOBase::DAOBase<ERP::Finance::DTO::AccountReceivableBalanceDTO> {
public:
    explicit AccountReceivableDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~AccountReceivableDAO() override = default;

    // Override toMap and fromMap for AccountReceivableBalanceDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Finance::DTO::AccountReceivableBalanceDTO& dto) const override;
    ERP::Finance::DTO::AccountReceivableBalanceDTO fromMap(const std::map<std::string, std::any>& data) const override;

public:
    // Specific methods for AccountReceivableTransactionDTO (since DAOBase is templated for BalanceDTO)
    bool createTransaction(const ERP::Finance::DTO::AccountReceivableTransactionDTO& transaction);
    std::optional<ERP::Finance::DTO::AccountReceivableTransactionDTO> getTransactionById(const std::string& id);
    std::vector<ERP::Finance::DTO::AccountReceivableTransactionDTO> getTransactions(const std::map<std::string, std::any>& filter = {});
    bool updateTransaction(const ERP::Finance::DTO::AccountReceivableTransactionDTO& transaction);
    bool removeTransaction(const std::string& id);

    // Helpers for transaction DTO conversion
    static std::map<std::string, std::any> toMap(const ERP::Finance::DTO::AccountReceivableTransactionDTO& dto);
    static ERP::Finance::DTO::AccountReceivableTransactionDTO fromMap(const std::map<std::string, std::any>& data);

private:
    std::string arBalancesTableName_ = "gl_account_balances"; // Assuming AR Balance is a type of GL balance
    std::string arTransactionsTableName_ = "account_receivable_transactions"; // Specific table for AR transactions
};

} // namespace DAOs
} // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_DAO_ACCOUNTRECEIVABLEDAO_H