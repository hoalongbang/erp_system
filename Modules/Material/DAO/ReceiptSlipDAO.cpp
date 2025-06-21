// Modules/Material/DAO/ReceiptSlipDAO.cpp
#include "ReceiptSlipDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions

namespace ERP {
namespace Material {
namespace DAOs {

ReceiptSlipDAO::ReceiptSlipDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Material::DTO::ReceiptSlipDTO>(connectionPool, "receipt_slips") {
    ERP::Logger::Logger::getInstance().info("ReceiptSlipDAO: Initialized.");
}

std::map<std::string, std::any> ReceiptSlipDAO::toMap(const ERP::Material::DTO::ReceiptSlipDTO& slip) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(slip); // BaseDTO fields

    data["receipt_number"] = slip.receiptNumber;
    data["warehouse_id"] = slip.warehouseId;
    data["received_by_user_id"] = slip.receivedByUserId;
    data["receipt_date"] = ERP::Utils::DateUtils::formatDateTime(slip.receiptDate, ERP::Common::DATETIME_FORMAT);
    data["status"] = static_cast<int>(slip.status);
    ERP::DAOHelpers::putOptionalString(data, "reference_document_id", slip.referenceDocumentId);
    ERP::DAOHelpers::putOptionalString(data, "reference_document_type", slip.referenceDocumentType);
    ERP::DAOHelpers::putOptionalString(data, "notes", slip.notes);

    return data;
}

ERP::Material::DTO::ReceiptSlipDTO ReceiptSlipDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Material::DTO::ReceiptSlipDTO slip;
    ERP::Utils::DTOUtils::fromMap(data, slip); // BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "receipt_number", slip.receiptNumber);
        ERP::DAOHelpers::getPlainValue(data, "warehouse_id", slip.warehouseId);
        ERP::DAOHelpers::getPlainValue(data, "received_by_user_id", slip.receivedByUserId);
        ERP::DAOHelpers::getPlainTimeValue(data, "receipt_date", slip.receiptDate);
        
        int statusInt;
        ERP::DAOHelpers::getPlainValue(data, "status", statusInt);
        slip.status = static_cast<ERP::Material::DTO::ReceiptSlipStatus>(statusInt);

        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_id", slip.referenceDocumentId);
        ERP::DAOHelpers::getOptionalStringValue(data, "reference_document_type", slip.referenceDocumentType);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", slip.notes);

    } catch (const std::bad_any_cast& e) {
        ERP::Logger::Logger::getInstance().error("ReceiptSlipDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ReceiptSlipDAO: Data type mismatch in fromMap: " + std::string(e.what()));
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("ReceiptSlipDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ReceiptSlipDAO: Unexpected error in fromMap: " + std::string(e.what()));
    }
    return slip;
}

bool ReceiptSlipDAO::save(const ERP::Material::DTO::ReceiptSlipDTO& slip) {
    return create(slip);
}

std::optional<ERP::Material::DTO::ReceiptSlipDTO> ReceiptSlipDAO::findById(const std::string& id) {
    return getById(id);
}

bool ReceiptSlipDAO::update(const ERP::Material::DTO::ReceiptSlipDTO& slip) {
    return DAOBase<ERP::Material::DTO::ReceiptSlipDTO>::update(slip);
}

bool ReceiptSlipDAO::remove(const std::string& id) {
    return DAOBase<ERP::Material::DTO::ReceiptSlipDTO>::remove(id);
}

std::vector<ERP::Material::DTO::ReceiptSlipDTO> ReceiptSlipDAO::findAll() {
    return DAOBase<ERP::Material::DTO::ReceiptSlipDTO>::findAll();
}

std::optional<ERP::Material::DTO::ReceiptSlipDTO> ReceiptSlipDAO::getReceiptSlipByNumber(const std::string& receiptNumber) {
    std::map<std::string, std::any> filters;
    filters["receipt_number"] = receiptNumber;
    std::vector<ERP::Material::DTO::ReceiptSlipDTO> results = get(filters); // Use templated get
    if (!results.empty()) {
        return results[0];
    }
    return std::nullopt;
}

