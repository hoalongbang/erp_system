// Modules/Database/ConnectionPool.cpp
#include "ConnectionPool.h"
#include "Logger.h" // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h" // Standard includes

#include <stdexcept> // For std::runtime_error

namespace ERP {
namespace Database {

// Static member initialization
ConnectionPool* ConnectionPool::instance_ = nullptr;
std::once_flag ConnectionPool::onceFlag_;

ConnectionPool& ConnectionPool::getInstance() {
    // Use std::call_once to ensure thread-safe and one-time initialization
    std::call_once(onceFlag_, []() {
        instance_ = new ConnectionPool();
    });
    return *instance_;
}

std::shared_ptr<ConnectionPool> ConnectionPool::getInstancePtr() {
    // Return a shared_ptr to the singleton instance
    // Note: It's important to use a custom deleter to call delete on the raw pointer
    // without invoking the default delete (which would be invalid for a singleton managed by new directly)
    // For singletons, returning a shared_ptr directly requires careful management to avoid double deletion.
    // A common approach is to return a shared_ptr to a static local.
    static std::shared_ptr<ConnectionPool> singletonPtr(
        &getInstance(), // Get raw pointer to singleton
        [](ConnectionPool* p) { /* Custom deleter: do nothing, as singleton's lifetime is managed externally */ }
    );
    return singletonPtr;
}

ConnectionPool::ConnectionPool() : initialized_(false), shuttingDown_(false) {
    ERP::Logger::Logger::getInstance().info("ConnectionPool: Constructor called.");
}

void ConnectionPool::initialize(const DTO::DatabaseConfig& config) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (initialized_) {
        ERP::Logger::Logger::getInstance().warning("ConnectionPool: Already initialized. Skipping re-initialization.");
        return;
    }
    if (shuttingDown_) {
        ERP::Logger::Logger::getInstance().error("ConnectionPool: Cannot initialize while shutting down.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "ConnectionPool: Cannot initialize while shutting down.");
        return;
    }

    config_ = config;
    ERP::Logger::Logger::getInstance().info("ConnectionPool: Initializing with max connections: " + std::to_string(config_.maxConnections));

    for (int i = 0; i < config_.maxConnections; ++i) {
        try {
            std::shared_ptr<DBConnection> conn = createConnection();
            if (conn && conn->open()) {
                availableConnections_.push(conn);
                allConnections_.push_back(conn); // Keep track for shutdown
                ERP::Logger::Logger::getInstance().debug("ConnectionPool: Created and opened connection " + std::to_string(i + 1));
            } else {
                ERP::Logger::Logger::getInstance().error("ConnectionPool: Failed to open connection " + std::to_string(i + 1));
                // Handle error: perhaps throw or continue with fewer connections
            }
        } catch (const std::exception& e) {
            ERP::Logger::Logger::getInstance().critical("ConnectionPool: Exception creating connection: " + std::string(e.what()));
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "ConnectionPool: Exception creating connection: " + std::string(e.what()));
        }
    }

