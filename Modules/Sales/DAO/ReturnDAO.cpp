// Modules/Sales/DAO/ReturnDAO.cpp
#include "ReturnDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions

namespace ERP {
    namespace Sales {
        namespace DAOs {

            ReturnDAO::ReturnDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Sales::DTO::ReturnDTO>(connectionPool, "returns") {
                // DAOBase constructor handles connectionPool and tableName_ initialization
                ERP::Logger::Logger::getInstance().info("ReturnDAO: Initialized.");
            }

            std::map<std::string, std::any> ReturnDAO::toMap(const ERP::Sales::DTO::ReturnDTO& returnObj) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(returnObj); // BaseDTO fields

                data["sales_order_id"] = returnObj.salesOrderId;
                data["customer_id"] = returnObj.customerId;
                data["return_number"] = returnObj.returnNumber;
                data["return_date"] = ERP::Utils::DateUtils::formatDateTime(returnObj.returnDate, ERP::Common::DATETIME_FORMAT);
                ERP::DAOHelpers::putOptionalString(data, "reason", returnObj.reason);
                data["total_amount"] = returnObj.totalAmount;
                data["status"] = static_cast<int>(returnObj.status);
                ERP::DAOHelpers::putOptionalString(data, "warehouse_id", returnObj.warehouseId);
                ERP::DAOHelpers::putOptionalString(data, "notes", returnObj.notes);

                // Nested details are not directly serialized into the main DTO's map for storage in main table.
                // They are handled by separate methods (createReturnDetail, getReturnDetailsByReturnId etc.)
                // If you ever need to store them as JSON in main table, you'd add:
                // data["details_json"] = serializeDetails(returnObj.details);

                return data;
            }

            ERP::Sales::DTO::ReturnDTO ReturnDAO::fromMap(const std::map<std::string, std::any>& data) const {
                ERP::Sales::DTO::ReturnDTO returnObj;
                ERP::Utils::DTOUtils::fromMap(data, returnObj); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "sales_order_id", returnObj.salesOrderId);
                    ERP::DAOHelpers::getPlainValue(data, "customer_id", returnObj.customerId);
                    ERP::DAOHelpers::getPlainValue(data, "return_number", returnObj.returnNumber);
                    ERP::DAOHelpers::getPlainTimeValue(data, "return_date", returnObj.returnDate);
                    ERP::DAOHelpers::getOptionalStringValue(data, "reason", returnObj.reason);
                    ERP::DAOHelpers::getPlainValue(data, "total_amount", returnObj.totalAmount);

                    int statusInt;
                    ERP::DAOHelpers::getPlainValue(data, "status", statusInt);
                    returnObj.status = static_cast<ERP::Sales::DTO::ReturnStatus>(statusInt);

                    ERP::DAOHelpers::getOptionalStringValue(data, "warehouse_id", returnObj.warehouseId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", returnObj.notes);

                    // Nested details are not deserialized from JSON in the main DTO's fromMap.
                    // They are typically loaded via a separate DAO method like getReturnDetailsByReturnId.
                    // If you had a 'details_json' field, you'd add:
                    // std::string detailsJsonString;
                    // ERP::DAOHelpers::getPlainValue(data, "details_json", detailsJsonString);
                    // returnObj.details = deserializeDetails(detailsJsonString);

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("ReturnDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ReturnDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("ReturnDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ReturnDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return returnObj;
            }

            bool ReturnDAO::save(const ERP::Sales::DTO::ReturnDTO& returnObj) {
                return create(returnObj);
            }

            std::optional<ERP::Sales::DTO::ReturnDTO> ReturnDAO::findById(const std::string& id) {
                return getById(id);
            }

            bool ReturnDAO::update(const ERP::Sales::DTO::ReturnDTO& returnObj) {
                return DAOBase<ERP::Sales::DTO::ReturnDTO>::update(returnObj);
            }

            bool ReturnDAO::remove(const std::string& id) {
                return DAOBase<ERP::Sales::DTO::ReturnDTO>::remove(id);
            }

            std::vector<ERP::Sales::DTO::ReturnDTO> ReturnDAO::findAll() {
                return DAOBase<ERP::Sales::DTO::ReturnDTO>::findAll();
            }

            std::vector<ERP::Sales::DTO::ReturnDTO> ReturnDAO::getReturns(const std::map<std::string, std::any>& filters) {
                return get(filters); // Use templated get
            }

            int ReturnDAO::countReturns(const std::map<std::string, std::any>& filters) {
                return count(filters); // Use templated count
            }

            // --- ReturnDetail operations ---

            std::map<std::string, std::any> ReturnDAO::returnDetailToMap(const ERP::Sales::DTO::ReturnDetailDTO& detail) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(detail); // BaseDTO fields

                data["return_id"] = detail.returnId;
                data["product_id"] = detail.productId;
                data["quantity"] = detail.quantity;
                data["unit_of_measure_id"] = detail.unitOfMeasureId;
                data["unit_price"] = detail.unitPrice;
                data["refunded_amount"] = detail.refundedAmount;
                ERP::DAOHelpers::putOptionalString(data, "condition", detail.condition);
                ERP::DAOHelpers::putOptionalString(data, "notes", detail.notes);
                ERP::DAOHelpers::putOptionalString(data, "sales_order_detail_id", detail.salesOrderDetailId);
                ERP::DAOHelpers::putOptionalString(data, "inventory_transaction_id", detail.inventoryTransactionId);

                return data;
            }

