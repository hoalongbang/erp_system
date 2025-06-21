// Modules/Sales/Service/SalesPaymentService.h
#ifndef MODULES_SALES_SERVICE_SALESPAYMENTSERVICE_H
#define MODULES_SALES_SERVICE_SALESPAYMENTSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

// Rút gọn các include paths
#include "BaseService.h"        // NEW: Kế thừa từ BaseService
#include "Payment.h"            // DTO
#include "Invoice.h"            // DTO (for dependency)
#include "Customer.h"           // DTO (for dependency)
#include "PaymentDAO.h"         // DAO
#include "CustomerService.h"    // Customer Service interface (dependency)
#include "InvoiceService.h"     // Invoice Service interface (dependency)
#include "AccountReceivableService.h" // Account Receivable Service interface (dependency)
#include "ISecurityManager.h"   // Security Manager interface
#include "EventBus.h"           // EventBus
#include "Logger.h"             // Logger
#include "ErrorHandler.h"       // ErrorHandler
#include "Common.h"             // Common enums/constants
#include "Utils.h"              // Utilities
#include "DateUtils.h"          // Date utilities

namespace ERP {
namespace Sales {
namespace Services {

/**
 * @brief IPaymentService interface defines operations for managing payments.
 */
class IPaymentService {
public:
    virtual ~IPaymentService() = default;
    /**
     * @brief Creates a new payment.
     * @param paymentDTO DTO containing new payment information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional PaymentDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::PaymentDTO> createPayment(
        const ERP::Sales::DTO::PaymentDTO& paymentDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves payment information by ID.
     * @param paymentId ID of the payment to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional PaymentDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::PaymentDTO> getPaymentById(
        const std::string& paymentId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves payment information by payment number.
     * @param paymentNumber Payment number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional PaymentDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::PaymentDTO> getPaymentByNumber(
        const std::string& paymentNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all payments or payments matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching PaymentDTOs.
     */
    virtual std::vector<ERP::Sales::DTO::PaymentDTO> getAllPayments(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates payment information.
     * @param paymentDTO DTO containing updated payment information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updatePayment(
        const ERP::Sales::DTO::PaymentDTO& paymentDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a payment.
     * @param paymentId ID of the payment.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updatePaymentStatus(
        const std::string& paymentId,
        ERP::Sales::DTO::PaymentStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a payment record by ID.
     * @param paymentId ID of the payment to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deletePayment(
        const std::string& paymentId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of IPaymentService.
 * This class uses PaymentDAO and ISecurityManager.
 */
class SalesPaymentService : public IPaymentService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for SalesPaymentService.
     * @param paymentDAO Shared pointer to PaymentDAO.
     * @param customerService Shared pointer to ICustomerService (dependency).
     * @param invoiceService Shared pointer to IInvoiceService (dependency).
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    SalesPaymentService(std::shared_ptr<DAOs::PaymentDAO> paymentDAO,
                        std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService,
                        std::shared_ptr<ERP::Sales::Services::IInvoiceService> invoiceService,
                        std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                        std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                        std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                        std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Sales::DTO::PaymentDTO> createPayment(
        const ERP::Sales::DTO::PaymentDTO& paymentDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Sales::DTO::PaymentDTO> getPaymentById(
        const std::string& paymentId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Sales::DTO::PaymentDTO> getPaymentByNumber(
        const std::string& paymentNumber,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Sales::DTO::PaymentDTO> getAllPayments(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updatePayment(
        const ERP::Sales::DTO::PaymentDTO& paymentDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updatePaymentStatus(
        const std::string& paymentId,
        ERP::Sales::DTO::PaymentStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deletePayment(
        const std::string& paymentId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::PaymentDAO> paymentDAO_;
    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService_; // Dependency
    std::shared_ptr<ERP::Sales::Services::IInvoiceService> invoiceService_;     // Dependency
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_SERVICE_SALESPAYMENTSERVICE_H