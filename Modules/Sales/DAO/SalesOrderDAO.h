// Modules/Sales/DAO/SalesOrderDAO.h
#ifndef MODULES_SALES_DAO_SALESORDERDAO_H
#define MODULES_SALES_DAO_SALESORDERDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Sales/DTO/SalesOrder.h" // For DTOs
#include "Modules/Sales/DTO/SalesOrderDetail.h" // For DTOs
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

// SalesOrderDAO handles two DTOs (SalesOrder and SalesOrderDetail).
// It will inherit from DAOBase for SalesOrderDTO, and
// have specific methods for SalesOrderDetailDTO.

class SalesOrderDAO : public ERP::DAOBase::DAOBase<ERP::Sales::DTO::SalesOrderDTO> {
public:
    explicit SalesOrderDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~SalesOrderDAO() override = default;

    // Override toMap and fromMap for SalesOrderDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Sales::DTO::SalesOrderDTO& dto) const override;
    ERP::Sales::DTO::SalesOrderDTO fromMap(const std::map<std::string, std::any>& data) const override;

public:
    // Specific methods for SalesOrderDetailDTO
    bool createSalesOrderDetail(const ERP::Sales::DTO::SalesOrderDetailDTO& detail);
    std::optional<ERP::Sales::DTO::SalesOrderDetailDTO> getSalesOrderDetailById(const std::string& id);
    std::vector<ERP::Sales::DTO::SalesOrderDetailDTO> getSalesOrderDetailsByOrderId(const std::string& orderId);
    bool updateSalesOrderDetail(const ERP::Sales::DTO::SalesOrderDetailDTO& detail);
    bool removeSalesOrderDetail(const std::string& id);
    bool removeSalesOrderDetailsByOrderId(const std::string& orderId); // Remove all details for an order

    // Helpers for SalesOrderDetailDTO conversion (static because not part of templated base)
    static std::map<std::string, std::any> toMap(const ERP::Sales::DTO::SalesOrderDetailDTO& dto);
    static ERP::Sales::DTO::SalesOrderDetailDTO fromMap(const std::map<std::string, std::any>& data);

private:
    std::string salesOrderDetailsTableName_ = "sales_order_details"; // Table for sales order details
};

} // namespace DAOs
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DAO_SALESORDERDAO_H