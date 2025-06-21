// Modules/User/Service/UserService.cpp
#include "UserService.h" // Standard includes
#include "User.h"               // User DTO
#include "UserProfile.h"        // UserProfile DTO
#include "Event.h"              // Event DTO
#include "ConnectionPool.h"     // ConnectionPool
#include "DBConnection.h"       // DBConnection
#include "Common.h"             // Common Enums/Constants
#include "Utils.h"              // Utility functions
#include "DateUtils.h"          // Date utility functions
#include "AutoRelease.h"        // RAII helper
#include "PasswordHasher.h"     // PasswordHasher utility
#include "ISecurityManager.h"   // Security Manager interface
#include "RoleService.h"        // RoleService (for role validation)
#include "UserDAO.h"            // UserDAO
#include "UserProfileDAO.h"     // UserProfileDAO
#include "UserRoleDAO.h"        // NEW: UserRoleDAO

#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed

namespace ERP {
    namespace User {
        namespace Services {

            UserService::UserService(
                std::shared_ptr<DAOs::UserDAO> userDAO,
                std::shared_ptr<DAOs::UserProfileDAO> userProfileDAO, // Optional
                std::shared_ptr<ERP::Security::DAOs::UserRoleDAO> userRoleDAO, // NEW
                std::shared_ptr<ERP::Catalog::Services::IRoleService> roleService,
                std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
                : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Initialize BaseService
                userDAO_(userDAO),
                userProfileDAO_(userProfileDAO), // Initialize optional dependency
                userRoleDAO_(userRoleDAO), // NEW
                roleService_(roleService) { // Initialize specific dependency

                // Basic dependency checks for non-optional services
                if (!userDAO_ || !roleService_ || !securityManager_) { // BaseService checks its own dependencies
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "UserService: Initialized with null DAO or core dependent services (UserDAO, RoleService, SecurityManager).", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ người dùng.");
                    ERP::Logger::Logger::getInstance().critical("UserService: One or more injected DAOs/Services are null.");
                    throw std::runtime_error("UserService: Null dependencies.");
                }
                // userProfileDAO_ is optional, so it can be null.
                // userRoleDAO_ is NEW and also crucial, so let's check it.
                if (!userRoleDAO_) {
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "UserService: UserRoleDAO is null.", "Lỗi hệ thống: Dịch vụ vai trò người dùng không khả dụng.");
                    ERP::Logger::Logger::getInstance().critical("UserService: Injected UserRoleDAO is null.");
                    throw std::runtime_error("UserService: Null UserRoleDAO dependency.");
                }

                ERP::Logger::Logger::getInstance().info("UserService: Initialized.");
            }

            // Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

            std::optional<ERP::User::DTO::UserDTO> UserService::createUser(
                const ERP::User::DTO::UserDTO& userDTO,
                const std::string& password,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("UserService: Attempting to create user: " + userDTO.username + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "User.CreateUser", "Bạn không có quyền tạo người dùng.")) {
                    return std::nullopt;
                }

                // 1. Validate input DTO
                if (userDTO.username.empty() || password.empty() || userDTO.roleId.empty()) {
                    ERP::Logger::Logger::getInstance().warning("UserService: Invalid input for user creation (empty username, password, or role ID).");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "UserService: Invalid input for user creation.", "Tên người dùng, mật khẩu hoặc ID vai trò không được để trống.");
                    return std::nullopt;
                }

                // Check if username already exists
                std::map<std::string, std::any> filterByUsername;
                filterByUsername["username"] = userDTO.username;
                if (userDAO_->countUsers(filterByUsername) > 0) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().warning("UserService: User with username " + userDTO.username + " already exists.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "UserService: User with username " + userDTO.username + " already exists.", "Tên người dùng đã tồn tại. Vui lòng chọn tên khác.");
                    return std::nullopt;
                }

                // Validate Role existence
                std::optional<ERP::Catalog::DTO::RoleDTO> role = roleService_->getRoleById(userDTO.roleId, userRoleIds);
                if (!role || role->status != ERP::Common::EntityStatus::ACTIVE) {
                    ERP::Logger::Logger::getInstance().warning("UserService: Invalid Role ID provided or role is not active: " + userDTO.roleId);
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID vai trò không hợp lệ hoặc vai trò không hoạt động.");
                    return std::nullopt;
                }

