// Modules/Database/SQLiteConnection.cpp
#include "SQLiteConnection.h"
#include "Logger.h" // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h" // Standard includes

namespace ERP {
namespace Database {

SQLiteConnection::SQLiteConnection(const std::string& dbPath) : dbPath_(dbPath), db_(nullptr) {
    ERP::Logger::Logger::getInstance().debug("SQLiteConnection: Constructing for DB: " + dbPath_);
}

SQLiteConnection::~SQLiteConnection() {
    close();
    ERP::Logger::Logger::getInstance().debug("SQLiteConnection: Destructor called for DB: " + dbPath_);
}

bool SQLiteConnection::open() {
    if (db_ != nullptr) {
        ERP::Logger::Logger::getInstance().warning("SQLiteConnection: Connection is already open for DB: " + dbPath_);
        return true; // Already open
    }

    ERP::Logger::Logger::getInstance().info("SQLiteConnection: Opening connection to DB: " + dbPath_);
    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        ERP::Logger::Logger::getInstance().error("SQLiteConnection: Failed to open database: " + lastError_);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "SQLiteConnection: Failed to open database.", "Không thể mở cơ sở dữ liệu.");
        db_ = nullptr; // Ensure db_ is null if open fails
        return false;
    }
    ERP::Logger::Logger::getInstance().info("SQLiteConnection: Database connection opened successfully.");
    return true;
}

void SQLiteConnection::close() {
    if (db_ != nullptr) {
        ERP::Logger::Logger::getInstance().info("SQLiteConnection: Closing connection to DB: " + dbPath_);
        int rc = sqlite3_close(db_);
        if (rc != SQLITE_OK) {
            lastError_ = sqlite3_errmsg(db_);
            ERP::Logger::Logger::getInstance().error("SQLiteConnection: Failed to close database: " + lastError_);
            // Handle error, but proceed to set db_ to nullptr
        }
        db_ = nullptr;
        ERP::Logger::Logger::getInstance().info("SQLiteConnection: Database connection closed.");
    }
}

bool SQLiteConnection::isOpen() const {
    return db_ != nullptr;
}

bool SQLiteConnection::execute(const std::string& sql, const std::map<std::string, std::any>& params) {
    if (!isOpen()) {
        lastError_ = "Database connection is not open.";
        ERP::Logger::Logger::getInstance().error("SQLiteConnection: " + lastError_ + " SQL: " + sql);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Database not open.", "Kết nối cơ sở dữ liệu chưa được mở.");
        return false;
    }

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        ERP::Logger::Logger::getInstance().error("SQLiteConnection: Failed to prepare statement '" + sql + "': " + lastError_);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "SQLiteConnection: Failed to prepare statement.", "Lỗi chuẩn bị câu lệnh SQL.");
        return false;
    }

    if (!bindParameters(stmt, params)) {
        sqlite3_finalize(stmt);
        return false; // Error binding parameters
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        lastError_ = sqlite3_errmsg(db_);
        ERP::Logger::Logger::getInstance().error("SQLiteConnection: Failed to execute statement '" + sql + "': " + lastError_);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "SQLiteConnection: Failed to execute statement.", "Lỗi thực thi câu lệnh SQL.");
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

std::vector<std::map<std::string, std::any>> SQLiteConnection::query(const std::string& sql, const std::map<std::string, std::any>& params) {
    std::vector<std::map<std::string, std::any>> results;
    if (!isOpen()) {
        lastError_ = "Database connection is not open.";
        ERP::Logger::Logger::getInstance().error("SQLiteConnection: " + lastError_ + " SQL: " + sql);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Database not open.", "Kết nối cơ sở dữ liệu chưa được mở.");
        return results;
    }

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        ERP::Logger::Logger::getInstance().error("SQLiteConnection: Failed to prepare query '" + sql + "': " + lastError_);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "SQLiteConnection: Failed to prepare query.", "Lỗi chuẩn bị câu truy vấn SQL.");
        return results;
    }

    if (!bindParameters(stmt, params)) {
        sqlite3_finalize(stmt);
        return results; // Error binding parameters
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::map<std::string, std::any> row;
        int colCount = sqlite3_column_count(stmt);
        for (int i = 0; i < colCount; ++i) {
            std::string colName = sqlite3_column_name(stmt, i);
            int colType = sqlite3_column_type(stmt, i);
            row[colName] = getColumnValue(stmt, colType, i);
        }
        results.push_back(row);
    }

    if (rc != SQLITE_DONE) {
        lastError_ = sqlite3_errmsg(db_);
        ERP::Logger::Logger::getInstance().error("SQLiteConnection: Query execution failed for '" + sql + "': " + lastError_);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "SQLiteConnection: Query execution failed.", "Lỗi thực thi câu truy vấn SQL.");
        results.clear(); // Clear partial results on error
    }

    sqlite3_finalize(stmt);
    return results;
}

bool SQLiteConnection::beginTransaction() {
    if (!isOpen()) {
        lastError_ = "Database connection is not open.";
        ERP::Logger::Logger::getInstance().error("SQLiteConnection: " + lastError_ + " (beginTransaction)");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Database not open.", "Kết nối cơ sở dữ liệu chưa được mở.");
        return false;
    }
    ERP::Logger::Logger::getInstance().debug("SQLiteConnection: Starting transaction.");
    int rc = sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        ERP::Logger::Logger::getInstance().error("SQLiteConnection: Failed to begin transaction: " + lastError_);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "SQLiteConnection: Failed to begin transaction.", "Lỗi bắt đầu giao dịch.");
        return false;
    }
    return true;
}

