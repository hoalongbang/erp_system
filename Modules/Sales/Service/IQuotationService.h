// Modules/Sales/Service/IQuotationService.h
#ifndef MODULES_SALES_SERVICE_IQUOTATIONSERVICE_H
#define MODULES_SALES_SERVICE_IQUOTATIONSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "Quotation.h"          // DTO
#include "QuotationDetail.h"    // DTO
#include "Common.h"             // Enum Common
#include "BaseService.h"        // Base Service

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
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of QuotationDetailDTOs.
     */
    virtual std::vector<ERP::Sales::DTO::QuotationDetailDTO> getQuotationDetails(
        const std::string& quotationId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
};

} // namespace Services
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_SERVICE_IQUOTATIONSERVICE_H