// Modules/Finance/DAO/AccountReceivableTransactionDAO.cpp
#include "AccountReceivableTransactionDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For BaseDTO conversions

namespace ERP {
namespace Finance {
namespace DAOs {

AccountReceivableTransactionDAO::AccountReceivableTransactionDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Finance::DTO::AccountReceivableTransactionDTO>(connectionPool, "accounts_receivable_transactions") {
    // DAOBase constructor handles connectionPool and tableName_ initialization
    ERP::Logger::Logger::getInstance().info("AccountReceivableTransactionDAO: Initialized.");
}

std::map<std::string, std::any> AccountReceivableTransactionDAO::toMap(const ERP::Finance::DTO::AccountReceivableTransactionDTO& transaction) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(transaction); // BaseDTO fields

    data["customer_id"] = transaction.customerId;
    data["type"] = static_cast<int>(transaction.type);
    data["amount"] = transaction.amount;
    data["currency"] = transaction.currency;
    data["transaction_date"] = ERP::Utils::DateUtils::formatDateTime(transaction.transactionDate, ERP::Common::DATETIME_FORMAT);
    ERP::DAOHelpers::putOptionalString(data, "reference_document_id", transaction.referenceDocumentId);
    ERP::DAOHelpers::putOptionalString(data, "reference_document_type", transaction.referenceDocumentType);
    ERP::DAOHelpers::putOptionalString(data, "notes", transaction.notes);

    return data;
}

ERP::Finance::DTO::AccountReceivableTransactionDTO AccountReceivableTransactionDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Finance::DTO::AccountReceivableTransactionDTO transaction;
    ERP::Utils::DTOUtils::fromMap(data, transaction); // BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "customer_id", transaction.customerId);
        
        int typeInt;
        ERP::DAOHelpers::getPlainValue(data, "type", typeInt);
        transaction.type = static_cast<ERP::Finance::DTO::ARTransactionType>(typeInt);

        ERP::DAOHelpers::getPlainValue(data, "amount", transaction.amount);
        ERP::DAOHelpers::getPlainValue(data, "currency", transaction.currency);
        ERP::DAOHelpers::getPlainTimeValue(data, "transaction_date", transaction.transactionDate);
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_id", transaction.referenceDocumentId);
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_type", transaction.referenceDocumentType);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", transaction.notes);

    } catch (const std::bad_any_cast& e) {
        ERP::Logger::Logger::getInstance().error("AccountReceivableTransactionDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "AccountReceivableTransactionDAO: Data type mismatch in fromMap: " + std::string(e.what()));
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("AccountReceivableTransactionDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "AccountReceivableTransactionDAO: Unexpected error in fromMap: " + std::string(e.what()));
    }
    return transaction;
}

bool AccountReceivableTransactionDAO::save(const ERP::Finance::DTO::AccountReceivableTransactionDTO& transaction) {
    return create(transaction);
}

std::optional<ERP::Finance::DTO::AccountReceivableTransactionDTO> AccountReceivableTransactionDAO::findById(const std::string& id) {
    return getById(id);
}

bool AccountReceivableTransactionDAO::update(const ERP::Finance::DTO::AccountReceivableTransactionDTO& transaction) {
    return DAOBase<ERP::Finance::DTO::AccountReceivableTransactionDTO>::update(transaction);
}

bool AccountReceivableTransactionDAO::remove(const std::string& id) {
    return DAOBase<ERP::Finance::DTO::AccountReceivableTransactionDTO>::remove(id);
}

std::vector<ERP::Finance::DTO::AccountReceivableTransactionDTO> AccountReceivableTransactionDAO::findAll() {
    return DAOBase<ERP::Finance::DTO::AccountReceivableTransactionDTO>::findAll();
}

std::vector<ERP::Finance::DTO::AccountReceivableTransactionDTO> AccountReceivableTransactionDAO::getAccountReceivableTransactions(const std::map<std::string, std::any>& filters) {
    return get(filters); // Use templated get
}

int AccountReceivableTransactionDAO::countAccountReceivableTransactions(const std::map<std::string, std::any>& filters) {
    return count(filters); // Use templated count
}

} // namespace DAOs
} // namespace Finance
} // namespace ERP