// Modules/Material/DAO/ReceiptSlipDetailDAO.cpp
#include "ReceiptSlipDetailDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions (BaseDTO)

namespace ERP {
    namespace Material {
        namespace DAOs {

            ReceiptSlipDetailDAO::ReceiptSlipDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Material::DTO::ReceiptSlipDetailDTO>(connectionPool, "receipt_slip_details") {
                // DAOBase constructor handles connectionPool and tableName_ initialization
                ERP::Logger::Logger::getInstance().info("ReceiptSlipDetailDAO: Initialized.");
            }

            std::map<std::string, std::any> ReceiptSlipDetailDAO::toMap(const ERP::Material::DTO::ReceiptSlipDetailDTO& detail) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(detail); // BaseDTO fields

                data["receipt_slip_id"] = detail.receiptSlipId;
                data["product_id"] = detail.productId;
                data["location_id"] = detail.locationId;
                data["expected_quantity"] = detail.expectedQuantity;
                data["received_quantity"] = detail.receivedQuantity;
                ERP::DAOHelpers::putOptionalDouble(data, "unit_cost", detail.unitCost); // unitCost might be optional/derived
                ERP::DAOHelpers::putOptionalString(data, "lot_number", detail.lotNumber);
                ERP::DAOHelpers::putOptionalString(data, "serial_number", detail.serialNumber);
                ERP::DAOHelpers::putOptionalTime(data, "manufacture_date", detail.manufactureDate);
                ERP::DAOHelpers::putOptionalTime(data, "expiration_date", detail.expirationDate);
                ERP::DAOHelpers::putOptionalString(data, "notes", detail.notes);
                data["is_fully_received"] = detail.isFullyReceived;
                ERP::DAOHelpers::putOptionalString(data, "inventory_transaction_id", detail.inventoryTransactionId);

                return data;
            }

            ERP::Material::DTO::ReceiptSlipDetailDTO ReceiptSlipDetailDAO::fromMap(const std::map<std::string, std::any>& data) const {
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

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("ReceiptSlipDetailDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ReceiptSlipDetailDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("ReceiptSlipDetailDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ReceiptSlipDetailDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return detail;
            }

            bool ReceiptSlipDetailDAO::save(const ERP::Material::DTO::ReceiptSlipDetailDTO& detail) {
                return create(detail);
            }

            std::optional<ERP::Material::DTO::ReceiptSlipDetailDTO> ReceiptSlipDetailDAO::findById(const std::string& id) {
                return getById(id);
            }

            bool ReceiptSlipDetailDAO::update(const ERP::Material::DTO::ReceiptSlipDetailDTO& detail) {
                return DAOBase<ERP::Material::DTO::ReceiptSlipDetailDTO>::update(detail);
            }

            bool ReceiptSlipDetailDAO::remove(const std::string& id) {
                return DAOBase<ERP::Material::DTO::ReceiptSlipDetailDTO>::remove(id);
            }

            std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> ReceiptSlipDetailDAO::findAll() {
                return DAOBase<ERP::Material::DTO::ReceiptSlipDetailDTO>::findAll();
            }

            std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> ReceiptSlipDAO::getReceiptSlipDetailsBySlipId(const std::string& receiptSlipId) {
                std::map<std::string, std::any> filters;
                filters["receipt_slip_id"] = receiptSlipId;
                return getReceiptSlipDetails(filters);
            }

            std::vector<ERP::Material::DTO::ReceiptSlipDetailDTO> ReceiptSlipDAO::getReceiptSlipDetails(const std::map<std::string, std::any>& filters) {
                return get(filters); // Use templated get
            }

            int ReceiptSlipDAO::countReceiptSlipDetails(const std::map<std::string, std::any>& filters) {
                return count(filters); // Use templated count
            }

            bool ReceiptSlipDAO::updateReceiptSlipDetail(const ERP::Material::DTO::ReceiptSlipDetailDTO& detail) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("ReceiptSlipDAO::updateReceiptSlipDetail: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "UPDATE " + tableName_ + " SET "
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

                std::map<std::string, std::any> params = toMap(detail); // Use toMap from this class

                // Ensure created_at/by are not actually updated if they come from DTOUtils::toMap
                // These should already be handled by DTOUtils::toMap, just ensure they're consistent

                // Explicitly set updated_at/by for the update operation if the DTO itself is not populated with them before calling this
                // If DTOUtils::toMap already sets updated_at/by when called before this, then these lines are redundant.
                // For safety, ensure updated_at/by are in the map before execution if they are part of the SET clause.
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

                std::string sql = "DELETE FROM " + tableName_ + " WHERE id = :id;";
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

                std::string sql = "DELETE FROM " + tableName_ + " WHERE receipt_slip_id = :receipt_slip_id;";
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