bool SQLiteConnection::commitTransaction() {
    if (!isOpen()) {
        lastError_ = "Database connection is not open.";
        ERP::Logger::Logger::getInstance().error("SQLiteConnection: " + lastError_ + " (commitTransaction)");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Database not open.", "Kết nối cơ sở dữ liệu chưa được mở.");
        return false;
    }
    ERP::Logger::Logger::getInstance().debug("SQLiteConnection: Committing transaction.");
    int rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        ERP::Logger::Logger::getInstance().error("SQLiteConnection: Failed to commit transaction: " + lastError_);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "SQLiteConnection: Failed to commit transaction.", "Lỗi xác nhận giao dịch.");
        return false;
    }
    return true;
}

bool SQLiteConnection::rollbackTransaction() {
    if (!isOpen()) {
        lastError_ = "Database connection is not open.";
        ERP::Logger::Logger::getInstance().error("SQLiteConnection: " + lastError_ + " (rollbackTransaction)");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Database not open.", "Kết nối cơ sở dữ liệu chưa được mở.");
        return false;
    }
    ERP::Logger::Logger::getInstance().debug("SQLiteConnection: Rolling back transaction.");
    int rc = sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        ERP::Logger::Logger::getInstance().error("SQLiteConnection: Failed to rollback transaction: " + lastError_);
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "SQLiteConnection: Failed to rollback transaction.", "Lỗi hoàn tác giao dịch.");
        return false;
    }
    return true;
}

std::string SQLiteConnection::getLastError() const {
    return lastError_;
}

void SQLiteConnection::reset() {
    if (isOpen()) {
        // Ensure any active transaction is rolled back
        rollbackTransaction(); // Attempt rollback, ignoring success result here
        // No other specific state to reset for a basic SQLite connection managed by API
    }
    ERP::Logger::Logger::getInstance().debug("SQLiteConnection: Connection state reset.");
}

bool SQLiteConnection::bindParameters(sqlite3_stmt* stmt, const std::map<std::string, std::any>& params) {
    for (const auto& pair : params) {
        std::string paramName = ":" + pair.first; // SQLite named parameters start with :
        int paramIndex = sqlite3_bind_parameter_index(stmt, paramName.c_str());
        if (paramIndex == 0) {
            ERP::Logger::Logger::getInstance().error("SQLiteConnection: Parameter '" + pair.first + "' not found in statement.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SQLiteConnection: Parameter not found.", "Tham số truy vấn SQL không tìm thấy.");
            return false;
        }

        int rc = SQLITE_OK;
        if (pair.second.type() == typeid(int)) {
            rc = sqlite3_bind_int(stmt, paramIndex, std::any_cast<int>(pair.second));
        } else if (pair.second.type() == typeid(long long)) { // Handle long long if used for IDs/timestamps
            rc = sqlite3_bind_int64(stmt, paramIndex, std::any_cast<long long>(pair.second));
        }
        else if (pair.second.type() == typeid(double)) {
            rc = sqlite3_bind_double(stmt, paramIndex, std::any_cast<double>(pair.second));
        } else if (pair.second.type() == typeid(std::string)) {
            std::string s_val = std::any_cast<std::string>(pair.second);
            rc = sqlite3_bind_text(stmt, paramIndex, s_val.c_str(), -1, SQLITE_TRANSIENT);
        } else if (pair.second.type() == typeid(bool)) {
            rc = sqlite3_bind_int(stmt, paramIndex, std::any_cast<bool>(pair.second) ? 1 : 0);
        } else if (pair.second.type() == typeid(std::any) && !std::any_cast<std::any>(pair.second).has_value()) { // Handle std::nullopt or empty std::any
            rc = sqlite3_bind_null(stmt, paramIndex);
        } else {
            ERP::Logger::Logger::getInstance().error("SQLiteConnection: Unsupported parameter type for parameter '" + pair.first + "'. Type: " + pair.second.type().name());
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "SQLiteConnection: Unsupported parameter type.", "Kiểu tham số truy vấn SQL không được hỗ trợ.");
            return false;
        }

        if (rc != SQLITE_OK) {
            lastError_ = sqlite3_errmsg(db_);
            ERP::Logger::Logger::getInstance().error("SQLiteConnection: Failed to bind parameter at index " + std::to_string(paramIndex) + " for '" + pair.first + "': " + lastError_);
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "SQLiteConnection: Failed to bind parameter: " + lastError_, "Lỗi ràng buộc tham số truy vấn SQL.");
            return false;
        }
    }
    return true;
}

std::any SQLiteConnection::getColumnValue(sqlite3_stmt* stmt, int colType, int colIndex) {
    switch (colType) {
        case SQLITE_INTEGER:
            return static_cast<long long>(sqlite3_column_int64(stmt, colIndex));
        case SQLITE_FLOAT:
            return sqlite3_column_double(stmt, colIndex);
        case SQLITE_TEXT:
            return std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, colIndex)));
        case SQLITE_BLOB:
            // Handle BLOB if necessary, perhaps return std::vector<char> or similar
            ERP::Logger::Logger::getInstance().warning("SQLiteConnection: BLOB column type encountered. Not fully supported.");
            return std::any(); // Return empty std::any for unsupported types
        case SQLITE_NULL:
            return std::any(); // Represent SQL NULL as empty std::any
        default:
            ERP::Logger::Logger::getInstance().warning("SQLiteConnection: Unknown column type " + std::to_string(colType) + " encountered.");
            return std::any();
    }
}

} // namespace Database
} // namespace ERP