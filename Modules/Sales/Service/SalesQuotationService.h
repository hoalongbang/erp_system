// Modules/Sales/Service/SalesQuotationService.h
#ifndef MODULES_SALES_SERVICE_SALESQUOTATIONSERVICE_H
#define MODULES_SALES_SERVICE_SALESQUOTATIONSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

// Rút gọn các include paths
#include "BaseService.h"        // NEW: Kế thừa từ BaseService
#include "Quotation.h"          // DTO
#include "QuotationDetail.h"    // DTO
#include "SalesOrder.h"         // DTO (for conversion)
#include "SalesOrderDetail.h"   // DTO (for conversion)
#include "QuotationDAO.h"       // DAO
#include "CustomerService.h"    // Customer Service interface (dependency)
#include "ProductService.h"     // Product Service interface (dependency)
#include "UnitOfMeasureService.h" // UnitOfMeasure Service interface (dependency)
#include "SalesOrderService.h"  // SalesOrder Service interface (dependency)
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
 * @brief IQuotationService interface defines operations for managing sales quotations.
 */
class IQuotationService {
public:
    virtual ~IQuotationService() = default;
    /**
     * @brief Creates a new sales quotation.
     * @param quotationDTO DTO containing new quotation information.
     * @param quotationDetails Vector of QuotationDetailDTOs for the quotation.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional QuotationDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::QuotationDTO> createQuotation(
        const ERP::Sales::DTO::QuotationDTO& quotationDTO,
        const std::vector<ERP::Sales::DTO::QuotationDetailDTO>& quotationDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves quotation information by ID.
     * @param quotationId ID of the quotation to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional QuotationDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::QuotationDTO> getQuotationById(
        const std::string& quotationId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves quotation information by quotation number.
     * @param quotationNumber Quotation number to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional QuotationDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::QuotationDTO> getQuotationByNumber(
        const std::string& quotationNumber,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all quotations or quotations matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching QuotationDTOs.
     */
    virtual std::vector<ERP::Sales::DTO::QuotationDTO> getAllQuotations(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates quotation information.
     * @param quotationDTO DTO containing updated quotation information (must have ID).
     * @param quotationDetails Vector of QuotationDetailDTOs for the quotation (full replacement).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateQuotation(
        const ERP::Sales::DTO::QuotationDTO& quotationDTO,
        const std::vector<ERP::Sales::DTO::QuotationDetailDTO>& quotationDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Updates the status of a quotation.
     * @param quotationId ID of the quotation.
     * @param newStatus New status.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if status update is successful, false otherwise.
     */
    virtual bool updateQuotationStatus(
        const std::string& quotationId,
        ERP::Sales::DTO::QuotationStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a quotation record by ID.
     * @param quotationId ID of the quotation to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteQuotation(
        const std::string& quotationId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Converts an accepted quotation into a new sales order.
     * @param quotationId ID of the quotation to convert.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional SalesOrderDTO if conversion is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Sales::DTO::SalesOrderDTO> convertQuotationToSalesOrder(
        const std::string& quotationId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves all details for a specific quotation.
     * @param quotationId ID of the quotation.
     * @param currentUserId ID of the user making the request.
     * @param userRoleIds Roles of the user making the request.
     * @return Vector of QuotationDetailDTOs.
     */
    virtual std::vector<ERP::Sales::DTO::QuotationDetailDTO> getQuotationDetails(
        const std::string& quotationId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0; // Added currentUserId and userRoleIds
};

/**
 * @brief Default implementation of IQuotationService.
 * This class uses QuotationDAO and ISecurityManager.
 */
class SalesQuotationService : public IQuotationService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for SalesQuotationService.
     * @param quotationDAO Shared pointer to QuotationDAO.
     * @param customerService Shared pointer to ICustomerService (dependency).
     * @param productService Shared pointer to IProductService (dependency).
     * @param unitOfMeasureService Shared pointer to IUnitOfMeasureService (dependency).
     * @param salesOrderService Shared pointer to ISalesOrderService (dependency).
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    SalesQuotationService(std::shared_ptr<DAOs::QuotationDAO> quotationDAO,
                          std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService,
                          std::shared_ptr<ERP::Product::Services::IProductService> productService,
                          std::shared_ptr<ERP::Catalog::Services::IUnitOfMeasureService> unitOfMeasureService,
                          std::shared_ptr<ERP::Sales::Services::ISalesOrderService> salesOrderService,
                          std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                          std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                          std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                          std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Sales::DTO::QuotationDTO> createQuotation(
        const ERP::Sales::DTO::QuotationDTO& quotationDTO,
        const std::vector<ERP::Sales::DTO::QuotationDetailDTO>& quotationDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Sales::DTO::QuotationDTO> getQuotationById(
        const std::string& quotationId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::optional<ERP::Sales::DTO::QuotationDTO> getQuotationByNumber(
        const std::string& quotationNumber,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Sales::DTO::QuotationDTO> getAllQuotations(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateQuotation(
        const ERP::Sales::DTO::QuotationDTO& quotationDTO,
        const std::vector<ERP::Sales::DTO::QuotationDetailDTO>& quotationDetails,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool updateQuotationStatus(
        const std::string& quotationId,
        ERP::Sales::DTO::QuotationStatus newStatus,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteQuotation(
        const std::string& quotationId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Sales::DTO::SalesOrderDTO> convertQuotationToSalesOrder(
        const std::string& quotationId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::vector<ERP::Sales::DTO::QuotationDetailDTO> getQuotationDetails(
        const std::string& quotationId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override; // Added currentUserId and userRoleIds

private:
    std::shared_ptr<DAOs::QuotationDAO> quotationDAO_;
    std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService_;
    std::shared_ptr<ERP::Product::Services::IProductService> productService_;
    std::shared_ptr<ERP::Catalog::Services::IUnitOfMeasureService> unitOfMeasureService_;
    std::shared_ptr<ERP::Sales::Services::ISalesOrderService> salesOrderService_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_SERVICE_SALESQUOTATIONSERVICE_H