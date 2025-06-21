// DAOBase/DAOBase.cpp
#include "DAOBase.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "AutoRelease.h" // Include for AutoRelease
#include "ConnectionPool.h" // Đã thêm 
#include "DBConnection.h"    // Đã thêm 

namespace ERP {
    namespace DAOBase {
        DAOBase::DAOBase() {
            Logger::Logger::getInstance().debug("DAOBase: Initialized.");
        }
        std::shared_ptr<Database::DBConnection> DAOBase::acquireConnection() {
            // Lấy instance của ConnectionPool và yêu cầu một kết nối
            std::shared_ptr<Database::DBConnection> conn = Database::ConnectionPool::getInstance().getConnection(); // 
            if (!conn) {
                Logger::Logger::getInstance().critical("DAOBase: Failed to acquire database connection from pool.");
                ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "DAOBase: Failed to acquire database connection.", "Không thể lấy kết nối cơ sở dữ liệu từ pool.");
            }
            return conn;
        }
        void DAOBase::releaseConnection(std::shared_ptr<Database::DBConnection> connection) {
            if (connection) {
                Database::ConnectionPool::getInstance().releaseConnection(connection); // 
            }
            else {
                Logger::Logger::getInstance().warning("DAOBase: Attempted to release a null database connection.");
            }
        }
        // Implement the new generic helper methods here
        bool DAOBase::executeDbOperation(
            std::function<bool(std::shared_ptr<Database::DBConnection>, const std::string&, const std::map<std::string, std::any>&)> operation_lambda,
            const std::string& daoName, const std::string& operationName, const std::string& sql, const std::map<std::string, std::any>& params) {
            std::shared_ptr<Database::DBConnection> conn = acquireConnection();
            AutoRelease releaseGuard([&]() { releaseConnection(conn); }); // Use AutoRelease to ensure connection is released
            if (!conn) {
                Logger::Logger::getInstance().error(daoName + ": Failed to acquire database connection for " + operationName + " operation.");
                ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::DatabaseError, daoName + ": Failed to acquire connection.");
                return false;
            }
            try {
                bool success = operation_lambda(conn, sql, params);
                if (success) {
                    Logger::Logger::getInstance().info(daoName + ": " + operationName + " operation completed successfully.");
                }
                else {
                    Logger::Logger::getInstance().error(daoName + ": Failed to complete " + operationName + " operation. SQL: " + sql);
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::DatabaseError, daoName + ": Failed to " + operationName + ". SQL: " + sql);
                }
                return success;
            }
            catch (const std::exception& e) {
                Logger::Logger::getInstance().error(daoName + ": Exception during " + operationName + " operation: " + std::string(e.what()));
                ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::DatabaseError, daoName + ": Exception during " + operationName + ": " + std::string(e.what()));
                return false;
            }
        }
        std::vector<std::map<std::string, std::any>> DAOBase::queryDbOperation(
            std::function<std::vector<std::map<std::string, std::any>>(std::shared_ptr<Database::DBConnection>, const std::string&, const std::map<std::string, std::any>&)> operation_lambda,
            const std::string& daoName, const std::string& operationName, const std::string& sql, const std::map<std::string, std::any>& params) {
            std::shared_ptr<Database::DBConnection> conn = acquireConnection();
            AutoRelease releaseGuard([&]() { releaseConnection(conn); }); // Use AutoRelease to ensure connection is released
            if (!conn) {
                Logger::Logger::getInstance().error(daoName + ": Failed to acquire database connection for " + operationName + " operation.");
                ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::DatabaseError, daoName + ": Failed to acquire connection.");
                return {};
            }
            try {
                std::vector<std::map<std::string, std::any>> results = operation_lambda(conn, sql, params);
                Logger::Logger::getInstance().info(daoName + ": Retrieved " + std::to_string(results.size()) + " records for " + operationName + " operation.");
                return results;
            }
            catch (const std::exception& e) {
                Logger::Logger::getInstance().error(daoName + ": Exception during " + operationName + " operation: " + std::string(e.what()));
                ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::DatabaseError, daoName + ": Exception during " + operationName + ": " + std::string(e.what()));
                return {};
            }
        }
    } // namespace DAOBase
} // namespace ERP