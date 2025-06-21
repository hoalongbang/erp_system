// Modules/Database/ConnectionPool.h
#ifndef MODULES_DATABASE_CONNECTIONPOOL_H
#define MODULES_DATABASE_CONNECTIONPOOL_H

#include <vector>       // For std::vector
#include <string>       // For std::string
#include <memory>       // For std::shared_ptr
#include <mutex>        // For std::mutex, std::unique_lock
#include <condition_variable> // For std::condition_variable
#include <queue>        // For std::queue
#include <chrono>       // For std::chrono::milliseconds
#include <atomic>       // For std::atomic_bool

// Rút gọn include paths
#include "DatabaseConfig.h"     // DTO for database configuration
#include "DBConnection.h"       // Database connection interface
#include "SQLiteConnection.h"   // SQLite implementation of DBConnection
#include "Logger.h"             // For logging
#include "ErrorHandler.h"       // For error handling
#include "Common.h"             // For ErrorCode

namespace ERP {
namespace Database {

/**
 * @brief The ConnectionPool class manages a pool of database connections.
 * It provides a thread-safe mechanism to acquire and release database connections,
 * ensuring efficient reuse and preventing resource exhaustion.
 * Implemented as a Singleton.
 */
class ConnectionPool {
public:
    /**
     * @brief Gets the singleton instance of the ConnectionPool.
     * @return A reference to the ConnectionPool instance.
     */
    static ConnectionPool& getInstance();

    /**
     * @brief Gets a shared_ptr to the singleton instance of the ConnectionPool.
     * Useful for services that need shared ownership without direct singleton pattern access.
     * @return A shared_ptr to the ConnectionPool instance.
     */
    static std::shared_ptr<ConnectionPool> getInstancePtr();

    /**
     * @brief Initializes the connection pool with database configuration.
     * This method must be called once at application startup.
     * @param config The database configuration to use for creating connections.
     */
    void initialize(const DTO::DatabaseConfig& config);

    /**
     * @brief Acquires a database connection from the pool.
     * If no connections are available, it waits until one becomes available or a timeout occurs.
     * @return A shared_ptr to an available DBConnection, or nullptr if a timeout occurs.
     */
    std::shared_ptr<DBConnection> getConnection();

    /**
     * @brief Releases a database connection back to the pool.
     * @param connection The shared_ptr to the DBConnection to release.
     */
    void releaseConnection(std::shared_ptr<DBConnection> connection);

    /**
     * @brief Shuts down the connection pool, closing all active connections.
     * This should be called gracefully during application shutdown.
     */
    void shutdown();

    // Delete copy constructor and assignment operator to enforce singleton
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

private:
    /**
     * @brief Private constructor to enforce singleton pattern.
     */
    ConnectionPool();

    /**
     * @brief Creates a new database connection based on the configured type.
     * @return A unique_ptr to the newly created DBConnection.
     */
    std::unique_ptr<DBConnection> createConnection();

    DTO::DatabaseConfig config_;
    std::queue<std::shared_ptr<DBConnection>> availableConnections_;
    std::vector<std::shared_ptr<DBConnection>> allConnections_; // To keep track of all connections for proper shutdown
    std::mutex mutex_;
    std::condition_variable condition_;
    bool initialized_ = false;
    std::atomic_bool shuttingDown_ = false;
};

} // namespace Database
} // namespace ERP

#endif // MODULES_DATABASE_CONNECTIONPOOL_H