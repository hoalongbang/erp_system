// Modules/Sales/DAO/ShipmentDAO.cpp
#include "Modules/Sales/DAO/ShipmentDAO.h"
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
namespace Sales {
namespace DAOs {

ShipmentDAO::ShipmentDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Sales::DTO::ShipmentDTO>(connectionPool, "shipments") { // Pass table name for ShipmentDTO
    Logger::Logger::getInstance().info("ShipmentDAO: Initialized.");
}

// toMap for ShipmentDTO
std::map<std::string, std::any> ShipmentDAO::toMap(const ERP::Sales::DTO::ShipmentDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["shipment_number"] = dto.shipmentNumber;
    data["sales_order_id"] = dto.salesOrderId;
    data["customer_id"] = dto.customerId;
    data["shipped_by_user_id"] = dto.shippedByUserId;
    data["shipment_date"] = ERP::Utils::DateUtils::formatDateTime(dto.shipmentDate, ERP::Common::DATETIME_FORMAT);
    ERP::DAOHelpers::putOptionalTime(data, "delivery_date", dto.deliveryDate);
    data["type"] = static_cast<int>(dto.type);
    data["status"] = static_cast<int>(dto.status);
    ERP::DAOHelpers::putOptionalString(data, "carrier_name", dto.carrierName);
    ERP::DAOHelpers::putOptionalString(data, "tracking_number", dto.trackingNumber);
    ERP::DAOHelpers::putOptionalString(data, "delivery_address", dto.deliveryAddress);
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);

    return data;
}

// fromMap for ShipmentDTO
ERP::Sales::DTO::ShipmentDTO ShipmentDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Sales::DTO::ShipmentDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "shipment_number", dto.shipmentNumber);
        ERP::DAOHelpers::getPlainValue(data, "sales_order_id", dto.salesOrderId);
        ERP::DAOHelpers::getPlainValue(data, "customer_id", dto.customerId);
        ERP::DAOHelpers::getPlainValue(data, "shipped_by_user_id", dto.shippedByUserId);
        ERP::DAOHelpers::getPlainTimeValue(data, "shipment_date", dto.shipmentDate);
        ERP::DAOHelpers::getOptionalTimeValue(data, "delivery_date", dto.deliveryDate);
        
        int typeInt;
        if (ERP::DAOHelpers::getPlainValue(data, "type", typeInt)) dto.type = static_cast<ERP::Sales::DTO::ShipmentType>(typeInt);
        
        int statusInt;
        if (ERP::DAOHelpers::getPlainValue(data, "status", statusInt)) dto.status = static_cast<ERP::Sales::DTO::ShipmentStatus>(statusInt);
        
        ERP::DAOHelpers::getOptionalStringValue(data, "carrier_name", dto.carrierName);
        ERP::DAOHelpers::getOptionalStringValue(data, "tracking_number", dto.trackingNumber);
        ERP::DAOHelpers::getOptionalStringValue(data, "delivery_address", dto.deliveryAddress);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("ShipmentDAO: fromMap (Shipment) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ShipmentDAO: Data type mismatch in fromMap (Shipment).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("ShipmentDAO: fromMap (Shipment) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ShipmentDAO: Unexpected error in fromMap (Shipment).");
    }
    return dto;
}

// --- ShipmentDetailDTO specific methods and helpers ---
std::map<std::string, std::any> ShipmentDAO::toMap(const ERP::Sales::DTO::ShipmentDetailDTO& dto) {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["shipment_id"] = dto.shipmentId;
    data["sales_order_detail_id"] = dto.salesOrderDetailId;
    data["product_id"] = dto.productId;
    data["warehouse_id"] = dto.warehouseId;
    data["location_id"] = dto.locationId;
    data["quantity"] = dto.quantity;
    ERP::DAOHelpers::putOptionalString(data, "lot_number", dto.lotNumber);
    ERP::DAOHelpers::putOptionalString(data, "serial_number", dto.serialNumber);
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);

    return data;
}

