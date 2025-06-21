// Modules/Database/DatabaseConnectionManager.h
#ifndef MODULES_DATABASE_DATABASECONNECTIONMANAGER_H
#define MODULES_DATABASE_DATABASECONNECTIONMANAGER_H

#include <memory>       // For std::shared_ptr
#include <string>       // For std::string
#include <optional>     // For std::optional

// Rút gọn include paths
#include "DBConnection.h"       // Base DB connection interface
#include "ConnectionPool.h"     // Connection pool management
#include "Logger.h"             // For logging
#include "ErrorHandler.h"       // For error handling
#include "Common.h"             // For ErrorCode

namespace ERP {
namespace Database {

/**
 * @brief The DatabaseConnectionManager class provides a convenient way for DAOs
 * to acquire and release database connections from the ConnectionPool.
 * This class abstracts away direct interaction with the ConnectionPool singleton,
 * making DAOs cleaner and more focused on data access logic.
 */
class DatabaseConnectionManager {
public:
    /**
     * @brief Constructor. Initializes with a shared pointer to the ConnectionPool.
     * @param connectionPool Shared pointer to the ConnectionPool instance.
     */
    explicit DatabaseConnectionManager(std::shared_ptr<ConnectionPool> connectionPool);

    /**
     * @brief Acquires a database connection.
     * @return A shared_ptr to a DBConnection, or nullptr if connection could not be acquired.
     */
    std::shared_ptr<DBConnection> acquireConnection();

    /**
     * @brief Releases a database connection back to the pool.
     * @param connection The connection to release.
     */
    void releaseConnection(std::shared_ptr<DBConnection> connection);

private:
    std::shared_ptr<ConnectionPool> connectionPool_; /**< Shared pointer to the database connection pool. */
};

} // namespace Database
} // namespace ERP

#endif // MODULES_DATABASE_DATABASECONNECTIONMANAGER_H