// Modules/Catalog/DAO/UnitOfMeasureDAO.cpp
#include "UnitOfMeasureDAO.h"
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
            UnitOfMeasureDAO::UnitOfMeasureDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Catalog::DTO::UnitOfMeasureDTO>(connectionPool, "unit_of_measures") { //
                Logger::Logger::getInstance().info("UnitOfMeasureDAO: Initialized.");
            }

            // The 'create' method is now implemented in DAOBase, so this specific implementation is removed.
            // bool UnitOfMeasureDAO::create(const std::map<std::string, std::any>& data) { /* ... */ }

            // The 'get' method is now implemented in DAOBase.
            // std::vector<std::map<std::string, std::any>> UnitOfMeasureDAO::get(const std::map<std::string, std::any>& filter) { /* ... */ }

            // The 'update' method is now implemented in DAOBase.
            // bool UnitOfMeasureDAO::update(const std::map<std::string, std::any>& filter, const std::map<std::string, std::any>& data) { /* ... */ }

            // The 'remove(string id)' method is now implemented in DAOBase.
            // bool UnitOfMeasureDAO::remove(const std::string& id) { /* ... */ }

            // The 'remove(map filter)' method is now implemented in DAOBase.
            // bool UnitOfMeasureDAO::remove(const std::map<std::string, std::any>& filter) { /* ... */ }

            // The 'getById' method is now implemented in DAOBase.
            // std::optional<std::map<std::string, std::any>> UnitOfMeasureDAO::getById(const std::string& id) { /* ... */ }

            // The 'count' method is now implemented in DAOBase.
            // int UnitOfMeasureDAO::count(const std::map<std::string, std::any>& filter) { /* ... */ }

            // Method to convert DTO to map
            std::map<std::string, std::any> UnitOfMeasureDAO::toMap(const ERP::Catalog::DTO::UnitOfMeasureDTO& uom) const { // const for method
                std::map<std::string, std::any> data;
                // BaseDTO fields (using DAOHelpers::putOptionalString/Time)
                data["id"] = uom.id;
                // 'id' is plain std::string
                data["status"] = static_cast<int>(uom.status);
                // 'status' is enum
                data["created_at"] = ERP::Utils::DateUtils::formatDateTime(uom.createdAt, ERP::Common::DATETIME_FORMAT);
                // 'createdAt' is plain time_point
                ERP::DAOHelpers::putOptionalTime(data, "updated_at", uom.updatedAt);
                ERP::DAOHelpers::putOptionalString(data, "created_by", uom.createdBy);
                ERP::DAOHelpers::putOptionalString(data, "updated_by", uom.updatedBy);
                // UnitOfMeasureDTO specific fields
                data["name"] = uom.name;
                // 'name' is plain std::string
                data["symbol"] = uom.symbol;
                // 'symbol' is plain std::string
                ERP::DAOHelpers::putOptionalString(data, "description", uom.description);
                return data;
            }
            ERP::Catalog::DTO::UnitOfMeasureDTO UnitOfMeasureDAO::fromMap(const std::map<std::string, std::any>& data) const { // const for method
                ERP::Catalog::DTO::UnitOfMeasureDTO uom;
                try {
                    // BaseDTO fields
                    // Assuming 'id' is a plain std::string in UnitOfMeasureDTO
                    ERP::DAOHelpers::getPlainValue(data, "id", uom.id);
                    auto statusIt = data.find("status");
                    if (statusIt != data.end() && statusIt->second.type() == typeid(int)) {
                        uom.status = static_cast<ERP::Common::EntityStatus>(std::any_cast<int>(statusIt->second));
                    }
                    else {
                        uom.status = ERP::Common::EntityStatus::UNKNOWN;
                        // Default status if not found or wrong type
                    }
                    // Assuming createdAt is a plain std::chrono::system_clock::time_point
                    // Using DAOHelpers::getPlainTimeValue
                    ERP::DAOHelpers::getPlainTimeValue(data, "created_at", uom.createdAt);
                    ERP::DAOHelpers::getOptionalTimeValue(data, "updated_at", uom.updatedAt);
                    ERP::DAOHelpers::getOptionalStringValue(data, "created_by", uom.createdBy);
                    ERP::DAOHelpers::getOptionalStringValue(data, "updated_by", uom.updatedBy);
                    // UnitOfMeasureDTO-specific fields
                    // Assuming 'name' and 'symbol' are plain std::string
                    ERP::DAOHelpers::getPlainValue(data, "name", uom.name);
                    ERP::DAOHelpers::getPlainValue(data, "symbol", uom.symbol);
                    ERP::DAOHelpers::getOptionalStringValue(data, "description", uom.description);
                }
                catch (const std::bad_any_cast& e) {
                    Logger::Logger::getInstance().error("UnitOfMeasureDAO::fromMap - bad_any_cast: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(
                        ERP::Common::ErrorCode::InvalidInput,
                        "UnitOfMeasureDAO: Data type mismatch in fromMap: " + std::string(e.what())
                    );
                }
                catch (const std::exception& e) {
                    Logger::Logger::getInstance().error("UnitOfMeasureDAO::fromMap - unexpected exception: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(
                        ERP::Common::ErrorCode::OperationFailed,
                        "UnitOfMeasureDAO: Unexpected error in fromMap: " + std::string(e.what())
                    );
                }
                return uom;
            }
        } // namespace DAOs
    } // namespace Catalog
} // namespace ERP