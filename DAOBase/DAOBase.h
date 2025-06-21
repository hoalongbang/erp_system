// DAOBase/DAOBase.h
#ifndef DAOBASE_DAOBASE_H
#define DAOBASE_DAOBASE_H
#include <string>       // For std::string
#include <vector>       // For std::vector
#include <map>          // For std::map
#include <any>          // For std::any (C++17) to handle various data types
#include <memory>       // For std::shared_ptr
#include <optional>     // For std::optional
#include <functional>   // For std::function
#include <sstream>      // For stringstream in generic CRUD
#include <stdexcept>    // For std::runtime_error in generic CRUD

// Include the ConnectionPool header
#include "Modules/Database/ConnectionPool.h" // Đã thêm Modules/Database để đường dẫn tuyệt đối hơn
#include "Modules/Database/DBConnection.h"    // Đã thêm Modules/Database để đường dẫn tuyệt đối hơn
#include "Logger.h"         // For logging
#include "ErrorHandler.h"   // For error handling
#include "Common.h"         // For ErrorCode
#include "AutoRelease.h"    // For AutoRelease
#include "DAOHelpers.h"     // For DAOHelpers (getPlainValue, putOptionalString etc.)

namespace ERP {
    namespace DAOBase {
        /**
         * @brief Abstract base class template for Data Access Objects (DAOs).
         * Provides a common interface and generic CRUD implementations for database operations
         * with a specific DTO type.
         * DAOs inheriting from this class will only need to implement toMap() and fromMap().
         *
         * This version uses a ConnectionPool to acquire and release database connections
         * for each operation, ensuring efficient resource management.
         * @tparam T The DTO type this DAO operates on.
         */
        template <typename T>
        class DAOBase {
        public:
            /**
             * @brief Constructor for DAOBase.
             * DAOs no longer hold a direct DBConnection. They will acquire one from the pool.
             * @param connectionPool Shared pointer to the ConnectionPool instance.
             * @param tableName The name of the database table this DAO operates on.
             */
            explicit DAOBase(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool, const std::string& tableName)
                : connectionPool_(connectionPool), tableName_(tableName) {
                if (!connectionPool_) {
                    ERP::Logger::Logger::getInstance().critical("DAOBase", "Initialized with null ConnectionPool.");
                    throw std::runtime_error("DAOBase: ConnectionPool is null.");
                }
                ERP::Logger::Logger::getInstance().debug("DAOBase: Initialized for table " + tableName_ + ".");
            }

            /**
             * @brief Virtual destructor for proper polymorphic cleanup.
             */
            virtual ~DAOBase() = default;

            /**
             * @brief Converts a DTO object into a data map for database storage.
             * This is a pure virtual method that must be implemented by derived classes.
             * @param dto DTO object.
             * @return Converted data map.
             */
            virtual std::map<std::string, std::any> toMap(const T& dto) const = 0;

            /**
             * @brief Converts a database data map into a DTO object.
             * This is a pure virtual method that must be implemented by derived classes.
             * @param data Database data map.
             * @return Converted DTO object.
             */
            virtual T fromMap(const std::map<std::string, std::any>& data) const = 0;

            /**
             * @brief Creates a new record in the database.
             * @param dto The DTO containing data for the new record.
             * @return true if the record was created successfully, false otherwise.
             */
            bool create(const T& dto) {
                ERP::Logger::Logger::getInstance().info("DAOBase: Attempting to create a new record in " + tableName_ + ".");
                std::map<std::string, std::any> data = toMap(dto);
                if (data.empty()) {
                    ERP::Logger::Logger::getInstance().warning("DAOBase: Create operation called with empty data for table " + tableName_ + ".");
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "DAOBase: Create operation called with empty data.", "DAOBase");
                    return false;
                }

                std::string columns;
                std::string placeholders;
                std::map<std::string, std::any> params;
                bool first = true;

                for (const auto& pair : data) {
                    if (!first) {
                        columns += ", ";
                        placeholders += ", ";
                    }
                    columns += pair.first;
                    placeholders += "?";
                    params[pair.first] = pair.second;
                    first = false;
                }

                std::string sql = "INSERT INTO " + tableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
                return executeDbOperation(
                    [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
                        return conn->execute(sql_l, p_l);
                    },
                    tableName_, "create", sql, params
                );
            }

