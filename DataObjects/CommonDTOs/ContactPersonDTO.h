// DataObjects/CommonDTOs/ContactPersonDTO.h
#ifndef DATAOBJECTS_COMMONDTOS_CONTACTPERSON_H
#define DATAOBJECTS_COMMONDTOS_CONTACTPERSON_H
#include <string>
#include <optional>
#include "Utils.h" // For UUID generation 

namespace ERP {
namespace DataObjects {
/**
 * @brief DTO for Contact Person entity, used across Customer and Supplier modules.
 * Represents a contact person associated with an organization.
 */
struct ContactPersonDTO {
    std::string id;
    std::string firstName;
    std::optional<std::string> lastName;
    std::optional<std::string> email;
    std::optional<std::string> phoneNumber;
    std::optional<std::string> position;
    bool isPrimary = false; // Indicates if this is the primary contact

    // Default constructor to generate UUID
    ContactPersonDTO() : id(ERP::Utils::generateUUID()) {} // 
};
} // namespace DataObjects
} // namespace ERP
#endif // DATAOBJECTS_COMMONDTOS_CONTACTPERSON_H