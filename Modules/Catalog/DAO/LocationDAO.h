// Modules/Catalog/DAO/LocationDAO.h
#ifndef MODULES_CATALOG_DAO_LOCATIONDAO_H
#define MODULES_CATALOG_DAO_LOCATIONDAO_H
#include "DAOBase.h" // Đã thêm 
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

#include "Location.h" // Updated include path
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h" 
#include "DateUtils.h"

namespace ERP {
namespace Catalog {
namespace DAOs {
/**
 * @brief LocationDAO class provides data access operations for LocationDTO objects.
 * It inherits from DAOBase and interacts with the database to manage storage locations within warehouses.
 */
class LocationDAO : public DAOBase::DAOBase {
public:
    /**
     * @brief Constructs a LocationDAO.
     * No longer takes shared_ptr<Database::DBConnection> as DAOBase manages the pool.
     */
    LocationDAO();
    /**
     * @brief Creates a new location record in the database.
     * @param data A map representing the data for the new location record.
     * @return true if the record was created successfully, false otherwise.
     */
    bool create(const std::map<std::string, std::any>& data) override;
    /**
     * @brief Reads location records from the database based on a filter.
     * @param filter A map representing the filter conditions.
     * @return A vector of maps, where each map represents a location record.
     */
    std::vector<std::map<std::string, std::any>> get(const std::map<std::string, std::any>& filter = {}) override;
    /**
     * @brief Updates existing location records in the database.
     * @param filter A map representing the conditions to select records for update.
     * @param data A map representing the new values to set for the selected records.
     * @return true if the records were updated successfully, false otherwise.
     */
    bool update(const std::map<std::string, std::any>& filter, const std::map<std::string, std::any>& data) override;
    /**
     * @brief Deletes a location by ID.
     * @param id ID of the location record to delete.
     * @return true if the record was deleted successfully, false otherwise.
     */
    bool remove(const std::string& id) override;
    /**
     * @brief Deletes location records from the database based on a filter.
     * @param filter A map representing the conditions to select records for deletion.
     * @return true if the records were deleted successfully, false otherwise.
     */
    bool remove(const std::map<std::string, std::any>& filter) override;
    /**
     * @brief Retrieves a single location record from the database based on its ID.
     * @param id ID of the location record to retrieve.
     * @return An optional map representing the record, or std::nullopt if not found.
     */
    std::optional<std::map<std::string, std::any>> getById(const std::string& id) override;
    /**
     * @brief Counts the number of location records matching a filter.
     * @param filter A map representing the filter conditions.
     * @return The number of matching records.
     */
    int count(const std::map<std::string, std::any>& filter = {}) override;
    /**
     * @brief Converts a database data map into a LocationDTO object.
     * @param data Database data map.
     * @return Converted LocationDTO object.
     */
    static ERP::Catalog::DTO::LocationDTO fromMap(const std::map<std::string, std::any>& data);
    /**
     * @brief Converts a LocationDTO object into a data map for database storage.
     * @param location LocationDTO object.
     * @return Converted data map.
     */
    static std::map<std::string, std::any> toMap(const ERP::Catalog::DTO::LocationDTO& location);
private:
    const std::string tableName_ = "locations"; /**< Name of the database table. */
};
} // namespace DAOs
} // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_DAO_LOCATIONDAO_H