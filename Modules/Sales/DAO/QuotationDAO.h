// Modules/Sales/DAO/QuotationDAO.h
#ifndef MODULES_SALES_DAO_QUOTATIONDAO_H
#define MODULES_SALES_DAO_QUOTATIONDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Sales/DTO/Quotation.h" // For DTOs
#include "Modules/Sales/DTO/QuotationDetail.h" // For DTOs
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "Modules/Utils/DTOUtils.h" // For DTO conversion helpers
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <any>
#include <optional>

namespace ERP {
namespace Sales {
namespace DAOs {

// QuotationDAO handles two DTOs (Quotation and QuotationDetail).
// It will inherit from DAOBase for QuotationDTO, and
// have specific methods for QuotationDetailDTO.

class QuotationDAO : public ERP::DAOBase::DAOBase<ERP::Sales::DTO::QuotationDTO> {
public:
    explicit QuotationDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~QuotationDAO() override = default;

    // Override toMap and fromMap for QuotationDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Sales::DTO::QuotationDTO& dto) const override;
    ERP::Sales::DTO::QuotationDTO fromMap(const std::map<std::string, std::any>& data) const override;

public:
    // Specific methods for QuotationDetailDTO
    bool createQuotationDetail(const ERP::Sales::DTO::QuotationDetailDTO& detail);
    std::optional<ERP::Sales::DTO::QuotationDetailDTO> getQuotationDetailById(const std::string& id);
    std::vector<ERP::Sales::DTO::QuotationDetailDTO> getQuotationDetailsByQuotationId(const std::string& quotationId);
    bool updateQuotationDetail(const ERP::Sales::DTO::QuotationDetailDTO& detail);
    bool removeQuotationDetail(const std::string& id);
    bool removeQuotationDetailsByQuotationId(const std::string& quotationId); // Remove all details for a quotation

    // Helpers for QuotationDetailDTO conversion (static because not part of templated base)
    static std::map<std::string, std::any> toMap(const ERP::Sales::DTO::QuotationDetailDTO& dto);
    static ERP::Sales::DTO::QuotationDetailDTO fromMap(const std::map<std::string, std::any>& data);

private:
    std::string quotationDetailsTableName_ = "quotation_details"; // Table for quotation details
};

} // namespace DAOs
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DAO_QUOTATIONDAO_H