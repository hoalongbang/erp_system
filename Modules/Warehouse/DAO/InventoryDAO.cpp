// Modules/Warehouse/DAO/InventoryDAO.cpp
#include "InventoryDAO.h" // Đã rút gọn include
#include "Logger.h" // Đã rút gọn include
#include "ErrorHandler.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "DAOHelpers.h" // Đã rút gọn include
#include "DTOUtils.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <typeinfo> // For std::bad_any_cast

namespace ERP {
namespace Warehouse {
namespace DAOs {

InventoryDAO::InventoryDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Warehouse::DTO::InventoryDTO>(connectionPool, "inventory") { // Pass table name for InventoryDTO
    Logger::Logger::getInstance().info("InventoryDAO: Initialized.");
}

// toMap for InventoryDTO
std::map<std::string, std::any> InventoryDAO::toMap(const ERP::Warehouse::DTO::InventoryDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["product_id"] = dto.productId;
    data["warehouse_id"] = dto.warehouseId;
    data["location_id"] = dto.locationId;
    data["quantity"] = dto.quantity;
    ERP::DAOHelpers::putOptionalDouble(data, "reserved_quantity", dto.reservedQuantity);
    ERP::DAOHelpers::putOptionalDouble(data, "available_quantity", dto.availableQuantity);
    ERP::DAOHelpers::putOptionalDouble(data, "unit_cost", dto.unitCost);
    ERP::DAOHelpers::putOptionalString(data, "lot_number", dto.lotNumber);
    ERP::DAOHelpers::putOptionalString(data, "serial_number", dto.serialNumber);
    ERP::DAOHelpers::putOptionalTime(data, "manufacture_date", dto.manufactureDate);
    ERP::DAOHelpers::putOptionalTime(data, "expiration_date", dto.expirationDate);
    ERP::DAOHelpers::putOptionalDouble(data, "reorder_level", dto.reorderLevel);
    ERP::DAOHelpers::putOptionalDouble(data, "reorder_quantity", dto.reorderQuantity);

    return data;
}

// fromMap for InventoryDTO
ERP::Warehouse::DTO::InventoryDTO InventoryDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Warehouse::DTO::InventoryDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "product_id", dto.productId);
        ERP::DAOHelpers::getPlainValue(data, "warehouse_id", dto.warehouseId);
        ERP::DAOHelpers::getPlainValue(data, "location_id", dto.locationId);
        ERP::DAOHelpers::getPlainValue(data, "quantity", dto.quantity);
        ERP::DAOHelpers::getOptionalDoubleValue(data, "reserved_quantity", dto.reservedQuantity);
        ERP::DAOHelpers::getOptionalDoubleValue(data, "available_quantity", dto.availableQuantity);
        ERP::DAOHelpers::getOptionalDoubleValue(data, "unit_cost", dto.unitCost);
        ERP::DAOHelpers::getOptionalStringValue(data, "lot_number", dto.lotNumber);
        ERP::DAOHelpers::getOptionalStringValue(data, "serial_number", dto.serialNumber);
        ERP::DAOHelpers::getOptionalTimeValue(data, "manufacture_date", dto.manufactureDate);
        ERP::DAOHelpers::getOptionalTimeValue(data, "expiration_date", dto.expirationDate);
        ERP::DAOHelpers::getOptionalDoubleValue(data, "reorder_level", dto.reorderLevel);
        ERP::DAOHelpers::getOptionalDoubleValue(data, "reorder_quantity", dto.reorderQuantity);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("InventoryDAO: fromMap - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "InventoryDAO: Data type mismatch in fromMap.");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("InventoryDAO: fromMap - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "InventoryDAO: Unexpected error in fromMap.");
    }
    return dto;
}

} // namespace DAOs
} // namespace Warehouse
} // namespace ERP