    if (availableConnections_.empty()) {
        ERP::Logger::Logger::getInstance().critical("ConnectionPool: Failed to create any database connections.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "ConnectionPool: Failed to create any database connections.", "Không thể tạo bất kỳ kết nối cơ sở dữ liệu nào.");
        throw std::runtime_error("Failed to create any database connections.");
    }

    initialized_ = true;
    ERP::Logger::Logger::getInstance().info("ConnectionPool: Initialization complete. " + std::to_string(availableConnections_.size()) + " connections ready.");
}

std::shared_ptr<DBConnection> ConnectionPool::getConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (shuttingDown_) {
        ERP::Logger::Logger::getInstance().warning("ConnectionPool: Attempted to get connection during shutdown.");
        return nullptr;
    }
    if (!initialized_) {
        ERP::Logger::Logger::getInstance().error("ConnectionPool: Attempted to get connection before initialization.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "ConnectionPool: Not initialized.", "Hệ thống chưa khởi tạo dịch vụ cơ sở dữ liệu.");
        return nullptr;
    }

    if (availableConnections_.empty()) {
        ERP::Logger::Logger::getInstance().info("ConnectionPool: No available connections. Waiting...");
        // Wait for a connection to become available, with a timeout
        if (condition_.wait_for(lock, std::chrono::seconds(config_.connectionTimeoutSeconds),
                                [&]{ return !availableConnections_.empty() || shuttingDown_; })) {
            if (shuttingDown_) {
                ERP::Logger::Logger::getInstance().warning("ConnectionPool: Waited for connection, but pool is shutting down.");
                return nullptr;
            }
            std::shared_ptr<DBConnection> conn = availableConnections_.front();
            availableConnections_.pop();
            ERP::Logger::Logger::getInstance().debug("ConnectionPool: Reused existing connection.");
            return conn;
        } else {
            ERP::Logger::Logger::getInstance().error("ConnectionPool: Timeout acquiring database connection.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "ConnectionPool: Timeout acquiring connection.", "Hết thời gian chờ kết nối cơ sở dữ liệu.");
            return nullptr; // Timeout
        }
    } else {
        std::shared_ptr<DBConnection> conn = availableConnections_.front();
        availableConnections_.pop();
        ERP::Logger::Logger::getInstance().debug("ConnectionPool: Provided existing connection.");
        return conn;
    }
}

void ConnectionPool::releaseConnection(std::shared_ptr<DBConnection> connection) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (shuttingDown_) {
        ERP::Logger::Logger::getInstance().info("ConnectionPool: Connection released during shutdown, closing it directly.");
        if (connection) {
            connection->close();
        }
        return;
    }
    if (connection) {
        // Optionally, reset connection state (e.g., clear transaction, reset auto-commit)
        connection->reset(); // Implement reset logic in DBConnection
        availableConnections_.push(connection);
        ERP::Logger::Logger::getInstance().debug("ConnectionPool: Connection released back to pool.");
        condition_.notify_one(); // Notify waiting threads
    } else {
        ERP::Logger::Logger::getInstance().warning("ConnectionPool: Attempted to release a null connection.");
    }
}

void ConnectionPool::shutdown() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!initialized_ && allConnections_.empty()) {
        ERP::Logger::Logger::getInstance().info("ConnectionPool: Already shut down or not initialized.");
        return;
    }

    shuttingDown_ = true;
    condition_.notify_all(); // Wake up all waiting threads so they can exit gracefully

    ERP::Logger::Logger::getInstance().info("ConnectionPool: Shutting down all connections.");

    // Clear available connections queue and close them
    while (!availableConnections_.empty()) {
        availableConnections_.front()->close();
        availableConnections_.pop();
    }

    // Explicitly close all connections held (even those checked out, though ideally they should be released first)
    // This part assumes that `allConnections_` holds shared_ptrs, so they will be destructed when out of scope
    // or when the vector is cleared. Calling close() explicitly ensures DB resources are freed.
    for (const auto& conn : allConnections_) {
        if (conn && conn->isOpen()) { // Check if still open
            conn->close();
        }
    }
    allConnections_.clear(); // Release shared_ptrs

    initialized_ = false;
    shuttingDown_ = false;
    ERP::Logger::Logger::getInstance().info("ConnectionPool: Shutdown complete.");
}

std::unique_ptr<DBConnection> ConnectionPool::createConnection() {
    // Factory method for creating specific connection types
    switch (config_.type) {
        case DTO::DatabaseType::SQLite:
            return std::make_unique<SQLiteConnection>(config_.database);
        // case DTO::DatabaseType::PostgreSQL:
        //     return std::make_unique<PostgreSQLConnection>(config_.host.value_or(""), config_.port.value_or(5432),
        //                                                   config_.database, config_.username.value_or(""),
        //                                                   config_.password.value_or(""));
        // case DTO::DatabaseType::MySQL:
        //     return std::make_unique<MySQLConnection>(config_.host.value_or(""), config_.port.value_or(3306),
        //                                              config_.database, config_.username.value_or(""),
        //                                              config_.password.value_or(""));
        default:
            ERP::Logger::Logger::getInstance().error("ConnectionPool: Unsupported database type configured.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Unsupported database type.", "Kiểu cơ sở dữ liệu không được hỗ trợ.");
            return nullptr;
    }
}

} // namespace Database
} // namespace ERP