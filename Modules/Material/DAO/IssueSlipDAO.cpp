// Modules/Material/DAO/IssueSlipDAO.cpp
#include "Modules/Material/DAO/IssueSlipDAO.h"
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

IssueSlipDAO::IssueSlipDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Material::DTO::IssueSlipDTO>(connectionPool, "issue_slips") { // Pass table name for IssueSlipDTO
    Logger::Logger::getInstance().info("IssueSlipDAO: Initialized.");
}

// toMap for IssueSlipDTO
std::map<std::string, std::any> IssueSlipDAO::toMap(const ERP::Material::DTO::IssueSlipDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["issue_number"] = dto.issueNumber;
    data["warehouse_id"] = dto.warehouseId;
    data["issued_by_user_id"] = dto.issuedByUserId;
    data["issue_date"] = ERP::Utils::DateUtils::formatDateTime(dto.issueDate, ERP::Common::DATETIME_FORMAT);
    ERP::DAOHelpers::putOptionalString(data, "material_request_slip_id", dto.materialRequestSlipId);
    data["status"] = static_cast<int>(dto.status);
    ERP::DAOHelpers::putOptionalString(data, "reference_document_id", dto.referenceDocumentId);
    ERP::DAOHelpers::putOptionalString(data, "reference_document_type", dto.referenceDocumentType);
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);

    return data;
}

// fromMap for IssueSlipDTO
ERP::Material::DTO::IssueSlipDTO IssueSlipDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Material::DTO::IssueSlipDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "issue_number", dto.issueNumber);
        ERP::DAOHelpers::getPlainValue(data, "warehouse_id", dto.warehouseId);
        ERP::DAOHelpers::getPlainValue(data, "issued_by_user_id", dto.issuedByUserId);
        ERP::DAOHelpers::getPlainTimeValue(data, "issue_date", dto.issueDate);
        ERP::DAOHelpers::getOptionalStringValue(data, "material_request_slip_id", dto.materialRequestSlipId);
        
        int statusInt;
        if (ERP::DAOHelpers::getPlainValue(data, "status", statusInt)) dto.status = static_cast<ERP::Material::DTO::IssueSlipStatus>(statusInt);
        
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_id", dto.referenceDocumentId);
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_type", dto.referenceDocumentType);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("IssueSlipDAO: fromMap (Slip) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "IssueSlipDAO: Data type mismatch in fromMap (Slip).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("IssueSlipDAO: fromMap (Slip) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "IssueSlipDAO: Unexpected error in fromMap (Slip).");
    }
    return dto;
}

// --- IssueSlipDetailDTO specific methods and helpers ---
std::map<std::string, std::any> IssueSlipDAO::toMap(const ERP::Material::DTO::IssueSlipDetailDTO& dto) {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["issue_slip_id"] = dto.issueSlipId;
    data["product_id"] = dto.productId;
    data["location_id"] = dto.locationId;
    data["requested_quantity"] = dto.requestedQuantity;
    data["issued_quantity"] = dto.issuedQuantity;
    ERP::DAOHelpers::putOptionalString(data, "lot_number", dto.lotNumber);
    ERP::DAOHelpers::putOptionalString(data, "serial_number", dto.serialNumber);
    ERP::DAOHelpers::putOptionalString(data, "inventory_transaction_id", dto.inventoryTransactionId);
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);
    data["is_fully_issued"] = dto.isFullyIssued;
    ERP::DAOHelpers::putOptionalString(data, "material_request_slip_detail_id", dto.materialRequestSlipDetailId);

    return data;
}

