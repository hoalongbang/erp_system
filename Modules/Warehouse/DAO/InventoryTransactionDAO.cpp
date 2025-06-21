// Modules/Warehouse/DAO/InventoryTransactionDAO.cpp
#include "InventoryTransactionDAO.h" // Đã rút gọn include
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

InventoryTransactionDAO::InventoryTransactionDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Warehouse::DTO::InventoryTransactionDTO>(connectionPool, "inventory_transactions") { // Pass table name for InventoryTransactionDTO
    Logger::Logger::getInstance().info("InventoryTransactionDAO: Initialized.");
}

// toMap for InventoryTransactionDTO
std::map<std::string, std::any> InventoryTransactionDAO::toMap(const ERP::Warehouse::DTO::InventoryTransactionDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["product_id"] = dto.productId;
    data["warehouse_id"] = dto.warehouseId;
    data["location_id"] = dto.locationId;
    data["type"] = static_cast<int>(dto.type);
    data["quantity"] = dto.quantity;
    data["unit_cost"] = dto.unitCost;
    data["transaction_date"] = ERP::Utils::DateUtils::formatDateTime(dto.transactionDate, ERP::Common::DATETIME_FORMAT);
    ERP::DAOHelpers::putOptionalString(data, "lot_number", dto.lotNumber);
    ERP::DAOHelpers::putOptionalString(data, "serial_number", dto.serialNumber);
    ERP::DAOHelpers::putOptionalTime(data, "manufacture_date", dto.manufactureDate);
    ERP::DAOHelpers::putOptionalTime(data, "expiration_date", dto.expirationDate);
    ERP::DAOHelpers::putOptionalString(data, "reference_document_id", dto.referenceDocumentId);
    ERP::DAOHelpers::putOptionalString(data, "reference_document_type", dto.referenceDocumentType);
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);

    return data;
}

// fromMap for InventoryTransactionDTO
ERP::Warehouse::DTO::InventoryTransactionDTO InventoryTransactionDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Warehouse::DTO::InventoryTransactionDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "product_id", dto.productId);
        ERP::DAOHelpers::getPlainValue(data, "warehouse_id", dto.warehouseId);
        ERP::DAOHelpers::getPlainValue(data, "location_id", dto.locationId);
        
        int typeInt;
        if (ERP::DAOHelpers::getPlainValue(data, "type", typeInt)) dto.type = static_cast<ERP::Warehouse::DTO::InventoryTransactionType>(typeInt);
        
        ERP::DAOHelpers::getPlainValue(data, "quantity", dto.quantity);
        ERP::DAOHelpers::getPlainValue(data, "unit_cost", dto.unitCost);
        ERP::DAOHelpers::getPlainTimeValue(data, "transaction_date", dto.transactionDate);
        ERP::DAOHelpers::getOptionalStringValue(data, "lot_number", dto.lotNumber);
        ERP::DAOHelpers::getOptionalStringValue(data, "serial_number", dto.serialNumber);
        ERP::DAOHelpers::getOptionalTimeValue(data, "manufacture_date", dto.manufactureDate);
        ERP::DAOHelpers::getOptionalTimeValue(data, "expiration_date", dto.expirationDate);
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_id", dto.referenceDocumentId);
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_type", dto.referenceDocumentType);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("InventoryTransactionDAO: fromMap - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "InventoryTransactionDAO: Data type mismatch in fromMap.");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("InventoryTransactionDAO: fromMap - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "InventoryTransactionDAO: Unexpected error in fromMap.");
    }
    return dto;
}

} // namespace DAOs
} // namespace Warehouse
} // namespace ERP