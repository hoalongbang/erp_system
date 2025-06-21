// Modules/Warehouse/DAO/InventoryCostLayerDAO.cpp
#include "InventoryCostLayerDAO.h" // Đã rút gọn include
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

InventoryCostLayerDAO::InventoryCostLayerDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Warehouse::DTO::InventoryCostLayerDTO>(connectionPool, "inventory_cost_layers") { // Pass table name for InventoryCostLayerDTO
    Logger::Logger::getInstance().info("InventoryCostLayerDAO: Initialized.");
}

// toMap for InventoryCostLayerDTO
std::map<std::string, std::any> InventoryCostLayerDAO::toMap(const ERP::Warehouse::DTO::InventoryCostLayerDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["product_id"] = dto.productId;
    data["warehouse_id"] = dto.warehouseId;
    data["location_id"] = dto.locationId;
    ERP::DAOHelpers::putOptionalString(data, "lot_number", dto.lotNumber);
    ERP::DAOHelpers::putOptionalString(data, "serial_number", dto.serialNumber);
    data["quantity"] = dto.quantity;
    data["unit_cost"] = dto.unitCost;
    data["receipt_date"] = ERP::Utils::DateUtils::formatDateTime(dto.receiptDate, ERP::Common::DATETIME_FORMAT);
    ERP::DAOHelpers::putOptionalString(data, "reference_transaction_id", dto.referenceTransactionId);
    ERP::DAOHelpers::putOptionalString(data, "reference_document_type", dto.referenceDocumentType);
    ERP::DAOHelpers::putOptionalString(data, "reference_document_number", dto.referenceDocumentNumber);

    return data;
}

// fromMap for InventoryCostLayerDTO
ERP::Warehouse::DTO::InventoryCostLayerDTO InventoryCostLayerDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Warehouse::DTO::InventoryCostLayerDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "product_id", dto.productId);
        ERP::DAOHelpers::getPlainValue(data, "warehouse_id", dto.warehouseId);
        ERP::DAOHelpers::getPlainValue(data, "location_id", dto.locationId);
        ERP::DAOHelpers::getOptionalStringValue(data, "lot_number", dto.lotNumber);
        ERP::DAOHelpers::getOptionalStringValue(data, "serial_number", dto.serialNumber);
        ERP::DAOHelpers::getPlainValue(data, "quantity", dto.quantity);
        ERP::DAOHelpers::getPlainValue(data, "unit_cost", dto.unitCost);
        ERP::DAOHelpers::getPlainTimeValue(data, "receipt_date", dto.receiptDate);
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_transaction_id", dto.referenceTransactionId);
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_type", dto.referenceDocumentType);
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_number", dto.referenceDocumentNumber);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("InventoryCostLayerDAO: fromMap - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "InventoryCostLayerDAO: Data type mismatch in fromMap.");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("InventoryCostLayerDAO: fromMap - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "InventoryCostLayerDAO: Unexpected error in fromMap.");
    }
    return dto;
}

} // namespace DAOs
} // namespace Warehouse
} // namespace ERP