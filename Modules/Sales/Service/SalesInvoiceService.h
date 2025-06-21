// Modules/Sales/Service/SalesInvoiceService.h
#ifndef MODULES_SALES_SERVICE_SALESINVOICESERVICE_H
#define MODULES_SALES_SERVICE_SALESINVOICESERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"      // NEW: Kế thừa từ BaseService
#include "Invoice.h"          // Đã rút gọn include
#include "InvoiceDAO.h"       // Đã rút gọn include
#include "SalesOrderService.h" // Forward declaration or include if needed for validation
#include "ISecurityManager.h" // Đã rút gọn include
#include "EventBus.h"         // Đã rút gọn include
#include "Logger.h"           // Đã rút gọn include
#include "ErrorHandler.h"     // Đã rút gọn include
#include "Common.h"           // Đã rút gọn include
#include "Utils.h"            // Đã rút gọn include
#include "DateUtils.h"        // Đã rút gọn include
namespace ERP {
namespace Sales {
namespace Services {

// Forward declare if SalesOrderService is only used via pointer/reference
// class ISalesOrderService;

/**
 * @brief ISalesInvoiceService interface defines operations for managing sales invoices.
 */
class ISalesInvoiceService {
public:
    virtual ~ISalesInvoiceService() = default;
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
/**
 * @brief Default implementation of ISalesInvoiceService.
 * This class uses InvoiceDAO and ISecurityManager.
 */
class SalesInvoiceService : public ISalesInvoiceService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for SalesInvoiceService.
     * @param invoiceDAO Shared pointer to InvoiceDAO.
     * @param salesOrderService Shared pointer to ISalesOrderService.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    SalesInvoiceService(std::shared_ptr<DAOs::InvoiceDAO> invoiceDAO,
                        std::shared_ptr<ISalesOrderService> salesOrderService, // Use interface
                        std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                        std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                        std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                        std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Sales::DTO::InvoiceDTO> createInvoice(
        const ERP::Sales::DTO::InvoiceDTO& invoiceDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Sales::DTO::InvoiceDTO> getInvoiceById(
        const std::string& invoiceId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Sales::DTO::InvoiceDTO> getInvoiceByNumber(
        const std::string& invoiceNumber,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Sales::DTO::InvoiceDTO> getAllInvoices(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateInvoice(
        const ERP::Sales::DTO::InvoiceDTO& invoiceDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateInvoiceStatus(
        const std::string& invoiceId,
        ERP::Sales::DTO::InvoiceStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteInvoice(
        const std::string& invoiceId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::InvoiceDAO> invoiceDAO_;
    std::shared_ptr<ISalesOrderService> salesOrderService_; // For sales order validation
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // Old private helper functions, now handled by BaseService or integrated directly
    // bool checkUserPermission(const std::string& userId, const std::vector<std::string>& roleIds, const std::string& permission, const std::string& errorMessage);
    // std::vector<std::string> getUserRoleIds(const std::string& userId);
};
} // namespace Services
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_SERVICE_SALESINVOICESERVICE_H