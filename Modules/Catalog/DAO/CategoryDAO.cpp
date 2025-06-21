// Modules/Catalog/DAO/CategoryDAO.cpp
#include "CategoryDAO.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "DAOHelpers.h"  // Ensure DAOHelpers is included for utility functions
#include <sstream>
#include <stdexcept>
#include <optional> // Include for std::optional
#include <any>      // Include for std::any
#include <type_traits> // For std::is_same (used by DAOHelpers)

namespace ERP {
    namespace Catalog {
        namespace DAOs {
            // Updated constructor to call base class constructor
            CategoryDAO::CategoryDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Catalog::DTO::CategoryDTO>(connectionPool, "categories") { //
                Logger::Logger::getInstance().info("CategoryDAO: Initialized.");
            }

            // The 'create' method is now implemented in DAOBase, so this specific implementation is removed.
            // Only toMap and fromMap are needed here.
            // bool CategoryDAO::create(const std::map<std::string, std::any>& data) {
            //     Logger::Logger::getInstance().info("CategoryDAO: Attempting to create a new category record.");
            //     if (data.empty()) {
            //         Logger::Logger::getInstance().warning("CategoryDAO: Create operation called with empty data.");
            //         ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "CategoryDAO: Create operation called with empty data.");
            //         return false;
            //     }
            //     // ... (rest of the old create logic) ...
            //     return this->executeDbOperation(
            //         [](std::shared_ptr<Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            //             return conn->execute(sql_l, p_l);
            //         },
            //         "CategoryDAO", "create", sql, params
            //     );
            // }

            // The 'get' method is now implemented in DAOBase.
            // std::vector<std::map<std::string, std::any>> CategoryDAO::get(const std::map<std::string, std::any>& filter) {
            //     Logger::Logger::getInstance().info("CategoryDAO: Attempting to retrieve category records.");
            //     // ... (rest of the old get logic) ...
            //     return this->queryDbOperation(
            //         [](std::shared_ptr<Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            //             return conn->query(sql_l, p_l);
            //         },
            //         "CategoryDAO", "get", sql, params
            //     );
            // }

            // The 'update' method is now implemented in DAOBase.
            // bool CategoryDAO::update(const std::map<std::string, std::any>& filter, const std::map<std::string, std::any>& data) {
            //     Logger::Logger::getInstance().info("CategoryDAO: Attempting to update category records.");
            //     // ... (rest of the old update logic) ...
            //     return this->executeDbOperation(
            //         [](std::shared_ptr<Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            //             return conn->execute(sql_l, p_l);
            //         },
            //         "CategoryDAO", "update", sql, params
            //     );
            // }

            // The 'remove(string id)' method is now implemented in DAOBase.
            // bool CategoryDAO::remove(const std::string& id) {
            //     Logger::Logger::getInstance().info("CategoryDAO: Attempting to remove category record with ID: " + id);
            //     // ... (rest of the old remove logic) ...
            //     return this->executeDbOperation(
            //         [](std::shared_ptr<Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            //             return conn->execute(sql_l, p_l);
            //         },
            //         "CategoryDAO", "remove", sql, filter
            //     );
            // }

            // The 'remove(map filter)' method is now implemented in DAOBase.
            // bool CategoryDAO::remove(const std::map<std::string, std::any>& filter) {
            //     Logger::Logger::getInstance().info("CategoryDAO: Attempting to remove category records by filter.");
            //     // ... (rest of the old remove by filter logic) ...
            //     return this->executeDbOperation(
            //         [](std::shared_ptr<Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            //             return conn->execute(sql_l, p_l);
            //         },
            //         "CategoryDAO", "remove by filter", sql, filter
            //     );
            // }

            // The 'getById' method is now implemented in DAOBase.
            // std::optional<std::map<std::string, std::any>> CategoryDAO::getById(const std::string& id) {
            //     Logger::Logger::getInstance().info("CategoryDAO: Attempting to get category by ID: " + id);
            //     std::map<std::string, std::any> filter;
            //     filter["id"] = id;
            //     // get(...) already uses DAOBase's queryDbOperation
            //     const auto results = get(filter);
            //     if (!results.empty()) {
            //         return results.front();
            //     }
            //     return std::nullopt;
            // }

