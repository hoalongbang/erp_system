// Modules/Integration/DAO/APIEndpointDAO.cpp
#include "APIEndpointDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversion

namespace ERP {
namespace Integration {
namespace DAOs {

APIEndpointDAO::APIEndpointDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Integration::DTO::APIEndpointDTO>(connectionPool, "api_endpoints") {
    ERP::Logger::Logger::getInstance().info("APIEndpointDAO: Initialized.");
}

std::map<std::string, std::any> APIEndpointDAO::toMap(const ERP::Integration::DTO::APIEndpointDTO& endpoint) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(endpoint); // BaseDTO fields

    data["integration_config_id"] = endpoint.integrationConfigId;
    data["endpoint_code"] = endpoint.endpointCode;
    data["method"] = static_cast<int>(endpoint.method);
    data["url"] = endpoint.url;
    ERP::DAOHelpers::putOptionalString(data, "description", endpoint.description);
    ERP::DAOHelpers::putOptionalString(data, "request_schema", endpoint.requestSchema);
    ERP::DAOHelpers::putOptionalString(data, "response_schema", endpoint.responseSchema);
    data["metadata_json"] = ERP::Utils::DTOUtils::mapToJsonString(endpoint.metadata); // Convert map to JSON string

    return data;
}

ERP::Integration::DTO::APIEndpointDTO APIEndpointDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Integration::DTO::APIEndpointDTO endpoint;
    ERP::Utils::DTOUtils::fromMap(data, endpoint); // BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "integration_config_id", endpoint.integrationConfigId);
        ERP::DAOHelpers::getPlainValue(data, "endpoint_code", endpoint.endpointCode);
        
        int methodInt;
        ERP::DAOHelpers::getPlainValue(data, "method", methodInt);
        endpoint.method = static_cast<ERP::Integration::DTO::HTTPMethod>(methodInt);

        ERP::DAOHelpers::getPlainValue(data, "url", endpoint.url);
        ERP::DAOHelpers::getOptionalStringValue(data, "description", endpoint.description);
        ERP::DAOHelpers::getOptionalStringValue(data, "request_schema", endpoint.requestSchema);
        ERP::DAOHelpers::getOptionalStringValue(data, "response_schema", endpoint.responseSchema);
        
        // Convert JSON string to map
        std::string metadataJsonString;
        ERP::DAOHelpers::getPlainValue(data, "metadata_json", metadataJsonString);
        endpoint.metadata = ERP::Utils::DTOUtils::jsonStringToMap(metadataJsonString);

    } catch (const std::bad_any_cast& e) {
        ERP::Logger::Logger::getInstance().error("APIEndpointDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "APIEndpointDAO: Data type mismatch in fromMap: " + std::string(e.what()));
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("APIEndpointDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "APIEndpointDAO: Unexpected error in fromMap: " + std::string(e.what()));
    }
    return endpoint;
}

bool APIEndpointDAO::save(const ERP::Integration::DTO::APIEndpointDTO& endpoint) {
    return create(endpoint);
}

std::optional<ERP::Integration::DTO::APIEndpointDTO> APIEndpointDAO::findById(const std::string& id) {
    return getById(id);
}

bool APIEndpointDAO::update(const ERP::Integration::DTO::APIEndpointDTO& endpoint) {
    return DAOBase<ERP::Integration::DTO::APIEndpointDTO>::update(endpoint);
}

bool APIEndpointDAO::remove(const std::string& id) {
    return DAOBase<ERP::Integration::DTO::APIEndpointDTO>::remove(id);
}

std::vector<ERP::Integration::DTO::APIEndpointDTO> APIEndpointDAO::findAll() {
    return DAOBase<ERP::Integration::DTO::APIEndpointDTO>::findAll();
}

std::vector<ERP::Integration::DTO::APIEndpointDTO> APIEndpointDAO::getAPIEndpointsByIntegrationConfigId(const std::string& integrationConfigId) {
    std::map<std::string, std::any> filters;
    filters["integration_config_id"] = integrationConfigId;
    return get(filters); // Use templated get
}

std::vector<ERP::Integration::DTO::APIEndpointDTO> APIEndpointDAO::getAPIEndpoints(const std::map<std::string, std::any>& filters) {
    return get(filters); // Use templated get
}

int APIEndpointDAO::countAPIEndpoints(const std::map<std::string, std::any>& filters) {
    return count(filters); // Use templated count
}

bool APIEndpointDAO::createAPIEndpoint(const ERP::Integration::DTO::APIEndpointDTO& endpoint, const std::string& integrationConfigId) {
    // This method assumes 'endpoint.integrationConfigId' is already set correctly,
    // or it can set it internally from the passed integrationConfigId.
    // For consistency with other DAOs, let's assume DTO is fully prepared.
    return save(endpoint);
}

bool APIEndpointDAO::removeAPIEndpointsByIntegrationConfigId(const std::string& integrationConfigId) {
    std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
    if (!conn) {
        ERP::Logger::Logger::getInstance().error("APIEndpointDAO::removeAPIEndpointsByIntegrationConfigId: Failed to get database connection.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
        return false;
    }
    
    std::string sql = "DELETE FROM " + tableName_ + " WHERE integration_config_id = :integration_config_id;";
    std::map<std::string, std::any> params;
    params["integration_config_id"] = integrationConfigId;

    bool success = conn->execute(sql, params);
    if (!success) {
        ERP::Logger::Logger::getInstance().error("APIEndpointDAO::removeAPIEndpointsByIntegrationConfigId: Failed to remove API endpoints for integration_config_id " + integrationConfigId + ". Error: " + conn->getLastError());
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove API endpoints.", "Không thể xóa điểm cuối API.");
    }
    connectionPool_->releaseConnection(conn);
    return success;
}

} // namespace DAOs
} // namespace Integration
} // namespace ERP