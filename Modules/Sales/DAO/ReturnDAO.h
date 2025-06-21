// Modules/Sales/DAO/ReturnDAO.h
#ifndef MODULES_SALES_DAO_RETURNDAO_H
#define MODULES_SALES_DAO_RETURNDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"       // Base DAO template
#include "Return.h"        // Return DTO (chứa ReturnDTO và ReturnDetailDTO)

namespace ERP {
    namespace Sales {
        namespace DAOs {
            /**
             * @brief ReturnDAO class provides data access operations for ReturnDTO objects.
             * It inherits from DAOBase and interacts with the database to manage sales returns and their details.
             */
            class ReturnDAO : public ERP::DAOBase::DAOBase<ERP::Sales::DTO::ReturnDTO> {
            public:
                ReturnDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~ReturnDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Sales::DTO::ReturnDTO& returnObj) override;
                std::optional<ERP::Sales::DTO::ReturnDTO> findById(const std::string& id) override;
                bool update(const ERP::Sales::DTO::ReturnDTO& returnObj) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Sales::DTO::ReturnDTO> findAll() override;

                // Specific methods for Return
                std::vector<ERP::Sales::DTO::ReturnDTO> getReturns(const std::map<std::string, std::any>& filters);
                int countReturns(const std::map<std::string, std::any>& filters);

                // ReturnDetail operations (nested/related entities)
                bool createReturnDetail(const ERP::Sales::DTO::ReturnDetailDTO& detail);
                std::optional<ERP::Sales::DTO::ReturnDetailDTO> getReturnDetailById(const std::string& id);
                std::vector<ERP::Sales::DTO::ReturnDetailDTO> getReturnDetailsByReturnId(const std::string& returnId);
                std::vector<ERP::Sales::DTO::ReturnDetailDTO> getReturnDetails(const std::map<std::string, std::any>& filters);
                int countReturnDetails(const std::map<std::string, std::any>& filters);
                bool updateReturnDetail(const ERP::Sales::DTO::ReturnDetailDTO& detail);
                bool removeReturnDetail(const std::string& id);
                bool removeReturnDetailsByReturnId(const std::string& returnId);


            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Sales::DTO::ReturnDTO& returnObj) const override;
                ERP::Sales::DTO::ReturnDTO fromMap(const std::map<std::string, std::any>& data) const override;

                // Mapping for ReturnDetailDTO (internal helper, not part of base DAO template)
                std::map<std::string, std::any> returnDetailToMap(const ERP::Sales::DTO::ReturnDetailDTO& detail) const;
                ERP::Sales::DTO::ReturnDetailDTO returnDetailFromMap(const std::map<std::string, std::any>& data) const;

            private:
                std::string tableName_ = "returns";
                std::string detailsTableName_ = "return_details";
            };
        } // namespace DAOs
    } // namespace Sales
} // namespace ERP
#endif // MODULES_SALES_DAO_RETURNDAO_H