ERP::Material::DTO::IssueSlipDetailDTO IssueSlipDAO::fromMap(const std::map<std::string, std::any>& data) {
    ERP::Material::DTO::IssueSlipDetailDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "issue_slip_id", dto.issueSlipId);
        ERP::DAOHelpers::getPlainValue(data, "product_id", dto.productId);
        ERP::DAOHelpers::getPlainValue(data, "location_id", dto.locationId);
        ERP::DAOHelpers::getPlainValue(data, "requested_quantity", dto.requestedQuantity);
        ERP::DAOHelpers::getPlainValue(data, "issued_quantity", dto.issuedQuantity);
        ERP::DAOHelpers::getOptionalStringValue(data, "lot_number", dto.lotNumber);
        ERP::DAOHelpers::getOptionalStringValue(data, "serial_number", dto.serialNumber);
        ERP::DAOHelpers::getOptionalStringValue(data, "inventory_transaction_id", dto.inventoryTransactionId);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
        ERP::DAOHelpers::getPlainValue(data, "is_fully_issued", dto.isFullyIssued);
        ERP::DAOHelpers::getOptionalStringValue(data, "material_request_slip_detail_id", dto.materialRequestSlipDetailId);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("IssueSlipDAO: fromMap (Detail) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "IssueSlipDAO: Data type mismatch in fromMap (Detail).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("IssueSlipDAO: fromMap (Detail) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "IssueSlipDAO: Unexpected error in fromMap (Detail).");
    }
    return dto;
}

bool IssueSlipDAO::createIssueSlipDetail(const ERP::Material::DTO::IssueSlipDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("IssueSlipDAO: Attempting to create new issue slip detail.");
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

    std::string sql = "INSERT INTO " + issueSlipDetailsTableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "IssueSlipDAO", "createIssueSlipDetail", sql, params
    );
}

std::optional<ERP::Material::DTO::IssueSlipDetailDTO> IssueSlipDAO::getIssueSlipDetailById(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("IssueSlipDAO: Attempting to get issue slip detail by ID: " + id);
    std::string sql = "SELECT * FROM " + issueSlipDetailsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "IssueSlipDAO", "getIssueSlipDetailById", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front());
    }
    return std::nullopt;
}

std::vector<ERP::Material::DTO::IssueSlipDetailDTO> IssueSlipDAO::getIssueSlipDetailsByIssueSlipId(const std::string& issueSlipId) {
    ERP::Logger::Logger::getInstance().info("IssueSlipDAO: Retrieving issue slip details for slip ID: " + issueSlipId);
    std::string sql = "SELECT * FROM " + issueSlipDetailsTableName_ + " WHERE issue_slip_id = ?;";
    std::map<std::string, std::any> params;
    params["issue_slip_id"] = issueSlipId;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "IssueSlipDAO", "getIssueSlipDetailsByIssueSlipId", sql, params
    );

    std::vector<ERP::Material::DTO::IssueSlipDetailDTO> resultsDto;
    for (const auto& rowMap : resultsMap) {
        resultsDto.push_back(fromMap(rowMap));
    }
    return resultsDto;
}

bool IssueSlipDAO::updateIssueSlipDetail(const ERP::Material::DTO::IssueSlipDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("IssueSlipDAO: Attempting to update issue slip detail with ID: " + detail.id);
    std::map<std::string, std::any> data = toMap(detail);
    if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
        ERP::Logger::Logger::getInstance().warning("IssueSlipDAO: Update issue slip detail called with empty data or missing ID.");
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "IssueSlipDAO: Update issue slip detail called with empty data or missing ID.");
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

    std::string sql = "UPDATE " + issueSlipDetailsTableName_ + " SET " + setClause + " WHERE id = ?;";
    params["id_filter"] = detail.id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "IssueSlipDAO", "updateIssueSlipDetail", sql, params
    );
}

bool IssueSlipDAO::removeIssueSlipDetail(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("IssueSlipDAO: Attempting to remove issue slip detail with ID: " + id);
    std::string sql = "DELETE FROM " + issueSlipDetailsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "IssueSlipDAO", "removeIssueSlipDetail", sql, params
    );
}

bool IssueSlipDAO::removeIssueSlipDetailsByIssueSlipId(const std::string& issueSlipId) {
    ERP::Logger::Logger::getInstance().info("IssueSlipDAO: Attempting to remove all details for issue slip ID: " + issueSlipId);
    std::string sql = "DELETE FROM " + issueSlipDetailsTableName_ + " WHERE issue_slip_id = ?;";
    std::map<std::string, std::any> params;
    params["issue_slip_id"] = issueSlipId;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "IssueSlipDAO", "removeIssueSlipDetailsByIssueSlipId", sql, params
    );
}

} // namespace DAOs
} // namespace Material
} // namespace ERP