            // The 'count' method is now implemented in DAOBase.
            // int CategoryDAO::count(const std::map<std::string, std::any>& filter) {
            //     Logger::Logger::getInstance().info("CategoryDAO: Counting category records.");
            //     // ... (rest of the old count logic) ...
            //     std::vector<std::map<std::string, std::any>> results = this->queryDbOperation(
            //         [](std::shared_ptr<Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            //             return conn->query(sql_l, p_l);
            //         },
            //         "CategoryDAO", "count", sql, params
            //     );
            //     if (!results.empty() && results[0].count("COUNT(*)")) {
            //         if (results[0].at("COUNT(*)").type() == typeid(long long)) {
            //             return static_cast<int>(std::any_cast<long long>(results[0].at("COUNT(*)")));
            //         }
            //         else if (results[0].at("COUNT(*)").type() == typeid(int)) {
            //             return std::any_cast<int>(results[0].at("COUNT(*)"));
            //         }
            //     }
            //     return 0;
            // }

            // Method to convert DTO to map
            std::map<std::string, std::any> CategoryDAO::toMap(const ERP::Catalog::DTO::CategoryDTO& category) const { // const for method
                std::map<std::string, std::any> data;
                // BaseDTO fields (using DAOHelpers::putOptionalString/Time)
                data["id"] = category.id;
                // 'id' is plain std::string
                data["status"] = static_cast<int>(category.status);
                // 'status' is enum
                data["created_at"] = ERP::Utils::DateUtils::formatDateTime(category.createdAt, ERP::Common::DATETIME_FORMAT);
                // 'createdAt' is plain time_point
                ERP::DAOHelpers::putOptionalTime(data, "updated_at", category.updatedAt);
                ERP::DAOHelpers::putOptionalString(data, "created_by", category.createdBy);
                ERP::DAOHelpers::putOptionalString(data, "updated_by", category.updatedBy);
                // CategoryDTO specific fields
                data["name"] = category.name;
                // 'name' is plain std::string
                ERP::DAOHelpers::putOptionalString(data, "description", category.description);
                ERP::DAOHelpers::putOptionalString(data, "parent_id", category.parentCategoryId);

                // Sort order and is_active fields
                data["sort_order"] = category.sortOrder;
                data["is_active"] = category.isActive;

                return data;
            }
            ERP::Catalog::DTO::CategoryDTO CategoryDAO::fromMap(const std::map<std::string, std::any>& data) const { // const for method
                ERP::Catalog::DTO::CategoryDTO category;
                try {
                    // BaseDTO fields - Use ERP::DAOHelpers::getPlainValue for non-optional fields
                    ERP::DAOHelpers::getPlainValue(data, "id", category.id);
                    auto statusIt = data.find("status");
                    if (statusIt != data.end() && statusIt->second.type() == typeid(int)) {
                        category.status = static_cast<ERP::Common::EntityStatus>(std::any_cast<int>(statusIt->second));
                    }
                    else {
                        category.status = ERP::Common::EntityStatus::UNKNOWN;
                        // Default status if not found or wrong type
                    }
                    // Use ERP::DAOHelpers::getPlainTimeValue for non-optional time_point
                    ERP::DAOHelpers::getPlainTimeValue(data, "created_at", category.createdAt);
                    // Optional fields - Use ERP::DAOHelpers::getOptionalStringValue and getOptionalTimeValue
                    ERP::DAOHelpers::getOptionalTimeValue(data, "updated_at", category.updatedAt);
                    ERP::DAOHelpers::getOptionalStringValue(data, "created_by", category.createdBy);
                    ERP::DAOHelpers::getOptionalStringValue(data, "updated_by", category.updatedBy);
                    // CategoryDTO specific fields
                    ERP::DAOHelpers::getPlainValue(data, "name", category.name);
                    ERP::DAOHelpers::getOptionalStringValue(data, "description", category.description);
                    ERP::DAOHelpers::getOptionalStringValue(data, "parent_id", category.parentCategoryId);

                    // Sort order and is_active fields
                    auto sortOrderIt = data.find("sort_order");
                    if (sortOrderIt != data.end() && sortOrderIt->second.type() == typeid(int)) {
                        category.sortOrder = std::any_cast<int>(sortOrderIt->second);
                    }
                    else {
                        category.sortOrder = 0;
                        // Default sort order
                    }

                    auto isActiveIt = data.find("is_active");
                    if (isActiveIt != data.end() && isActiveIt->second.type() == typeid(bool)) {
                        category.isActive = std::any_cast<bool>(isActiveIt->second);
                    }
                    else if (isActiveIt != data.end() && isActiveIt->second.type() == typeid(int)) { // Handle int to bool conversion
                        category.isActive = (std::any_cast<int>(isActiveIt->second) != 0);
                    }
                    else {
                        category.isActive = true;
                        // Default is active
                    }
                }
                catch (const std::bad_any_cast& e) {
                    Logger::Logger::getInstance().error("CategoryDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "CategoryDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    Logger::Logger::getInstance().error("CategoryDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "CategoryDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }

                return category;
            }
        } // namespace DAOs
    } // namespace Catalog
} // namespace ERP