// Modules/Catalog/DAO/PermissionDAO.cpp
#include "PermissionDAO.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
// #include "AutoRelease.h" // Not directly needed here if DAOBase manages connection lifecycle
#include "DAOHelpers.h"  // Ensure DAOHelpers is included for shared helper functions
#include <sstream>
#include <stdexcept>
#include <optional> // Include for std::optional
#include <any>      // Include for std::any
#include <type_traits> // For std::is_same (used by DAOHelpers)

namespace ERP {
    namespace Catalog {
        namespace DAOs {
            // Updated constructor to call base class constructor
            PermissionDAO::PermissionDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Catalog::DTO::PermissionDTO>(connectionPool, "permissions") { //
                Logger::Logger::getInstance().info("PermissionDAO: Initialized.");
            }

            // The 'create' method is now implemented in DAOBase, so this specific implementation is removed.
            // bool PermissionDAO::create(const std::map<std::string, std::any>& data) { /* ... */ }

            // The 'get' method is now implemented in DAOBase.
            // std::vector<std::map<std::string, std::any>> PermissionDAO::get(const std::map<std::string, std::any>& filter) { /* ... */ }

            // The 'update' method is now implemented in DAOBase.
            // bool PermissionDAO::update(const std::map<std::string, std::any>& filter, const std::map<std::string, std::any>& data) { /* ... */ }

            // The 'remove(string id)' method is now implemented in DAOBase.
            // bool PermissionDAO::remove(const std::string& id) { /* ... */ }

            // The 'remove(map filter)' method is now implemented in DAOBase.
            // bool PermissionDAO::remove(const std::map<std::string, std::any>& filter) { /* ... */ }

            // The 'getById' method is now implemented in DAOBase.
            // std::optional<std::map<std::string, std::any>> PermissionDAO::getById(const std::string& id) { /* ... */ }

            // The 'count' method is now implemented in DAOBase.
            // int PermissionDAO::count(const std::map<std::string, std::any>& filter) { /* ... */ }

            // Implementation for getByName - this method is specific to PermissionDAO and is not in DAOBase
            std::optional<std::map<std::string, std::any>> PermissionDAO::getByName(const std::string& name) {
                Logger::Logger::getInstance().info("PermissionDAO: Attempting to get permission by name: " + name);
                std::map<std::string, std::any> filter;
                filter["name"] = name;
                // Call the generic get() from DAOBase
                std::vector<ERP::Catalog::DTO::PermissionDTO> results = this->get(filter); // Use get that returns DTOs
                if (!results.empty()) {
                    return toMap(results[0]); // Convert back to map for optional<map> return
                }
                return std::nullopt;
            }

            // Method to convert DTO to map
            std::map<std::string, std::any> PermissionDAO::toMap(const ERP::Catalog::DTO::PermissionDTO& permission) const { // const for method
                std::map<std::string, std::any> data;
                // BaseDTO fields (using DAOHelpers::putOptionalString/Time)
                data["id"] = permission.id;
                // 'id' is plain std::string
                data["status"] = static_cast<int>(permission.status);
                // 'status' is enum
                data["created_at"] = ERP::Utils::DateUtils::formatDateTime(permission.createdAt, ERP::Common::DATETIME_FORMAT);
                // 'createdAt' is plain time_point
                ERP::DAOHelpers::putOptionalTime(data, "updated_at", permission.updatedAt);
                ERP::DAOHelpers::putOptionalString(data, "created_by", permission.createdBy);
                ERP::DAOHelpers::putOptionalString(data, "updated_by", permission.updatedBy);
                // PermissionDTO specific fields
                data["name"] = permission.name;
                // 'name' is plain std::string
                ERP::DAOHelpers::putOptionalString(data, "description", permission.description);
                // No need to add 'module' and 'action' fields directly to map if 'name' already combines them
                // unless they are separate columns in the database.
                // Assuming 'name' in DB stores "Module.Action"
                data["module"] = permission.module;
                data["action"] = permission.action;

                return data;
            }
            ERP::Catalog::DTO::PermissionDTO PermissionDAO::fromMap(const std::map<std::string, std::any>& data) const { // const for method
                ERP::Catalog::DTO::PermissionDTO permission;
                try {
                    // BaseDTO fields - Use ERP::DAOHelpers::getPlainValue for non-optional fields
                    ERP::DAOHelpers::getPlainValue(data, "id", permission.id);
                    auto statusIt = data.find("status");
                    if (statusIt != data.end() && statusIt->second.type() == typeid(int)) {
                        permission.status = static_cast<ERP::Common::EntityStatus>(std::any_cast<int>(statusIt->second));
                    }
                    else {
                        permission.status = ERP::Common::EntityStatus::UNKNOWN;
                        // Default status if not found or wrong type
                    }
                    // Use ERP::DAOHelpers::getPlainTimeValue for non-optional time_point
                    ERP::DAOHelpers::getPlainTimeValue(data, "created_at", permission.createdAt);
                    // Optional fields - Use ERP::DAOHelpers::getOptionalStringValue and getOptionalTimeValue
                    ERP::DAOHelpers::getOptionalTimeValue(data, "updated_at", permission.updatedAt);
                    ERP::DAOHelpers::getOptionalStringValue(data, "created_by", permission.createdBy);
                    ERP::DAOHelpers::getOptionalStringValue(data, "updated_by", permission.updatedBy);
                    // PermissionDTO specific fields
                    ERP::DAOHelpers::getPlainValue(data, "name", permission.name);
                    ERP::DAOHelpers::getPlainValue(data, "module", permission.module); // 'module' is plain std::string
                    ERP::DAOHelpers::getPlainValue(data, "action", permission.action);
                    // 'action' is plain std::string
                    ERP::DAOHelpers::getOptionalStringValue(data, "description", permission.description);
                }
                catch (const std::bad_any_cast& e) {
                    Logger::Logger::getInstance().error("PermissionDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "PermissionDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    Logger::Logger::getInstance().error("PermissionDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "PermissionDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return permission;
            }
        } // namespace DAOs
    } // namespace Catalog
} // namespace ERP