// Modules/Finance/DAO/AccountReceivableTransactionDAO.h
#ifndef MODULES_FINANCE_DAO_ACCOUNTRECEIVABLETRANSACTIONDAO_H
#define MODULES_FINANCE_DAO_ACCOUNTRECEIVABLETRANSACTIONDAO_H

#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"                 // Base DAO template
#include "AccountReceivableTransaction.h" // AccountReceivableTransaction DTO

namespace ERP {
namespace Finance {
namespace DAOs {

/**
 * @brief AccountReceivableTransactionDAO class provides data access operations for AccountReceivableTransactionDTO objects.
 * It inherits from DAOBase and interacts with the database to manage AR transactions.
 */
class AccountReceivableTransactionDAO : public ERP::DAOBase::DAOBase<ERP::Finance::DTO::AccountReceivableTransactionDTO> {
public:
    AccountReceivableTransactionDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~AccountReceivableTransactionDAO() override = default;

    // Override base methods (optional, but good practice if custom logic is needed)
    bool save(const ERP::Finance::DTO::AccountReceivableTransactionDTO& transaction) override;
    std::optional<ERP::Finance::DTO::AccountReceivableTransactionDTO> findById(const std::string& id) override;
    bool update(const ERP::Finance::DTO::AccountReceivableTransactionDTO& transaction) override;
    bool remove(const std::string& id) override;
    std::vector<ERP::Finance::DTO::AccountReceivableTransactionDTO> findAll() override;

    // Specific methods for AccountReceivableTransaction
    std::vector<ERP::Finance::DTO::AccountReceivableTransactionDTO> getAccountReceivableTransactions(const std::map<std::string, std::any>& filters);
    int countAccountReceivableTransactions(const std::map<std::string, std::any>& filters);

protected:
    // Required overrides for mapping between DTO and std::map<string, any>
    std::map<std::string, std::any> toMap(const ERP::Finance::DTO::AccountReceivableTransactionDTO& transaction) const override;
    ERP::Finance::DTO::AccountReceivableTransactionDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    const std::string tableName_ = "accounts_receivable_transactions";
};

} // namespace DAOs
} // namespace Finance
} // namespace ERP

#endif // MODULES_FINANCE_DAO_ACCOUNTRECEIVABLETRANSACTIONDAO_H