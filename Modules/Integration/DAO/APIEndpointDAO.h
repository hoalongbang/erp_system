// Modules/Integration/DAO/APIEndpointDAO.h
#ifndef MODULES_INTEGRATION_DAO_APIENDPOINTDAO_H
#define MODULES_INTEGRATION_DAO_APIENDPOINTDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"        // Base DAO template
#include "APIEndpoint.h"    // APIEndpoint DTO

namespace ERP {
namespace Integration {
namespace DAOs {
/**
 * @brief DAO class for APIEndpoint entity.
 * Handles database operations for APIEndpointDTO.
 */
class APIEndpointDAO : public ERP::DAOBase::DAOBase<ERP::Integration::DTO::APIEndpointDTO> {
public:
    APIEndpointDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~APIEndpointDAO() override = default;

    // Override base methods (optional, but good practice if custom logic is needed)
    bool save(const ERP::Integration::DTO::APIEndpointDTO& endpoint) override;
    std::optional<ERP::Integration::DTO::APIEndpointDTO> findById(const std::string& id) override;
    bool update(const ERP::Integration::DTO::APIEndpointDTO& endpoint) override;
    bool remove(const std::string& id) override;
    std::vector<ERP::Integration::DTO::APIEndpointDTO> findAll() override;

    // Specific methods for APIEndpoint
    std::vector<ERP::Integration::DTO::APIEndpointDTO> getAPIEndpointsByIntegrationConfigId(const std::string& integrationConfigId);
    std::vector<ERP::Integration::DTO::APIEndpointDTO> getAPIEndpoints(const std::map<std::string, std::any>& filters);
    int countAPIEndpoints(const std::map<std::string, std::any>& filters);
    bool createAPIEndpoint(const ERP::Integration::DTO::APIEndpointDTO& endpoint, const std::string& integrationConfigId);
    bool removeAPIEndpointsByIntegrationConfigId(const std::string& integrationConfigId);

protected:
    // Required overrides for mapping between DTO and std::map<string, any>
    std::map<std::string, std::any> toMap(const ERP::Integration::DTO::APIEndpointDTO& endpoint) const override;
    ERP::Integration::DTO::APIEndpointDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    std::string tableName_ = "api_endpoints";
};
} // namespace DAOs
} // namespace Integration
} // namespace ERP
#endif // MODULES_INTEGRATION_DAO_APIENDPOINTDAO_H