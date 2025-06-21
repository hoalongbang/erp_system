// Modules/Sales/DAO/PaymentDAO.h
#ifndef MODULES_SALES_DAO_PAYMENTDAO_H
#define MODULES_SALES_DAO_PAYMENTDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Sales/DTO/Payment.h" // For DTOs
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

class PaymentDAO : public ERP::DAOBase::DAOBase<ERP::Sales::DTO::PaymentDTO> {
public:
    explicit PaymentDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~PaymentDAO() override = default;

    // Override toMap and fromMap for PaymentDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Sales::DTO::PaymentDTO& dto) const override;
    ERP::Sales::DTO::PaymentDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    // tableName_ is now a member of DAOBase
};

} // namespace DAOs
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DAO_PAYMENTDAO_H