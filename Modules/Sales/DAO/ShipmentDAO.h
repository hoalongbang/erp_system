// Modules/Sales/DAO/ShipmentDAO.h
#ifndef MODULES_SALES_DAO_SHIPMENTDAO_H
#define MODULES_SALES_DAO_SHIPMENTDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Sales/DTO/Shipment.h" // For DTOs
#include "Modules/Sales/DTO/ShipmentDetail.h" // For DTOs
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

// ShipmentDAO handles two DTOs (Shipment and ShipmentDetail).
// It will inherit from DAOBase for ShipmentDTO, and
// have specific methods for ShipmentDetailDTO.

class ShipmentDAO : public ERP::DAOBase::DAOBase<ERP::Sales::DTO::ShipmentDTO> {
public:
    explicit ShipmentDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~ShipmentDAO() override = default;

    // Override toMap and fromMap for ShipmentDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Sales::DTO::ShipmentDTO& dto) const override;
    ERP::Sales::DTO::ShipmentDTO fromMap(const std::map<std::string, std::any>& data) const override;

public:
    // Specific methods for ShipmentDetailDTO
    bool createShipmentDetail(const ERP::Sales::DTO::ShipmentDetailDTO& detail);
    std::optional<ERP::Sales::DTO::ShipmentDetailDTO> getShipmentDetailById(const std::string& id);
    std::vector<ERP::Sales::DTO::ShipmentDetailDTO> getShipmentDetailsByShipmentId(const std::string& shipmentId);
    bool updateShipmentDetail(const ERP::Sales::DTO::ShipmentDetailDTO& detail);
    bool removeShipmentDetail(const std::string& id);
    bool removeShipmentDetailsByShipmentId(const std::string& shipmentId); // Remove all details for a shipment

    // Helpers for ShipmentDetailDTO conversion (static because not part of templated base)
    static std::map<std::string, std::any> toMap(const ERP::Sales::DTO::ShipmentDetailDTO& dto);
    static ERP::Sales::DTO::ShipmentDetailDTO fromMap(const std::map<std::string, std::any>& data);

private:
    std::string shipmentDetailsTableName_ = "shipment_details"; // Table for shipment details
};

} // namespace DAOs
} // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DAO_SHIPMENTDAO_H