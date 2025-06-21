// Modules/Material/DAO/MaterialRequestSlipDAO.cpp
#include "Modules/Material/DAO/MaterialRequestSlipDAO.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "DAOHelpers.h"
#include "Modules/Utils/DTOUtils.h" // For common DTO to map conversions
#include <sstream>
#include <stdexcept>
#include <typeinfo> // For std::bad_any_cast

namespace ERP {
namespace Material {
namespace DAOs {

MaterialRequestSlipDAO::MaterialRequestSlipDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Material::DTO::MaterialRequestSlipDTO>(connectionPool, "material_request_slips") { // Pass table name for Slip DTO
    Logger::Logger::getInstance().info("MaterialRequestSlipDAO: Initialized.");
}

// toMap for MaterialRequestSlipDTO
std::map<std::string, std::any> MaterialRequestSlipDAO::toMap(const ERP::Material::DTO::MaterialRequestSlipDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["request_number"] = dto.requestNumber;
    data["requesting_department"] = dto.requestingDepartment;
    data["requested_by_user_id"] = dto.requestedByUserId;
    data["request_date"] = ERP::Utils::DateUtils::formatDateTime(dto.requestDate, ERP::Common::DATETIME_FORMAT);
    data["status"] = static_cast<int>(dto.status);
    ERP::DAOHelpers::putOptionalString(data, "approved_by_user_id", dto.approvedByUserId);
    ERP::DAOHelpers::putOptionalTime(data, "approval_date", dto.approvalDate);
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);
    ERP::DAOHelpers::putOptionalString(data, "reference_document_id", dto.referenceDocumentId);
    ERP::DAOHelpers::putOptionalString(data, "reference_document_type", dto.referenceDocumentType);

    return data;
}

// fromMap for MaterialRequestSlipDTO
ERP::Material::DTO::MaterialRequestSlipDTO MaterialRequestSlipDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Material::DTO::MaterialRequestSlipDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "request_number", dto.requestNumber);
        ERP::DAOHelpers::getPlainValue(data, "requesting_department", dto.requestingDepartment);
        ERP::DAOHelpers::getPlainValue(data, "requested_by_user_id", dto.requestedByUserId);
        ERP::DAOHelpers::getPlainTimeValue(data, "request_date", dto.requestDate);
        
        int statusInt;
        if (ERP::DAOHelpers::getPlainValue(data, "status", statusInt)) dto.status = static_cast<ERP::Material::DTO::MaterialRequestSlipStatus>(statusInt);
        
        ERP::DAOHelpers::getOptionalStringValue(data, "approved_by_user_id", dto.approvedByUserId);
        ERP::DAOHelpers::getOptionalTimeValue(data, "approval_date", dto.approvalDate);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_id", dto.referenceDocumentId);
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_type", dto.referenceDocumentType);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("MaterialRequestSlipDAO: fromMap (Slip) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "MaterialRequestSlipDAO: Data type mismatch in fromMap (Slip).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("MaterialRequestSlipDAO: fromMap (Slip) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "MaterialRequestSlipDAO: Unexpected error in fromMap (Slip).");
    }
    return dto;
}

// --- MaterialRequestSlipDetailDTO specific methods and helpers ---
std::map<std::string, std::any> MaterialRequestSlipDAO::toMap(const ERP::Material::DTO::MaterialRequestSlipDetailDTO& dto) {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["material_request_slip_id"] = dto.materialRequestSlipId;
    data["product_id"] = dto.productId;
    data["requested_quantity"] = dto.requestedQuantity;
    data["issued_quantity"] = dto.issuedQuantity;
    data["is_fully_issued"] = dto.isFullyIssued;
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);

    return data;
}

ERP::Material::DTO::MaterialRequestSlipDetailDTO MaterialRequestSlipDAO::fromMap(const std::map<std::string, std::any>& data) {
    ERP::Material::DTO::MaterialRequestSlipDetailDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "material_request_slip_id", dto.materialRequestSlipId);
        ERP::DAOHelpers::getPlainValue(data, "product_id", dto.productId);
        ERP::DAOHelpers::getPlainValue(data, "requested_quantity", dto.requestedQuantity);
        ERP::DAOHelpers::getPlainValue(data, "issued_quantity", dto.issuedQuantity);
        ERP::DAOHelpers::getPlainValue(data, "is_fully_issued", dto.isFullyIssued);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("MaterialRequestSlipDAO: fromMap (Detail) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "MaterialRequestSlipDAO: Data type mismatch in fromMap (Detail).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("MaterialRequestSlipDAO: fromMap (Detail) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "MaterialRequestSlipDAO: Unexpected error in fromMap (Detail).");
    }
    return dto;
}

