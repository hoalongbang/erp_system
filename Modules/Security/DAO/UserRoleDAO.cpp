// Modules/Security/DAO/UserRoleDAO.cpp
#include "UserRoleDAO.h"
#include "DAOHelpers.h" // Standard includes for mapping helpers
#include "Logger.h"     // Standard includes for logging
#include "ErrorHandler.h" // Standard includes for error handling
#include "Common.h"     // Standard includes for common enums/constants
#include "DateUtils.h"  // Standard includes for date/time utilities

namespace ERP {
    namespace Security {
        namespace DAOs {

            UserRoleDAO::UserRoleDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<void>(connectionPool, "user_roles") { // Pass "user_roles" as tableName_
                // DAOBase constructor handles connectionPool and tableName_ initialization
                ERP::Logger::Logger::getInstance().info("UserRoleDAO: Initialized.");
            }

            // Overridden methods from DAOBase to work with join table logic

            bool UserRoleDAO::create(const std::map<std::string, std::any>& data) {
                // This override assumes 'data' contains "user_id" and "role_id"
                ERP::Logger::Logger::getInstance().info("UserRoleDAO: Attempting to create a new user-role record.");
                if (data.empty() || data.find("user_id") == data.end() || data.find("role_id") == data.end()) {
                    ERP::Logger::Logger::getInstance().warning("UserRoleDAO: Create operation called with incomplete data (requires user_id, role_id).");
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "UserRoleDAO: Create operation called with incomplete data.");
                    return false;
                }

                std::string sql = "INSERT INTO " + tableName_ + " (user_id, role_id) VALUES (:user_id, :role_id);";

                // Ensure params are correctly named for SQLite bind (matching :param_name in SQL)
                std::map<std::string, std::any> params;
                params["user_id"] = std::any_cast<std::string>(data.at("user_id"));
                params["role_id"] = std::any_cast<std::string>(data.at("role_id"));

                // Use the executeDbOperation inherited from DAOBase
                return this->executeDbOperation(
                    [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
                        return conn->execute(sql_l, p_l);
                    },
                    "UserRoleDAO", "create", sql, params
                );
            }

            std::vector<std::map<std::string, std::any>> UserRoleDAO::get(const std::map<std::string, std::any>& filter) {
                ERP::Logger::Logger::getInstance().info("UserRoleDAO: Attempting to retrieve user-role records.");

                std::string sql = "SELECT user_id, role_id FROM " + tableName_;
                std::string whereClause = buildWhereClause(filter); // Use helper for WHERE clause
                sql += whereClause + ";";

                // Use the queryDbOperation inherited from DAOBase
                return this->queryDbOperation(
                    [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
                        return conn->query(sql_l, p_l);
                    },
                    "UserRoleDAO", "get", sql, filter // Pass filter map directly as params
                );
            }

            bool UserRoleDAO::update(const std::map<std::string, std::any>& filter, const std::map<std::string, std::any>& data) {
                ERP::Logger::Logger::getInstance().warning("UserRoleDAO: Direct update operation on join table is not recommended. Use assignRoleToUser/removeRoleFromUser.");
                ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "UserRoleDAO: Direct update not supported for user_roles. Use assignRoleToUser/removeRoleFromUser.");
                return false;
            }

