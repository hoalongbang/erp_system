// Modules/User/DAO/UserDAO.cpp
#include "Modules/User/DAO/UserDAO.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "EncryptionService.h" // For encryption/decryption
#include "DAOHelpers.h"
#include "Modules/Utils/DTOUtils.h" // For common DTO to map conversions
#include <nlohmann/json.hpp> // For JSON serialization/deserialization
#include <sstream>
#include <stdexcept>
#include <typeinfo> // For std::bad_any_cast

namespace ERP {
namespace User {
namespace DAOs {

using json = nlohmann::json;

UserDAO::UserDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::User::DTO::UserDTO>(connectionPool, "users") { // Pass table name for UserDTO
    Logger::Logger::getInstance().info("UserDAO: Initialized.");
}

// toMap for UserDTO
std::map<std::string, std::any> UserDAO::toMap(const ERP::User::DTO::UserDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["username"] = dto.username;
    data["password_hash"] = dto.passwordHash;
    data["password_salt"] = dto.passwordSalt;
    // Encrypt sensitive fields
    if (dto.email.has_value()) {
        try {
            data["email"] = ERP::Security::Service::EncryptionService::getInstance().encrypt(*dto.email);
        } catch (const std::exception& e) {
            Logger::Logger::getInstance().error("UserDAO: toMap - Failed to encrypt email: " + std::string(e.what()));
            data["email"] = ""; // Store empty on encryption failure
        }
    } else {
        data["email"] = std::any();
    }
    ERP::DAOHelpers::putOptionalString(data, "first_name", dto.firstName);
    ERP::DAOHelpers::putOptionalString(data, "last_name", dto.lastName);
    if (dto.phoneNumber.has_value()) {
        try {
            data["phone_number"] = ERP::Security::Service::EncryptionService::getInstance().encrypt(*dto.phoneNumber);
        } catch (const std::exception& e) {
            Logger::Logger::getInstance().error("UserDAO: toMap - Failed to encrypt phone_number: " + std::string(e.what()));
            data["phone_number"] = ""; // Store empty on encryption failure
        }
    } else {
        data["phone_number"] = std::any();
    }
    data["type"] = static_cast<int>(dto.type);
    data["role_id"] = dto.roleId;
    ERP::DAOHelpers::putOptionalTime(data, "last_login_time", dto.lastLoginTime);
    ERP::DAOHelpers::putOptionalString(data, "last_login_ip", dto.lastLoginIp);
    data["is_locked"] = dto.isLocked;
    data["failed_login_attempts"] = dto.failedLoginAttempts;
    ERP::DAOHelpers::putOptionalTime(data, "lock_until_time", dto.lockUntilTime);
    ERP::DAOHelpers::putOptionalString(data, "profile_id", dto.profileId);

    // Serialize std::map<string, any> to JSON string for metadata
    try {
        if (!dto.metadata.empty()) {
            json metadataJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.metadata));
            data["metadata_json"] = metadataJson.dump();
        } else {
            data["metadata_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("UserDAO: toMap - Error serializing metadata: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "UserDAO: Error serializing metadata.");
        data["metadata_json"] = "";
    }
    return data;
}

// fromMap for UserDTO
ERP::User::DTO::UserDTO UserDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::User::DTO::UserDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "username", dto.username);
        ERP::DAOHelpers::getPlainValue(data, "password_hash", dto.passwordHash);
        ERP::DAOHelpers::getPlainValue(data, "password_salt", dto.passwordSalt);
        // Decrypt sensitive fields
        if (data.count("email") && data.at("email").type() == typeid(std::string)) {
            try {
                dto.email = ERP::Security::Service::EncryptionService::getInstance().decrypt(std::any_cast<std::string>(data.at("email")));
            } catch (const std::exception& e) {
                Logger::Logger::getInstance().error("UserDAO: fromMap - Failed to decrypt email: " + std::string(e.what()));
                dto.email = std::nullopt; // Clear on decryption failure
            }
        } else if (data.count("email") && data.at("email").type() == typeid(std::any) && !data.at("email").has_value()) {
            dto.email = std::nullopt;
        }