ERP::Sales::DTO::ShipmentDetailDTO ShipmentDAO::fromMap(const std::map<std::string, std::any>& data) {
    ERP::Sales::DTO::ShipmentDetailDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "shipment_id", dto.shipmentId);
        ERP::DAOHelpers::getPlainValue(data, "sales_order_detail_id", dto.salesOrderDetailId);
        ERP::DAOHelpers::getPlainValue(data, "product_id", dto.productId);
        ERP::DAOHelpers::getPlainValue(data, "warehouse_id", dto.warehouseId);
        ERP::DAOHelpers::getPlainValue(data, "location_id", dto.locationId);
        ERP::DAOHelpers::getPlainValue(data, "quantity", dto.quantity);
        ERP::DAOHelpers::getOptionalStringValue(data, "lot_number", dto.lotNumber);
        ERP::DAOHelpers::getOptionalStringValue(data, "serial_number", dto.serialNumber);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("ShipmentDAO: fromMap (Detail) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ShipmentDAO: Data type mismatch in fromMap (Detail).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("ShipmentDAO: fromMap (Detail) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ShipmentDAO: Unexpected error in fromMap (Detail).");
    }
    return dto;
}

bool ShipmentDAO::createShipmentDetail(const ERP::Sales::DTO::ShipmentDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("ShipmentDAO: Attempting to create new shipment detail.");
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

    std::string sql = "INSERT INTO " + shipmentDetailsTableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "ShipmentDAO", "createShipmentDetail", sql, params
    );
}

std::optional<ERP::Sales::DTO::ShipmentDetailDTO> ShipmentDAO::getShipmentDetailById(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("ShipmentDAO: Attempting to get shipment detail by ID: " + id);
    std::string sql = "SELECT * FROM " + shipmentDetailsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "ShipmentDAO", "getShipmentDetailById", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front());
    }
    return std::nullopt;
}

std::vector<ERP::Sales::DTO::ShipmentDetailDTO> ShipmentDAO::getShipmentDetailsByShipmentId(const std::string& shipmentId) {
    ERP::Logger::Logger::getInstance().info("ShipmentDAO: Retrieving shipment details for shipment ID: " + shipmentId);
    std::string sql = "SELECT * FROM " + shipmentDetailsTableName_ + " WHERE shipment_id = ?;";
    std::map<std::string, std::any> params;
    params["shipment_id"] = shipmentId;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "ShipmentDAO", "getShipmentDetailsByShipmentId", sql, params
    );

    std::vector<ERP::Sales::DTO::ShipmentDetailDTO> resultsDto;
    for (const auto& rowMap : resultsMap) {
        resultsDto.push_back(fromMap(rowMap));
    }
    return resultsDto;
}

bool ShipmentDAO::updateShipmentDetail(const ERP::Sales::DTO::ShipmentDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("ShipmentDAO: Attempting to update shipment detail with ID: " + detail.id);
    std::map<std::string, std::any> data = toMap(detail);
    if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
        ERP::Logger::Logger::getInstance().warning("ShipmentDAO: Update detail called with empty data or missing ID.");
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ShipmentDAO: Update detail called with empty data or missing ID.");
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

    std::string sql = "UPDATE " + shipmentDetailsTableName_ + " SET " + setClause + " WHERE id = ?;";
    params["id_filter"] = detail.id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "ShipmentDAO", "updateShipmentDetail", sql, params
    );
}

bool ShipmentDAO::removeShipmentDetail(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("ShipmentDAO: Attempting to remove shipment detail with ID: " + id);
    std::string sql = "DELETE FROM " + shipmentDetailsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "ShipmentDAO", "removeShipmentDetail", sql, params
    );
}

bool ShipmentDAO::removeShipmentDetailsByShipmentId(const std::string& shipmentId) {
    ERP::Logger::Logger::getInstance().info("ShipmentDAO: Attempting to remove all details for shipment ID: " + shipmentId);
    std::string sql = "DELETE FROM " + shipmentDetailsTableName_ + " WHERE shipment_id = ?;";
    std::map<std::string, std::any> params;
    params["shipment_id"] = shipmentId;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "ShipmentDAO", "removeShipmentDetailsByShipmentId", sql, params
    );
}

} // namespace DAOs
} // namespace Sales
} // namespace ERP