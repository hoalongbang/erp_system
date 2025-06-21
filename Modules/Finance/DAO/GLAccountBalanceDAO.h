// Modules/Finance/DAO/GLAccountBalanceDAO.h
#ifndef MODULES_FINANCE_DAO_GLACCOUNTBALANCEDAO_H
#define MODULES_FINANCE_DAO_GLACCOUNTBALANCEDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"        // Base DAO template
#include "GLAccountBalance.h" // GLAccountBalance DTO

namespace ERP {
    namespace Finance {
        namespace DAOs {
            /**
             * @brief DAO class for GLAccountBalance entity.
             * Handles database operations for GLAccountBalanceDTO.
             */
            class GLAccountBalanceDAO : public ERP::DAOBase::DAOBase<ERP::Finance::DTO::GLAccountBalanceDTO> {
            public:
                GLAccountBalanceDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~GLAccountBalanceDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Finance::DTO::GLAccountBalanceDTO& balance) override;
                std::optional<ERP::Finance::DTO::GLAccountBalanceDTO> findById(const std::string& id) override;
                bool update(const ERP::Finance::DTO::GLAccountBalanceDTO& balance) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Finance::DTO::GLAccountBalanceDTO> findAll() override;

                // Specific methods for GLAccountBalance
                std::optional<ERP::Finance::DTO::GLAccountBalanceDTO> getGLAccountBalanceByAccountId(const std::string& glAccountId);
                std::vector<ERP::Finance::DTO::GLAccountBalanceDTO> getGLAccountBalances(const std::map<std::string, std::any>& filters);
                int countGLAccountBalances(const std::map<std::string, std::any>& filters);

                // Note: createGLAccountBalance and updateGLAccountBalance are now part of this class too
                bool createGLAccountBalance(const ERP::Finance::DTO::GLAccountBalanceDTO& balance);
                bool updateGLAccountBalance(const ERP::Finance::DTO::GLAccountBalanceDTO& balance);


            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Finance::DTO::GLAccountBalanceDTO& balance) const override;
                ERP::Finance::DTO::GLAccountBalanceDTO fromMap(const std::map<std::string, std::any>& data) const override;

            private:
                std::string tableName_ = "gl_account_balances";
            };
        } // namespace DAOs
    } // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_DAO_GLACCOUNTBALANCEDAO_H