bool MaterialRequestSlipDAO::createMaterialRequestSlipDetail(const ERP::Material::DTO::MaterialRequestSlipDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("MaterialRequestSlipDAO: Attempting to create new material request slip detail.");
    std::map<std::string, std::any> data = toMap(detail);
    
    std::string columns;
    std::string placeholders;
    std::map<std::string, std::any> params;
    bool first = true;

    for (const auto& pair : data) {
        if (!first) { columns += ", "; placeholders += ", "; }
        columns += pair.first;
        placeholders += "?";
        params[pair.first] = pair.second;
        first = false;
    }

    std::string sql = "INSERT INTO " + materialRequestSlipDetailsTableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "MaterialRequestSlipDAO", "createMaterialRequestSlipDetail", sql, params
    );
}

std::optional<ERP::Material::DTO::MaterialRequestSlipDetailDTO> MaterialRequestSlipDAO::getMaterialRequestSlipDetailById(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("MaterialRequestSlipDAO: Attempting to get material request slip detail by ID: " + id);
    std::string sql = "SELECT * FROM " + materialRequestSlipDetailsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "MaterialRequestSlipDAO", "getMaterialRequestSlipDetailById", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front());
    }
    return std::nullopt;
}

std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> MaterialRequestSlipDAO::getMaterialRequestSlipDetailsByRequestId(const std::string& requestId) {
    ERP::Logger::Logger::getInstance().info("MaterialRequestSlipDAO: Retrieving material request slip details for request ID: " + requestId);
    std::string sql = "SELECT * FROM " + materialRequestSlipDetailsTableName_ + " WHERE material_request_slip_id = ?;";
    std::map<std::string, std::any> params;
    params["material_request_slip_id"] = requestId;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "MaterialRequestSlipDAO", "getMaterialRequestSlipDetailsByRequestId", sql, params
    );

    std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> resultsDto;
    for (const auto& rowMap : resultsMap) {
        resultsDto.push_back(fromMap(rowMap));
    }
    return resultsDto;
}

bool MaterialRequestSlipDAO::updateMaterialRequestSlipDetail(const ERP::Material::DTO::MaterialRequestSlipDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("MaterialRequestSlipDAO: Attempting to update material request slip detail with ID: " + detail.id);
    std::map<std::string, std::any> data = toMap(detail);
    if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
        ERP::Logger::Logger::getInstance().warning("MaterialRequestSlipDAO: Update detail called with empty data or missing ID.");
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "MaterialRequestSlipDAO: Update detail called with empty data or missing ID.");
        return false;
    }

    std::string setClause;
    std::map<std::string, std::any> params;
    bool firstSet = true;

    for (const auto& pair : data) {
        if (pair.first == "id") continue;
        if (!firstSet) setClause += ", ";
        setClause += pair.first + " = ?";
        params[pair.first] = pair.second;
        firstSet = false;
    }

    std::string sql = "UPDATE " + materialRequestSlipDetailsTableName_ + " SET " + setClause + " WHERE id = ?;";
    params["id_filter"] = detail.id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "MaterialRequestSlipDAO", "updateMaterialRequestSlipDetail", sql, params
    );
}

bool MaterialRequestSlipDAO::removeMaterialRequestSlipDetail(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("MaterialRequestSlipDAO: Attempting to remove material request slip detail with ID: " + id);
    std::string sql = "DELETE FROM " + materialRequestSlipDetailsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "MaterialRequestSlipDAO", "removeMaterialRequestSlipDetail", sql, params
    );
}

bool MaterialRequestSlipDAO::removeMaterialRequestSlipDetailsByRequestId(const std::string& requestId) {
    ERP::Logger::Logger::getInstance().info("MaterialRequestSlipDAO: Attempting to remove all details for material request slip ID: " + requestId);
    std::string sql = "DELETE FROM " + materialRequestSlipDetailsTableName_ + " WHERE material_request_slip_id = ?;";
    std::map<std::string, std::any> params;
    params["material_request_slip_id"] = requestId;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "MaterialRequestSlipDAO", "removeMaterialRequestSlipDetailsByRequestId", sql, params
    );
}


} // namespace DAOs
} // namespace Material
} // namespace ERP