        ERP::DAOHelpers::getOptionalStringValue(data, "first_name", dto.firstName);
        ERP::DAOHelpers::getOptionalStringValue(data, "last_name", dto.lastName);
        if (data.count("phone_number") && data.at("phone_number").type() == typeid(std::string)) {
            try {
                dto.phoneNumber = ERP::Security::Service::EncryptionService::getInstance().decrypt(std::any_cast<std::string>(data.at("phone_number")));
            } catch (const std::exception& e) {
                Logger::Logger::getInstance().error("UserDAO: fromMap - Failed to decrypt phone_number: " + std::string(e.what()));
                dto.phoneNumber = std::nullopt; // Clear on decryption failure
            }
        } else if (data.count("phone_number") && data.at("phone_number").type() == typeid(std::any) && !data.at("phone_number").has_value()) {
            dto.phoneNumber = std::nullopt;
        }
        
        int typeInt;
        if (ERP::DAOHelpers::getPlainValue(data, "type", typeInt)) dto.type = static_cast<ERP::User::DTO::UserType>(typeInt);
        
        ERP::DAOHelpers::getPlainValue(data, "role_id", dto.roleId);
        ERP::DAOHelpers::getOptionalTimeValue(data, "last_login_time", dto.lastLoginTime);
        ERP::DAOHelpers::getOptionalStringValue(data, "last_login_ip", dto.lastLoginIp);
        ERP::DAOHelpers::getPlainValue(data, "is_locked", dto.isLocked);
        ERP::DAOHelpers::getPlainValue(data, "failed_login_attempts", dto.failedLoginAttempts);
        ERP::DAOHelpers::getOptionalTimeValue(data, "lock_until_time", dto.lockUntilTime);
        ERP::DAOHelpers::getOptionalStringValue(data, "profile_id", dto.profileId);