            /**
             * @brief Reads records from the database based on a filter.
             * @param filter A map representing the filter conditions.
             * @return A vector of DTOs, where each element represents a record.
             */
            std::vector<T> get(const std::map<std::string, std::any>& filter = {}) {
                ERP::Logger::Logger::getInstance().info("DAOBase: Attempting to retrieve records from " + tableName_ + ".");
                // Note: SELECT * is used for simplicity. In production, list columns explicitly.
                std::string sql = "SELECT * FROM " + tableName_;
                std::string whereClause;
                std::map<std::string, std::any> params;

                if (!filter.empty()) {
                    whereClause = " WHERE ";
                    bool first = true;
                    for (const auto& pair : filter) {
                        if (!first) {
                            whereClause += " AND ";
                        }
                        whereClause += pair.first + " = ?"; // Basic equality filter
                        params[pair.first] = pair.second;
                        first = false;
                    }
                }
                sql += whereClause + ";";

                std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
                    [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
                        return conn->query(sql_l, p_l);
                    },
                    tableName_, "get", sql, params
                );

                std::vector<T> resultsDto;
                for (const auto& rowMap : resultsMap) {
                    resultsDto.push_back(fromMap(rowMap)); // Gọi phương thức ảo của lớp con
                }
                ERP::Logger::Logger::getInstance().info("DAOBase: Retrieved " + std::to_string(resultsDto.size()) + " records from " + tableName_ + ".");
                return resultsDto;
            }

