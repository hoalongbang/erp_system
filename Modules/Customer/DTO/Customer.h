// Modules/Customer/DTO/Customer.h
#ifndef MODULES_CUSTOMER_DTO_CUSTOMER_H
#define MODULES_CUSTOMER_DTO_CUSTOMER_H
#include <string>
#include <vector>
#include <optional>
#include <chrono> // For std::chrono::system_clock::time_point

#include "DataObjects/BaseDTO.h" // Đã sửa: Dùng tên tệp trực tiếp
#include "Common.h"  // Đã sửa: Dùng tên tệp trực tiếp
#include "Modules/Utils/Utils.h"   // Đã sửa: Dùng tên tệp trực tiếp (chứ không phải Utils.h)
// NEW: Include common DTOs 
#include "DataObjects/CommonDTOs/ContactPersonDTO.h" // 
#include "DataObjects/CommonDTOs/AddressDTO.h"       // 

using ERP::DataObjects::BaseDTO;
// ✅ Rút gọn tên class cơ sở

namespace ERP {
namespace Customer { // Namespace for Customer module
namespace DTO {
// ĐÃ BỎ: Định nghĩa ContactPersonDTO đã được chuyển đến DataObjects/CommonDTOs/ContactPersonDTO.h 
// ĐÃ BỎ: Định nghĩa AddressDTO đã được chuyển đến DataObjects/CommonDTOs/AddressDTO.h 
/**
 * @brief DTO for Customer entity.
 */
struct CustomerDTO : public BaseDTO {
    std::string name;
    std::optional<std::string> contactPerson; // Old single contact field, will be deprecated
    std::optional<std::string> email; // Old single email field, will be deprecated
    std::optional<std::string> phoneNumber; // Old single phone field, will be deprecated
    std::optional<std::string> address; // Old single address field, will be deprecated
    std::optional<std::string> taxId;
    std::optional<std::string> notes;
    std::optional<std::string> defaultPaymentTerms; // MỚI: Điều khoản thanh toán mặc định
    std::optional<double> creditLimit; // MỚI: Hạn mức tín dụng
    std::vector<ERP::DataObjects::ContactPersonDTO> contactPersons; // MỚI: Danh sách người liên hệ 
    std::vector<ERP::DataObjects::AddressDTO> addresses; // MỚI: Danh sách địa chỉ 
    // Default constructor to initialize BaseDTO and specific fields
    CustomerDTO() : BaseDTO() {}
    // Virtual destructor for proper polymorphic cleanup
    virtual ~CustomerDTO() = default;
};
} // namespace DTO
} // namespace Customer
} // namespace ERP
#endif // MODULES_CUSTOMER_DTO_CUSTOMER_H