// Modules/Finance/DAO/GLAccountBalanceDAO.cpp
#include "GLAccountBalanceDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions (for toMap/fromMap BaseDTO)

namespace ERP {
    namespace Finance {
        namespace DAOs {

            GLAccountBalanceDAO::GLAccountBalanceDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Finance::DTO::GLAccountBalanceDTO>(connectionPool, "gl_account_balances") {
                // DAOBase constructor handles connectionPool and tableName_ initialization
                ERP::Logger::Logger::getInstance().info("GLAccountBalanceDAO: Initialized.");
            }

            std::map<std::string, std::any> GLAccountBalanceDAO::toMap(const ERP::Finance::DTO::GLAccountBalanceDTO& balance) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(balance); // BaseDTO fields

                data["gl_account_id"] = balance.glAccountId;
                data["current_debit_balance"] = balance.currentDebitBalance;
                data["current_credit_balance"] = balance.currentCreditBalance;
                data["currency"] = balance.currency;
                data["last_posted_date"] = ERP::Utils::DateUtils::formatDateTime(balance.lastPostedDate, ERP::Common::DATETIME_FORMAT);

                return data;
            }

            ERP::Finance::DTO::GLAccountBalanceDTO GLAccountBalanceDAO::fromMap(const std::map<std::string, std::any>& data) const {
                ERP::Finance::DTO::GLAccountBalanceDTO balance;
                ERP::Utils::DTOUtils::fromMap(data, balance); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "gl_account_id", balance.glAccountId);
                    ERP::DAOHelpers::getPlainValue(data, "current_debit_balance", balance.currentDebitBalance);
                    ERP::DAOHelpers::getPlainValue(data, "current_credit_balance", balance.currentCreditBalance);
                    ERP::DAOHelpers::getPlainValue(data, "currency", balance.currency);
                    ERP::DAOHelpers::getPlainTimeValue(data, "last_posted_date", balance.lastPostedDate);
                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("GLAccountBalanceDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "GLAccountBalanceDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("GLAccountBalanceDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "GLAccountBalanceDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return balance;
            }

            bool GLAccountBalanceDAO::save(const ERP::Finance::DTO::GLAccountBalanceDTO& balance) {
                // This will use the DAOBase's create method
                return create(balance);
            }

            std::optional<ERP::Finance::DTO::GLAccountBalanceDTO> GLAccountBalanceDAO::findById(const std::string& id) {
                // This will use the DAOBase's getById method
                return getById(id);
            }

            bool GLAccountBalanceDAO::update(const ERP::Finance::DTO::GLAccountBalanceDTO& balance) {
                // This will use the DAOBase's update method
                return DAOBase<ERP::Finance::DTO::GLAccountBalanceDTO>::update(balance);
            }

            bool GLAccountBalanceDAO::remove(const std::string& id) {
                // This will use the DAOBase's remove method
                return DAOBase<ERP::Finance::DTO::GLAccountBalanceDTO>::remove(id);
            }

            std::vector<ERP::Finance::DTO::GLAccountBalanceDTO> GLAccountBalanceDAO::findAll() {
                // This will use the DAOBase's findAll method
                return DAOBase<ERP::Finance::DTO::GLAccountBalanceDTO>::findAll();
            }

            std::optional<ERP::Finance::DTO::GLAccountBalanceDTO> GLAccountBalanceDAO::getGLAccountBalanceByAccountId(const std::string& glAccountId) {
                std::map<std::string, std::any> filters;
                filters["gl_account_id"] = glAccountId;
                std::vector<ERP::Finance::DTO::GLAccountBalanceDTO> results = getGLAccountBalances(filters);
                if (!results.empty()) {
                    return results[0];
                }
                return std::nullopt;
            }

            std::vector<ERP::Finance::DTO::GLAccountBalanceDTO> GLAccountBalanceDAO::getGLAccountBalances(const std::map<std::string, std::any>& filters) {
                // Use the templated get method from DAOBase
                return get(filters);
            }

            int GLAccountBalanceDAO::countGLAccountBalances(const std::map<std::string, std::any>& filters) {
                // Use the templated count method from DAOBase
                return count(filters);
            }

            bool GLAccountBalanceDAO::createGLAccountBalance(const ERP::Finance::DTO::GLAccountBalanceDTO& balance) {
                // Calls the templated save method from DAOBase, which then calls create.
                return save(balance);
            }

            bool GLAccountBalanceDAO::updateGLAccountBalance(const ERP::Finance::DTO::GLAccountBalanceDTO& balance) {
                // Calls the templated update method from DAOBase.
                return update(balance);
            }

        } // namespace DAOs
    } // namespace Finance
} // namespace ERP