// Modules/Sales/Service/IInvoiceService.h
#ifndef MODULES_SALES_SERVICE_IINVOICESERVICE_H
#define MODULES_SALES_SERVICE_IINVOICESERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "Invoice.h"          // DTO
#include "InvoiceDetail.h"    // DTO
#include "Common.h"           // Enum Common
#include "BaseService.h"      // Base Service

namespace ERP {
namespace Sales {
namespace Services {

/**
 * @brief IInvoiceService interface defines operations for managing sales invoices.
 */
class IInvoiceService {
public:
    virtual ~IInvoiceService() = default;
    /**
     * @brief Creates a new sales invoice.
     * @param invoiceDTO DTO containing new invoice information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional InvoiceDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::InvoiceDTO> createInvoice(
        const ERP::Sales::DTO::InvoiceDTO& invoiceDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves invoice information by ID.
     * @param invoiceId ID of the invoice to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional InvoiceDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::InvoiceDTO> getInvoiceById(
        const std::string& invoiceId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves invoice information by invoice number.
     * @param invoiceNumber Invoice number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional InvoiceDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::InvoiceDTO> getInvoiceByNumber(
        const std::string& invoiceNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all invoices or invoices matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching InvoiceDTOs.
     */
    virtual std::vector<ERP::Sales::DTO::InvoiceDTO> getAllInvoices(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates invoice information.
     * @param invoiceDTO DTO containing updated invoice information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateInvoice(
        const ERP::Sales::DTO::InvoiceDTO& invoiceDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of an invoice.
     * @param invoiceId ID of the invoice.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateInvoiceStatus(
        const std::string& invoiceId,
        ERP::Sales::DTO::InvoiceStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes an invoice record by ID.
     * @param invoiceId ID of the invoice to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteInvoice(
        const std::string& invoiceId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};

} // namespace Services
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_SERVICE_IINVOICESERVICE_H