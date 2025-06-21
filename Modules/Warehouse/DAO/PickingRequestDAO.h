// Modules/Warehouse/DAO/PickingRequestDAO.h
#ifndef MODULES_WAREHOUSE_DAO_PICKINGREQUESTDAO_H
#define MODULES_WAREHOUSE_DAO_PICKINGREQUESTDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"            // Base DAO template
#include "PickingRequest.h"     // PickingRequest DTO (renamed from PickingList)
#include "PickingDetail.h"      // PickingDetail DTO (renamed from PickingListItem)

namespace ERP {
    namespace Warehouse {
        namespace DAOs {
            /**
             * @brief PickingRequestDAO class provides data access operations for PickingRequestDTO objects.
             * It inherits from DAOBase and interacts with the database to manage picking requests and their details.
             * (Previously named PickingListDAO, now unified as PickingRequestDAO)
             */
            class PickingRequestDAO : public ERP::DAOBase::DAOBase<ERP::Warehouse::DTO::PickingRequestDTO> {
            public:
                PickingRequestDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~PickingRequestDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Warehouse::DTO::PickingRequestDTO& request) override;
                std::optional<ERP::Warehouse::DTO::PickingRequestDTO> findById(const std::string& id) override;
                bool update(const ERP::Warehouse::DTO::PickingRequestDTO& request) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Warehouse::DTO::PickingRequestDTO> findAll() override;

                // Specific methods for PickingRequest
                std::vector<ERP::Warehouse::DTO::PickingRequestDTO> getPickingRequests(const std::map<std::string, std::any>& filters);
                int countPickingRequests(const std::map<std::string, std::any>& filters);

                // PickingDetail operations (nested/related entities)
                bool createPickingDetail(const ERP::Warehouse::DTO::PickingDetailDTO& detail);
                std::optional<ERP::Warehouse::DTO::PickingDetailDTO> getPickingDetailById(const std::string& id);
                std::vector<ERP::Warehouse::DTO::PickingDetailDTO> getPickingDetailsByRequestId(const std::string& pickingRequestId);
                std::vector<ERP::Warehouse::DTO::PickingDetailDTO> getPickingDetails(const std::map<std::string, std::any>& filters);
                int countPickingDetails(const std::map<std::string, std::any>& filters);
                bool updatePickingDetail(const ERP::Warehouse::DTO::PickingDetailDTO& detail);
                bool removePickingDetail(const std::string& id);
                bool removePickingDetailsByRequestId(const std::string& pickingRequestId);

            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Warehouse::DTO::PickingRequestDTO& request) const override;
                ERP::Warehouse::DTO::PickingRequestDTO fromMap(const std::map<std::string, std::any>& data) const override;

                // Mapping for PickingDetailDTO (internal helper)
                std::map<std::string, std::any> pickingDetailToMap(const ERP::Warehouse::DTO::PickingDetailDTO& detail) const;
                ERP::Warehouse::DTO::PickingDetailDTO pickingDetailFromMap(const std::map<std::string, std::any>& data) const;

            private:
                std::string tableName_ = "picking_requests";
                std::string detailsTableName_ = "picking_details"; // Table for details
            };
        } // namespace DAOs
    } // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_DAO_PICKINGREQUESTDAO_H