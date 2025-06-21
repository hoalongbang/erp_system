// Modules/Database/SQLiteConnection.h
#ifndef MODULES_DATABASE_SQLITECONNECTION_H
#define MODULES_DATABASE_SQLITECONNECTION_H

#include "DBConnection.h"       // Base class for database connection
#include "Logger.h"             // For logging
#include "ErrorHandler.h"       // For error handling
#include "Common.h"             // For ErrorCode

#include <sqlite3.h>            // SQLite C API
#include <string>               // For std::string
#include <map>                  // For std::map
#include <any>                  // For std::any
#include <vector>               // For std::vector
#include <stdexcept>            // For std::runtime_error
#include <utility>              // For std::move

namespace ERP {
namespace Database {

/**
 * @brief The SQLiteConnection class provides a concrete implementation of DBConnection
 * for SQLite databases.
 */
class SQLiteConnection : public DBConnection {
public:
    /**
     * @brief Constructs a SQLiteConnection object.
     * @param dbPath The file path to the SQLite database.
     */
    explicit SQLiteConnection(const std::string& dbPath);

    /**
     * @brief Destructor. Closes the database connection if it's still open.
     */
    ~SQLiteConnection() override;

    /**
     * @brief Opens the SQLite database connection.
     * @return True if the connection was successfully opened, false otherwise.
     */
    bool open() override;

    /**
     * @brief Closes the SQLite database connection.
     */
    void close() override;

    /**
     * @brief Checks if the SQLite database connection is currently open.
     * @return True if the connection is open, false otherwise.
     */
    bool isOpen() const override;

    /**
     * @brief Executes a non-query SQL statement with optional parameters.
     * @param sql The SQL statement to execute.
     * @param params Optional map of parameters for prepared statements.
     * @return True if the statement was executed successfully, false otherwise.
     */
    bool execute(const std::string& sql, const std::map<std::string, std::any>& params = {}) override;

    /**
     * @brief Executes a query SQL statement with optional parameters.
     * @param sql The SQL query to execute.
     * @param params Optional map of parameters for prepared statements.
     * @return A vector of maps, each map representing a row from the result set.
     * Keys are column names, values are std::any.
     */
    std::vector<std::map<std::string, std::any>> query(const std::string& sql, const std::map<std::string, std::any>& params = {}) override;

    /**
     * @brief Starts a database transaction.
     * @return True if the transaction was successfully started, false otherwise.
     */
    bool beginTransaction() override;

    /**
     * @brief Commits the current database transaction.
     * @return True if the transaction was successfully committed, false otherwise.
     */
    bool commitTransaction() override;

    /**
     * @brief Rolls back the current database transaction.
     * @return True if the transaction was successfully rolled back, false otherwise.
     */
    bool rollbackTransaction() override;

    /**
     * @brief Gets the last error message from the SQLite database.
     * @return The last error message as a string.
     */
    std::string getLastError() const override;

    /**
     * @brief Resets the connection state. For SQLite, this typically means
     * ensuring no active transaction and clearing any temporary state.
     */
    void reset() override;

private:
    std::string dbPath_; /**< The path to the SQLite database file. */
    sqlite3* db_ = nullptr; /**< Pointer to the SQLite database handle. */
    mutable std::string lastError_; /**< Stores the last error message. */

    // Helper to bind parameters to a prepared statement
    bool bindParameters(sqlite3_stmt* stmt, const std::map<std::string, std::any>& params);

    // Helper for safe std::any_cast (specific to SQLite column types)
    std::any getColumnValue(sqlite3_stmt* stmt, int colType, int colIndex);
};

} // namespace Database
} // namespace ERP

#endif // MODULES_DATABASE_SQLITECONNECTION_H