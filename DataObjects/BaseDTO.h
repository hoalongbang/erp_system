// DataObjects/BaseDTO.h
#ifndef DATAOBJECTS_BASEDTO_H
#define DATAOBJECTS_BASEDTO_H
#include <string>       // For std::string
#include <optional>     // For std::optional
#include <chrono>       // For std::chrono::system_clock::time_point
#include "Common.h" // For EntityStatus (Updated include path)
// #include "Utils.h" // For UUID generation (if needed for default ID) - Đã bỏ include này vì UUID thường tạo ở Service/DAO, không phải BaseDTO 

namespace ERP {
namespace DataObjects {
/**
 * @brief Base Data Transfer Object (DTO) for common fields.
 * All specific DTOs will inherit from this class to ensure consistency
 * in common fields like ID, status, creation/update timestamps and user IDs.
 */
struct BaseDTO {
    std::string id;
    ERP::Common::EntityStatus status = ERP::Common::EntityStatus::ACTIVE;
    std::chrono::system_clock::time_point createdAt;
    std::optional<std::chrono::system_clock::time_point> updatedAt;
    std::optional<std::string> createdBy;
    std::optional<std::string> updatedBy;
    // Default constructor to initialize createdAt to current time
    BaseDTO() : createdAt(std::chrono::system_clock::now()) {}
    // Virtual destructor for proper polymorphic cleanup
    virtual ~BaseDTO() = default;
};
} // namespace DataObjects
} // namespace ERP
#endif // DATAOBJECTS_BASEDTO_H