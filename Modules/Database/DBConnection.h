// Modules/Database/DBConnection.h
#ifndef MODULES_DATABASE_DBCONNECTION_H
#define MODULES_DATABASE_DBCONNECTION_H

#include <string>       // For std::string
#include <map>          // For std::map
#include <any>          // For std::any
#include <vector>       // For std::vector
#include <optional>     // For std::optional

namespace ERP {
namespace Database {

/**
 * @brief IQueryResult interface represents a single row of data from a database query.
 */
class IQueryResult {
public:
    virtual ~IQueryResult() = default;

    /**
     * @brief Gets a value from the current row by column name.
     * @tparam T The expected type of the value.
     * @param columnName The name of the column.
     * @return An optional value of type T, or std::nullopt if column not found or type mismatch.
     */
    template<typename T>
    std::optional<T> getValue(const std::string& columnName) const {
        return getValueInternal<T>(columnName);
    }

    /**
     * @brief Gets all values of the current row as a map.
     * @return A map where keys are column names and values are std::any.
     */
    virtual std::map<std::string, std::any> getAllValues() const = 0;

protected:
    // Internal helper for type-safe retrieval, to be implemented by concrete classes
    template<typename T>
    virtual std::optional<T> getValueInternal(const std::string& columnName) const = 0;
};


/**
 * @brief The DBConnection class defines an abstract interface for database connections.
 * Concrete implementations (e.g., SQLiteConnection, PostgreSQLConnection) will
 * inherit from this class.
 */
class DBConnection {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~DBConnection() = default;

    /**
     * @brief Opens the database connection.
     * @return True if the connection was successfully opened, false otherwise.
     */
    virtual bool open() = 0;

    /**
     * @brief Closes the database connection.
     */
    virtual void close() = 0;

    /**
     * @brief Checks if the database connection is currently open.
     * @return True if the connection is open, false otherwise.
     */
    virtual bool isOpen() const = 0;

    /**
     * @brief Executes a non-query SQL statement (e.g., CREATE, INSERT, UPDATE, DELETE).
     * @param sql The SQL statement to execute.
     * @param params Optional map of parameters for prepared statements.
     * @return True if the statement was executed successfully, false otherwise.
     */
    virtual bool execute(const std::string& sql, const std::map<std::string, std::any>& params = {}) = 0;

    /**
     * @brief Executes a query SQL statement (e.g., SELECT).
     * @param sql The SQL query to execute.
     * @param params Optional map of parameters for prepared statements.
     * @return A vector of IQueryResult objects, each representing a row from the result set.
     * Returns an empty vector on error or no results.
     */
    virtual std::vector<std::map<std::string, std::any>> query(const std::string& sql, const std::map<std::string, std::any>& params = {}) = 0;

    /**
     * @brief Starts a database transaction.
     * @return True if the transaction was successfully started, false otherwise.
     */
    virtual bool beginTransaction() = 0;

    /**
     * @brief Commits the current database transaction.
     * @return True if the transaction was successfully committed, false otherwise.
     */
    virtual bool commitTransaction() = 0;

    /**
     * @brief Rolls back the current database transaction.
     * @return True if the transaction was successfully rolled back, false otherwise.
     */
    virtual bool rollbackTransaction() = 0;

    /**
     * @brief Gets the last error message from the database.
     * @return The last error message as a string.
     */
    virtual std::string getLastError() const = 0;

    /**
     * @brief Resets the connection state (e.g., clear transaction, reset auto-commit).
     * This is called by the connection pool before returning a connection to the pool.
     */
    virtual void reset() = 0;
};

} // namespace Database
} // namespace ERP

#endif // MODULES_DATABASE_DBCONNECTION_H