        // Deserialize JSON string from metadata
        if (data.count("metadata_json") && data.at("metadata_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("metadata_json"));
            if (!jsonStr.empty()) {
                dto.metadata = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("UserDAO: fromMap - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "UserDAO: Data type mismatch in fromMap.");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("UserDAO: fromMap - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "UserDAO: Unexpected error in fromMap.");
    }
    return dto;
}

// --- UserProfileDTO specific methods and helpers ---
std::map<std::string, std::any> UserDAO::toMap(const ERP::User::DTO::UserProfileDTO& dto) {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["user_id"] = dto.userId;
    ERP::DAOHelpers::putOptionalString(data, "job_title", dto.jobTitle);
    ERP::DAOHelpers::putOptionalString(data, "department", dto.department);
    ERP::DAOHelpers::putOptionalString(data, "employee_id", dto.employeeId);
    ERP::DAOHelpers::putOptionalTime(data, "date_of_birth", dto.dateOfBirth);
    data["gender"] = static_cast<int>(dto.gender);
    ERP::DAOHelpers::putOptionalString(data, "nationality", dto.nationality);
    ERP::DAOHelpers::putOptionalString(data, "language_preference", dto.languagePreference);
    ERP::DAOHelpers::putOptionalString(data, "timezone", dto.timezone);
    ERP::DAOHelpers::putOptionalString(data, "profile_picture_url", dto.profilePictureUrl);

    // Serialize vector of ContactPersonDTO to JSON string
    try {
        json emergencyContactsJson = json::array();
        for (const auto& cp : dto.emergencyContacts) {
            json cpJson;
            cpJson["id"] = cp.id;
            cpJson["first_name"] = cp.firstName;
            if (cp.lastName.has_value()) cpJson["last_name"] = *cp.lastName;
            if (cp.email.has_value()) cpJson["email"] = *cp.email;
            if (cp.phoneNumber.has_value()) cpJson["phone_number"] = *cp.phoneNumber;
            if (cp.position.has_value()) cpJson["position"] = *cp.position;
            cpJson["is_primary"] = cp.isPrimary;
            emergencyContactsJson.push_back(cpJson);
        }
        data["emergency_contacts_json"] = emergencyContactsJson.dump();
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("UserDAO: toMap - Error serializing emergency contacts: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "UserDAO: Error serializing emergency contacts.");
        data["emergency_contacts_json"] = "";
    }

    // Serialize vector of AddressDTO to JSON string
    try {
        json personalAddressesJson = json::array();
        for (const auto& addr : dto.personalAddresses) {
            json addrJson;
            addrJson["id"] = addr.id;
            addrJson["street"] = addr.street;
            addrJson["city"] = addr.city;
            addrJson["state_province"] = addr.stateProvince;
            addrJson["postal_code"] = addr.postalCode;
            addrJson["country"] = addr.country;
            if (addr.addressType.has_value()) addrJson["address_type"] = *addr.addressType;
            addrJson["is_primary"] = addr.isPrimary;
            personalAddressesJson.push_back(addrJson);
        }
        data["personal_addresses_json"] = personalAddressesJson.dump();
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("UserDAO: toMap - Error serializing personal addresses: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "UserDAO: Error serializing personal addresses.");
        data["personal_addresses_json"] = "";
    }

    // Serialize std::map<string, any> to JSON string for custom_fields
    try {
        if (!dto.customFields.empty()) {
            json customFieldsJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.customFields));
            data["custom_fields_json"] = customFieldsJson.dump();
        } else {
            data["custom_fields_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("UserDAO: toMap - Error serializing custom fields: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "UserDAO: Error serializing custom fields.");
        data["custom_fields_json"] = "";
    }
    return data;
}

ERP::User::DTO::UserProfileDTO UserDAO::fromMap(const std::map<std::string, std::any>& data) {
    ERP::User::DTO::UserProfileDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "user_id", dto.userId);
        ERP::DAOHelpers::getOptionalStringValue(data, "job_title", dto.jobTitle);
        ERP::DAOHelpers::getOptionalStringValue(data, "department", dto.department);
        ERP::DAOHelpers::getOptionalStringValue(data, "employee_id", dto.employeeId);
        ERP::DAOHelpers::getOptionalTimeValue(data, "date_of_birth", dto.dateOfBirth);
        
        int genderInt;
        if (ERP::DAOHelpers::getPlainValue(data, "gender", genderInt)) dto.gender = static_cast<ERP::User::DTO::Gender>(genderInt);
        
        ERP::DAOHelpers::getOptionalStringValue(data, "nationality", dto.nationality);
        ERP::DAOHelpers::getOptionalStringValue(data, "language_preference", dto.languagePreference);
        ERP::DAOHelpers::getOptionalStringValue(data, "timezone", dto.timezone);
        ERP::DAOHelpers::getOptionalStringValue(data, "profile_picture_url", dto.profilePictureUrl);

        // Deserialize JSON string to vector of ContactPersonDTO
        if (data.count("emergency_contacts_json") && data.at("emergency_contacts_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("emergency_contacts_json"));
            if (!jsonStr.empty()) {
                try {
                    json contactsJson = json::parse(jsonStr);
                    for (const auto& cpJson : contactsJson) {
                        ERP::DataObjects::ContactPersonDTO cp;
                        ERP::DAOHelpers::getPlainValue(cpJson, "id", cp.id);
                        ERP::DAOHelpers::getPlainValue(cpJson, "first_name", cp.firstName);
                        ERP::DAOHelpers::getOptionalStringValue(cpJson, "last_name", cp.lastName);
                        ERP::DAOHelpers::getOptionalStringValue(cpJson, "email", cp.email);
                        ERP::DAOHelpers::getOptionalStringValue(cpJson, "phone_number", cp.phoneNumber);
                        ERP::DAOHelpers::getOptionalStringValue(cpJson, "position", cp.position);
                        ERP::DAOHelpers::getPlainValue(cpJson, "is_primary", cp.isPrimary);
                        dto.emergencyContacts.push_back(cp);
                    }
                } catch (const std::exception& e) {
                    Logger::Logger::getInstance().error("UserDAO: fromMap - Error deserializing emergency contacts: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "UserDAO: Error deserializing emergency contacts.");
                }
            }
        }

        // Deserialize JSON string to vector of AddressDTO
        if (data.count("personal_addresses_json") && data.at("personal_addresses_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("personal_addresses_json"));
            if (!jsonStr.empty()) {
                try {
                    json addressesJson = json::parse(jsonStr);
                    for (const auto& addrJson : addressesJson) {
                        ERP::DataObjects::AddressDTO addr;
                        ERP::DAOHelpers::getPlainValue(addrJson, "id", addr.id);
                        ERP::DAOHelpers::getPlainValue(addrJson, "street", addr.street);
                        ERP::DAOHelpers::getPlainValue(addrJson, "city", addr.city);
                        ERP::DAOHelpers::getPlainValue(addrJson, "state_province", addr.stateProvince);
                        ERP::DAOHelpers::getPlainValue(addrJson, "postal_code", addr.postalCode);
                        ERP::DAOHelpers::getPlainValue(addrJson, "country", addr.country);
                        ERP::DAOHelpers::getOptionalStringValue(addrJson, "address_type", addr.addressType);
                        ERP::DAOHelpers::getPlainValue(addrJson, "is_primary", addr.isPrimary);
                        dto.personalAddresses.push_back(addr);
                    }
                } catch (const std::exception& e) {
                    Logger::Logger::getInstance().error("UserDAO: fromMap - Error deserializing personal addresses: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "UserDAO: Error deserializing personal addresses.");
                }
            }
        }

        // Deserialize JSON string to std::map<string, any> for custom_fields
        if (data.count("custom_fields_json") && data.at("custom_fields_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("custom_fields_json"));
            if (!jsonStr.empty()) {
                dto.customFields = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }

    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("UserDAO: fromMap (Profile) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "UserDAO: Data type mismatch in fromMap (Profile).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("UserDAO: fromMap (Profile) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "UserDAO: Unexpected error in fromMap (Profile).");
    }
    return dto;
}

bool UserDAO::createUserProfile(const ERP::User::DTO::UserProfileDTO& profile) {
    ERP::Logger::Logger::getInstance().info("UserDAO: Attempting to create new user profile for user: " + profile.userId);
    std::map<std::string, std::any> data = toMap(profile);
    
    std::string columns;
    std::string placeholders;
    std::map<std::string, std::any> params;
    bool first = true;

    for (const auto& pair : data) {
        if (!first) { columns += ", "; placeholders += ", "; }
        columns += pair.first;
        placeholders += "?";
        params[pair.first] = pair.second;
        first = false;
    }

    std::string sql = "INSERT INTO " + userProfilesTableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "UserDAO", "createUserProfile", sql, params
    );
}

std::optional<ERP::User::DTO::UserProfileDTO> UserDAO::getUserProfileById(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("UserDAO: Attempting to get user profile by ID: " + id);
    std::string sql = "SELECT * FROM " + userProfilesTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "UserDAO", "getUserProfileById", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front());
    }
    return std::nullopt;
}

std::optional<ERP::User::DTO::UserProfileDTO> UserDAO::getUserProfileByUserId(const std::string& userId) {
    ERP::Logger::Logger::getInstance().info("UserDAO: Attempting to get user profile by user ID: " + userId);
    std::string sql = "SELECT * FROM " + userProfilesTableName_ + " WHERE user_id = ?;";
    std::map<std::string, std::any> params;
    params["user_id"] = userId;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "UserDAO", "getUserProfileByUserId", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front());
    }
    return std::nullopt;
}


bool UserDAO::updateUserProfile(const ERP::User::DTO::UserProfileDTO& profile) {
    ERP::Logger::Logger::getInstance().info("UserDAO: Attempting to update user profile with ID: " + profile.id);
    std::map<std::string, std::any> data = toMap(profile);
    if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
        ERP::Logger::Logger::getInstance().warning("UserDAO: Update user profile called with empty data or missing ID.");
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "UserDAO: Update user profile called with empty data or missing ID.");
        return false;
    }

    std::string setClause;
    std::map<std::string, std::any> params;
    bool firstSet = true;

    for (const auto& pair : data) {
        if (pair.first == "id") continue;
        if (!firstSet) setClause += ", ";
        setClause += pair.first + " = ?";
        params[pair.first] = pair.second;
        firstSet = false;
    }

    std::string sql = "UPDATE " + userProfilesTableName_ + " SET " + setClause + " WHERE id = ?;";
    params["id_filter"] = profile.id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "UserDAO", "updateUserProfile", sql, params
    );
}

bool UserDAO::removeUserProfile(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("UserDAO: Attempting to remove user profile with ID: " + id);
    std::string sql = "DELETE FROM " + userProfilesTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "UserDAO", "removeUserProfile", sql, params
    );
}

