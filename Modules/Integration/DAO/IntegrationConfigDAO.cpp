// Modules/Integration/DAO/IntegrationConfigDAO.cpp
#include "IntegrationConfigDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions

namespace ERP {
namespace Integration {
namespace DAOs {

IntegrationConfigDAO::IntegrationConfigDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Integration::DTO::IntegrationConfigDTO>(connectionPool, "integration_configs") {
    ERP::Logger::Logger::getInstance().info("IntegrationConfigDAO: Initialized.");
}

std::map<std::string, std::any> IntegrationConfigDAO::toMap(const ERP::Integration::DTO::IntegrationConfigDTO& config) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(config); // BaseDTO fields

    data["system_name"] = config.systemName;
    data["system_code"] = config.systemCode;
    data["type"] = static_cast<int>(config.type);
    ERP::DAOHelpers::putOptionalString(data, "base_url", config.baseUrl);
    ERP::DAOHelpers::putOptionalString(data, "username", config.username);
    ERP::DAOHelpers::putOptionalString(data, "password", config.password); // Store encrypted/plain as is
    data["is_encrypted"] = config.isEncrypted;
    data["metadata_json"] = ERP::Utils::DTOUtils::mapToJsonString(config.metadata); // Convert map to JSON string

    return data;
}

ERP::Integration::DTO::IntegrationConfigDTO IntegrationConfigDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Integration::DTO::IntegrationConfigDTO config;
    ERP::Utils::DTOUtils::fromMap(data, config); // BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "system_name", config.systemName);
        ERP::DAOHelpers::getPlainValue(data, "system_code", config.systemCode);
        
        int typeInt;
        ERP::DAOHelpers::getPlainValue(data, "type", typeInt);
        config.type = static_cast<ERP::Integration::DTO::IntegrationType>(typeInt);

        ERP::DAOHelpers::getOptionalStringValue(data, "base_url", config.baseUrl);
        ERP::DAOHelpers::getOptionalStringValue(data, "username", config.username);
        ERP::DAOHelpers::getOptionalStringValue(data, "password", config.password);
        ERP::DAOHelpers::getPlainValue(data, "is_encrypted", config.isEncrypted);
        