std::vector<ERP::Material::DTO::ReceiptSlipDTO> ReceiptSlipDAO::getReceiptSlips(const std::map<std::string, std::any>& filters) {
    return get(filters); // Use templated get
}

int ReceiptSlipDAO::countReceiptSlips(const std::map<std::string, std::any>& filters) {
    return count(filters); // Use templated count
}

// --- ReceiptSlipDetail operations ---

std::map<std::string, std::any> ReceiptSlipDAO::receiptSlipDetailToMap(const ERP::Material::DTO::ReceiptSlipDetailDTO& detail) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(detail); // BaseDTO fields

    data["receipt_slip_id"] = detail.receiptSlipId;
    data["product_id"] = detail.productId;
    data["location_id"] = detail.locationId;
    data["expected_quantity"] = detail.expectedQuantity;
    data["received_quantity"] = detail.receivedQuantity;
    ERP::DAOHelpers::putOptionalString(data, "unit_cost", detail.unitCost); // unitCost might be optional/derived
    ERP::DAOHelpers::putOptionalString(data, "lot_number", detail.lotNumber);
    ERP::DAOHelpers::putOptionalString(data, "serial_number", detail.serialNumber);
    ERP::DAOHelpers::putOptionalTime(data, "manufacture_date", detail.manufactureDate);
    ERP::DAOHelpers::putOptionalTime(data, "expiration_date", detail.expirationDate);
    ERP::DAOHelpers::putOptionalString(data, "notes", detail.notes);
    data["is_fully_received"] = detail.isFullyReceived;
    ERP::DAOHelpers::putOptionalString(data, "inventory_transaction_id", detail.inventoryTransactionId);

    return data;
}

ERP::Material::DTO::ReceiptSlipDetailDTO ReceiptSlipDAO::receiptSlipDetailFromMap(const std::map<std::string, std::any>& data) const {
    ERP::Material::DTO::ReceiptSlipDetailDTO detail;
    ERP::Utils::DTOUtils::fromMap(data, detail); // BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "receipt_slip_id", detail.receiptSlipId);
        ERP::DAOHelpers::getPlainValue(data, "product_id", detail.productId);
        ERP::DAOHelpers::getPlainValue(data, "location_id", detail.locationId);
        ERP::DAOHelpers::getPlainValue(data, "expected_quantity", detail.expectedQuantity);
        ERP::DAOHelpers::getPlainValue(data, "received_quantity", detail.receivedQuantity);
        
        // unitCost comes as double from DB, convert to optional double in DTO
        ERP::DAOHelpers::getOptionalDoubleValue(data, "unit_cost", detail.unitCost);

        ERP::DAOHelpers::getOptionalStringValue(data, "lot_number", detail.lotNumber);
        ERP::DAOHelpers::getOptionalStringValue(data, "serial_number", detail.serialNumber);
        ERP::DAOHelpers::getOptionalTimeValue(data, "manufacture_date", detail.manufactureDate);
        ERP::DAOHelpers::getOptionalTimeValue(data, "expiration_date", detail.expirationDate);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", detail.notes);
        ERP::DAOHelpers::getPlainValue(data, "is_fully_received", detail.isFullyReceived); // Should be int/bool
        ERP::DAOHelpers::getOptionalStringValue(data, "inventory_transaction_id", detail.inventoryTransactionId);

    } catch (const std::bad_any_cast& e) {
        ERP::Logger::Logger::getInstance().error("ReceiptSlipDAO: receiptSlipDetailFromMap - Data type mismatch during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ReceiptSlipDAO: Data type mismatch in receiptSlipDetailFromMap: " + std::string(e.what()));
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("ReceiptSlipDAO: receiptSlipDetailFromMap - Unexpected error during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ReceiptSlipDAO: Unexpected error in receiptSlipDetailFromMap: " + std::string(e.what()));
    }
    return detail;
}