            ERP::Sales::DTO::ReturnDetailDTO ReturnDAO::returnDetailFromMap(const std::map<std::string, std::any>& data) const {
                ERP::Sales::DTO::ReturnDetailDTO detail;
                ERP::Utils::DTOUtils::fromMap(data, detail); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "return_id", detail.returnId);
                    ERP::DAOHelpers::getPlainValue(data, "product_id", detail.productId);
                    ERP::DAOHelpers::getPlainValue(data, "quantity", detail.quantity);
                    ERP::DAOHelpers::getPlainValue(data, "unit_of_measure_id", detail.unitOfMeasureId);
                    ERP::DAOHelpers::getPlainValue(data, "unit_price", detail.unitPrice);
                    ERP::DAOHelpers::getPlainValue(data, "refunded_amount", detail.refundedAmount);
                    ERP::DAOHelpers::getOptionalStringValue(data, "condition", detail.condition);
                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", detail.notes);
                    ERP::DAOHelpers::getOptionalStringValue(data, "sales_order_detail_id", detail.salesOrderDetailId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "inventory_transaction_id", detail.inventoryTransactionId);

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("ReturnDAO: returnDetailFromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ReturnDAO: Data type mismatch in returnDetailFromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("ReturnDAO: returnDetailFromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ReturnDAO: Unexpected error in returnDetailFromMap: " + std::string(e.what()));
                }
                return detail;
            }

