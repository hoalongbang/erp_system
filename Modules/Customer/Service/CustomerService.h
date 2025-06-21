// Modules/Customer/Service/CustomerService.h
#ifndef MODULES_CUSTOMER_SERVICE_CUSTOMERSERVICE_H
#define MODULES_CUSTOMER_SERVICE_CUSTOMERSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"      // NEW: Kế thừa từ BaseService
#include "Customer.h"         // Đã rút gọn include
#include "CustomerDAO.h"      // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "EventBus.h"         // Đã rút gọn include
#include "Logger.h"           // Đã rút gọn include
#include "ErrorHandler.h"     // Đã rút gọn include
#include "Common.h"           // Đã rút gọn include
#include "Utils.h"            // Đã rút gọn include
#include "DateUtils.h"        // Đã rút gọn include

namespace ERP {
namespace Customer {
namespace Services {

/**
 * @brief ICustomerService interface defines operations for managing customer accounts.
 */
class ICustomerService {
public:
    virtual ~ICustomerService() = default;
    /**
     * @brief Creates a new customer.
     * @param customerDTO DTO containing new customer information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional CustomerDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Customer::DTO::CustomerDTO> createCustomer(
        const ERP::Customer::DTO::CustomerDTO& customerDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves customer information by ID.
     * @param customerId ID of the customer to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional CustomerDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Customer::DTO::CustomerDTO> getCustomerById(
        const std::string& customerId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves customer information by name.
     * @param customerName Name of the customer to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional CustomerDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Customer::DTO::CustomerDTO> getCustomerByName(
        const std::string& customerName,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all customers or customers matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching CustomerDTOs.
     */
    virtual std::vector<ERP::Customer::DTO::CustomerDTO> getAllCustomers(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates customer information.
     * @param customerDTO DTO containing updated customer information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateCustomer(
        const ERP::Customer::DTO::CustomerDTO& customerDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a customer.
     * @param customerId ID of the customer.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateCustomerStatus(
        const std::string& customerId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a customer record by ID (soft delete).
     * @param customerId ID of the customer to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteCustomer(
        const std::string& customerId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of ICustomerService.
 * This class uses CustomerDAO and ISecurityManager.
 */
class CustomerService : public ICustomerService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for CustomerService.
     * @param customerDAO Shared pointer to CustomerDAO.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    CustomerService(std::shared_ptr<DAOs::CustomerDAO> customerDAO,
                    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                    std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Customer::DTO::CustomerDTO> createCustomer(
        const ERP::Customer::DTO::CustomerDTO& customerDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Customer::DTO::CustomerDTO> getCustomerById(
        const std::string& customerId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Customer::DTO::CustomerDTO> getCustomerByName(
        const std::string& customerName,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Customer::DTO::CustomerDTO> getAllCustomers(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateCustomer(
        const ERP::Customer::DTO::CustomerDTO& customerDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateCustomerStatus(
        const std::string& customerId,
        ERP::Common::EntityStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteCustomer(
        const std::string& customerId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::CustomerDAO> customerDAO_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Customer
} // namespace ERP
#endif // MODULES_CUSTOMER_SERVICE_CUSTOMERSERVICE_H