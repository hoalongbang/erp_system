// Modules/Supplier/DTO/Supplier.h
#ifndef MODULES_SUPPLIER_DTO_SUPPLIER_H
#define MODULES_SUPPLIER_DTO_SUPPLIER_H
#include <string>
#include <vector>
#include <optional>
#include <chrono> // For std::chrono::system_clock::time_point
#include <map>    // For std::map<string, any> for JSON data

#include "DataObjects/BaseDTO.h"   // ĐÃ SỬA: Dùng tên tệp trực tiếp
#include "Modules/Common/Common.h"  // ĐÃ SỬA: Dùng tên tệp trực tiếp
#include "Modules/Utils/Utils.h"   // ĐÃ SỬA: Dùng tên tệp trực tiếp
// NEW: Include common DTOs
#include "DataObjects/CommonDTOs/ContactPersonDTO.h"
#include "DataObjects/CommonDTOs/AddressDTO.h"

using ERP::DataObjects::BaseDTO;
// ✅ Rút gọn tên class cơ sở
namespace ERP {
namespace Supplier { // Namespace for Supplier module
namespace DTO {

/**
 * @brief DTO for Supplier entity.
 */
struct SupplierDTO : public BaseDTO {
    std::string name;
    // Old fields, deprecated, removed in DB schema change (DatabaseInitializer.cpp)
    // std::optional<std::string> contactPerson;
    // std::optional<std::string> email;
    // std::optional<std::string> phoneNumber;
    // std::optional<std::string> address;

    std::optional<std::string> taxId;
    std::optional<std::string> notes;
    std::optional<std::string> defaultPaymentTerms;    // Điều khoản thanh toán mặc định
    std::optional<std::string> defaultDeliveryTerms;   // Điều khoản giao hàng mặc định

    std::vector<ERP::DataObjects::ContactPersonDTO> contactPersons; // MỚI: Danh sách người liên hệ
    std::vector<ERP::DataObjects::AddressDTO> addresses;            // MỚI: Danh sách địa chỉ

    // Default constructor to initialize BaseDTO and specific fields
    SupplierDTO() : BaseDTO() {}
    // Virtual destructor for proper polymorphic cleanup
    virtual ~SupplierDTO() = default;
};
} // namespace DTO
} // namespace Supplier
} // namespace ERP
#endif // MODULES_SUPPLIER_DTO_SUPPLIER_H