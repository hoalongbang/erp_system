// Modules/Manufacturing/DAO/BillOfMaterialDAO.cpp
#include "Modules/Manufacturing/DAO/BillOfMaterialDAO.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "DAOHelpers.h"
#include "Modules/Utils/DTOUtils.h" // For common DTO to map conversions
#include <nlohmann/json.hpp> // For JSON serialization/deserialization of std::map<string, any>
#include <sstream>
#include <stdexcept>
#include <typeinfo> // For std::bad_any_cast

namespace ERP {
namespace Manufacturing {
namespace DAOs {

using json = nlohmann::json;

BillOfMaterialDAO::BillOfMaterialDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Manufacturing::DTO::BillOfMaterialDTO>(connectionPool, "bill_of_materials") { // Pass table name for BillOfMaterialDTO
    Logger::Logger::getInstance().info("BillOfMaterialDAO: Initialized.");
}

// toMap for BillOfMaterialDTO
std::map<std::string, std::any> BillOfMaterialDAO::toMap(const ERP::Manufacturing::DTO::BillOfMaterialDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["bom_name"] = dto.bomName;
    data["product_id"] = dto.productId;
    ERP::DAOHelpers::putOptionalString(data, "description", dto.description);
    data["base_quantity_unit_id"] = dto.baseQuantityUnitId;
    data["base_quantity"] = dto.baseQuantity;
    data["status"] = static_cast<int>(dto.status);
    ERP::DAOHelpers::putOptionalIntValue(data, "version", dto.version);

    // Serialize std::map<string, any> to JSON string for metadata
    try {
        if (!dto.metadata.empty()) {
            json metadataJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.metadata));
            data["metadata_json"] = metadataJson.dump();
        } else {
            data["metadata_json"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("BillOfMaterialDAO: toMap - Error serializing metadata: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "BillOfMaterialDAO: Error serializing metadata.");
        data["metadata_json"] = "";
    }
    
    // items are handled in specific methods, not directly in BOM table

    return data;
}

// fromMap for BillOfMaterialDTO
ERP::Manufacturing::DTO::BillOfMaterialDTO BillOfMaterialDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Manufacturing::DTO::BillOfMaterialDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "bom_name", dto.bomName);
        ERP::DAOHelpers::getPlainValue(data, "product_id", dto.productId);
        ERP::DAOHelpers::getOptionalStringValue(data, "description", dto.description);
        ERP::DAOHelpers::getPlainValue(data, "base_quantity_unit_id", dto.baseQuantityUnitId);
        ERP::DAOHelpers::getPlainValue(data, "base_quantity", dto.baseQuantity);
        
        int statusInt;
        if (ERP::DAOHelpers::getPlainValue(data, "status", statusInt)) dto.status = static_cast<ERP::Manufacturing::DTO::BillOfMaterialStatus>(statusInt);
        
        ERP::DAOHelpers::getOptionalIntValue(data, "version", dto.version);

