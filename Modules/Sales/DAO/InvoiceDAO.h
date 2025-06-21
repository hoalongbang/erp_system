// Modules/Sales/DAO/InvoiceDAO.h
#ifndef MODULES_SALES_DAO_INVOICEDAO_H
#define MODULES_SALES_DAO_INVOICEDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Sales/DTO/Invoice.h" // For DTOs
#include "Modules/Sales/DTO/InvoiceDetail.h" // For DTOs
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

// InvoiceDAO handles two DTOs (Invoice and InvoiceDetail).
// It will inherit from DAOBase for InvoiceDTO, and
// have specific methods for InvoiceDetailDTO.

class InvoiceDAO : public ERP::DAOBase::DAOBase<ERP::Sales::DTO::InvoiceDTO> {
public:
    explicit InvoiceDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~InvoiceDAO() override = default;

    // Override toMap and fromMap for InvoiceDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Sales::DTO::InvoiceDTO& dto) const override;
    ERP::Sales::DTO::InvoiceDTO fromMap(const std::map<std::string, std::any>& data) const override;

public:
    // Specific methods for InvoiceDetailDTO
    bool createInvoiceDetail(const ERP::Sales::DTO::InvoiceDetailDTO& detail);
    std::optional<ERP::Sales::DTO::InvoiceDetailDTO> getInvoiceDetailById(const std::string& id);
    std::vector<ERP::Sales::DTO::InvoiceDetailDTO> getInvoiceDetailsByInvoiceId(const std::string& invoiceId);
    bool updateInvoiceDetail(const ERP::Sales::DTO::InvoiceDetailDTO& detail);
    bool removeInvoiceDetail(const std::string& id);
    bool removeInvoiceDetailsByInvoiceId(const std::string& invoiceId); // Remove all details for an invoice

    // Helpers for InvoiceDetailDTO conversion (static because not part of templated base)
    static std::map<std::string, std::any> toMap(const ERP::Sales::DTO::InvoiceDetailDTO& dto);
    static ERP::Sales::DTO::InvoiceDetailDTO fromMap(const std::map<std::string, std::any>& data);

private:
    std::string invoiceDetailsTableName_ = "invoice_details"; // Table for invoice details
};

} // namespace DAOs
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DAO_INVOICEDAO_H