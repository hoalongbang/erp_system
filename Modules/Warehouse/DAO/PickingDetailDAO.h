// Modules/Warehouse/DAO/PickingDetailDAO.h
#ifndef MODULES_WAREHOUSE_DAO_PICKINGDETAILDAO_H
#define MODULES_WAREHOUSE_DAO_PICKINGDETAILDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"        // Base DAO template
#include "PickingDetail.h"  // PickingDetail DTO (renamed from PickingListItem)

namespace ERP {
    namespace Warehouse {
        namespace DAOs {
            /**
             * @brief PickingDetailDAO class provides data access operations for PickingDetailDTO objects.
             * It inherits from DAOBase and interacts with the database to manage picking request details.
             * (Previously named PickingListItemDAO, now unified as PickingDetailDAO)
             */
            class PickingDetailDAO : public ERP::DAOBase::DAOBase<ERP::Warehouse::DTO::PickingDetailDTO> {
            public:
                PickingDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~PickingDetailDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Warehouse::DTO::PickingDetailDTO& detail) override;
                std::optional<ERP::Warehouse::DTO::PickingDetailDTO> findById(const std::string& id) override;
                bool update(const ERP::Warehouse::DTO::PickingDetailDTO& detail) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Warehouse::DTO::PickingDetailDTO> findAll() override;

                // Specific methods for PickingDetail
                std::vector<ERP::Warehouse::DTO::PickingDetailDTO> getPickingDetailsByRequestId(const std::string& pickingRequestId);
                std::vector<ERP::Warehouse::DTO::PickingDetailDTO> getPickingDetails(const std::map<std::string, std::any>& filters);
                int countPickingDetails(const std::map<std::string, std::any>& filters);
                bool removePickingDetailsByRequestId(const std::string& pickingRequestId);

            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Warehouse::DTO::PickingDetailDTO& detail) const override;
                ERP::Warehouse::DTO::PickingDetailDTO fromMap(const std::map<std::string, std::any>& data) const override;

            private:
                std::string tableName_ = "picking_details"; // Corresponds to `picking_details` table
            };
        } // namespace DAOs
    } // namespace Warehouse
} // namespace ERP
#endif // MODULES_WAREHOUSE_DAO_PICKINGDETAILDAO_H