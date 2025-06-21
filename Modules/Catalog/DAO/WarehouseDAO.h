// Modules/Catalog/DAO/WarehouseDAO.h
#ifndef MODULES_CATALOG_DAO_WAREHOUSEDAO_H
#define MODULES_CATALOG_DAO_WAREHOUSEDAO_H
#include "DAOBase/DAOBase.h" // Đã thêm 
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

#include "Warehouse.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"

namespace ERP {
namespace Catalog {
namespace DAOs {
/**
 * @brief WarehouseDAO class provides data access operations for WarehouseDTO objects.
 * It inherits from DAOBase and interacts with the database to manage warehouses.
 */
class WarehouseDAO : public DAOBase::DAOBase {
public:
    /**
     * @brief Constructs a WarehouseDAO.
     * No longer takes shared_ptr<Database::DBConnection> as DAOBase manages the pool.
     */
    WarehouseDAO();
    /**
     * @brief Creates a new warehouse record in the database.
     * @param data A map representing the data for the new warehouse record.
     * @return true if the record was created successfully, false otherwise.
     */
    bool create(const std::map<std::string, std::any>& data) override;
    /**
     * @brief Reads warehouse records from the database based on a filter.
     * @param filter A map representing the filter conditions.
     * @return A vector of maps, where each map represents a warehouse record.
     */
    std::vector<std::map<std::string, std::any>> get(const std::map<std::string, std::any>& filter = {}) override;
    /**
     * @brief Updates existing warehouse records in the database.
     * @param filter A map representing the conditions to select records for update.
     * @param data A map representing the new values to set for the selected records.
     * @return true if the records were updated successfully, false otherwise.
     */
    bool update(const std::map<std::string, std::any>& filter, const std::map<std::string, std::any>& data) override;
    /**
     * @brief Deletes a warehouse by ID.
     * @param id ID of the warehouse record to delete.
     * @return true if the record was deleted successfully, false otherwise.
     */
    bool remove(const std::string& id) override;
    /**
     * @brief Deletes warehouse records from the database based on a filter.
     * @param filter A map representing the conditions to select records for deletion.
     * @return true if the records were deleted successfully, false otherwise.
     */
    bool remove(const std::map<std::string, std::any>& filter) override;
    /**
     * @brief Retrieves a single warehouse record from the database based on its ID.
     * @param id ID of the warehouse record to retrieve.
     * @return An optional map representing the record, or std::nullopt if not found.
     */
    std::optional<std::map<std::string, std::any>> getById(const std::string& id) override;
    /**
     * @brief Counts the number of warehouse records matching a filter.
     * @param filter A map representing the filter conditions.
     * @return The number of matching records.
     */
    int count(const std::map<std::string, std::any>& filter = {}) override;
    /**
     * @brief Converts a database data map into a WarehouseDTO object.
     * @param data Database data map.
     * @return Converted WarehouseDTO object.
     */
    static ERP::Catalog::DTO::WarehouseDTO fromMap(const std::map<std::string, std::any>& data);
    /**
     * @brief Converts a WarehouseDTO object into a data map for database storage.
     * @param warehouse WarehouseDTO object.
     * @return Converted data map.
     */
    static std::map<std::string, std::any> toMap(const ERP::Catalog::DTO::WarehouseDTO& warehouse);
private:
    const std::string tableName_ = "warehouses"; /**< Name of the database table. */
};
} // namespace DAOs
} // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_DAO_WAREHOUSEDAO_H