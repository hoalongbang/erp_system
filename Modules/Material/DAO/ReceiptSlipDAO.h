// Modules/Material/DAO/ReceiptSlipDAO.h
#ifndef MODULES_MATERIAL_DAO_RECEIPTSLIPDAO_H
#define MODULES_MATERIAL_DAO_RECEIPTSLIPDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Material/DTO/ReceiptSlip.h" // For DTOs
#include "Modules/Material/DTO/ReceiptSlipDetail.h" // For DTOs
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
namespace Material {
namespace DAOs {

// ReceiptSlipDAO will handle two DTOs (Slip and SlipDetail).
// It will inherit from DAOBase for ReceiptSlipDTO, and
// have specific methods for ReceiptSlipDetailDTO.

class ReceiptSlipDAO : public ERP::DAOBase::DAOBase<ERP::Material::DTO::ReceiptSlipDTO> {
public:
    explicit ReceiptSlipDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~ReceiptSlipDAO() override = default;

    // Override toMap and fromMap for ReceiptSlipDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Material::DTO::ReceiptSlipDTO& dto) const override;
    ERP::Material::DTO::ReceiptSlipDTO fromMap(const std::map<std::string, std::any>& data) const override;

public:
    // Specific methods for ReceiptSlipDetailDTO
    bool createReceiptSlipDetail(const ERP::Material::DTO::ReceiptSlipDetailDTO& detail);
    std::optional<ERP::Material::DTO::ReceiptSlipDetailDTO> getReceiptSlipDetailById(const std::string& id);
    std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> getReceiptSlipDetailsByReceiptSlipId(const std::string& receiptSlipId);
    bool updateReceiptSlipDetail(const ERP::Material::DTO::ReceiptSlipDetailDTO& detail);
    bool removeReceiptSlipDetail(const std::string& id);
    bool removeReceiptSlipDetailsByReceiptSlipId(const std::string& receiptSlipId); // Remove all details for a slip

    // Helpers for ReceiptSlipDetailDTO conversion (static because not part of templated base)
    static std::map<std::string, std::any> toMap(const ERP::Material::DTO::ReceiptSlipDetailDTO& dto);
    static ERP::Material::DTO::ReceiptSlipDetailDTO fromMap(const std::map<std::string, std::any>& data);

private:
    std::string receiptSlipDetailsTableName_ = "receipt_slip_details"; // Table for receipt slip details
};

} // namespace DAOs
} // namespace Material
} // namespace ERP
#endif // MODULES_MATERIAL_DAO_RECEIPTSLIPDAO_H