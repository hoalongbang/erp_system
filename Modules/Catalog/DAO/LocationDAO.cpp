// Modules/Catalog/DAO/LocationDAO.cpp
#include "LocationDAO.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "AutoRelease.h" // Include for AutoRelease
#include "DAOHelpers.h"  // NEW: Include DAOHelpers for shared helper functions
#include <sstream>
#include <stdexcept>
#include <optional> // Include for std::optional
#include <any>      // Include for std::any

namespace ERP {
    namespace Catalog {
        namespace DAOs {
            // Updated constructor to call base class constructor
            LocationDAO::LocationDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Catalog::DTO::LocationDTO>(connectionPool, "locations") { //
                Logger::Logger::getInstance().info("LocationDAO: Initialized.");
            }

            // The 'create' method is now implemented in DAOBase, so this specific implementation is removed.
            // bool LocationDAO::create(const std::map<std::string, std::any>& data) { /* ... */ }

            // The 'get' method is now implemented in DAOBase.
            // std::vector<std::map<std::string, std::any>> LocationDAO::get(const std::map<std::string, std::any>& filter) { /* ... */ }

            // The 'update' method is now implemented in DAOBase.
            // bool LocationDAO::update(const std::map<std::string, std::any>& filter, const std::map<std::string, std::any>& data) { /* ... */ }

            // The 'remove(string id)' method is now implemented in DAOBase.
            // bool LocationDAO::remove(const std::string& id) { /* ... */ }

            // The 'remove(map filter)' method is now implemented in DAOBase.
            // bool LocationDAO::remove(const std::map<std::string, std::any>& filter) { /* ... */ }

            // The 'getById' method is now implemented in DAOBase.
            // std::optional<std::map<std::string, std::any>> LocationDAO::getById(const std::string& id) { /* ... */ }

            // The 'count' method is now implemented in DAOBase.
            // int LocationDAO::count(const std::map<std::string, std::any>& filter) { /* ... */ }

            // Method to convert DTO to map
            std::map<std::string, std::any> LocationDAO::toMap(const ERP::Catalog::DTO::LocationDTO& location) const { // const for method
                std::map<std::string, std::any> data;
                // BaseDTO fields (assuming BaseDTO members based on other DAOs)
                data["id"] = location.id;
                data["status"] = static_cast<int>(location.status);
                data["created_at"] = ERP::Utils::DateUtils::formatDateTime(location.createdAt, ERP::Common::DATETIME_FORMAT);
                ERP::DAOHelpers::putOptionalTime(data, "updated_at", location.updatedAt);
                ERP::DAOHelpers::putOptionalString(data, "created_by", location.createdBy);
                ERP::DAOHelpers::putOptionalString(data, "updated_by", location.updatedBy);
                // LocationDTO specific fields
                data["warehouse_id"] = location.warehouseId;
                data["name"] = location.name;
                ERP::DAOHelpers::putOptionalString(data, "type", location.type);
                ERP::DAOHelpers::putOptionalDouble(data, "capacity", location.capacity);
                ERP::DAOHelpers::putOptionalString(data, "unit_of_capacity", location.unitOfCapacity);
                return data;
            }
            ERP::Catalog::DTO::LocationDTO LocationDAO::fromMap(const std::map<std::string, std::any>& data) const { // const for method
                ERP::Catalog::DTO::LocationDTO location;
                try {
                    // BaseDTO fields
                    ERP::DAOHelpers::getPlainValue(data, "id", location.id);
                    auto statusIt = data.find("status");
                    if (statusIt != data.end() && statusIt->second.type() == typeid(int)) {
                        location.status = static_cast<ERP::Common::EntityStatus>(std::any_cast<int>(statusIt->second));
                    }
                    else {
                        location.status = ERP::Common::EntityStatus::UNKNOWN;
                        // Default status if not found or wrong type
                    }
                    // Assuming createdAt is a plain std::chrono::system_clock::time_point
                    // Using DAOHelpers::getPlainTimeValue
                    ERP::DAOHelpers::getPlainTimeValue(data, "created_at", location.createdAt);
                    ERP::DAOHelpers::getOptionalTimeValue(data, "updated_at", location.updatedAt);
                    ERP::DAOHelpers::getOptionalStringValue(data, "created_by", location.createdBy);
                    ERP::DAOHelpers::getOptionalStringValue(data, "updated_by", location.updatedBy);
                    // LocationDTO specific fields
                    ERP::DAOHelpers::getPlainValue(data, "warehouse_id", location.warehouseId);
                    ERP::DAOHelpers::getPlainValue(data, "name", location.name);
                    ERP::DAOHelpers::getOptionalStringValue(data, "type", location.type);
                    ERP::DAOHelpers::getOptionalDoubleValue(data, "capacity", location.capacity);
                    ERP::DAOHelpers::getOptionalStringValue(data, "unit_of_capacity", location.unitOfCapacity);
                }
                catch (const std::bad_any_cast& e) {
                    Logger::Logger::getInstance().error("LocationDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "LocationDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    Logger::Logger::getInstance().error("LocationDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "LocationDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return location;
            }
        } // namespace DAOs
    } // namespace Catalog
} // namespace ERP