            /**
             * @brief Updates existing records in the database.
             * @param dto The DTO containing updated data (must have 'id' field set).
             * @return true if the records were updated successfully, false otherwise.
             */
            bool update(const T& dto) {
                ERP::Logger::Logger::getInstance().info("DAOBase: Attempting to update record in " + tableName_ + " with ID: " + dto.id + ".");
                std::map<std::string, std::any> data = toMap(dto);
                if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
                    ERP::Logger::Logger::getInstance().warning("DAOBase: Update operation called with empty data or missing ID for table " + tableName_ + ".");
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "DAOBase: Update operation called with empty data or missing ID.", "DAOBase");
                    return false;
                }

                std::string setClause;
                std::map<std::string, std::any> params;
                bool firstSet = true;

                for (const auto& pair : data) {
                    if (pair.first == "id") continue; // Don't update the ID in SET clause
                    if (!firstSet) setClause += ", ";
                    setClause += pair.first + " = ?";
                    params[pair.first] = pair.second;
                    firstSet = false;
                }

                std::string sql = "UPDATE " + tableName_ + " SET " + setClause + " WHERE id = ?;";
                params["id_filter"] = dto.id; // Add ID for WHERE clause

                return executeDbOperation(
                    [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
                        return conn->execute(sql_l, p_l);
                    },
                    tableName_, "update", sql, params
                );
            }
            
            /**
             * @brief Deletes a record from the database based on its ID.
             * @param id ID of the record to delete.
             * @return true if the record was deleted successfully, false otherwise.
             */
            bool remove(const std::string& id) {
                ERP::Logger::Logger::getInstance().info("DAOBase: Attempting to remove record from " + tableName_ + " with ID: " + id + ".");
                std::map<std::string, std::any> filter;
                filter["id"] = id;
                std::string sql = "DELETE FROM " + tableName_ + " WHERE id = ?;";
                return executeDbOperation(
                    [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
                        return conn->execute(sql_l, p_l);
                    },
                    tableName_, "remove", sql, filter
                );
            }

            /**
             * @brief Retrieves a single record from the database based on its ID.
             * @param id ID of the record to retrieve.
             * @return An optional DTO representing the record, or std::nullopt if not found.
             */
            std::optional<T> getById(const std::string& id) {
                ERP::Logger::Logger::getInstance().info("DAOBase: Attempting to get record from " + tableName_ + " by ID: " + id + ".");
                std::map<std::string, std::any> filter;
                filter["id"] = id;
                const auto results = get(filter); // Calls generic get() which uses fromMap()
                if (!results.empty()) {
                    return results.front();
                }
                ERP::Logger::Logger::getInstance().debug("DAOBase: Record with ID " + id + " not found in " + tableName_ + ".");
                return std::nullopt;
            }

            /**
             * @brief Counts the number of records matching a filter.
             * @param filter A map representing the filter conditions.
             * @return The number of matching records.
             */
            int count(const std::map<std::string, std::any>& filter = {}) {
                ERP::Logger::Logger::getInstance().info("DAOBase: Counting records in " + tableName_ + ".");
                std::string sql = "SELECT COUNT(*) FROM " + tableName_;
                std::string whereClause;
                std::map<std::string, std::any> params;

                if (!filter.empty()) {
                    whereClause = " WHERE ";
                    bool first = true;
                    for (const auto& pair : filter) {
                        if (!first) {
                            whereClause += " AND ";
                        }
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
                    tableName_, "count", sql, params
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

        protected:
            std::shared_ptr<ERP::Database::ConnectionPool> connectionPool_;
            std::string tableName_;

            /**
             * @brief Helper method to acquire a database connection from the pool.
             * @return A shared pointer to an active DBConnection.
             */
            std::shared_ptr<ERP::Database::DBConnection> acquireConnection() {
                // Lấy instance của ConnectionPool và yêu cầu một kết nối
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().critical("DAOBase", "Failed to acquire database connection from pool.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "DAOBase: Failed to acquire database connection.", "Không thể lấy kết nối cơ sở dữ liệu từ pool.");
                }
                return conn;
            }

            /**
             * @brief Helper method to release a database connection back to the pool.
             * @param connection The connection to release.
             */
            void releaseConnection(std::shared_ptr<ERP::Database::DBConnection> connection) {
                if (connection) {
                    connectionPool_->releaseConnection(connection);
                } else {
                    ERP::Logger::Logger::getInstance().warning("DAOBase", "Attempted to release a null database connection.");
                }
            }

            /**
             * @brief Generic helper for executing database operations (insert, update, delete).
             * This function manages connection acquisition and release via AutoRelease,
             * and handles common logging and error reporting.
             * @param operation_lambda A lambda representing the specific database operation (e.g., conn->execute).
             * @param daoName Name of the DAO for logging.
             * @param operationName Name of the operation for logging.
             * @param sql SQL string to execute.
             * @param params Parameters for the SQL query.
             * @return true if successful, false otherwise.
             */
            bool executeDbOperation(
                std::function<bool(std::shared_ptr<ERP::Database::DBConnection>, const std::string&, const std::map<std::string, std::any>&)> operation_lambda,
                const std::string& daoName, const std::string& operationName, const std::string& sql, const std::map<std::string, std::any>& params) {
                std::shared_ptr<ERP::Database::DBConnection> conn = acquireConnection();
                ERP::Utils::AutoRelease releaseGuard([&]() { releaseConnection(conn); }); // Use AutoRelease to ensure connection is released
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error(daoName, "Failed to acquire database connection for " + operationName + " operation.");
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::DatabaseError, daoName + ": Failed to acquire connection.", daoName);
                    return false;
                }
                try {
                    bool success = operation_lambda(conn, sql, params);
                    if (success) {
                        ERP::Logger::Logger::getInstance().info(daoName, operationName + " operation completed successfully.");
                    } else {
                        ERP::Logger::Logger::getInstance().error(daoName, "Failed to complete " + operationName + " operation. SQL: " + sql);
                        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::DatabaseError, daoName + ": Failed to " + operationName + ". SQL: " + sql, daoName);
                    }
                    return success;
                } catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error(daoName, "Exception during " + operationName + " operation: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::DatabaseError, daoName + ": Exception during " + operationName + ": " + std::string(e.what()), daoName);
                    return false;
                }
            }

            /**
             * @brief Generic helper for querying database operations (select).
             * This function manages connection acquisition and release via AutoRelease,
             * and handles common logging and error reporting.
             * @param operation_lambda A lambda representing the specific database query operation (e.g., conn->query).
             * @param daoName Name of the DAO for logging.
             * @param operationName Name of the operation for logging.
             * @param sql SQL string to query.
             * @param params Parameters for the SQL query.
             * @return A vector of maps representing query results, or empty vector on failure.
             */
            std::vector<std::map<std::string, std::any>> queryDbOperation(
                std::function<std::vector<std::map<std::string, std::any>>(std::shared_ptr<ERP::Database::DBConnection>, const std::string&, const std::map<std::string, std::any>&)> operation_lambda,
                const std::string& daoName, const std::string& operationName, const std::string& sql, const std::map<std::string, std::any>& params) {
                std::shared_ptr<ERP::Database::DBConnection> conn = acquireConnection();
                ERP::Utils::AutoRelease releaseGuard([&]() { releaseConnection(conn); }); // Use AutoRelease to ensure connection is released
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error(daoName, "Failed to acquire database connection for " + operationName + " operation.");
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::DatabaseError, daoName + ": Failed to acquire connection.", daoName);
                    return {};
                }
                try {
                    std::vector<std::map<std::string, std::any>> results = operation_lambda(conn, sql, params);
                    ERP::Logger::Logger::getInstance().info(daoName, "Retrieved " + std::to_string(results.size()) + " records for " + operationName + " operation.");
                    return results;
                } catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error(daoName, "Exception during " + operationName + " operation: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::DatabaseError, daoName + ": Exception during " + operationName + ": " + std::string(e.what()), daoName);
                    return {};
                }
            }
        };
    } // namespace DAOBase
} // namespace ERP
#endif // DAOBASE_DAOBASE_H