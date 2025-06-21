// Modules/User/DTO/UserProfile.h
#ifndef MODULES_USER_DTO_USERPROFILE_H
#define MODULES_USER_DTO_USERPROFILE_H
#include <string>
#include <optional>
#include <chrono>
#include <vector>
#include <map>    // For std::map<string, any>
#include <any>    // For std::any
// #include <QJsonObject> // Removed
// #include <QString>     // Removed
// #include <QDateTime>   // Removed
#include "DataObjects/BaseDTO.h"   // Updated include path
#include "Modules/Common/Common.h>    // For EntityStatus (Updated include path)
#include "Modules/Utils/Utils.h"     // For UUID generation
#include "DataObjects/CommonDTOs/ContactPersonDTO.h" // Use common ContactPersonDTO
#include "DataObjects/CommonDTOs/AddressDTO.h"       // Use common AddressDTO
namespace ERP {
    namespace User {
        namespace DTO {
            /**
             * @brief Enum định nghĩa giới tính của người dùng.
             */
            enum class Gender {
                MALE,
                FEMALE,
                OTHER,
                UNKNOWN
            };
            /**
             * @brief DTO for User Profile entity.
             * Contains additional details about a user that are not part of the core UserDTO.
             */
            struct UserProfileDTO : public BaseDTO {
                std::string userId; // Foreign key to UserDTO
                std::optional<std::string> jobTitle;
                std::optional<std::string> department;
                std::optional<std::string> employeeId; // Internal employee ID
                std::optional<std::chrono::system_clock::time_point> dateOfBirth;
                Gender gender;
                std::optional<std::string> nationality;
                std::optional<std::string> languagePreference;
                std::optional<std::string> timezone;
                std::optional<std::string> profilePictureUrl;
                std::vector<ERP::DataObjects::ContactPersonDTO> emergencyContacts; // Emergency contacts
                std::vector<ERP::DataObjects::AddressDTO> personalAddresses;       // Personal addresses
                std::map<std::string, std::any> customFields; // Flexible custom fields dạng map (thay thế QJsonObject)

                UserProfileDTO() : BaseDTO(), userId(""), gender(Gender::UNKNOWN) {}
                virtual ~UserProfileDTO() = default;

                // Utility methods
                std::string getGenderString() const {
                    switch (gender) {
                        case Gender::MALE: return "Male";
                        case Gender::FEMALE: return "Female";
                        case Gender::OTHER: return "Other";
                        case Gender::UNKNOWN: return "Unknown";
                        default: return "Unknown"; // Should not happen
                    }
                }
            };
        } // namespace DTO
    } // namespace User
} // namespace ERP
#endif // MODULES_USER_DTO_USERPROFILE_H