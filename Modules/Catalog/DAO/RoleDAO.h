// Modules/Catalog/DAO/RoleDAO.h
#ifndef MODULES_CATALOG_DAO_ROLEDAO_H
#define MODULES_CATALOG_DAO_ROLEDAO_H

#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

#include "DAOBase.h" // Đã thêm 
#include "Role.h" // Updated include path
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"

namespace ERP {
namespace Catalog {
namespace DAOs {
/**
 * @brief RoleDAO class provides data access operations for RoleDTO objects.
 * It inherits from DAOBase and interacts with the database to manage roles.
 */
class RoleDAO : public DAOBase::DAOBase {
public:
    /**
     * @brief Constructs a RoleDAO.
     * No longer takes shared_ptr<Database::DBConnection> as DAOBase manages the pool.
     */
    RoleDAO();
    /**
     * @brief Creates a new role record in the database.
     * @param data A map representing the data for the new role record.
     * @return true if the record was created successfully, false otherwise.
     */
    bool create(const std::map<std::string, std::any>& data) override;
    /**
     * @brief Reads role records from the database based on a filter.
     * @param filter A map representing the filter conditions.
     * @return A vector of maps, where each map represents a role record.
     */
    std::vector<std::map<std::string, std::any>> get(const std::map<std::string, std::any>& filter = {}) override;
    /**
     * @brief Updates existing role records in the database.
     * @param filter A map representing the conditions to select records for update.
     * @param data A map representing the new values to set for the selected records.
     * @return true if the records were updated successfully, false otherwise.
     */
    bool update(const std::map<std::string, std::any>& filter, const std::map<std::string, std::any>& data) override;
    /**
     * @brief Deletes a role by ID.
     * @param id ID of the role record to delete.
     * @return true if the record was deleted successfully, false otherwise.
     */
    bool remove(const std::string& id) override;
    /**
     * @brief Deletes role records from the database based on a filter.
     * @param filter A map representing the conditions to select records for deletion.
     * @return true if the records were deleted successfully, false otherwise.
     */
    bool remove(const std::map<std::string, std::any>& filter) override;
    /**
     * @brief Retrieves a single role record from the database based on its ID.
     * @param id ID of the role record to retrieve.
     * @return An optional map representing the record, or std::nullopt if not found.
     */
    std::optional<std::map<std::string, std::any>> getById(const std::string& id) override;
    /**
     * @brief Counts the number of role records matching a filter.
     * @param filter A map representing the filter conditions.
     * @return The number of matching records.
     */
    int count(const std::map<std::string, std::any>& filter = {}) override;
    /**
     * @brief Converts a database data map into a RoleDTO object.
     * @param data Database data map.
     * @return Converted RoleDTO object.
     */
    static ERP::Catalog::DTO::RoleDTO fromMap(const std::map<std::string, std::any>& data);
    /**
     * @brief Converts a RoleDTO object into a data map for database storage.
     * @param role RoleDTO object.
     * @return Converted data map.
     */
    static std::map<std::string, std::any> toMap(const ERP::Catalog::DTO::RoleDTO& role);
    /**
     * @brief Lấy tất cả các quyền được gán cho một vai trò từ bảng role_permissions.
     * @param roleId ID của vai trò.
     * @return Vector các map đại diện cho các bản ghi quyền của vai trò.
     */
    std::vector<std::map<std::string, std::any>> getRolePermissions(const std::string& roleId);

    /**
     * @brief Thêm một quyền vào một vai trò trong bảng role_permissions.
     * @param roleId ID của vai trò.
     * @param permissionName Tên của quyền (ví dụ: "Sales.CreateOrder").
     * @return True nếu thành công, false nếu ngược lại.
     */
    bool addRolePermission(const std::string& roleId, const std::string& permissionName);
    /**
     * @brief Xóa một quyền khỏi một vai trò trong bảng role_permissions.
     * @param roleId ID của vai trò.
     * @param permissionName Tên của quyền.
     * Nếu rỗng, xóa tất cả quyền của vai trò đó.
     * @return True nếu thành công, false nếu ngược lại.
     */
    bool removeRolePermission(const std::string& roleId, const std::string& permissionName);
private:
    const std::string tableName_ = "roles"; /**< Name of the database table. */
    const std::string rolePermissionsTableName_ = "role_permissions"; /**< Name of the role_permissions join table. */
};
} // namespace DAOs
} // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_DAO_ROLEDAO_H