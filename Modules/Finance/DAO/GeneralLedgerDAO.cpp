// Modules/Finance/DAO/GeneralLedgerDAO.cpp
#include "Modules/Finance/DAO/GeneralLedgerDAO.h"
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

GeneralLedgerDAO::GeneralLedgerDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Finance::DTO::GLAccountBalanceDTO>(connectionPool, "gl_account_balances") { // Pass table name for GLAccountBalance
    Logger::Logger::getInstance().info("GeneralLedgerDAO: Initialized.");
}

// toMap for GLAccountBalanceDTO
std::map<std::string, std::any> GeneralLedgerDAO::toMap(const ERP::Finance::DTO::GLAccountBalanceDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["gl_account_id"] = dto.glAccountId;
    data["current_debit_balance"] = dto.currentDebitBalance;
    data["current_credit_balance"] = dto.currentCreditBalance;
    data["currency"] = dto.currency;
    data["last_posted_date"] = ERP::Utils::DateUtils::formatDateTime(dto.lastPostedDate, ERP::Common::DATETIME_FORMAT);

    return data;
}

// fromMap for GLAccountBalanceDTO
ERP::Finance::DTO::GLAccountBalanceDTO GeneralLedgerDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Finance::DTO::GLAccountBalanceDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "gl_account_id", dto.glAccountId);
        ERP::DAOHelpers::getPlainValue(data, "current_debit_balance", dto.currentDebitBalance);
        ERP::DAOHelpers::getPlainValue(data, "current_credit_balance", dto.currentCreditBalance);
        ERP::DAOHelpers::getPlainValue(data, "currency", dto.currency);
        ERP::DAOHelpers::getPlainTimeValue(data, "last_posted_date", dto.lastPostedDate);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("GeneralLedgerDAO: fromMap (Balance) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "GeneralLedgerDAO: Data type mismatch in fromMap (Balance).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("GeneralLedgerDAO: fromMap (Balance) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "GeneralLedgerDAO: Unexpected error in fromMap (Balance).");
    }
    return dto;
}

// --- GeneralLedgerAccountDTO specific methods and helpers ---
std::map<std::string, std::any> GeneralLedgerDAO::toMap(const ERP::Finance::DTO::GeneralLedgerAccountDTO& dto) {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["account_number"] = dto.accountNumber;
    data["account_name"] = dto.accountName;
    ERP::DAOHelpers::putOptionalString(data, "description", dto.description);
    data["type"] = static_cast<int>(dto.type);
    ERP::DAOHelpers::putOptionalString(data, "parent_account_id", dto.parentAccountId);

    return data;
}

ERP::Finance::DTO::GeneralLedgerAccountDTO GeneralLedgerDAO::fromMap(const std::map<std::string, std::any>& data) {
    ERP::Finance::DTO::GeneralLedgerAccountDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "account_number", dto.accountNumber);
        ERP::DAOHelpers::getPlainValue(data, "account_name", dto.accountName);
        ERP::DAOHelpers::getOptionalStringValue(data, "description", dto.description);
        
        int typeInt;
        if (ERP::DAOHelpers::getPlainValue(data, "type", typeInt)) dto.type = static_cast<ERP::Finance::DTO::GLAccountType>(typeInt);
        
        ERP::DAOHelpers::getOptionalStringValue(data, "parent_account_id", dto.parentAccountId);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("GeneralLedgerDAO: fromMap (Account) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "GeneralLedgerDAO: Data type mismatch in fromMap (Account).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("GeneralLedgerDAO: fromMap (Account) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "GeneralLedgerDAO: Unexpected error in fromMap (Account).");
    }
    return dto;
}

bool GeneralLedgerDAO::createGLAccount(const ERP::Finance::DTO::GeneralLedgerAccountDTO& account) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Attempting to create new GL account.");
    std::map<std::string, std::any> data = toMap(account);
    
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

    std::string sql = "INSERT INTO " + glAccountsTableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "GeneralLedgerDAO", "createGLAccount", sql, params
    );
}

std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> GeneralLedgerDAO::getGLAccountById(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Attempting to get GL account by ID: " + id);
    std::string sql = "SELECT * FROM " + glAccountsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "GeneralLedgerDAO", "getGLAccountById", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front());
    }
    return std::nullopt;
}

std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> GeneralLedgerDAO::getGLAccountByNumber(const std::string& accountNumber) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Attempting to get GL account by number: " + accountNumber);
    std::string sql = "SELECT * FROM " + glAccountsTableName_ + " WHERE account_number = ?;";
    std::map<std::string, std::any> params;
    params["account_number"] = accountNumber;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "GeneralLedgerDAO", "getGLAccountByNumber", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front());
    }
    return std::nullopt;
}


std::vector<ERP::Finance::DTO::GeneralLedgerAccountDTO> GeneralLedgerDAO::getGLAccounts(const std::map<std::string, std::any>& filter) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Attempting to retrieve GL accounts.");
    std::string sql = "SELECT * FROM " + glAccountsTableName_;
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
        "GeneralLedgerDAO", "getGLAccounts", sql, params
    );

    std::vector<ERP::Finance::DTO::GeneralLedgerAccountDTO> resultsDto;
    for (const auto& rowMap : resultsMap) {
        resultsDto.push_back(fromMap(rowMap));
    }
    return resultsDto;
}