        // Convert JSON string to map
        std::string metadataJsonString;
        ERP::DAOHelpers::getPlainValue(data, "metadata_json", metadataJsonString);
        config.metadata = ERP::Utils::DTOUtils::jsonStringToMap(metadataJsonString);

    } catch (const std::bad_any_cast& e) {
        ERP::Logger::Logger::getInstance().error("IntegrationConfigDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "IntegrationConfigDAO: Data type mismatch in fromMap: " + std::string(e.what()));
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("IntegrationConfigDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "IntegrationConfigDAO: Unexpected error in fromMap: " + std::string(e.what()));
    }
    return config;
}

bool IntegrationConfigDAO::save(const ERP::Integration::DTO::IntegrationConfigDTO& config) {
    return create(config);
}

std::optional<ERP::Integration::DTO::IntegrationConfigDTO> IntegrationConfigDAO::findById(const std::string& id) {
    return getById(id);
}

bool IntegrationConfigDAO::update(const ERP::Integration::DTO::IntegrationConfigDTO& config) {
    return DAOBase<ERP::Integration::DTO::IntegrationConfigDTO>::update(config);
}

bool IntegrationConfigDAO::remove(const std::string& id) {
    return DAOBase<ERP::Integration::DTO::IntegrationConfigDTO>::remove(id);
}

std::vector<ERP::Integration::DTO::IntegrationConfigDTO> IntegrationConfigDAO::findAll() {
    return DAOBase<ERP::Integration::DTO::IntegrationConfigDTO>::findAll();
}

std::optional<ERP::Integration::DTO::IntegrationConfigDTO> IntegrationConfigDAO::getIntegrationConfigBySystemCode(const std::string& systemCode) {
    std::map<std::string, std::any> filters;
    filters["system_code"] = systemCode;
    std::vector<ERP::Integration::DTO::IntegrationConfigDTO> results = get(filters); // Use templated get
    if (!results.empty()) {
        return results[0];
    }
    return std::nullopt;
}

std::vector<ERP::Integration::DTO::IntegrationConfigDTO> IntegrationConfigDAO::getIntegrationConfigs(const std::map<std::string, std::any>& filters) {
    return get(filters); // Use templated get
}

int IntegrationConfigDAO::countIntegrationConfigs(const std::map<std::string, std::any>& filters) {
    return count(filters); // Use templated count
}

// --- APIEndpoint operations (nested/related entities) ---

std::map<std::string, std::any> IntegrationConfigDAO::apiEndpointToMap(const ERP::Integration::DTO::APIEndpointDTO& endpoint) const {
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

ERP::Integration::DTO::APIEndpointDTO IntegrationConfigDAO::apiEndpointFromMap(const std::map<std::string, std::any>& data) const {
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
        ERP::Logger::Logger::getInstance().error("IntegrationConfigDAO: apiEndpointFromMap - Data type mismatch during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "IntegrationConfigDAO: Data type mismatch in apiEndpointFromMap: " + std::string(e.what()));
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("IntegrationConfigDAO: apiEndpointFromMap - Unexpected error during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "IntegrationConfigDAO: Unexpected error in apiEndpointFromMap: " + std::string(e.what()));
    }
    return endpoint;
}

bool IntegrationConfigDAO::createAPIEndpoint(const ERP::Integration::DTO::APIEndpointDTO& endpoint, const std::string& integrationConfigId) {
    std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
    if (!conn) {
        ERP::Logger::Logger::getInstance().error("IntegrationConfigDAO::createAPIEndpoint: Failed to get database connection.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
        return false;
    }

    std::string sql = "INSERT INTO " + apiEndpointTableName_ + " (id, integration_config_id, endpoint_code, method, url, description, request_schema, response_schema, metadata_json, status, created_at, created_by, updated_at, updated_by) VALUES (:id, :integration_config_id, :endpoint_code, :method, :url, :description, :request_schema, :response_schema, :metadata_json, :status, :created_at, :created_by, :updated_at, :updated_by);";
    
    std::map<std::string, std::any> params = apiEndpointToMap(endpoint);
    // Remove updated_at/by as they are not used in create
    params.erase("updated_at");
    params.erase("updated_by");

    bool success = conn->execute(sql, params);
    if (!success) {
        ERP::Logger::Logger::getInstance().error("IntegrationConfigDAO::createAPIEndpoint: Failed to create API endpoint. Error: " + conn->getLastError());
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to create API endpoint.", "Không thể tạo điểm cuối API.");
    }
    connectionPool_->releaseConnection(conn);
    return success;
}

std::optional<ERP::Integration::DTO::APIEndpointDTO> IntegrationConfigDAO::getAPIEndpointById(const std::string& endpointId) {
    std::map<std::string, std::any> filters;
    filters["id"] = endpointId;
    std::vector<ERP::Integration::DTO::APIEndpointDTO> results = getAPIEndpoints(filters);
    if (!results.empty()) {
        return results[0];
    }
    return std::nullopt;
}


std::vector<ERP::Integration::DTO::APIEndpointDTO> IntegrationConfigDAO::getAPIEndpointsByIntegrationConfigId(const std::string& integrationConfigId) {
    std::map<std::string, std::any> filters;
    filters["integration_config_id"] = integrationConfigId;
    return getAPIEndpoints(filters); // Use templated get
}

std::vector<ERP::Integration::DTO::APIEndpointDTO> IntegrationConfigDAO::getAPIEndpoints(const std::map<std::string, std::any>& filters) {
    std::vector<std::map<std::string, std::any>> results = executeQuery(apiEndpointTableName_, filters);
    std::vector<ERP::Integration::DTO::APIEndpointDTO> endpoints;
    for (const auto& row : results) {
        endpoints.push_back(apiEndpointFromMap(row));
    }
    return endpoints;
}

int IntegrationConfigDAO::countAPIEndpoints(const std::map<std::string, std::any>& filters) {
    std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
    if (!conn) {
        ERP::Logger::Logger::getInstance().error("IntegrationConfigDAO::countAPIEndpoints: Failed to get database connection.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
        return 0;
    }

    std::string sql = "SELECT COUNT(*) FROM " + apiEndpointTableName_;
    std::string whereClause = buildWhereClause(filters);
    sql += whereClause;

    std::vector<std::map<std::string, std::any>> results = conn->query(sql, filters);
    connectionPool_->releaseConnection(conn);

    if (!results.empty() && results[0].count("COUNT(*)")) {
        return std::any_cast<long long>(results[0].at("COUNT(*)")); // SQLite COUNT(*) returns long long
    }
    return 0;
}

bool IntegrationConfigDAO::removeAPIEndpointsByIntegrationConfigId(const std::string& integrationConfigId) {
    std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
    if (!conn) {
        ERP::Logger::Logger::getInstance().error("IntegrationConfigDAO::removeAPIEndpointsByIntegrationConfigId: Failed to get database connection.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
        return false;
    }
    
    std::string sql = "DELETE FROM " + apiEndpointTableName_ + " WHERE integration_config_id = :integration_config_id;";
    std::map<std::string, std::any> params;
    params["integration_config_id"] = integrationConfigId;

    bool success = conn->execute(sql, params);
    if (!success) {
        ERP::Logger::Logger::getInstance().error("IntegrationConfigDAO::removeAPIEndpointsByIntegrationConfigId: Failed to remove API endpoints for integration_config_id " + integrationConfigId + ". Error: " + conn->getLastError());
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove API endpoints.", "Không thể xóa điểm cuối API.");
    }
    connectionPool_->releaseConnection(conn);
    return success;
}

} // namespace DAOs
} // namespace Integration
} // namespace ERP