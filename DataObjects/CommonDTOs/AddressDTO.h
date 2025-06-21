// DataObjects/CommonDTOs/AddressDTO.h
#ifndef DATAOBJECTS_COMMONDTOS_ADDRESS_H
#define DATAOBJECTS_COMMONDTOS_ADDRESS_H
#include <string>
#include <optional>
#include "Utils.h" // For UUID generation 

namespace ERP {
namespace DataObjects {
/**
 * @brief DTO for Address entity, used across Customer and Supplier modules.
 * Represents a physical address associated with an organization.
 */
struct AddressDTO {
    std::string id;
    std::string street;
    std::string city;
    std::string stateProvince;
    std::string postalCode;
    std::string country;
    std::optional<std::string> addressType; // e.g., "Shipping", "Billing", "Main"
    bool isPrimary = false;     // Indicates if this is the primary address

    // Default constructor to generate UUID
    AddressDTO() : id(ERP::Utils::generateUUID()) {} // 
};
} // namespace DataObjects
} // namespace ERP
#endif // DATAOBJECTS_COMMONDTOS_ADDRESS_H