            bool UserRoleDAO::remove(const std::string& id) {
                // For join tables with composite primary keys (user_id, role_id), a single 'id' string is usually insufficient.
                // This override should ideally be unused or log an error suggesting to use remove(const std::map<std::string, std::any>& filter).
                ERP::Logger::Logger::getInstance().warning("UserRoleDAO: Removing by single ID is not standard for composite key join tables. Use remove(filter) or removeRoleFromUser instead.");
                ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "UserRoleDAO: Remove by single ID not supported for this join table.");
                return false;
            }

            bool UserRoleDAO::remove(const std::map<std::string, std::any>& filter) {
                ERP::Logger::Logger::getInstance().info("UserRoleDAO: Attempting to remove user-role records by filter.");
                if (filter.empty()) {
                    ERP::Logger::Logger::getInstance().warning("UserRoleDAO: Remove operation called with empty filter. Aborting to prevent mass deletion.");
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "UserRoleDAO: Remove operation called with empty filter.");
                    return false;
                }

                std::string sql = "DELETE FROM " + tableName_;
                std::string whereClause = buildWhereClause(filter); // Use helper for WHERE clause
                sql += whereClause + ";";

                // Use the executeDbOperation inherited from DAOBase
                return this->executeDbOperation(
                    [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
                        return conn->execute(sql_l, p_l);
                    },
                    "UserRoleDAO", "remove by filter", sql, filter // Pass filter map directly as params
                );
            }

            std::optional<std::map<std::string, std::any>> UserRoleDAO::getById(const std::string& id) {
                // Not applicable for join table with composite key without a dedicated ID column.
                // This override is to satisfy DAOBase, but should return nullopt or throw.
                ERP::Logger::Logger::getInstance().warning("UserRoleDAO: getById is not typically supported for join tables. Use get(filter) instead.");
                return std::nullopt;
            }

            int UserRoleDAO::count(const std::map<std::string, std::any>& filter) {
                ERP::Logger::Logger::getInstance().info("UserRoleDAO: Counting user-role records.");

                std::string sql = "SELECT COUNT(*) FROM " + tableName_;
                std::string whereClause = buildWhereClause(filter); // Use helper for WHERE clause
                sql += whereClause + ";";

                // Use the queryDbOperation inherited from DAOBase, and handle the int return
                std::vector<std::map<std::string, std::any>> results = this->queryDbOperation(
                    [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
                        return conn->query(sql_l, p_l);
                    },
                    "UserRoleDAO", "count", sql, filter // Pass filter map directly as params
                );
                if (!results.empty() && results[0].count("COUNT(*)")) {
                    return std::any_cast<long long>(results[0].at("COUNT(*)")); // SQLite COUNT(*) returns long long
                }
                return 0;
            }

            bool UserRoleDAO::assignRoleToUser(const std::string& userId, const std::string& roleId) {
                ERP::Logger::Logger::getInstance().info("UserRoleDAO: Attempting to assign role " + roleId + " to user " + userId + ".");

                std::map<std::string, std::any> data;
                data["user_id"] = userId;
                data["role_id"] = roleId;

                // Call the overridden create method
                return create(data);
            }

            bool UserRoleDAO::removeRoleFromUser(const std::string& userId, const std::string& roleId) {
                ERP::Logger::Logger::getInstance().info("UserRoleDAO: Attempting to remove role " + roleId + " from user " + userId + ".");

                std::map<std::string, std::any> filter;
                filter["user_id"] = userId;
                filter["role_id"] = roleId;

                // Call the overridden remove by filter method
                return remove(filter);
            }

            bool UserRoleDAO::removeAllRolesFromUser(const std::string& userId) {
                ERP::Logger::Logger::getInstance().info("UserRoleDAO: Attempting to remove all roles from user " + userId + ".");

                std::map<std::string, std::any> filter;
                filter["user_id"] = userId;

                // Call the overridden remove by filter method
                return remove(filter);
            }


            std::vector<std::string> UserRoleDAO::getRolesByUserId(const std::string& userId) {
                ERP::Logger::Logger::getInstance().info("UserRoleDAO: Getting roles for user ID: " + userId + ".");

                std::map<std::string, std::any> filter;
                filter["user_id"] = userId;

                std::vector<std::map<std::string, std::any>> results = get(filter); // Use the overridden get method

                std::vector<std::string> roleIds;
                for (const auto& row : results) {
                    if (row.count("role_id") && row.at("role_id").type() == typeid(std::string)) {
                        roleIds.push_back(std::any_cast<std::string>(row.at("role_id")));
                    }
                }
                ERP::Logger::Logger::getInstance().info("UserRoleDAO: Retrieved " + std::to_string(roleIds.size()) + " roles for user " + userId + ".");
                return roleIds;
            }

        } // namespace DAOs
    } // namespace Security
} // namespace ERP