// Modules/Database/DatabaseConnectionManager.cpp
#include "DatabaseConnectionManager.h"
#include "Logger.h" // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h" // Standard includes

namespace ERP {
namespace Database {

DatabaseConnectionManager::DatabaseConnectionManager(std::shared_ptr<ConnectionPool> connectionPool)
    : connectionPool_(connectionPool) {
    if (!connectionPool_) {
        ERP::Logger::Logger::getInstance().critical("DatabaseConnectionManager", "ConnectionPool is null during initialization.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "ConnectionPool is null.", "Lỗi hệ thống: Dịch vụ quản lý kết nối cơ sở dữ liệu không khả dụng.");
        throw std::runtime_error("DatabaseConnectionManager: Null ConnectionPool.");
    }
    ERP::Logger::Logger::getInstance().debug("DatabaseConnectionManager: Initialized.");
}

std::shared_ptr<DBConnection> DatabaseConnectionManager::acquireConnection() {
    if (!connectionPool_) {
        ERP::Logger::Logger::getInstance().error("DatabaseConnectionManager", "Cannot acquire connection: ConnectionPool is null.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "ConnectionPool is null.", "Lỗi hệ thống: Dịch vụ quản lý kết nối cơ sở dữ liệu không khả dụng.");
        return nullptr;
    }
    std::shared_ptr<DBConnection> conn = connectionPool_->getConnection();
    if (!conn) {
        ERP::Logger::Logger::getInstance().error("DatabaseConnectionManager", "Failed to acquire database connection from pool.");
        // Error already handled by ConnectionPool, so just return nullptr
    }
    return conn;
}

void DatabaseConnectionManager::releaseConnection(std::shared_ptr<DBConnection> connection) {
    if (!connectionPool_) {
        ERP::Logger::Logger::getInstance().warning("DatabaseConnectionManager", "Cannot release connection: ConnectionPool is null. Connection might leak.");
        // Cannot handle, connection might be leaked. Log critical error perhaps.
        return;
    }
    connectionPool_->releaseConnection(connection);
}

} // namespace Database
} // namespace ERP