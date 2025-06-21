// Modules/Customer/DAO/CustomerDAO.h
#ifndef MODULES_CUSTOMER_DAO_CUSTOMERDAO_H
#define MODULES_CUSTOMER_DAO_CUSTOMERDAO_H
#include "DAOBase/DAOBase.h" // Đã thêm
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>
#include "Customer.h" // Updated include path for CustomerDTO
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "EncryptionService.h" // For encryption/decryption of sensitive fields
#include <nlohmann/json.hpp> // MỚI: Để tái sử dụng ContactPersonDTO và AddressDTO
namespace ERP {
namespace Customer { // New namespace
namespace DAOs {
/**
 * @brief CustomerDAO class provides data access operations for CustomerDTO objects.
 * It inherits from DAOBase and interacts with the database to manage customers.
 * Handles encryption/decryption of sensitive fields like email and phone number.
 * MỚI: Hỗ trợ lưu trữ và truy xuất ContactPersonDTO và AddressDTO dưới dạng JSON.
 */
class CustomerDAO : public DAOBase::DAOBase<ERP::Customer::DTO::CustomerDTO> { // Kế thừa từ template DAOBase
public:
    /**
     * @brief Constructs a CustomerDAO.
     * @param connectionPool Shared pointer to the ConnectionPool instance.
     */
    explicit CustomerDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool); // Thêm connectionPool

    ~CustomerDAO() override = default;

    // Các phương thức CRUD đã được triển khai trong DAOBase, nên không cần khai báo lại ở đây
    // bool create(const std::map<std::string, std::any>& data) override;
    // std::vector<std::map<std::string, std::any>> get(const std::map<std::string, std::any>& filter = {}) override;
    // bool update(const std::map<std::string, std::any>& filter, const std::map<std::string, std::any>& data) override;
    // bool remove(const std::string& id) override;
    // bool remove(const std::map<std::string, std::any>& filter) override;
    // std::optional<std::map<std::string, std::any>> getById(const std::string& id) override;
    // int count(const std::map<std::string, std::any>& filter = {}) override;

protected:
    /**
     * @brief Converts a database data map into a CustomerDTO object.
     * Handles decryption of sensitive fields and parsing of JSON fields for contacts/addresses.
     * This is an override of the pure virtual method in DAOBase.
     * @param data Database data map.
     * @return Converted CustomerDTO object.
     */
    ERP::Customer::DTO::CustomerDTO fromMap(const std::map<std::string, std::any>& data) const override;
    /**
     * @brief Converts a CustomerDTO object into a data map for database storage.
     * Handles encryption of sensitive fields and serialization of contacts/addresses to JSON.
     * This is an override of the pure virtual method in DAOBase.
     * @param customer CustomerDTO object.
     * @return Converted data map.
     */
    std::map<std::string, std::any> toMap(const ERP::Customer::DTO::CustomerDTO& customer) const override;

private:
    // tableName_ is now a member of DAOBase
    // Helper methods for JSON serialization/deserialization of nested DTOs - can be in DTOUtils or here
    // Tạm thời giữ lại nếu chúng là private helper của riêng DAO này
    // static std::string serializeContactPersons(const std::vector<DTO::ContactPersonDTO>& contacts);
    // static std::vector<DTO::ContactPersonDTO> deserializeContactPersons(const std::string& jsonString);
    // static std::string serializeAddresses(const std::vector<DTO::AddressDTO>& addresses);
    // static std::vector<DTO::AddressDTO> deserializeAddresses(const std::string& jsonString);
};
} // namespace DAOs
} // namespace Customer
} // namespace ERP
#endif // MODULES_CUSTOMER_DAO_CUSTOMERDAO_H