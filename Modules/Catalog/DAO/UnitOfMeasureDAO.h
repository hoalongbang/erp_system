// Modules/Catalog/DAO/UnitOfMeasureDAO.h
#ifndef MODULES_CATALOG_DAO_UNITOFMEASUREDAO_H
#define MODULES_CATALOG_DAO_UNITOFMEASUREDAO_H
#include "DAOBase/DAOBase.h" // Đã thêm 
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

#include "UnitOfMeasure.h" 
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"

namespace ERP {
namespace Catalog {
namespace DAOs {
/**
 * @brief UnitOfMeasureDAO class provides data access operations for UnitOfMeasureDTO objects.
 * It inherits from DAOBase and interacts with the database to manage units of measure.
 */
class UnitOfMeasureDAO : public DAOBase::DAOBase {
public:
    /**
     * @brief Constructs a UnitOfMeasureDAO.
     * No longer takes shared_ptr<Database::DBConnection> as DAOBase manages the pool.
     */
    UnitOfMeasureDAO();
    /**
     * @brief Creates a new unit of measure record in the database.
     * @param data A map representing the data for the new unit of measure record.
     * @return true if the record was created successfully, false otherwise.
     */
    bool create(const std::map<std::string, std::any>& data) override;
    /**
     * @brief Reads unit of measure records from the database based on a filter.
     * @param filter A map representing the filter conditions.
     * @return A vector of maps, where each map represents a unit of measure record.
     */
    std::vector<std::map<std::string, std::any>> get(const std::map<std::string, std::any>& filter = {}) override;
    /**
     * @brief Updates existing unit of measure records in the database.
     * @param filter A map representing the conditions to select records for update.
     * @param data A map representing the new values to set for the selected records.
     * @return true if the records were updated successfully, false otherwise.
     */
    bool update(const std::map<std::string, std::any>& filter, const std::map<std::string, std::any>& data) override;
    /**
     * @brief Deletes a unit of measure by ID.
     * @param id ID of the unit of measure record to delete.
     * @return true if the record was deleted successfully, false otherwise.
     */
    bool remove(const std::string& id) override;
    /**
     * @brief Deletes unit of measure records from the database based on a filter.
     * @param filter A map representing the conditions to select records for deletion.
     * @return true if the records were deleted successfully, false otherwise.
     */
    bool remove(const std::map<std::string, std::any>& filter) override;
    /**
     * @brief Retrieves a single unit of measure record from the database based on its ID.
     * @param id ID of the unit of measure record to retrieve.
     * @return An optional map representing the record, or std::nullopt if not found.
     */
    std::optional<std::map<std::string, std::any>> getById(const std::string& id) override;
    /**
     * @brief Counts the number of unit of measure records matching a filter.
     * @param filter A map representing the filter conditions.
     * @return The number of matching records.
     */
    int count(const std::map<std::string, std::any>& filter = {}) override;
    /**
     * @brief Converts a database data map into a UnitOfMeasureDTO object.
     * @param data Database data map.
     * @return Converted UnitOfMeasureDTO object.
     */
    static ERP::Catalog::DTO::UnitOfMeasureDTO fromMap(const std::map<std::string, std::any>& data);
    /**
     * @brief Converts a UnitOfMeasureDTO object into a data map for database storage.
     * @param uom UnitOfMeasureDTO object.
     * @return Converted data map.
     */
    static std::map<std::string, std::any> toMap(const ERP::Catalog::DTO::UnitOfMeasureDTO& uom);
private:
    const std::string tableName_ = "unit_of_measures"; /**< Name of the database table. */
};
} // namespace DAOs
} // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_DAO_UNITOFMEASUREDAO_H