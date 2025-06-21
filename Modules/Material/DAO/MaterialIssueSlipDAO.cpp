// Modules/Material/DAO/MaterialIssueSlipDAO.cpp
#include "Modules/Material/DAO/MaterialIssueSlipDAO.h"
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

MaterialIssueSlipDAO::MaterialIssueSlipDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Material::DTO::MaterialIssueSlipDTO>(connectionPool, "material_issue_slips") { // Pass table name for Slip DTO
    Logger::Logger::getInstance().info("MaterialIssueSlipDAO: Initialized.");
}

// toMap for MaterialIssueSlipDTO
std::map<std::string, std::any> MaterialIssueSlipDAO::toMap(const ERP::Material::DTO::MaterialIssueSlipDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["issue_number"] = dto.issueNumber;
    data["production_order_id"] = dto.productionOrderId;
    data["warehouse_id"] = dto.warehouseId;
    data["issued_by_user_id"] = dto.issuedByUserId;
    data["issue_date"] = ERP::Utils::DateUtils::formatDateTime(dto.issueDate, ERP::Common::DATETIME_FORMAT);
    data["status"] = static_cast<int>(dto.status);
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);

    return data;
}

// fromMap for MaterialIssueSlipDTO
ERP::Material::DTO::MaterialIssueSlipDTO MaterialIssueSlipDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Material::DTO::MaterialIssueSlipDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "issue_number", dto.issueNumber);
        ERP::DAOHelpers::getPlainValue(data, "production_order_id", dto.productionOrderId);
        ERP::DAOHelpers::getPlainValue(data, "warehouse_id", dto.warehouseId);
        ERP::DAOHelpers::getPlainValue(data, "issued_by_user_id", dto.issuedByUserId);
        ERP::DAOHelpers::getPlainTimeValue(data, "issue_date", dto.issueDate);
        
        int statusInt;
        if (ERP::DAOHelpers::getPlainValue(data, "status", statusInt)) dto.status = static_cast<ERP::Material::DTO::MaterialIssueSlipStatus>(statusInt);
        
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("MaterialIssueSlipDAO: fromMap (Slip) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "MaterialIssueSlipDAO: Data type mismatch in fromMap (Slip).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("MaterialIssueSlipDAO: fromMap (Slip) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "MaterialIssueSlipDAO: Unexpected error in fromMap (Slip).");
    }
    return dto;
}

// --- MaterialIssueSlipDetailDTO specific methods and helpers ---
std::map<std::string, std::any> MaterialIssueSlipDAO::toMap(const ERP::Material::DTO::MaterialIssueSlipDetailDTO& dto) {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["material_issue_slip_id"] = dto.materialIssueSlipId;
    data["product_id"] = dto.productId;
    data["issued_quantity"] = dto.issuedQuantity;
    ERP::DAOHelpers::putOptionalString(data, "lot_number", dto.lotNumber);
    ERP::DAOHelpers::putOptionalString(data, "serial_number", dto.serialNumber);
    ERP::DAOHelpers::putOptionalString(data, "inventory_transaction_id", dto.inventoryTransactionId);
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);

    return data;
}

ERP::Material::DTO::MaterialIssueSlipDetailDTO MaterialIssueSlipDAO::fromMap(const std::map<std::string, std::any>& data) {
    ERP::Material::DTO::MaterialIssueSlipDetailDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "material_issue_slip_id", dto.materialIssueSlipId);
        ERP::DAOHelpers::getPlainValue(data, "product_id", dto.productId);
        ERP::DAOHelpers::getPlainValue(data, "issued_quantity", dto.issuedQuantity);
        ERP::DAOHelpers::getOptionalStringValue(data, "lot_number", dto.lotNumber);
        ERP::DAOHelpers::getOptionalStringValue(data, "serial_number", dto.serialNumber);
        ERP::DAOHelpers::getOptionalStringValue(data, "inventory_transaction_id", dto.inventoryTransactionId);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("MaterialIssueSlipDAO: fromMap (Detail) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "MaterialIssueSlipDAO: Data type mismatch in fromMap (Detail).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("MaterialIssueSlipDAO: fromMap (Detail) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "MaterialIssueSlipDAO: Unexpected error in fromMap (Detail).");
    }
    return dto;
}

bool MaterialIssueSlipDAO::createMaterialIssueSlipDetail(const ERP::Material::DTO::MaterialIssueSlipDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipDAO: Attempting to create new material issue slip detail.");
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

    std::string sql = "INSERT INTO " + materialIssueSlipDetailsTableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "MaterialIssueSlipDAO", "createMaterialIssueSlipDetail", sql, params
    );
}

std::optional<ERP::Material::DTO::MaterialIssueSlipDetailDTO> MaterialIssueSlipDAO::getMaterialIssueSlipDetailById(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipDAO: Attempting to get material issue slip detail by ID: " + id);
    std::string sql = "SELECT * FROM " + materialIssueSlipDetailsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "MaterialIssueSlipDAO", "getMaterialIssueSlipDetailById", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front());
    }
    return std::nullopt;
}

std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> MaterialIssueSlipDAO::getMaterialIssueSlipDetailsByIssueSlipId(const std::string& issueSlipId) {
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipDAO: Retrieving material issue slip details for slip ID: " + issueSlipId);
    std::string sql = "SELECT * FROM " + materialIssueSlipDetailsTableName_ + " WHERE material_issue_slip_id = ?;";
    std::map<std::string, std::any> params;
    params["material_issue_slip_id"] = issueSlipId;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "MaterialIssueSlipDAO", "getMaterialIssueSlipDetailsByIssueSlipId", sql, params
    );

    std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> resultsDto;
    for (const auto& rowMap : resultsMap) {
        resultsDto.push_back(fromMap(rowMap));
    }
    return resultsDto;
}

bool MaterialIssueSlipDAO::updateMaterialIssueSlipDetail(const ERP::Material::DTO::MaterialIssueSlipDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipDAO: Attempting to update material issue slip detail with ID: " + detail.id);
    std::map<std::string, std::any> data = toMap(detail);
    if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
        ERP::Logger::Logger::getInstance().warning("MaterialIssueSlipDAO: Update detail called with empty data or missing ID.");
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "MaterialIssueSlipDAO: Update detail called with empty data or missing ID.");
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

    std::string sql = "UPDATE " + materialIssueSlipDetailsTableName_ + " SET " + setClause + " WHERE id = ?;";
    params["id_filter"] = detail.id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "MaterialIssueSlipDAO", "updateMaterialIssueSlipDetail", sql, params
    );
}

bool MaterialIssueSlipDAO::removeMaterialIssueSlipDetail(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipDAO: Attempting to remove material issue slip detail with ID: " + id);
    std::string sql = "DELETE FROM " + materialIssueSlipDetailsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "MaterialIssueSlipDAO", "removeMaterialIssueSlipDetail", sql, params
    );
}

bool MaterialIssueSlipDAO::removeMaterialIssueSlipDetailsByIssueSlipId(const std::string& issueSlipId) {
    ERP::Logger::Logger::getInstance().info("MaterialIssueSlipDAO: Attempting to remove all details for material issue slip ID: " + issueSlipId);
    std::string sql = "DELETE FROM " + materialIssueSlipDetailsTableName_ + " WHERE material_issue_slip_id = ?;";
    std::map<std::string, std::any> params;
    params["material_issue_slip_id"] = issueSlipId;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "MaterialIssueSlipDAO", "removeMaterialIssueSlipDetailsByIssueSlipId", sql, params
    );
}


} // namespace DAOs
} // namespace Material
} // namespace ERP