bool GeneralLedgerDAO::updateGLAccount(const ERP::Finance::DTO::GeneralLedgerAccountDTO& account) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Attempting to update GL account with ID: " + account.id);
    std::map<std::string, std::any> data = toMap(account);
    if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerDAO: Update GL account called with empty data or missing ID.");
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "GeneralLedgerDAO: Update GL account called with empty data or missing ID.");
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

    std::string sql = "UPDATE " + glAccountsTableName_ + " SET " + setClause + " WHERE id = ?;";
    params["id_filter"] = account.id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "GeneralLedgerDAO", "updateGLAccount", sql, params
    );
}

bool GeneralLedgerDAO::removeGLAccount(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Attempting to remove GL account with ID: " + id);
    std::string sql = "DELETE FROM " + glAccountsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "GeneralLedgerDAO", "removeGLAccount", sql, params
    );
}

int GeneralLedgerDAO::countGLAccounts(const std::map<std::string, std::any>& filter) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Counting GL accounts.");
    std::string sql = "SELECT COUNT(*) FROM " + glAccountsTableName_;
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

    std::vector<std::map<std::string, std::any>> results = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "GeneralLedgerDAO", "countGLAccounts", sql, params
    );

    if (!results.empty() && results[0].count("COUNT(*)")) {
        if (results[0].at("COUNT(*)").type() == typeid(long long)) {
            return static_cast<int>(std::any_cast<long long>(results[0].at("COUNT(*)")));
        } else if (results[0].at("COUNT(*)").type() == typeid(int)) {
            return std::any_cast<int>(results[0].at("COUNT(*)"));
        }
    }
    return 0;
}

// --- JournalEntryDTO specific methods and helpers ---
std::map<std::string, std::any> GeneralLedgerDAO::toMap(const ERP::Finance::DTO::JournalEntryDTO& dto) {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["journal_number"] = dto.journalNumber;
    data["entry_date"] = ERP::Utils::DateUtils::formatDateTime(dto.entryDate, ERP::Common::DATETIME_FORMAT);
    data["type"] = static_cast<int>(dto.type);
    data["description"] = dto.description;
    ERP::DAOHelpers::putOptionalString(data, "reference_document_id", dto.referenceDocumentId);
    ERP::DAOHelpers::putOptionalString(data, "reference_document_type", dto.referenceDocumentType);
    data["is_posted"] = dto.isPosted;
    ERP::DAOHelpers::putOptionalTime(data, "posting_date", dto.postingDate);
    ERP::DAOHelpers::putOptionalString(data, "posted_by_user_id", dto.postedByUserId);

    return data;
}

ERP::Finance::DTO::JournalEntryDTO GeneralLedgerDAO::fromMap(const std::map<std::string, std::any>& data) {
    ERP::Finance::DTO::JournalEntryDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "journal_number", dto.journalNumber);
        ERP::DAOHelpers::getPlainTimeValue(data, "entry_date", dto.entryDate);
        
        int typeInt;
        if (ERP::DAOHelpers::getPlainValue(data, "type", typeInt)) dto.type = static_cast<ERP::Finance::DTO::JournalEntryType>(typeInt);
        
        ERP::DAOHelpers::getPlainValue(data, "description", dto.description);
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_id", dto.referenceDocumentId);
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_type", dto.referenceDocumentType);
        ERP::DAOHelpers::getPlainValue(data, "is_posted", dto.isPosted);
        ERP::DAOHelpers::getOptionalTimeValue(data, "posting_date", dto.postingDate);
        ERP::DAOHelpers::getOptionalStringValue(data, "posted_by_user_id", dto.postedByUserId);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("GeneralLedgerDAO: fromMap (JournalEntry) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "GeneralLedgerDAO: Data type mismatch in fromMap (JournalEntry).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("GeneralLedgerDAO: fromMap (JournalEntry) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "GeneralLedgerDAO: Unexpected error in fromMap (JournalEntry).");
    }
    return dto;
}

bool GeneralLedgerDAO::createJournalEntry(const ERP::Finance::DTO::JournalEntryDTO& entry) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Attempting to create new journal entry.");
    std::map<std::string, std::any> data = toMap(entry);
    
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

    std::string sql = "INSERT INTO " + journalEntriesTableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "GeneralLedgerDAO", "createJournalEntry", sql, params
    );
}

std::optional<ERP::Finance::DTO::JournalEntryDTO> GeneralLedgerDAO::getJournalEntryById(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Attempting to get journal entry by ID: " + id);
    std::string sql = "SELECT * FROM " + journalEntriesTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "GeneralLedgerDAO", "getJournalEntryById", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front());
    }
    return std::nullopt;
}

