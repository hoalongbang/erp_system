// Modules/Finance/DAO/AccountReceivableDAO.cpp
#include "Modules/Finance/DAO/AccountReceivableDAO.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "DAOHelpers.h"
#include "Modules/Utils/DTOUtils.h" // For common DTO to map conversions
#include <sstream>
#include <stdexcept>
#include <typeinfo> // For std::bad_any_cast

namespace ERP {
namespace Finance {
namespace DAOs {

AccountReceivableDAO::AccountReceivableDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Finance::DTO::AccountReceivableBalanceDTO>(connectionPool, "gl_account_balances") { // Pass table name for Balance DTO
    Logger::Logger::getInstance().info("AccountReceivableDAO: Initialized.");
}

// toMap for AccountReceivableBalanceDTO
std::map<std::string, std::any> AccountReceivableDAO::toMap(const ERP::Finance::DTO::AccountReceivableBalanceDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["customer_id"] = dto.customerId;
    data["current_balance"] = dto.currentBalance;
    data["currency"] = dto.currency;
    data["last_transaction_date"] = ERP::Utils::DateUtils::formatDateTime(dto.lastTransactionDate, ERP::Common::DATETIME_FORMAT);

    return data;
}

// fromMap for AccountReceivableBalanceDTO
ERP::Finance::DTO::AccountReceivableBalanceDTO AccountReceivableDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Finance::DTO::AccountReceivableBalanceDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "customer_id", dto.customerId);
        ERP::DAOHelpers::getPlainValue(data, "current_balance", dto.currentBalance);
        ERP::DAOHelpers::getPlainValue(data, "currency", dto.currency);
        ERP::DAOHelpers::getPlainTimeValue(data, "last_transaction_date", dto.lastTransactionDate);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("AccountReceivableDAO: fromMap (Balance) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "AccountReceivableDAO: Data type mismatch in fromMap (Balance).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("AccountReceivableDAO: fromMap (Balance) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "AccountReceivableDAO: Unexpected error in fromMap (Balance).");
    }
    return dto;
}

// toMap for AccountReceivableTransactionDTO (static helper for specific methods)
std::map<std::string, std::any> AccountReceivableDAO::toMap(const ERP::Finance::DTO::AccountReceivableTransactionDTO& dto) {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["customer_id"] = dto.customerId;
    data["type"] = static_cast<int>(dto.type);
    data["amount"] = dto.amount;
    data["currency"] = dto.currency;
    data["transaction_date"] = ERP::Utils::DateUtils::formatDateTime(dto.transactionDate, ERP::Common::DATETIME_FORMAT);
    ERP::DAOHelpers::putOptionalString(data, "reference_document_id", dto.referenceDocumentId);
    ERP::DAOHelpers::putOptionalString(data, "reference_document_type", dto.referenceDocumentType);
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);

    return data;
}

// fromMap for AccountReceivableTransactionDTO (static helper for specific methods)
ERP::Finance::DTO::AccountReceivableTransactionDTO AccountReceivableDAO::fromMap(const std::map<std::string, std::any>& data) {
    ERP::Finance::DTO::AccountReceivableTransactionDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "customer_id", dto.customerId);
        
        int typeInt;
        if (ERP::DAOHelpers::getPlainValue(data, "type", typeInt)) dto.type = static_cast<ERP::Finance::DTO::ARTransactionType>(typeInt);
        
        ERP::DAOHelpers::getPlainValue(data, "amount", dto.amount);
        ERP::DAOHelpers::getPlainValue(data, "currency", dto.currency);
        ERP::DAOHelpers::getPlainTimeValue(data, "transaction_date", dto.transactionDate);
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_id", dto.referenceDocumentId);
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_type", dto.referenceDocumentType);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("AccountReceivableDAO: fromMap (Transaction) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "AccountReceivableDAO: Data type mismatch in fromMap (Transaction).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("AccountReceivableDAO: fromMap (Transaction) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "AccountReceivableDAO: Unexpected error in fromMap (Transaction).");
    }
    return dto;
}