bool ReceiptSlipDAO::createReceiptSlipDetail(const ERP::Material::DTO::ReceiptSlipDetailDTO& detail) {
    std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
    if (!conn) {
        ERP::Logger::Logger::getInstance().error("ReceiptSlipDAO::createReceiptSlipDetail: Failed to get database connection.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
        return false;
    }

    std::string sql = "INSERT INTO receipt_slip_details (id, receipt_slip_id, product_id, location_id, expected_quantity, received_quantity, unit_cost, lot_number, serial_number, manufacture_date, expiration_date, notes, is_fully_received, inventory_transaction_id, status, created_at, created_by, updated_at, updated_by) VALUES (:id, :receipt_slip_id, :product_id, :location_id, :expected_quantity, :received_quantity, :unit_cost, :lot_number, :serial_number, :manufacture_date, :expiration_date, :notes, :is_fully_received, :inventory_transaction_id, :status, :created_at, :created_by, :updated_at, :updated_by);";
    
    std::map<std::string, std::any> params = receiptSlipDetailToMap(detail);
    // Remove updated_at/by as they are not used in create
    params.erase("updated_at");
    params.erase("updated_by");


    bool success = conn->execute(sql, params);
    if (!success) {
        ERP::Logger::Logger::getInstance().error("ReceiptSlipDAO::createReceiptSlipDetail: Failed to create receipt slip detail. Error: " + conn->getLastError());
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to create receipt slip detail.", "Không thể tạo chi tiết phiếu nhập kho.");
    }
    connectionPool_->releaseConnection(conn);
    return success;
}

std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> ReceiptSlipDAO::getReceiptSlipDetails(const std::map<std::string, std::any>& filters) {
    std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
    if (!conn) {
        ERP::Logger::Logger::getInstance().error("ReceiptSlipDAO::getReceiptSlipDetails: Failed to get database connection.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
        return {};
    }

    std::string sql = "SELECT * FROM receipt_slip_details";
    std::string whereClause = buildWhereClause(filters);
    sql += whereClause;

    std::vector<std::map<std::string, std::any>> results = conn->query(sql, filters);
    connectionPool_->releaseConnection(conn);

    std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> details;
    for (const auto& row : results) {
        details.push_back(receiptSlipDetailFromMap(row));
    }
    return details;
}

std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> ReceiptSlipDAO::getReceiptSlipDetailsBySlipId(const std::string& receiptSlipId) {
    std::map<std::string, std::any> filters;
    filters["receipt_slip_id"] = receiptSlipId;
    return getReceiptSlipDetails(filters);
}

int ReceiptSlipDAO::countReceiptSlipDetails(const std::map<std::string, std::any>& filters) {
    std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
    if (!conn) {
        ERP::Logger::Logger::getInstance().error("ReceiptSlipDAO::countReceiptSlipDetails: Failed to get database connection.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
        return 0;
    }

    std::string sql = "SELECT COUNT(*) FROM receipt_slip_details";
    std::string whereClause = buildWhereClause(filters);
    sql += whereClause;

    std::vector<std::map<std::string, std::any>> results = conn->query(sql, filters);
    connectionPool_->releaseConnection(conn);

    if (!results.empty() && results[0].count("COUNT(*)")) {
        return std::any_cast<long long>(results[0].at("COUNT(*)")); // SQLite COUNT(*) returns long long
    }
    return 0;
}

bool ReceiptSlipDAO::updateReceiptSlipDetail(const ERP::Material::DTO::ReceiptSlipDetailDTO& detail) {
    std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
    if (!conn) {
        ERP::Logger::Logger::getInstance().error("ReceiptSlipDAO::updateReceiptSlipDetail: Failed to get database connection.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
        return false;
    }

    std::string sql = "UPDATE receipt_slip_details SET "
                      "receipt_slip_id = :receipt_slip_id, "
                      "product_id = :product_id, "
                      "location_id = :location_id, "
                      "expected_quantity = :expected_quantity, "
                      "received_quantity = :received_quantity, "
                      "unit_cost = :unit_cost, "
                      "lot_number = :lot_number, "
                      "serial_number = :serial_number, "
                      "manufacture_date = :manufacture_date, "
                      "expiration_date = :expiration_date, "
                      "notes = :notes, "
                      "is_fully_received = :is_fully_received, "
                      "inventory_transaction_id = :inventory_transaction_id, "
                      "status = :status, "
                      "created_at = :created_at, " // Should not be updated
                      "created_by = :created_by, " // Should not be updated
                      "updated_at = :updated_at, "
                      "updated_by = :updated_by "
                      "WHERE id = :id;";

    std::map<std::string, std::any> params = receiptSlipDetailToMap(detail);
    
    // Ensure created_at/by are not actually updated if they come from DTOUtils::toMap
    params["created_at"] = ERP::Utils::DateUtils::formatDateTime(detail.createdAt, ERP::Common::DATETIME_FORMAT);
    params["created_by"] = detail.createdBy.value_or("");
    
    // Set updated_at/by for the update operation
    params["updated_at"] = ERP::Utils::DateUtils::formatDateTime(ERP::Utils::DateUtils::now(), ERP::Common::DATETIME_FORMAT);
    params["updated_by"] = detail.updatedBy.value_or("");

    bool success = conn->execute(sql, params);
    if (!success) {
        ERP::Logger::Logger::getInstance().error("ReceiptSlipDAO::updateReceiptSlipDetail: Failed to update receipt slip detail " + detail.id + ". Error: " + conn->getLastError());
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to update receipt slip detail.", "Không thể cập nhật chi tiết phiếu nhập kho.");
    }
    connectionPool_->releaseConnection(conn);
    return success;
}

bool ReceiptSlipDAO::removeReceiptSlipDetail(const std::string& id) {
    std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
    if (!conn) {
        ERP::Logger::Logger::getInstance().error("ReceiptSlipDAO::removeReceiptSlipDetail: Failed to get database connection.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
        return false;
    }
    
    std::string sql = "DELETE FROM receipt_slip_details WHERE id = :id;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    bool success = conn->execute(sql, params);
    if (!success) {
        ERP::Logger::Logger::getInstance().error("ReceiptSlipDAO::removeReceiptSlipDetail: Failed to remove receipt slip detail " + id + ". Error: " + conn->getLastError());
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove receipt slip detail.", "Không thể xóa chi tiết phiếu nhập kho.");
    }
    connectionPool_->releaseConnection(conn);
    return success;
}

bool ReceiptSlipDAO::removeReceiptSlipDetailsBySlipId(const std::string& receiptSlipId) {
    std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
    if (!conn) {
        ERP::Logger::Logger::getInstance().error("ReceiptSlipDAO::removeReceiptSlipDetailsBySlipId: Failed to get database connection.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
        return false;
    }
    
    std::string sql = "DELETE FROM receipt_slip_details WHERE receipt_slip_id = :receipt_slip_id;";
    std::map<std::string, std::any> params;
    params["receipt_slip_id"] = receiptSlipId;

    bool success = conn->execute(sql, params);
    if (!success) {
        ERP::Logger::Logger::getInstance().error("ReceiptSlipDAO::removeReceiptSlipDetailsBySlipId: Failed to remove receipt slip details for slip_id " + receiptSlipId + ". Error: " + conn->getLastError());
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove receipt slip details.", "Không thể xóa các chi tiết phiếu nhập kho.");
    }
    connectionPool_->releaseConnection(conn);
    return success;
}

std::optional<ERP::Material::DTO::ReceiptSlipDetailDTO> ReceiptSlipDAO::getReceiptSlipDetailById(const std::string& id) {
    std::map<std::string, std::any> filters;
    filters["id"] = id;
    std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> results = getReceiptSlipDetails(filters);
    if (!results.empty()) {
        return results[0];
    }
    return std::nullopt;
}

} // namespace DAOs
} // namespace Material
} // namespace ERP