bool UserDAO::removeUserProfileByUserId(const std::string& userId) {
    ERP::Logger::Logger::getInstance().info("UserDAO: Attempting to remove user profile for user ID: " + userId);
    std::string sql = "DELETE FROM " + userProfilesTableName_ + " WHERE user_id = ?;";
    std::map<std::string, std::any> params;
    params["user_id"] = userId;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "UserDAO", "removeUserProfileByUserId", sql, params
    );
}

// --- User Roles specific methods ---
std::vector<std::string> UserDAO::getUserRoleIds(const std::string& userId) {
    ERP::Logger::Logger::getInstance().info("UserDAO: Getting role IDs for user: " + userId);
    std::string sql = "SELECT role_id FROM " + userRolesTableName_ + " WHERE user_id = ?;";
    std::map<std::string, std::any> params;
    params["user_id"] = userId;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "UserDAO", "getUserRoleIds", sql, params
    );

    std::vector<std::string> roleIds;
    for (const auto& row : resultsMap) {
        if (row.count("role_id") && row.at("role_id").type() == typeid(std::string)) {
            roleIds.push_back(std::any_cast<std::string>(row.at("role_id")));
        }
    }
    return roleIds;
}

bool UserDAO::assignUserRole(const std::string& userId, const std::string& roleId) {
    ERP::Logger::Logger::getInstance().info("UserDAO: Assigning role " + roleId + " to user " + userId);
    std::string sql = "INSERT INTO " + userRolesTableName_ + " (user_id, role_id) VALUES (?, ?);";
    std::map<std::string, std::any> params;
    params["user_id"] = userId;
    params["role_id"] = roleId;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "UserDAO", "assignUserRole", sql, params
    );
}

bool UserDAO::removeUserRole(const std::string& userId, const std::string& roleId) {
    ERP::Logger::Logger::getInstance().info("UserDAO: Removing role " + roleId + " from user " + userId);
    std::string sql = "DELETE FROM " + userRolesTableName_ + " WHERE user_id = ? AND role_id = ?;";
    std::map<std::string, std::any> params;
    params["user_id"] = userId;
    params["role_id"] = roleId;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "UserDAO", "removeUserRole", sql, params
    );
}

bool UserDAO::removeAllUserRoles(const std::string& userId) {
    ERP::Logger::Logger::getInstance().info("UserDAO: Removing all roles for user: " + userId);
    std::string sql = "DELETE FROM " + userRolesTableName_ + " WHERE user_id = ?;";
    std::map<std::string, std::any> params;
    params["user_id"] = userId;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "UserDAO", "removeAllUserRoles", sql, params
    );
}

} // namespace DAOs
} // namespace User
} // namespace ERP