std::vector<ERP::Finance::DTO::JournalEntryDTO> GeneralLedgerDAO::getJournalEntries(const std::map<std::string, std::any>& filter) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Attempting to retrieve journal entries.");
    std::string sql = "SELECT * FROM " + journalEntriesTableName_;
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
        "GeneralLedgerDAO", "getJournalEntries", sql, params
    );

    std::vector<ERP::Finance::DTO::JournalEntryDTO> resultsDto;
    for (const auto& rowMap : resultsMap) {
        resultsDto.push_back(fromMap(rowMap));
    }
    return resultsDto;
}

bool GeneralLedgerDAO::updateJournalEntry(const ERP::Finance::DTO::JournalEntryDTO& entry) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Attempting to update journal entry with ID: " + entry.id);
    std::map<std::string, std::any> data = toMap(entry);
    if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerDAO: Update journal entry called with empty data or missing ID.");
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "GeneralLedgerDAO: Update journal entry called with empty data or missing ID.");
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

    std::string sql = "UPDATE " + journalEntriesTableName_ + " SET " + setClause + " WHERE id = ?;";
    params["id_filter"] = entry.id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "GeneralLedgerDAO", "updateJournalEntry", sql, params
    );
}

bool GeneralLedgerDAO::removeJournalEntry(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Attempting to remove journal entry with ID: " + id);
    std::string sql = "DELETE FROM " + journalEntriesTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "GeneralLedgerDAO", "removeJournalEntry", sql, params
    );
}

// --- JournalEntryDetailDTO specific methods and helpers ---
std::map<std::string, std::any> GeneralLedgerDAO::toMap(const ERP::Finance::DTO::JournalEntryDetailDTO& dto) {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["journal_entry_id"] = dto.journalEntryId;
    data["gl_account_id"] = dto.glAccountId;
    data["debit_amount"] = dto.debitAmount;
    data["credit_amount"] = dto.creditAmount;
    data["currency"] = dto.currency;
    ERP::DAOHelpers::putOptionalString(data, "memo", dto.memo);

    return data;
}

ERP::Finance::DTO::JournalEntryDetailDTO GeneralLedgerDAO::fromMap(const std::map<std::string, std::any>& data) {
    ERP::Finance::DTO::JournalEntryDetailDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "journal_entry_id", dto.journalEntryId);
        ERP::DAOHelpers::getPlainValue(data, "gl_account_id", dto.glAccountId);
        ERP::DAOHelpers::getPlainValue(data, "debit_amount", dto.debitAmount);
        ERP::DAOHelpers::getPlainValue(data, "credit_amount", dto.creditAmount);
        ERP::DAOHelpers::getPlainValue(data, "currency", dto.currency);
        ERP::DAOHelpers::getOptionalStringValue(data, "memo", dto.memo);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("GeneralLedgerDAO: fromMap (Detail) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "GeneralLedgerDAO: Data type mismatch in fromMap (Detail).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("GeneralLedgerDAO: fromMap (Detail) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "GeneralLedgerDAO: Unexpected error in fromMap (Detail).");
    }
    return dto;
}

bool GeneralLedgerDAO::createJournalEntryDetail(const ERP::Finance::DTO::JournalEntryDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Attempting to create new journal entry detail.");
    std::map<std::string, std::any> data = toMap(detail);
    
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

    std::string sql = "INSERT INTO " + journalEntryDetailsTableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "GeneralLedgerDAO", "createJournalEntryDetail", sql, params
    );
}

std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> GeneralLedgerDAO::getJournalEntryDetailsByEntryId(const std::string& journalEntryId) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Retrieving journal entry details for entry ID: " + journalEntryId);
    std::string sql = "SELECT * FROM " + journalEntryDetailsTableName_ + " WHERE journal_entry_id = ?;";
    std::map<std::string, std::any> params;
    params["journal_entry_id"] = journalEntryId;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "GeneralLedgerDAO", "getJournalEntryDetailsByEntryId", sql, params
    );

    std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> resultsDto;
    for (const auto& rowMap : resultsMap) {
        resultsDto.push_back(fromMap(rowMap));
    }
    return resultsDto;
}

bool GeneralLedgerDAO::updateJournalEntryDetail(const ERP::Finance::DTO::JournalEntryDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Attempting to update journal entry detail with ID: " + detail.id);
    std::map<std::string, std::any> data = toMap(detail);
    if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerDAO: Update journal entry detail called with empty data or missing ID.");
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "GeneralLedgerDAO: Update journal entry detail called with empty data or missing ID.");
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

    std::string sql = "UPDATE " + journalEntryDetailsTableName_ + " SET " + setClause + " WHERE id = ?;";
    params["id_filter"] = detail.id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "GeneralLedgerDAO", "updateJournalEntryDetail", sql, params
    );
}

bool GeneralLedgerDAO::removeJournalEntryDetail(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerDAO: Attempting to remove journal entry detail with ID: " + id);
    std::string sql = "DELETE FROM " + journalEntryDetailsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "GeneralLedgerDAO", "removeJournalEntryDetail", sql, params
    );
}

} // namespace DAOs
} // namespace Finance
} // namespace ERP