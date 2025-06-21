// Modules/Supplier/DAO/SupplierDAO.h
#ifndef MODULES_SUPPLIER_DAO_SUPPLIERDAO_H
#define MODULES_SUPPLIER_DAO_SUPPLIERDAO_H
#include "DAOBase/DAOBase.h" // Đã thêm
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>
#include "Supplier.h" // Updated include path for SupplierDTO
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "EncryptionService.h" // For encryption/decryption of sensitive fields
#include <nlohmann/json.hpp> // MỚI: Để tái sử dụng ContactPersonDTO và AddressDTO

namespace ERP {
namespace Supplier { // New namespace
namespace DAOs {
/**
 * @brief SupplierDAO class provides data access operations for SupplierDTO objects.
 * It inherits from DAOBase and interacts with the database to manage suppliers.
 * Handles encryption/decryption of sensitive fields like email and phone number.
 * MỚI: Hỗ trợ lưu trữ và truy xuất ContactPersonDTO và AddressDTO dưới dạng JSON.
 */
class SupplierDAO : public DAOBase::DAOBase<ERP::Supplier::DTO::SupplierDTO> { // Kế thừa từ template DAOBase
public:
    /**
     * @brief Constructs a SupplierDAO.
     * @param connectionPool Shared pointer to the ConnectionPool instance.
     */
    explicit SupplierDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool); // Thêm connectionPool

    ~SupplierDAO() override = default;

    // Các phương thức CRUD đã được triển khai trong DAOBase, nên không cần khai báo lại ở đây

protected:
    /**
     * @brief Converts a database data map into a SupplierDTO object.
     * Handles decryption of sensitive fields and parsing of JSON fields for contacts/addresses.
     * This is an override of the pure virtual method in DAOBase.
     * @param data Database data map.
     * @return Converted SupplierDTO object.
     */
    ERP::Supplier::DTO::SupplierDTO fromMap(const std::map<std::string, std::any>& data) const override;
    /**
     * @brief Converts a SupplierDTO object into a data map for database storage.
     * Handles encryption of sensitive fields and serialization of contacts/addresses to JSON.
     * This is an override of the pure virtual method in DAOBase.
     * @param supplier SupplierDTO object.
     * @return Converted data map.
     */
    std::map<std::string, std::any> toMap(const ERP::Supplier::DTO::SupplierDTO& supplier) const override;

private:
    // tableName_ is now a member of DAOBase
};
} // namespace DAOs
} // namespace Supplier
} // namespace ERP
#endif // MODULES_SUPPLIER_DAO_SUPPLIERDAO_H