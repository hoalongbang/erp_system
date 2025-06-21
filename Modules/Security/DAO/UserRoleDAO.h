// Modules/Security/DAO/UserRoleDAO.h
#ifndef MODULES_SECURITY_DAO_USERROLEDAO_H
#define MODULES_SECURITY_DAO_USERROLEDAO_H

#include <string>       // For std::string
#include <vector>       // For std::vector
#include <map>          // For std::map
#include <any>          // For std::any
#include <memory>       // For std::shared_ptr
#include <optional>     // For std::optional

// Rút gọn includes
#include "DAOBase.h"    // Base DAO template
// No specific DTO for UserRole typically, as it's a join table (User ID, Role ID)
// However, it often deals with string IDs directly.

namespace ERP {
    namespace Security {
        namespace DAOs {

            /**
             * @brief UserRoleDAO class provides data access operations for user-role relationships.
             * It manages records in a join table that links users to roles (user_id, role_id).
             * This DAO does not use a specific DTO for the join table itself, but operates on maps
             * representing the relationship, or directly on user/role IDs.
             * It inherits from DAOBase to leverage connection pool management, but overrides
             * basic CRUD methods to fit a join table's many-to-many nature.
             */
            class UserRoleDAO : public ERP::DAOBase::DAOBase<void> { // Using void as DTO type since it's a join table
            public:
                /**
                 * @brief Constructs a UserRoleDAO.
                 * @param connectionPool Shared pointer to the ConnectionPool.
                 */
                UserRoleDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);

                /**
                 * @brief Destructor.
                 */
                ~UserRoleDAO() override = default;

                // Overriding DAOBase methods, but they will operate on maps for this join table.
                // Note: 'id' for join tables is typically a composite key, so single-string ID methods might not be intuitive.
                // We override to prevent their default templated behavior for specific use cases.
                bool save(const void& dummy) override { return false; } // Not applicable for join table
                std::optional<void> findById(const std::string& id) override { return std::nullopt; } // Not applicable
                bool update(const void& dummy) override { return false; } // Not applicable
                bool remove(const std::string& id) override; // Overridden to handle specific ID if composite key is serialized
                std::vector<void> findAll() override { return {}; } // Not applicable

                // Overloaded/Specialized CRUD operations for join table
                /**
                 * @brief Creates a new user-role relationship record in the database.
                 * @param data A map representing the data for the new relationship (user_id, role_id).
                 * @return true if the record was created successfully, false otherwise.
                 */
                bool create(const std::map<std::string, std::any>& data); // Manual override, not from DAOBase template

                /**
                 * @brief Reads user-role relationship records from the database based on a filter.
                 * @param filter A map representing the filter conditions (e.g., {"user_id", userId}).
                 * @return A vector of maps, where each map represents a user-role record ({"user_id", userId, "role_id", roleId}).
                 */
                std::vector<std::map<std::string, std::any>> get(const std::map<std::string, std::any>& filter = {}); // Manual override

                /**
                 * @brief Updates user-role relationship records in the database.
                 * For join tables, this typically means deleting existing and creating new.
                 * @param filter A map representing the conditions to select records for update.
                 * @param data A map representing the new values to set for the selected records.
                 * @return true if the records were updated successfully, false otherwise.
                 */
                bool update(const std::map<std::string, std::any>& filter, const std::map<std::string, std::any>& data) override;

                /**
                 * @brief Removes user-role relationship records from the database based on a filter.
                 * @param filter A map representing the conditions to select records for deletion (e.g., {"user_id", userId}, or {"user_id", userId, "role_id", roleId}).
                 * @return true if the records were deleted successfully, false otherwise.
                 */
                bool remove(const std::map<std::string, std::any>& filter) override; // Manual override

                /**
                 * @brief Retrieves a single user-role record from the database based on its ID.
                 * Not typically applicable for composite primary keys, but required by DAOBase.
                 * @param id ID of the user-role record to retrieve.
                 * @return An optional map representing the record, or std::nullopt if not found.
                 */
                std::optional<std::map<std::string, std::any>> getById(const std::string& id) override;

                /**
                 * @brief Counts the number of user-role records matching a filter.
                 * @param filter A map representing the filter conditions.
                 * @return The number of matching records.
                 */
                int count(const std::map<std::string, std::any>& filter = {}) override;

                // Specific methods for managing user-role relationships directly
                /**
                 * @brief Assigns a role to a user.
                 * @param userId ID of the user.
                 * @param roleId ID of the role.
                 * @return true if assignment is successful, false otherwise.
                 */
                bool assignRoleToUser(const std::string& userId, const std::string& roleId);

                /**
                 * @brief Removes a specific role from a user.
                 * @param userId ID of the user.
                 * @param roleId ID of the role to remove.
                 * @return true if removal is successful, false otherwise.
                 */
                bool removeRoleFromUser(const std::string& userId, const std::string& roleId);

                /**
                 * @brief Removes all roles from a specific user.
                 * @param userId ID of the user.
                 * @return true if removal is successful, false otherwise.
                 */
                bool removeAllRolesFromUser(const std::string& userId);


                /**
                 * @brief Retrieves all role IDs assigned to a specific user.
                 * @param userId ID of the user.
                 * @return A vector of role IDs.
                 */
                std::vector<std::string> getRolesByUserId(const std::string& userId);

            private:
                const std::string tableName_ = "user_roles"; /**< Name of the database join table. */
            };

        } // namespace DAOs
    } // namespace Security
} // namespace ERP

#endif // MODULES_SECURITY_DAO_USERROLEDAO_H