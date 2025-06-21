// Modules/Integration/DAO/IntegrationConfigDAO.h
#ifndef MODULES_INTEGRATION_DAO_INTEGRATIONCONFIGDAO_H
#define MODULES_INTEGRATION_DAO_INTEGRATIONCONFIGDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"            // Base DAO template
#include "IntegrationConfig.h"  // IntegrationConfig DTO
#include "APIEndpoint.h"        // APIEndpoint DTO (for related operations)

namespace ERP {
namespace Integration {
namespace DAOs {
/**
 * @brief DAO class for IntegrationConfig entity.
 * Handles database operations for IntegrationConfigDTO and related APIEndpointDTOs.
 */
class IntegrationConfigDAO : public ERP::DAOBase::DAOBase<ERP::Integration::DTO::IntegrationConfigDTO> {
public:
    IntegrationConfigDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~IntegrationConfigDAO() override = default;

    // Override base methods (optional, but good practice if custom logic is needed)
    bool save(const ERP::Integration::DTO::IntegrationConfigDTO& config) override;
    std::optional<ERP::Integration::DTO::IntegrationConfigDTO> findById(const std::string& id) override;
    bool update(const ERP::Integration::DTO::IntegrationConfigDTO& config) override;
    bool remove(const std::string& id) override;
    std::vector<ERP::Integration::DTO::IntegrationConfigDTO> findAll() override;

    // Specific methods for IntegrationConfig
    std::optional<ERP::Integration::DTO::IntegrationConfigDTO> getIntegrationConfigBySystemCode(const std::string& systemCode);
    std::vector<ERP::Integration::DTO::IntegrationConfigDTO> getIntegrationConfigs(const std::map<std::string, std::any>& filters);
    int countIntegrationConfigs(const std::map<std::string, std::any>& filters);

    // APIEndpoint operations (nested/related entities)
    bool createAPIEndpoint(const ERP::Integration::DTO::APIEndpointDTO& endpoint, const std::string& integrationConfigId);
    std::optional<ERP::Integration::DTO::APIEndpointDTO> getAPIEndpointById(const std::string& endpointId);
    std::vector<ERP::Integration::DTO::APIEndpointDTO> getAPIEndpointsByIntegrationConfigId(const std::string& integrationConfigId);
    std::vector<ERP::Integration::DTO::APIEndpointDTO> getAPIEndpoints(const std::map<std::string, std::any>& filters);
    int countAPIEndpoints(const std::map<std::string, std::any>& filters);
    bool removeAPIEndpointsByIntegrationConfigId(const std::string& integrationConfigId);


protected:
    // Required overrides for mapping between DTO and std::map<string, any>
    std::map<std::string, std::any> toMap(const ERP::Integration::DTO::IntegrationConfigDTO& config) const override;
    ERP::Integration::DTO::IntegrationConfigDTO fromMap(const std::map<std::string, std::any>& data) const override;

    // Mapping for APIEndpointDTO (internal helper, not part of base DAO template)
    std::map<std::string, std::any> apiEndpointToMap(const ERP::Integration::DTO::APIEndpointDTO& endpoint) const;
    ERP::Integration::DTO::APIEndpointDTO apiEndpointFromMap(const std::map<std::string, std::any>& data) const;

private:
    std::string tableName_ = "integration_configs";
    std::string apiEndpointTableName_ = "api_endpoints";
};
} // namespace DAOs
} // namespace Integration
} // namespace ERP
#endif // MODULES_INTEGRATION_DAO_INTEGRATIONCONFIGDAO_H