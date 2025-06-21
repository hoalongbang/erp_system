// Modules/Sales/Service/IPaymentService.h
#ifndef MODULES_SALES_SERVICE_IPAYMENTSERVICE_H
#define MODULES_SALES_SERVICE_IPAYMENTSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "Payment.h"       // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

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

} // namespace Services
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_SERVICE_IPAYMENTSERVICE_H