        // Deserialize JSON string to std::map<string, any> for metadata
        if (data.count("metadata_json") && data.at("metadata_json").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("metadata_json"));
            if (!jsonStr.empty()) {
                dto.metadata = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("BillOfMaterialDAO: fromMap (BOM) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "BillOfMaterialDAO: Data type mismatch in fromMap (BOM).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("BillOfMaterialDAO: fromMap (BOM) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "BillOfMaterialDAO: Unexpected error in fromMap (BOM).");
    }
    return dto;
}

// --- BillOfMaterialItemDTO specific methods and helpers ---
std::map<std::string, std::any> BillOfMaterialDAO::toMap(const ERP::Manufacturing::DTO::BillOfMaterialItemDTO& dto) {
    std::map<std::string, std::any> data;
    // BillOfMaterialItemDTO doesn't inherit BaseDTO, so populate fields directly
    data["id"] = dto.id;
    data["product_id"] = dto.productId;
    data["quantity"] = dto.quantity;
    data["unit_of_measure_id"] = dto.unitOfMeasureId;
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);
    
    // Serialize std::map<string, any> to JSON string for metadata
    try {
        if (!dto.metadata.empty()) {
            json metadataJson = json::parse(ERP::Utils::DTOUtils::mapToJsonString(dto.metadata));
            data["metadata"] = metadataJson.dump();
        } else {
            data["metadata"] = std::string("");
        }
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("BillOfMaterialDAO: toMap (Item) - Error serializing metadata: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "BillOfMaterialDAO: Error serializing item metadata.");
        data["metadata"] = "";
    }
    return data;
}

ERP::Manufacturing::DTO::BillOfMaterialItemDTO BillOfMaterialDAO::fromMap(const std::map<std::string, std::any>& data) {
    ERP::Manufacturing::DTO::BillOfMaterialItemDTO dto;
    try {
        ERP::DAOHelpers::getPlainValue(data, "id", dto.id);
        ERP::DAOHelpers::getPlainValue(data, "product_id", dto.productId);
        ERP::DAOHelpers::getPlainValue(data, "quantity", dto.quantity);
        ERP::DAOHelpers::getPlainValue(data, "unit_of_measure_id", dto.unitOfMeasureId);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);

        // Deserialize JSON string to std::map<string, any> for metadata
        if (data.count("metadata") && data.at("metadata").type() == typeid(std::string)) {
            std::string jsonStr = std::any_cast<std::string>(data.at("metadata"));
            if (!jsonStr.empty()) {
                dto.metadata = ERP::Utils::DTOUtils::jsonStringToMap(jsonStr);
            }
        }
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("BillOfMaterialDAO: fromMap (Item) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "BillOfMaterialDAO: Data type mismatch in fromMap (Item).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("BillOfMaterialDAO: fromMap (Item) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "BillOfMaterialDAO: Unexpected error in fromMap (Item).");
    }
    return dto;
}

bool BillOfMaterialDAO::createBomItem(const ERP::Manufacturing::DTO::BillOfMaterialItemDTO& item, const std::string& bomId) {
    ERP::Logger::Logger::getInstance().info("BillOfMaterialDAO: Attempting to create new BOM item for BOM ID: " + bomId);
    std::map<std::string, std::any> data = toMap(item);
    data["bom_id"] = bomId; // Add BOM ID to the map for insertion
    
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

    std::string sql = "INSERT INTO " + bomItemsTableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "BillOfMaterialDAO", "createBomItem", sql, params
    );
}

std::optional<ERP::Manufacturing::DTO::BillOfMaterialItemDTO> BillOfMaterialDAO::getBomItemById(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("BillOfMaterialDAO: Attempting to get BOM item by ID: " + id);
    std::string sql = "SELECT * FROM " + bomItemsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "BillOfMaterialDAO", "getBomItemById", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front());
    }
    return std::nullopt;
}

std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO> BillOfMaterialDAO::getBomItemsByBomId(const std::string& bomId) {
    ERP::Logger::Logger::getInstance().info("BillOfMaterialDAO: Retrieving BOM items for BOM ID: " + bomId);
    std::string sql = "SELECT * FROM " + bomItemsTableName_ + " WHERE bom_id = ?;";
    std::map<std::string, std::any> params;
    params["bom_id"] = bomId;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "BillOfMaterialDAO", "getBomItemsByBomId", sql, params
    );

    std::vector<ERP::Manufacturing::DTO::BillOfMaterialItemDTO> resultsDto;
    for (const auto& rowMap : resultsMap) {
        resultsDto.push_back(fromMap(rowMap));
    }
    return resultsDto;
}

bool BillOfMaterialDAO::updateBomItem(const ERP::Manufacturing::DTO::BillOfMaterialItemDTO& item) {
    ERP::Logger::Logger::getInstance().info("BillOfMaterialDAO: Attempting to update BOM item with ID: " + item.id);
    std::map<std::string, std::any> data = toMap(item);
    if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
        ERP::Logger::Logger::getInstance().warning("BillOfMaterialDAO: Update BOM item called with empty data or missing ID.");
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "BillOfMaterialDAO: Update BOM item called with empty data or missing ID.");
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

    std::string sql = "UPDATE " + bomItemsTableName_ + " SET " + setClause + " WHERE id = ?;";
    params["id_filter"] = item.id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "BillOfMaterialDAO", "updateBomItem", sql, params
    );
}

bool BillOfMaterialDAO::removeBomItem(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("BillOfMaterialDAO: Attempting to remove BOM item with ID: " + id);
    std::string sql = "DELETE FROM " + bomItemsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "BillOfMaterialDAO", "removeBomItem", sql, params
    );
}

bool BillOfMaterialDAO::removeBomItemsByBomId(const std::string& bomId) {
    ERP::Logger::Logger::getInstance().info("BillOfMaterialDAO: Attempting to remove all BOM items for BOM ID: " + bomId);
    std::string sql = "DELETE FROM " + bomItemsTableName_ + " WHERE bom_id = ?;";
    std::map<std::string, std::any> params;
    params["bom_id"] = bomId;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "BillOfMaterialDAO", "removeBomItemsByBomId", sql, params
    );
}


} // namespace DAOs
} // namespace Manufacturing
} // namespace ERP