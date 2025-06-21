// Modules/Sales/DAO/ShipmentDetailDAO.h
#ifndef MODULES_SALES_DAO_SHIPMENTDETAILDAO_H
#define MODULES_SALES_DAO_SHIPMENTDETAILDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"        // Base DAO template
#include "ShipmentDetail.h" // ShipmentDetail DTO

namespace ERP {
    namespace Sales {
        namespace DAOs {
            /**
             * @brief ShipmentDetailDAO class provides data access operations for ShipmentDetailDTO objects.
             * It inherits from DAOBase and interacts with the database to manage shipment details.
             */
            class ShipmentDetailDAO : public ERP::DAOBase::DAOBase<ERP::Sales::DTO::ShipmentDetailDTO> {
            public:
                ShipmentDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~ShipmentDetailDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Sales::DTO::ShipmentDetailDTO& detail) override;
                std::optional<ERP::Sales::DTO::ShipmentDetailDTO> findById(const std::string& id) override;
                bool update(const ERP::Sales::DTO::ShipmentDetailDTO& detail) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Sales::DTO::ShipmentDetailDTO> findAll() override;

                // Specific methods for ShipmentDetail
                std::vector<ERP::Sales::DTO::ShipmentDetailDTO> getShipmentDetailsByShipmentId(const std::string& shipmentId);
                std::vector<ERP::Sales::DTO::ShipmentDetailDTO> getShipmentDetails(const std::map<std::string, std::any>& filters);
                int countShipmentDetails(const std::map<std::string, std::any>& filters);
                bool removeShipmentDetailsByShipmentId(const std::string& shipmentId);

            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Sales::DTO::ShipmentDetailDTO& detail) const override;
                ERP::Sales::DTO::ShipmentDetailDTO fromMap(const std::map<std::string, std::any>& data) const override;

            private:
                std::string tableName_ = "shipment_details";
            };
        } // namespace DAOs
    } // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DAO_SHIPMENTDETAILDAO_H