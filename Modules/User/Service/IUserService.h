// Modules/User/Service/IUserService.h
#ifndef MODULES_USER_SERVICE_IUSERSERVICE_H
#define MODULES_USER_SERVICE_IUSERSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "BaseService.h"        // Base Service
#include "User.h"               // DTO
#include "UserProfile.h"        // DTO

namespace ERP {
    namespace User {
        namespace Services {

            /**
             * @brief IUserService interface defines operations for managing user accounts and profiles.
             */
            class IUserService {
            public:
                virtual ~IUserService() = default;
                /**
                 * @brief Creates a new user account.
                 * @param userDTO DTO containing new user information.
                 * @param password Plain text password for the new user.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return An optional UserDTO if creation is successful, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::User::DTO::UserDTO> createUser(
                    const ERP::User::DTO::UserDTO& userDTO,
                    const std::string& password,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Retrieves user information by ID.
                 * @param userId ID of the user to retrieve.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return An optional UserDTO if found, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::User::DTO::UserDTO> getUserById(
                    const std::string& userId,
                    const std::vector<std::string>& userRoleIds = {}) = 0;
                /**
                 * @brief Retrieves user information by username.
                 * @param username Username to retrieve.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return An optional UserDTO if found, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::User::DTO::UserDTO> getUserByUsername(
                    const std::string& username,
                    const std::vector<std::string>& userRoleIds = {}) = 0;
                /**
                 * @brief Retrieves all users or users matching a filter.
                 * @param filter Map of filter conditions.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return Vector of matching UserDTOs.
                 */
                virtual std::vector<ERP::User::DTO::UserDTO> getAllUsers(
                    const std::map<std::string, std::any>& filter = {},
                    const std::vector<std::string>& userRoleIds = {}) = 0;
                /**
                 * @brief Updates user information (excluding password).
                 * @param userDTO DTO containing updated user information (must have ID).
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if update is successful, false otherwise.
                 */
                virtual bool updateUser(
                    const ERP::User::DTO::UserDTO& userDTO,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Updates the status of a user account.
                 * @param userId ID of the user.
                 * @param newStatus New status.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if status update is successful, false otherwise.
                 */
                virtual bool updateUserStatus(
                    const std::string& userId,
                    ERP::Common::EntityStatus newStatus,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Deletes a user account record by ID (soft delete).
                 * @param userId ID of the user to delete.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if deletion is successful, false otherwise.
                 */
                virtual bool deleteUser(
                    const std::string& userId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Changes a user's password.
                 * @param userId ID of the user whose password is to be changed.
                 * @param newPassword The new plain text password.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if password change is successful, false otherwise.
                 */
                virtual bool changePassword(
                    const std::string& userId,
                    const std::string& newPassword,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;

                /**
                 * @brief Retrieves all role IDs assigned to a specific user (including the primary role from UserDTO).
                 * @param userId ID of the user.
                 * @param userRoleIds Roles of the user performing the operation (for permission checks).
                 * @return A vector of role IDs.
                 */
                virtual std::vector<std::string> getUserRoles(
                    const std::string& userId,
                    const std::vector<std::string>& userRoleIds = {}) = 0;

                /**
                 * @brief Retrieves the username for a given user ID.
                 * @param userId The ID of the user.
                 * @return The username string, or "N/A" if not found.
                 */
                virtual std::string getUserName(const std::string& userId) = 0;

                // User Profile operations
                /**
                 * @brief Retrieves user profile information by user ID.
                 * @param userId ID of the user whose profile to retrieve.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return An optional UserProfileDTO if found, std::nullopt otherwise.
                 */
                virtual std::optional<ERP::User::DTO::UserProfileDTO> getUserProfile(
                    const std::string& userId,
                    const std::vector<std::string>& userRoleIds = {}) = 0;
                /**
                 * @brief Updates a user's profile.
                 * @param userProfileDTO DTO containing updated profile information (must have user ID).
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if update is successful, false otherwise.
                 */
                virtual bool updateUserProfile(
                    const ERP::User::DTO::UserProfileDTO& userProfileDTO,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;

                // NEW: Methods for managing additional roles via user_roles join table
                /**
                 * @brief Assigns an additional role to a user.
                 * @param userId ID of the user.
                 * @param roleId ID of the role to assign.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if assignment is successful, false otherwise.
                 */
                virtual bool assignAdditionalRoleToUser(
                    const std::string& userId,
                    const std::string& roleId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Removes an additional role from a user.
                 * @param userId ID of the user.
                 * @param roleId ID of the role to remove.
                 * @param currentUserId ID of the user performing the operation.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return true if removal is successful, false otherwise.
                 */
                virtual bool removeAdditionalRoleFromUser(
                    const std::string& userId,
                    const std::string& roleId,
                    const std::string& currentUserId,
                    const std::vector<std::string>& userRoleIds) = 0;
                /**
                 * @brief Retrieves all additional role IDs assigned to a specific user (from the join table).
                 * @param userId ID of the user.
                 * @param userRoleIds Roles of the user performing the operation.
                 * @return A vector of additional role IDs.
                 */
                virtual std::vector<std::string> getAdditionalRolesByUserId(
                    const std::string& userId,
                    const std::vector<std::string>& userRoleIds = {}) = 0;
            };

        } // namespace Services
    } // namespace User
} // namespace ERP
#endif // MODULES_USER_SERVICE_IUSERSERVICE_H