                ERP::User::DTO::UserDTO newUser = userDTO;
                newUser.id = ERP::Utils::generateUUID();
                newUser.passwordSalt = ERP::Security::Utils::PasswordHasher::generateSalt();
                newUser.passwordHash = ERP::Security::Utils::PasswordHasher::hashPassword(password, newUser.passwordSalt);
                newUser.createdAt = ERP::Utils::DateUtils::now();
                newUser.createdBy = currentUserId;
                newUser.status = ERP::Common::EntityStatus::ACTIVE; // Default status
                newUser.isLocked = false;
                newUser.failedLoginAttempts = 0;

                std::optional<ERP::User::DTO::UserDTO> createdUser = std::nullopt;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!userDAO_->create(newUser)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("UserService: Failed to create user in DAO.");
                            return false;
                        }
                        // Create a default user profile if userProfileDAO is available
                        if (userProfileDAO_) {
                            ERP::User::DTO::UserProfileDTO defaultProfile;
                            defaultProfile.id = ERP::Utils::generateUUID();
                            defaultProfile.userId = newUser.id;
                            defaultProfile.createdAt = newUser.createdAt;
                            defaultProfile.createdBy = newUser.createdBy;
                            defaultProfile.status = ERP::Common::EntityStatus::ACTIVE;
                            if (!userProfileDAO_->create(defaultProfile)) { // Specific DAO method
                                ERP::Logger::Logger::getInstance().warning("UserService: Failed to create default user profile for " + newUser.username);
                                // This is not a critical error for user creation, so don't return false, just log.
                            }
                        }
                        createdUser = newUser;
                        // Optionally, publish event
                        eventBus_.publish(std::make_shared<EventBus::UserCreatedEvent>(newUser.id, newUser.username));
                        return true;
                    },
                    "UserService", "createUser"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("UserService: User " + newUser.username + " created successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                        "User", "UserAccount", newUser.id, "User", newUser.username,
                        std::nullopt, newUser.toMap(), "User account created.");
                    return createdUser;
                }
                return std::nullopt;
            }

            std::optional<ERP::User::DTO::UserDTO> UserService::getUserById(
                const std::string& userId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().debug("UserService: Retrieving user by ID: " + userId + ".");

                // Check permission: User can view self, or Admin can view any user
                if (userId != currentUserId && !checkPermission(currentUserId, userRoleIds, "User.ViewUsers", "Bạn không có quyền xem người dùng này.")) {
                    return std::nullopt;
                }
                return userDAO_->getUserById(userId); // Specific DAO method
            }

            std::optional<ERP::User::DTO::UserDTO> UserService::getUserByUsername(
                const std::string& username,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().debug("UserService: Retrieving user by username: " + username + ".");

                // No direct permission check needed here if this is used internally by Auth.
                // If exposed to UI for general search, then permission is needed.
                // For now, assume it's for internal use or permission is checked by caller.
                std::map<std::string, std::any> filter;
                filter["username"] = username;
                std::vector<ERP::User::DTO::UserDTO> users = userDAO_->getUsers(filter); // Specific DAO method
                if (!users.empty()) {
                    return users[0];
                }
                return std::nullopt;
            }

            std::vector<ERP::User::DTO::UserDTO> UserService::getAllUsers(
                const std::map<std::string, std::any>& filter,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("UserService: Retrieving all users with filter.");

                if (!checkPermission(currentUserId, userRoleIds, "User.ViewUsers", "Bạn không có quyền xem tất cả người dùng.")) {
                    return {};
                }
                return userDAO_->getUsers(filter); // Specific DAO method
            }

            bool UserService::updateUser(
                const ERP::User::DTO::UserDTO& userDTO,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("UserService: Attempting to update user: " + userDTO.id + " by " + currentUserId + ".");

                // Check permission: User can update self, or Admin can update any user
                if (userDTO.id != currentUserId && !checkPermission(currentUserId, userRoleIds, "User.UpdateUser", "Bạn không có quyền cập nhật người dùng này.")) {
                    return false;
                }

                std::optional<ERP::User::DTO::UserDTO> oldUserOpt = userDAO_->getUserById(userDTO.id); // Specific DAO method
                if (!oldUserOpt) {
                    ERP::Logger::Logger::getInstance().warning("UserService: User with ID " + userDTO.id + " not found for update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy người dùng cần cập nhật.");
                    return false;
                }

                // If username is changed, check for uniqueness
                if (userDTO.username != oldUserOpt->username) {
                    std::map<std::string, std::any> filterByUsername;
                    filterByUsername["username"] = userDTO.username;
                    if (userDAO_->countUsers(filterByUsername) > 0) { // Specific DAO method
                        ERP::Logger::Logger::getInstance().warning("UserService: New username " + userDTO.username + " already exists.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "UserService: New username " + userDTO.username + " already exists.", "Tên người dùng mới đã tồn tại. Vui lòng chọn tên khác.");
                        return false;
                    }
                }

                // Validate Role existence if roleId is changed
                if (userDTO.roleId != oldUserOpt->roleId) { // Only validate if changed
                    std::optional<ERP::Catalog::DTO::RoleDTO> role = roleService_->getRoleById(userDTO.roleId, userRoleIds);
                    if (!role || role->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("UserService: Invalid Role ID provided or role is not active: " + userDTO.roleId);
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID vai trò không hợp lệ hoặc vai trò không hoạt động.");
                        return false;
                    }
                }

                ERP::User::DTO::UserDTO updatedUser = userDTO;
                updatedUser.updatedAt = ERP::Utils::DateUtils::now();
                updatedUser.updatedBy = currentUserId;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!userDAO_->update(updatedUser)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("UserService: Failed to update user " + updatedUser.id + " in DAO.");
                            return false;
                        }
                        // Optionally, publish event
                        eventBus_.publish(std::make_shared<EventBus::UserUpdatedEvent>(updatedUser.id, updatedUser.username));
                        return true;
                    },
                    "UserService", "updateUser"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("UserService: User " + updatedUser.id + " updated successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                        "User", "UserAccount", updatedUser.id, "User", updatedUser.username,
                        oldUserOpt->toMap(), updatedUser.toMap(), "User account updated.");
                    return true;
                }
                return false;
            }

            bool UserService::updateUserStatus(
                const std::string& userId,
                ERP::Common::EntityStatus newStatus,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("UserService: Attempting to update status for user: " + userId + " to " + ERP::Common::entityStatusToString(newStatus) + " by " + currentUserId + ".");

                // Check permission: Admin can update any user's status, or user can change self status (if applicable)
                if (userId != currentUserId && !checkPermission(currentUserId, userRoleIds, "User.UpdateUser", "Bạn không có quyền cập nhật trạng thái người dùng này.")) {
                    return false;
                }

                std::optional<ERP::User::DTO::UserDTO> userOpt = userDAO_->getUserById(userId); // Specific DAO method
                if (!userOpt) {
                    ERP::Logger::Logger::getInstance().warning("UserService: User with ID " + userId + " not found for status update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy người dùng để cập nhật trạng thái.");
                    return false;
                }

                ERP::User::DTO::UserDTO oldUser = *userOpt;
                if (oldUser.status == newStatus) {
                    ERP::Logger::Logger::getInstance().info("UserService: User " + userId + " is already in status " + ERP::Common::entityStatusToString(newStatus) + ".");
                    return true; // Already in desired status
                }

                // Add state transition validation logic here
                // For example: Cannot change from DELETED to ACTIVE directly without restore process.

                ERP::User::DTO::UserDTO updatedUser = oldUser;
                updatedUser.status = newStatus;
                updatedUser.updatedAt = ERP::Utils::DateUtils::now();
                updatedUser.updatedBy = currentUserId;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!userDAO_->update(updatedUser)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("UserService: Failed to update status for user " + userId + " in DAO.");
                            return false;
                        }
                        // If user status changes to INACTIVE or DELETED, also deactivate their sessions
                        if (newStatus == ERP::Common::EntityStatus::INACTIVE || newStatus == ERP::Common::EntityStatus::DELETED) {
                            if (securityManager_->getSessionService()) { // Check if session service is available
                                securityManager_->getSessionService()->deactivateSessionsByUserId(userId, currentUserId, userRoleIds);
                            }
                        }
                        // Optionally, publish event for status change
                        eventBus_.publish(std::make_shared<EventBus::UserStatusChangedEvent>(userId, newStatus));
                        return true;
                    },
                    "UserService", "updateUserStatus"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("UserService: Status for user " + userId + " updated successfully to " + ERP::Common::entityStatusToString(newStatus) + ".");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                        "User", "UserStatus", userId, "User", oldUser.username,
                        oldUser.toMap(), updatedUser.toMap(), "User status changed to " + ERP::Common::entityStatusToString(newStatus) + ".");
                    return true;
                }
                return false;
            }

            bool UserService::deleteUser(
                const std::string& userId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("UserService: Attempting to delete user: " + userId + " by " + currentUserId + ".");

                // Check permission: Admin can delete any user
                if (!checkPermission(currentUserId, userRoleIds, "User.DeleteUser", "Bạn không có quyền xóa người dùng này.")) {
                    return false;
                }
                // Prevent self-deletion
                if (userId == currentUserId) {
                    ERP::Logger::Logger::getInstance().warning("UserService: User " + currentUserId + " attempted to delete self.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::Forbidden, "Bạn không thể xóa tài khoản của chính mình.");
                    return false;
                }

                std::optional<ERP::User::DTO::UserDTO> userOpt = userDAO_->getUserById(userId); // Specific DAO method
                if (!userOpt) {
                    ERP::Logger::Logger::getInstance().warning("UserService: User with ID " + userId + " not found for deletion.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy người dùng cần xóa.");
                    return false;
                }

                ERP::User::DTO::UserDTO userToDelete = *userOpt;

                // Additional checks: Prevent deletion if user has active sessions, or other associated data
                // This requires cross-module service dependencies
                if (securityManager_->getSessionService() && securityManager_->getSessionService()->countActiveSessionsByUserId(userId, currentUserId, userRoleIds) > 0) {
                    ERP::Logger::Logger::getInstance().warning("UserService: Cannot delete user " + userId + " as they have active sessions.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa người dùng có phiên đăng nhập đang hoạt động.");
                    return false;
                }
                // Further checks could involve: associated orders (Sales, Production), created documents, etc.

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        // Remove associated user profile (if userProfileDAO is available)
                        if (userProfileDAO_ && userProfileDAO_->deleteProfileByUserId(userId)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().info("UserService: User profile for " + userId + " deleted.");
                        }
                        // Remove all associated roles from user_roles join table
                        if (userRoleDAO_->removeAllRolesFromUser(userId)) { // NEW
                            ERP::Logger::Logger::getInstance().info("UserService: All additional roles for user " + userId + " removed.");
                        }
                        if (!userDAO_->remove(userId)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("UserService: Failed to delete user " + userId + " in DAO.");
                            return false;
                        }
                        return true;
                    },
                    "UserService", "deleteUser"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("UserService: User " + userId + " deleted successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                        "User", "UserAccount", userId, "User", userToDelete.username,
                        userToDelete.toMap(), std::nullopt, "User account deleted.");
                    eventBus_.publish(std::make_shared<EventBus::UserDeletedEvent>(userId, userToDelete.username)); // Assuming UserDeletedEvent exists
                    return true;
                }
                return false;
            }

            bool UserService::changePassword(
                const std::string& userId,
                const std::string& newPassword,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("UserService: Attempting to change password for user: " + userId + " by " + currentUserId + ".");

                // Check permission: User can change own password, or Admin can change any password
                if (userId != currentUserId && !checkPermission(currentUserId, userRoleIds, "User.ChangeAnyPassword", "Bạn không có quyền thay đổi mật khẩu người dùng này.")) {
                    return false;
                }

                // Validate newPassword complexity if required (e.g., minimum length, complexity rules)
                if (newPassword.empty() || newPassword.length() < 6) { // Example rule
                    ERP::Logger::Logger::getInstance().warning("UserService: New password for user " + userId + " is too short.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Mật khẩu mới quá ngắn. Vui lòng chọn mật khẩu dài hơn.");
                    return false;
                }

                std::optional<ERP::User::DTO::UserDTO> userOpt = userDAO_->getUserById(userId); // Specific DAO method
                if (!userOpt) {
                    ERP::Logger::Logger::getInstance().warning("UserService: User with ID " + userId + " not found for password change.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy người dùng để thay đổi mật khẩu.");
                    return false;
                }

                ERP::User::DTO::UserDTO userToUpdate = *userOpt;
                std::string oldPasswordHash = userToUpdate.passwordHash; // For audit logging

                userToUpdate.passwordSalt = ERP::Security::Utils::PasswordHasher::generateSalt(); // Generate new salt
                userToUpdate.passwordHash = ERP::Security::Utils::PasswordHasher::hashPassword(newPassword, userToUpdate.passwordSalt); // Hash new password
                userToUpdate.updatedAt = ERP::Utils::DateUtils::now();
                userToUpdate.updatedBy = currentUserId;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!userDAO_->update(userToUpdate)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("UserService: Failed to change password for user " + userId + " in DAO.");
                            return false;
                        }
                        // Optionally, invalidate all existing sessions for this user (force re-login)
                        if (securityManager_->getSessionService()) { // Check if session service is available
                            securityManager_->getSessionService()->deactivateSessionsByUserId(userId, currentUserId, userRoleIds);
                        }
                        return true;
                    },
                    "UserService", "changePassword"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("UserService: Password for user " + userId + " changed successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::PASSWORD_CHANGE, ERP::Common::LogSeverity::INFO,
                        "User", "UserPassword", userId, "User", userToUpdate.username,
                        std::map<std::string, std::any>{{"old_password_hash", oldPasswordHash}}, // Log old hash (sensitive)
                        std::map<std::string, std::any>{{"new_password_hash", userToUpdate.passwordHash}}, // Log new hash (sensitive)
                        "User password changed.");
                    return true;
                }
                return false;
            }

            std::vector<std::string> UserService::getUserRoles(
                const std::string& userId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("UserService: Getting all roles for user ID: " + userId + ".");

                // Check permission: User can view self roles, or Admin can view any user's roles
                if (userId != currentUserId && !checkPermission(currentUserId, userRoleIds, "User.ViewUsers", "Bạn không có quyền xem vai trò của người dùng này.")) {
                    return {};
                }

                std::vector<std::string> roles;
                std::optional<ERP::User::DTO::UserDTO> userOpt = userDAO_->getUserById(userId);
                if (!userOpt) {
                    ERP::Logger::Logger::getInstance().warning("UserService: User " + userId + " not found when getting roles.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Người dùng không tồn tại.");
                    return {};
                }

                // Add primary role from UserDTO
                roles.push_back(userOpt->roleId);

                // Add additional roles from user_roles join table
                std::vector<std::string> additionalRoles = userRoleDAO_->getAdditionalRolesByUserId(userId); // NEW
                roles.insert(roles.end(), additionalRoles.begin(), additionalRoles.end());

                // Ensure uniqueness if a role might be in both primary and additional (though should be designed to avoid)
                std::sort(roles.begin(), roles.end());
                roles.erase(std::unique(roles.begin(), roles.end()), roles.end());

                ERP::Logger::Logger::getInstance().info("UserService: Retrieved " + std::to_string(roles.size()) + " roles for user " + userId + ".");
                return roles;
            }

            std::string UserService::getUserName(const std::string& userId) {
                std::optional<ERP::User::DTO::UserDTO> userOpt = userDAO_->getUserById(userId);
                if (userOpt) {
                    return userOpt->username;
                }
                return "N/A"; // Or throw, or return "System" for system actions
            }

            // User Profile operations
            std::optional<ERP::User::DTO::UserProfileDTO> UserService::getUserProfile(
                const std::string& userId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().debug("UserService: Retrieving user profile for user: " + userId + ".");

                // Check permission: User can view own profile, or Admin can view any profile
                if (userId != currentUserId && !checkPermission(currentUserId, userRoleIds, "User.ViewUsers", "Bạn không có quyền xem hồ sơ người dùng này.")) {
                    return std::nullopt;
                }

                if (!userProfileDAO_) {
                    ERP::Logger::Logger::getInstance().error("UserService: UserProfileDAO is null. Cannot retrieve profile.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "User profile service not available.", "Dịch vụ hồ sơ người dùng không khả dụng.");
                    return std::nullopt;
                }

                return userProfileDAO_->getProfileByUserId(userId); // Specific DAO method
            }

            bool UserService::updateUserProfile(
                const ERP::User::DTO::UserProfileDTO& userProfileDTO,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("UserService: Attempting to update user profile for user: " + userProfileDTO.userId + " by " + currentUserId + ".");

                // Check permission: User can update own profile, or Admin can update any profile
                if (userProfileDTO.userId != currentUserId && !checkPermission(currentUserId, userRoleIds, "User.UpdateUser", "Bạn không có quyền cập nhật hồ sơ người dùng này.")) {
                    return false;
                }

                if (!userProfileDAO_) {
                    ERP::Logger::Logger::getInstance().error("UserService: UserProfileDAO is null. Cannot update profile.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "User profile service not available.", "Dịch vụ hồ sơ người dùng không khả dụng.");
                    return false;
                }

                std::optional<ERP::User::DTO::UserProfileDTO> oldProfileOpt = userProfileDAO_->getProfileByUserId(userProfileDTO.userId);
                if (!oldProfileOpt) {
                    ERP::Logger::Logger::getInstance().warning("UserService: User profile for user ID " + userProfileDTO.userId + " not found for update. Attempting to create.");
                    // If profile doesn't exist, try to create it.
                    ERP::User::DTO::UserProfileDTO newProfile = userProfileDTO;
                    newProfile.id = ERP::Utils::generateUUID(); // New ID for new profile
                    newProfile.createdAt = ERP::Utils::DateUtils::now();
                    newProfile.createdBy = currentUserId;
                    newProfile.status = ERP::Common::EntityStatus::ACTIVE;

                    bool success = executeTransaction(
                        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                            return userProfileDAO_->create(newProfile); // Specific DAO method
                        },
                        "UserService", "createUserProfile"
                    );
                    if (success) {
                        ERP::Logger::Logger::getInstance().info("UserService: New user profile created for user " + userProfileDTO.userId + ".");
                        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                            ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                            "User", "UserProfile", newProfile.id, "UserProfile", newProfile.userId,
                            std::nullopt, newProfile.toMap(), "User profile created (was missing).");
                    }
                    else {
                        ERP::Logger::Logger::getInstance().error("UserService: Failed to create user profile for user ID " + userProfileDTO.userId + ".");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể tạo hồ sơ người dùng.");
                    }
                    return success; // Return result of create attempt
                }

                ERP::User::DTO::UserProfileDTO updatedProfile = userProfileDTO;
                updatedProfile.updatedAt = ERP::Utils::DateUtils::now();
                updatedProfile.updatedBy = currentUserId;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        return userProfileDAO_->update(updatedProfile); // Specific DAO method
                    },
                    "UserService", "updateUserProfile"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("UserService: User profile for user " + userProfileDTO.userId + " updated successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                        "User", "UserProfile", updatedProfile.id, "UserProfile", updatedProfile.userId,
                        oldProfileOpt->toMap(), updatedProfile.toMap(), "User profile updated.");
                    return true;
                }
                return false;
            }

            // NEW: Additional Roles Management Implementations

            bool UserService::assignAdditionalRoleToUser(
                const std::string& userId,
                const std::string& roleId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("UserService: Attempting to assign additional role " + roleId + " to user " + userId + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "User.ManageRoles", "Bạn không có quyền quản lý vai trò của người dùng.")) {
                    return false;
                }

                // Validate User existence
                if (!getUserById(userId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().warning("UserService: User " + userId + " not found for role assignment.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Người dùng không tồn tại.");
                    return false;
                }

                // Validate Role existence
                std::optional<ERP::Catalog::DTO::RoleDTO> role = roleService_->getRoleById(roleId, userRoleIds);
                if (!role || role->status != ERP::Common::EntityStatus::ACTIVE) {
                    ERP::Logger::Logger::getInstance().warning("UserService: Invalid Role ID provided or role is not active: " + roleId);
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID vai trò không hợp lệ hoặc vai trò không hoạt động.");
                    return false;
                }

                // Check if role is already assigned (primary or additional)
                std::vector<std::string> existingRoles = getUserRoles(userId, userRoleIds); // Gets all roles (primary + additional)
                if (std::find(existingRoles.begin(), existingRoles.end(), roleId) != existingRoles.end()) {
                    ERP::Logger::Logger::getInstance().warning("UserService: Role " + roleId + " is already assigned to user " + userId + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Vai trò đã được gán cho người dùng này.");
                    return true; // Already assigned, consider it successful
                }

                // If the roleId is the user's primary role, don't add as "additional"
                std::optional<ERP::User::DTO::UserDTO> userOpt = userDAO_->getUserById(userId);
                if (userOpt && userOpt->roleId == roleId) {
                    ERP::Logger::Logger::getInstance().warning("UserService: Role " + roleId + " is the primary role for user " + userId + ". Not adding as additional.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Vai trò này đã là vai trò chính của người dùng.");
                    return true;
                }

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!userRoleDAO_->assignRoleToUser(userId, roleId)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("UserService: Failed to assign additional role " + roleId + " to user " + userId + " in DAO.");
                            return false;
                        }
                        return true;
                    },
                    "UserService", "assignAdditionalRoleToUser"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("UserService: Additional role " + roleId + " assigned to user " + userId + " successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::PERMISSION_CHANGE, ERP::Common::LogSeverity::INFO,
                        "User", "UserRoleAssignment", userId, "User", getUserName(userId),
                        std::map<std::string, std::any>{{"role_id", roleId, "action", "assigned"}}, // old values for audit
                        std::map<std::string, std::any>{{"role_id", roleId, "action", "assigned"}}, // new values for audit
                        "Assigned additional role " + role->name + " to user."); // Note: should query role name
                    eventBus_.publish(std::make_shared<EventBus::UserRoleChangedEvent>(userId, roleId, true)); // Assuming UserRoleChangedEvent exists
                    return true;
                }
                return false;
            }

            bool UserService::removeAdditionalRoleFromUser(
                const std::string& userId,
                const std::string& roleId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("UserService: Attempting to remove additional role " + roleId + " from user " + userId + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "User.ManageRoles", "Bạn không có quyền quản lý vai trò của người dùng.")) {
                    return false;
                }

                // Validate User existence
                if (!getUserById(userId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().warning("UserService: User " + userId + " not found for role removal.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Người dùng không tồn tại.");
                    return false;
                }

                // Prevent removing the user's primary role via this method
                std::optional<ERP::User::DTO::UserDTO> userOpt = userDAO_->getUserById(userId);
                if (userOpt && userOpt->roleId == roleId) {
                    ERP::Logger::Logger::getInstance().warning("UserService: Cannot remove primary role " + roleId + " from user " + userId + " via removeAdditionalRoleFromUser.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::Forbidden, "Không thể xóa vai trò chính của người dùng bằng phương thức này.");
                    return false;
                }

                // Check if the role is actually assigned as an additional role
                std::vector<std::string> additionalRoles = userRoleDAO_->getRolesByUserId(userId); // Use getRolesByUserId which gets all from join table
                if (std::find(additionalRoles.begin(), additionalRoles.end(), roleId) == additionalRoles.end()) {
                    ERP::Logger::Logger::getInstance().warning("UserService: Role " + roleId + " is not an additional role for user " + userId + ". No action needed.");
                    return true; // Not assigned as additional role, consider it successful
                }


                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!userRoleDAO_->removeRoleFromUser(userId, roleId)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("UserService: Failed to remove additional role " + roleId + " from user " + userId + " in DAO.");
                            return false;
                        }
                        return true;
                    },
                    "UserService", "removeAdditionalRoleFromUser"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("UserService: Additional role " + roleId + " removed from user " + userId + " successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::PERMISSION_CHANGE, ERP::Common::LogSeverity::INFO,
                        "User", "UserRoleAssignment", userId, "User", getUserName(userId),
                        std::map<std::string, std::any>{{"role_id", roleId, "action", "removed"}}, // old values for audit
                        std::map<std::string, std::any>{{"role_id", roleId, "action", "removed"}}, // new values for audit
                        "Removed additional role " + roleId + " from user.");
                    eventBus_.publish(std::make_shared<EventBus::UserRoleChangedEvent>(userId, roleId, false)); // Assuming UserRoleChangedEvent exists
                    return true;
                }
                return false;
            }

            std::vector<std::string> UserService::getAdditionalRolesByUserId(
                const std::string& userId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("UserService: Getting additional roles for user ID: " + userId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "User.ViewUsers", "Bạn không có quyền xem vai trò bổ sung của người dùng.")) {
                    return {};
                }

                // Validate User existence
                if (!getUserById(userId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().warning("UserService: User " + userId + " not found when getting additional roles.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Người dùng không tồn tại.");
                    return {};
                }

                // Get all roles from the join table
                std::vector<std::string> additionalRoles = userRoleDAO_->getRolesByUserId(userId);

                // Remove the primary role if it exists in the list (as this method is for *additional* roles)
                std::optional<ERP::User::DTO::UserDTO> userOpt = userDAO_->getUserById(userId);
                if (userOpt) {
                    std::string primaryRoleId = userOpt->roleId;
                    additionalRoles.erase(std::remove(additionalRoles.begin(), additionalRoles.end(), primaryRoleId), additionalRoles.end());
                }

                ERP::Logger::Logger::getInstance().info("UserService: Retrieved " + std::to_string(additionalRoles.size()) + " additional roles for user " + userId + ".");
                return additionalRoles;
            }

        } // namespace Services
    } // namespace User
} // namespace ERP