// Modules/Catalog/DAO/RoleDAO.cpp
#include "RoleDAO.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
// #include "AutoRelease.h" // Not directly needed here if DAOBase manages connection lifecycle
#include "DAOHelpers.h"  // Rely on DAOHelpers for utility functions (getPlainValue, getOptionalStringValue, etc.)
#include <sstream>
#include <stdexcept>
#include <optional> // Include for std::optional
#include <any>      // Include for std::any
#include <type_traits> // For std::is_same (used in DAOHelpers)

namespace ERP {
    namespace Catalog {
        namespace DAOs {
            // Updated constructor to call base class constructor
            RoleDAO::RoleDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Catalog::DTO::RoleDTO>(connectionPool, "roles") { //
                Logger::Logger::getInstance().info("RoleDAO: Initialized.");
            }

            // The 'create' method is now implemented in DAOBase, so this specific implementation is removed.
            // bool RoleDAO::create(const std::map<std::string, std::any>& data) { /* ... */ }

            // The 'get' method is now implemented in DAOBase.
            // std::vector<std::map<std::string, std::any>> RoleDAO::get(const std::map<std::string, std::any>& filter) { /* ... */ }

            // The 'update' method is now implemented in DAOBase.
            // bool RoleDAO::update(const std::map<std::string, std::any>& filter, const std::map<std::string, std::any>& data) { /* ... */ }

            // The 'remove(string id)' method is now implemented in DAOBase.
            // bool RoleDAO::remove(const std::string& id) { /* ... */ }

            // The 'remove(map filter)' method is now implemented in DAOBase.
            // bool RoleDAO::remove(const std::map<std::string, std::any>& filter) { /* ... */ }

            // The 'getById' method is now implemented in DAOBase.
            // std::optional<std::map<std::string, std::any>> RoleDAO::getById(const std::string& id) { /* ... */ }

            // The 'count' method is now implemented in DAOBase.
            // int RoleDAO::count(const std::map<std::string, std::any>& filter) { /* ... */ }

            std::vector<std::map<std::string, std::any>> RoleDAO::getRolePermissions(const std::string& roleId) {
                Logger::Logger::getInstance().info("RoleDAO: Getting permissions for role ID: " + roleId);
                std::string sql = "SELECT permission_name FROM " + rolePermissionsTableName_ + " WHERE role_id = ?;";
                std::map<std::string, std::any> params;
                params["role_id"] = roleId;
                // Use the queryDbOperation inherited from DAOBase
                return this->queryDbOperation(
                    [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
                        return conn->query(sql_l, p_l);
                    },
                    "RoleDAO", "getRolePermissions", sql, params
                );
            }
            bool RoleDAO::addRolePermission(const std::string& roleId, const std::string& permissionName) {
                Logger::Logger::getInstance().info("RoleDAO: Adding permission " + permissionName + " to role " + roleId);
                std::string sql = "INSERT INTO " + rolePermissionsTableName_ + " (role_id, permission_name) VALUES (?, ?);";
                std::map<std::string, std::any> params;
                params["role_id"] = roleId;
                params["permission_name"] = permissionName;
                // Use the executeDbOperation inherited from DAOBase
                return this->executeDbOperation(
                    [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
                        return conn->execute(sql_l, p_l);
                    },
                    "RoleDAO", "addRolePermission", sql, params
                );
            }
            bool RoleDAO::removeRolePermission(const std::string& roleId, const std::string& permissionName) {
                Logger::Logger::getInstance().info("RoleDAO: Removing permission " + permissionName + " from role " + roleId);
                std::string sql;
                std::map<std::string, std::any> params;
                if (permissionName.empty()) { // If permissionName is empty, remove all permissions for the role
                    sql = "DELETE FROM " + rolePermissionsTableName_ + " WHERE role_id = ?;";
                    params["role_id"] = roleId;
                }
                else {
                    sql = "DELETE FROM " + rolePermissionsTableName_ + " WHERE role_id = ? AND permission_name = ?;";
                    params["role_id"] = roleId;
                    params["permission_name"] = permissionName;
                }
                // Use the executeDbOperation inherited from DAOBase
                return this->executeDbOperation(
                    [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
                        return conn->execute(sql_l, p_l);
                    },
                    "RoleDAO", "removeRolePermission", sql, params
                );
            }
            // Method to convert DTO to map
            std::map<std::string, std::any> RoleDAO::toMap(const ERP::Catalog::DTO::RoleDTO& role) const { // const for method
                std::map<std::string, std::any> data;
                data["id"] = role.id;
                data["name"] = role.name;
                // Handle std::optional fields using DAOHelpers
                ERP::DAOHelpers::putOptionalString(data, "description", role.description);
                // Using DAOHelpers
                data["status"] = static_cast<int>(role.status);
                data["created_at"] = ERP::Utils::DateUtils::formatDateTime(role.createdAt, ERP::Common::DATETIME_FORMAT);
                ERP::DAOHelpers::putOptionalTime(data, "updated_at", role.updatedAt); // Using DAOHelpers
                ERP::DAOHelpers::putOptionalString(data, "created_by", role.createdBy);
                // Using DAOHelpers
                ERP::DAOHelpers::putOptionalString(data, "updated_by", role.updatedBy);
                // Using DAOHelpers
                return data;
            }
            // Method to convert map to DTO
            ERP::Catalog::DTO::RoleDTO RoleDAO::fromMap(const std::map<std::string, std::any>& data) const { // const for method
                ERP::Catalog::DTO::RoleDTO role;
                try {
                    // BaseDTO fields - Use ERP::DAOHelpers::getPlainValue for non-optional fields
                    ERP::DAOHelpers::getPlainValue(data, "id", role.id);
                    auto statusIt = data.find("status");
                    if (statusIt != data.end() && statusIt->second.type() == typeid(int)) {
                        role.status = static_cast<ERP::Common::EntityStatus>(std::any_cast<int>(statusIt->second));
                    }
                    else {
                        role.status = ERP::Common::EntityStatus::UNKNOWN;
                        // Default status if not found or wrong type
                    }
                    // Assuming createdAt is a plain std::chrono::system_clock::time_point
                    ERP::DAOHelpers::getPlainTimeValue(data, "created_at", role.createdAt);
                    // Optional fields - Use ERP::DAOHelpers::getOptionalStringValue and getOptionalTimeValue
                    ERP::DAOHelpers::getOptionalTimeValue(data, "updated_at", role.updatedAt);
                    ERP::DAOHelpers::getOptionalStringValue(data, "created_by", role.createdBy);
                    ERP::DAOHelpers::getOptionalStringValue(data, "updated_by", role.updatedBy);
                    // RoleDTO specific fields
                    ERP::DAOHelpers::getPlainValue(data, "name", role.name);
                    ERP::DAOHelpers::getOptionalStringValue(data, "description", role.description);
                    // Permissions are typically loaded separately via getRolePermissions
                }
                catch (const std::bad_any_cast& e) {
                    Logger::Logger::getInstance().error("RoleDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "RoleDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    Logger::Logger::getInstance().error("RoleDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "RoleDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return role;
            }
        } // namespace DAOs
    } // namespace Catalog
} // namespace ERP