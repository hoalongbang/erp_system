// Modules/Catalog/DAO/WarehouseDAO.cpp
#include "WarehouseDAO.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
// #include "AutoRelease.h" // Not directly needed here if DAOBase manages connection lifecycle
#include "DAOHelpers.h"  // Ensure DAOHelpers is included for shared helper functions
// IMPORTANT: Ensure this header defines ERP::Common::EntityStatus and includes the UNKNOWN enumerator.
// If EntityStatus is defined in a separate file (e.g., "ERP/Common/EntityStatus.h"), include it here.
// For this fix, we assume EntityStatus is either defined in Common.h or a file included by it,
// and the UNKNOWN enumerator was missing from the enum definition.
// If it's in its own file, you might need: #include "ERP/Common/EntityStatus.h"
#include <sstream>
#include <stdexcept>
#include <optional> // Include for std::optional
#include <any>      // Include for std::any
#include <type_traits> // For std::is_same (used by DAOHelpers)

namespace ERP {
    namespace Catalog {
        namespace DAOs {
            // Updated constructor to call base class constructor
            WarehouseDAO::WarehouseDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Catalog::DTO::WarehouseDTO>(connectionPool, "warehouses") { //
                Logger::Logger::getInstance().info("WarehouseDAO: Initialized.");
            }

            // The 'create' method is now implemented in DAOBase, so this specific implementation is removed.
            // bool WarehouseDAO::create(const std::map<std::string, std::any>& data) { /* ... */ }

            // The 'get' method is now implemented in DAOBase.
            // std::vector<std::map<std::string, std::any>> WarehouseDAO::get(const std::map<std::string, std::any>& filter) { /* ... */ }

            // The 'update' method is now implemented in DAOBase.
            // bool WarehouseDAO::update(const std::map<std::string, std::any>& filter, const std::map<std::string, std::any>& data) { /* ... */ }

            // The 'remove(string id)' method is now implemented in DAOBase.
            // bool WarehouseDAO::remove(const std::string& id) { /* ... */ }

            // The 'remove(map filter)' method is now implemented in DAOBase.
            // bool WarehouseDAO::remove(const std::map<std::string, std::any>& filter) { /* ... */ }

            // The 'getById' method is now implemented in DAOBase.
            // std::optional<std::map<std::string, std::any>> WarehouseDAO::getById(const std::string& id) { /* ... */ }

            // The 'count' method is now implemented in DAOBase.
            // int WarehouseDAO::count(const std::map<std::string, std::any>& filter) { /* ... */ }

            // Method to convert DTO to map
            std::map<std::string, std::any> WarehouseDAO::toMap(const ERP::Catalog::DTO::WarehouseDTO& warehouse) const { // const for method
                std::map<std::string, std::any> data;
                // BaseDTO fields (using DAOHelpers::putOptionalString/Time)
                data["id"] = warehouse.id;
                // 'id' is plain std::string
                data["status"] = static_cast<int>(warehouse.status);
                // 'status' is enum
                data["created_at"] = ERP::Utils::DateUtils::formatDateTime(warehouse.createdAt, ERP::Common::DATETIME_FORMAT);
                // 'createdAt' is plain time_point
                ERP::DAOHelpers::putOptionalTime(data, "updated_at", warehouse.updatedAt);
                ERP::DAOHelpers::putOptionalString(data, "created_by", warehouse.createdBy);
                ERP::DAOHelpers::putOptionalString(data, "updated_by", warehouse.updatedBy);
                // WarehouseDTO specific fields
                data["name"] = warehouse.name;
                // 'name' is plain std::string
                ERP::DAOHelpers::putOptionalString(data, "location", warehouse.location);
                ERP::DAOHelpers::putOptionalString(data, "contact_person", warehouse.contactPerson);
                ERP::DAOHelpers::putOptionalString(data, "contact_phone", warehouse.contactPhone);
                ERP::DAOHelpers::putOptionalString(data, "email", warehouse.email);
                return data;
            }
            // Method to convert map to DTO
            ERP::Catalog::DTO::WarehouseDTO WarehouseDAO::fromMap(const std::map<std::string, std::any>& data) const { // const for method
                ERP::Catalog::DTO::WarehouseDTO warehouse;
                try {
                    // BaseDTO fields
                    ERP::DAOHelpers::getPlainValue(data, "id", warehouse.id);
                    auto statusIt = data.find("status");
                    if (statusIt != data.end() && statusIt->second.type() == typeid(int)) {
                        warehouse.status = static_cast<ERP::Common::EntityStatus>(std::any_cast<int>(statusIt->second));
                    }
                    else {
                        // Corrected: Ensure ERP::Common::EntityStatus::UNKNOWN is defined.
                        warehouse.status = ERP::Common::EntityStatus::UNKNOWN; // Default status if not found or wrong type
                    }
                    // Assuming createdAt is a plain std::chrono::system_clock::time_point
                    ERP::DAOHelpers::getPlainTimeValue(data, "created_at", warehouse.createdAt);
                    // Optional fields
                    ERP::DAOHelpers::getOptionalTimeValue(data, "updated_at", warehouse.updatedAt);
                    ERP::DAOHelpers::getOptionalStringValue(data, "created_by", warehouse.createdBy);
                    ERP::DAOHelpers::getOptionalStringValue(data, "updated_by", warehouse.updatedBy);
                    // WarehouseDTO specific fields
                    ERP::DAOHelpers::getPlainValue(data, "name", warehouse.name);
                    ERP::DAOHelpers::getOptionalStringValue(data, "location", warehouse.location);
                    ERP::DAOHelpers::getOptionalStringValue(data, "contact_person", warehouse.contactPerson);
                    ERP::DAOHelpers::getOptionalStringValue(data, "contact_phone", warehouse.contactPhone);
                    ERP::DAOHelpers::getOptionalStringValue(data, "email", warehouse.email);
                }
                catch (const std::bad_any_cast& e) {
                    Logger::Logger::getInstance().error("WarehouseDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "WarehouseDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    Logger::Logger::getInstance().error("WarehouseDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "WarehouseDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return warehouse;
            }
        } // namespace DAOs
    } // namespace Catalog
} // namespace ERP