// createTransaction - uses explicit executeDbOperation and static toMap
bool AccountReceivableDAO::createTransaction(const ERP::Finance::DTO::AccountReceivableTransactionDTO& transaction) {
    ERP::Logger::Logger::getInstance().info("AccountReceivableDAO: Attempting to create new AR transaction.");
    std::map<std::string, std::any> data = toMap(transaction); // Use static toMap for transaction
    
    std::string columns;
    std::string placeholders;
    std::map<std::string, std::any> params;
    bool first = true;

    for (const auto& pair : data) {
        if (!first) { columns += ", "; placeholders += ", "; }
        columns += pair.first;
        placeholders += "?";
        params[pair.first] = pair.second;
        first = false;
    }

    std::string sql = "INSERT INTO " + arTransactionsTableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "AccountReceivableDAO", "createTransaction", sql, params
    );
}

// getTransactionById - uses explicit queryDbOperation and static fromMap
std::optional<ERP::Finance::DTO::AccountReceivableTransactionDTO> AccountReceivableDAO::getTransactionById(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("AccountReceivableDAO: Attempting to get AR transaction by ID: " + id);
    std::string sql = "SELECT * FROM " + arTransactionsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "AccountReceivableDAO", "getTransactionById", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front()); // Use static fromMap for transaction
    }
    return std::nullopt;
}

// getTransactions - uses explicit queryDbOperation and static fromMap
std::vector<ERP::Finance::DTO::AccountReceivableTransactionDTO> AccountReceivableDAO::getTransactions(const std::map<std::string, std::any>& filter) {
    ERP::Logger::Logger::getInstance().info("AccountReceivableDAO: Attempting to retrieve AR transactions.");
    std::string sql = "SELECT * FROM " + arTransactionsTableName_;
    std::string whereClause;
    std::map<std::string, std::any> params;

    if (!filter.empty()) {
        whereClause = " WHERE ";
        bool first = true;
        for (const auto& pair : filter) {
            if (!first) { whereClause += " AND "; }
            whereClause += pair.first + " = ?";
            params[pair.first] = pair.second;
            first = false;
        }
    }
    sql += whereClause + ";";

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "AccountReceivableDAO", "getTransactions", sql, params
    );

    std::vector<ERP::Finance::DTO::AccountReceivableTransactionDTO> resultsDto;
    for (const auto& rowMap : resultsMap) {
        resultsDto.push_back(fromMap(rowMap)); // Use static fromMap for transaction
    }
    return resultsDto;
}

// updateTransaction - uses explicit executeDbOperation and static toMap
bool AccountReceivableDAO::updateTransaction(const ERP::Finance::DTO::AccountReceivableTransactionDTO& transaction) {
    ERP::Logger::Logger::getInstance().info("AccountReceivableDAO: Attempting to update AR transaction with ID: " + transaction.id);
    std::map<std::string, std::any> data = toMap(transaction); // Use static toMap for transaction
    if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
        ERP::Logger::Logger::getInstance().warning("AccountReceivableDAO: Update transaction called with empty data or missing ID.");
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "AccountReceivableDAO: Update transaction called with empty data or missing ID.");
        return false;
    }

    std::string setClause;
    std::map<std::string, std::any> params;
    bool firstSet = true;

    for (const auto& pair : data) {
        if (pair.first == "id") continue;
        if (!firstSet) setClause += ", ";
        setClause += pair.first + " = ?";
        params[pair.first] = pair.second;
        firstSet = false;
    }

    std::string sql = "UPDATE " + arTransactionsTableName_ + " SET " + setClause + " WHERE id = ?;";
    params["id_filter"] = transaction.id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "AccountReceivableDAO", "updateTransaction", sql, params
    );
}

// removeTransaction - uses explicit executeDbOperation
bool AccountReceivableDAO::removeTransaction(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("AccountReceivableDAO: Attempting to remove AR transaction with ID: " + id);
    std::string sql = "DELETE FROM " + arTransactionsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "AccountReceivableDAO", "removeTransaction", sql, params
    );
}

} // namespace DAOs
} // namespace Finance
} // namespace ERP