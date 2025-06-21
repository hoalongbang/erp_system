// Modules/User/DTO/User.h
#ifndef MODULES_USER_DTO_USER_H
#define MODULES_USER_DTO_USER_H
#include <string>
#include <optional>
#include <chrono>
#include <vector> // For nested DTOs like ContactPersonDTO, AddressDTO if user has them
#include "DataObjects/BaseDTO.h"   // ĐÃ SỬA: Dùng tên tệp trực tiếp
#include "Modules/Common/Common.h"    // ĐÃ SỬA: Dùng tên tệp trực tiếp
#include "Modules/Utils/Utils.h"     // ĐÃ SỬA: Dùng tên tệp trực tiếp
#include "DataObjects/CommonDTOs/ContactPersonDTO.h"  // Assuming ContactPersonDTO and AddressDTO are defined here, if user has contact/address info
#include "DataObjects/CommonDTOs/AddressDTO.h"
#include <map> // For std::map<string, any> metadata
#include <any> // For std::any metadata
namespace ERP {
    namespace User {
        namespace DTO {
            enum class UserType {
                ADMIN,
                EMPLOYEE,
                CUSTOMER_PORTAL, // For external customer logins
                SUPPLIER_PORTAL, // For external supplier logins
                OTHER,
                UNKNOWN
            };
            /**
             * @brief DTO for User entity.
             */
            struct UserDTO : public BaseDTO {
                std::string username;
                std::string passwordHash;
                std::string passwordSalt; // For password hashing
                std::optional<std::string> email;
                std::optional<std::string> firstName;
                std::optional<std::string> lastName;
                std::optional<std::string> phoneNumber;
                UserType type; // Admin, Employee, CustomerPortal, SupplierPortal, Other
                std::string roleId; // Foreign key to RoleDTO
                std::optional<std::chrono::system_clock::time_point> lastLoginTime;
                std::optional<std::string> lastLoginIp;
                bool isLocked; // True if account is locked due to multiple failed login attempts
                int failedLoginAttempts; // Counter for failed login attempts
                std::optional<std::chrono::system_clock::time_point> lockUntilTime; // Time until account is locked
                std::optional<std::string> profileId; // MỚI: Liên kết đến UserProfileDTO (nếu có)
                std::map<std::string, std::any> metadata; // MỚI: Dữ liệu bổ sung dạng map

                UserDTO() : BaseDTO(), username(""), passwordHash(""), passwordSalt(""),
                            type(UserType::UNKNOWN), roleId(""), isLocked(false), failedLoginAttempts(0) {}
                virtual ~UserDTO() = default;

                // Utility methods
                std::string getTypeString() const {
                    switch (type) {
                        case UserType::ADMIN: return "Admin";
                        case UserType::EMPLOYEE: return "Employee";
                        case UserType::CUSTOMER_PORTAL: return "Customer Portal";
                        case UserType::SUPPLIER_PORTAL: return "Supplier Portal";
                        case UserType::OTHER: return "Other";
                        case UserType::UNKNOWN: return "Unknown";
                        default: return "Unknown"; // Should not happen
                    }
                }
            };
        } // namespace DTO
    } // namespace User
} // namespace ERP
#endif // MODULES_USER_DTO_USER_H