            bool ReturnDAO::createReturnDetail(const ERP::Sales::DTO::ReturnDetailDTO& detail) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("ReturnDAO::createReturnDetail: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "INSERT INTO " + detailsTableName_ + " (id, return_id, product_id, quantity, unit_of_measure_id, unit_price, refunded_amount, condition, notes, sales_order_detail_id, inventory_transaction_id, status, created_at, created_by, updated_at, updated_by) VALUES (:id, :return_id, :product_id, :quantity, :unit_of_measure_id, :unit_price, :refunded_amount, :condition, :notes, :sales_order_detail_id, :inventory_transaction_id, :status, :created_at, :created_by, :updated_at, :updated_by);";

                std::map<std::string, std::any> params = returnDetailToMap(detail);
                // Remove updated_at/by as they are not used in create
                params.erase("updated_at");
                params.erase("updated_by");

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("ReturnDAO::createReturnDetail: Failed to create return detail. Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to create return detail.", "Không thể tạo chi tiết trả hàng.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

            std::optional<ERP::Sales::DTO::ReturnDetailDTO> ReturnDAO::getReturnDetailById(const std::string& id) {
                std::map<std::string, std::any> filters;
                filters["id"] = id;
                std::vector<ERP::Sales::DTO::ReturnDetailDTO> results = getReturnDetails(filters);
                if (!results.empty()) {
                    return results[0];
                }
                return std::nullopt;
            }

            std::vector<ERP::Sales::DTO::ReturnDetailDTO> ReturnDAO::getReturnDetailsByReturnId(const std::string& returnId) {
                std::map<std::string, std::any> filters;
                filters["return_id"] = returnId;
                return getReturnDetails(filters);
            }

            std::vector<ERP::Sales::DTO::ReturnDetailDTO> ReturnDAO::getReturnDetails(const std::map<std::string, std::any>& filters) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("ReturnDAO::getReturnDetails: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return {};
                }

                std::string sql = "SELECT * FROM " + detailsTableName_;
                std::string whereClause = buildWhereClause(filters);
                sql += whereClause;

                std::vector<std::map<std::string, std::any>> results = conn->query(sql, filters);
                connectionPool_->releaseConnection(conn);

                std::vector<ERP::Sales::DTO::ReturnDetailDTO> details;
                for (const auto& row : results) {
                    details.push_back(returnDetailFromMap(row));
                }
                return details;
            }

            int ReturnDAO::countReturnDetails(const std::map<std::string, std::any>& filters) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("ReturnDAO::countReturnDetails: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return 0;
                }

                std::string sql = "SELECT COUNT(*) FROM " + detailsTableName_;
                std::string whereClause = buildWhereClause(filters);
                sql += whereClause;

                std::vector<std::map<std::string, std::any>> results = conn->query(sql, filters);
                connectionPool_->releaseConnection(conn);

                if (!results.empty() && results[0].count("COUNT(*)")) {
                    return std::any_cast<long long>(results[0].at("COUNT(*)")); // SQLite COUNT(*) returns long long
                }
                return 0;
            }

            bool ReturnDAO::updateReturnDetail(const ERP::Sales::DTO::ReturnDetailDTO& detail) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("ReturnDAO::updateReturnDetail: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "UPDATE " + detailsTableName_ + " SET "
                    "return_id = :return_id, "
                    "product_id = :product_id, "
                    "quantity = :quantity, "
                    "unit_of_measure_id = :unit_of_measure_id, "
                    "unit_price = :unit_price, "
                    "refunded_amount = :refunded_amount, "
                    "condition = :condition, "
                    "notes = :notes, "
                    "sales_order_detail_id = :sales_order_detail_id, "
                    "inventory_transaction_id = :inventory_transaction_id, "
                    "status = :status, "
                    "created_at = :created_at, "
                    "created_by = :created_by, "
                    "updated_at = :updated_at, "
                    "updated_by = :updated_by "
                    "WHERE id = :id;";

                std::map<std::string, std::any> params = returnDetailToMap(detail);

                // Explicitly set updated_at/by for the update operation
                params["updated_at"] = ERP::Utils::DateUtils::formatDateTime(ERP::Utils::DateUtils::now(), ERP::Common::DATETIME_FORMAT);
                params["updated_by"] = detail.updatedBy.value_or(""); // Ensure this matches DTO's actual field for updatedBy

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("ReturnDAO::updateReturnDetail: Failed to update return detail " + detail.id + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to update return detail.", "Không thể cập nhật chi tiết trả hàng.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

            bool ReturnDAO::removeReturnDetail(const std::string& id) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("ReturnDAO::removeReturnDetail: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + detailsTableName_ + " WHERE id = :id;";
                std::map<std::string, std::any> params;
                params["id"] = id;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("ReturnDAO::removeReturnDetail: Failed to remove return detail " + id + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove return detail.", "Không thể xóa chi tiết trả hàng.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

            bool ReturnDAO::removeReturnDetailsByReturnId(const std::string& returnId) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("ReturnDAO::removeReturnDetailsByReturnId: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + detailsTableName_ + " WHERE return_id = :return_id;";
                std::map<std::string, std::any> params;
                params["return_id"] = returnId;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("ReturnDAO::removeReturnDetailsByReturnId: Failed to remove return details for return_id " + returnId + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove return details.", "Không thể xóa các chi tiết trả hàng.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

        } // namespace DAOs
    } // namespace Sales
} // namespace ERP