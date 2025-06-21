// Modules/User/DAO/UserDAO.h
#ifndef MODULES_USER_DAO_USERDAO_H
#define MODULES_USER_DAO_USERDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/User/DTO/User.h" // For DTOs
#include "Modules/User/DTO/UserProfile.h" // For UserProfileDTO
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "EncryptionService.h" // For encryption/decryption of sensitive fields
#include "Modules/Utils/DTOUtils.h" // For DTO conversion helpers
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <any>
#include <optional>
#include <nlohmann/json.hpp> // For JSON serialization/deserialization

namespace ERP {
namespace User {
namespace DAOs {

// UserDAO handles two DTOs (User and UserProfile).
// It will inherit from DAOBase for UserDTO, and
// have specific methods for UserProfileDTO.

class UserDAO : public ERP::DAOBase::DAOBase<ERP::User::DTO::UserDTO> {
public:
    explicit UserDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~UserDAO() override = default;

    // Override toMap and fromMap for UserDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::User::DTO::UserDTO& dto) const override;
    ERP::User::DTO::UserDTO fromMap(const std::map<std::string, std::any>& data) const override;

public:
    // Specific methods for UserProfileDTO
    bool createUserProfile(const ERP::User::DTO::UserProfileDTO& profile);
    std::optional<ERP::User::DTO::UserProfileDTO> getUserProfileById(const std::string& id);
    std::optional<ERP::User::DTO::UserProfileDTO> getUserProfileByUserId(const std::string& userId);
    bool updateUserProfile(const ERP::User::DTO::UserProfileDTO& profile);
    bool removeUserProfile(const std::string& id);
    bool removeUserProfileByUserId(const std::string& userId);

    // Helpers for UserProfileDTO conversion (static because not part of templated base)
    static std::map<std::string, std::any> toMap(const ERP::User::DTO::UserProfileDTO& dto);
    static ERP::User::DTO::UserProfileDTO fromMap(const std::map<std::string, std::any>& data);

    // Specific methods for user roles (join table)
    std::vector<std::string> getUserRoleIds(const std::string& userId);
    bool assignUserRole(const std::string& userId, const std::string& roleId);
    bool removeUserRole(const std::string& userId, const std::string& roleId);
    bool removeAllUserRoles(const std::string& userId); // Remove all roles for a user

private:
    std::string userProfilesTableName_ = "user_profiles";
    std::string userRolesTableName_ = "user_roles";
};

} // namespace DAOs
} // namespace User
} // namespace ERP
#endif // MODULES_USER_DAO_USERDAO_H