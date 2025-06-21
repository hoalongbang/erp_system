// Modules/Warehouse/DAO/StocktakeRequestDAO.cpp
#include "StocktakeRequestDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For BaseDTO conversions

namespace ERP {
    namespace Warehouse {
        namespace DAOs {

            StocktakeRequestDAO::StocktakeRequestDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Warehouse::DTO::StocktakeRequestDTO>(connectionPool, "stocktake_requests") {
                // DAOBase constructor handles connectionPool and tableName_ initialization
                ERP::Logger::Logger::getInstance().info("StocktakeRequestDAO: Initialized.");
            }

            std::map<std::string, std::any> StocktakeRequestDAO::toMap(const ERP::Warehouse::DTO::StocktakeRequestDTO& request) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(request); // BaseDTO fields

                data["warehouse_id"] = request.warehouseId;
                ERP::DAOHelpers::putOptionalString(data, "location_id", request.locationId);
                data["requested_by_user_id"] = request.requestedByUserId;
                ERP::DAOHelpers::putOptionalString(data, "counted_by_user_id", request.countedByUserId);
                data["count_date"] = ERP::Utils::DateUtils::formatDateTime(request.countDate, ERP::Common::DATETIME_FORMAT);
                data["status"] = static_cast<int>(request.status);
                ERP::DAOHelpers::putOptionalString(data, "notes", request.notes);

                return data;
            }

            ERP::Warehouse::DTO::StocktakeRequestDTO StocktakeRequestDAO::fromMap(const std::map<std::string, std::any>& data) const {
                ERP::Warehouse::DTO::StocktakeRequestDTO request;
                ERP::Utils::DTOUtils::fromMap(data, request); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "warehouse_id", request.warehouseId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "location_id", request.locationId);
                    ERP::DAOHelpers::getPlainValue(data, "requested_by_user_id", request.requestedByUserId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "counted_by_user_id", request.countedByUserId);
                    ERP::DAOHelpers::getPlainTimeValue(data, "count_date", request.countDate);

                    int statusInt;
                    ERP::DAOHelpers::getPlainValue(data, "status", statusInt);
                    request.status = static_cast<ERP::Warehouse::DTO::StocktakeRequestStatus>(statusInt);

                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", request.notes);

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("StocktakeRequestDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "StocktakeRequestDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("StocktakeRequestDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "StocktakeRequestDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return request;
            }

            bool StocktakeRequestDAO::save(const ERP::Warehouse::DTO::StocktakeRequestDTO& request) {
                return create(request);
            }

            std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> StocktakeRequestDAO::findById(const std::string& id) {
                return getById(id);
            }

            bool StocktakeRequestDAO::update(const ERP::Warehouse::DTO::StocktakeRequestDTO& request) {
                return DAOBase<ERP::Warehouse::DTO::StocktakeRequestDTO>::update(request);
            }

            bool StocktakeRequestDAO::remove(const std::string& id) {
                return DAOBase<ERP::Warehouse::DTO::StocktakeRequestDTO>::remove(id);
            }

            std::vector<ERP::Warehouse::DTO::StocktakeRequestDTO> StocktakeRequestDAO::findAll() {
                return DAOBase<ERP::Warehouse::DTO::StocktakeRequestDTO>::findAll();
            }

            std::vector<ERP::Warehouse::DTO::StocktakeRequestDTO> StocktakeRequestDAO::getStocktakeRequests(const std::map<std::string, std::any>& filters) {
                return get(filters); // Use templated get
            }

            int StocktakeRequestDAO::countStocktakeRequests(const std::map<std::string, std::any>& filters) {
                return count(filters); // Use templated count
            }

            // --- StocktakeDetail operations ---

            std::map<std::string, std::any> StocktakeRequestDAO::stocktakeDetailToMap(const ERP::Warehouse::DTO::StocktakeDetailDTO& detail) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(detail); // BaseDTO fields

                data["stocktake_request_id"] = detail.stocktakeRequestId;
                data["product_id"] = detail.productId;
                data["warehouse_id"] = detail.warehouseId;
                data["location_id"] = detail.locationId;
                data["system_quantity"] = detail.systemQuantity;
                data["counted_quantity"] = detail.countedQuantity;
                data["difference"] = detail.difference;
                ERP::DAOHelpers::putOptionalString(data, "lot_number", detail.lotNumber);
                ERP::DAOHelpers::putOptionalString(data, "serial_number", detail.serialNumber);
                ERP::DAOHelpers::putOptionalString(data, "notes", detail.notes);
                ERP::DAOHelpers::putOptionalString(data, "adjustment_transaction_id", detail.adjustmentTransactionId);

                return data;
            }

            ERP::Warehouse::DTO::StocktakeDetailDTO StocktakeRequestDAO::stocktakeDetailFromMap(const std::map<std::string, std::any>& data) const {
                ERP::Warehouse::DTO::StocktakeDetailDTO detail;
                ERP::Utils::DTOUtils::fromMap(data, detail); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "stocktake_request_id", detail.stocktakeRequestId);
                    ERP::DAOHelpers::getPlainValue(data, "product_id", detail.productId);
                    ERP::DAOHelpers::getPlainValue(data, "warehouse_id", detail.warehouseId);
                    ERP::DAOHelpers::getPlainValue(data, "location_id", detail.locationId);
                    ERP::DAOHelpers::getPlainValue(data, "system_quantity", detail.systemQuantity);
                    ERP::DAOHelpers::getPlainValue(data, "counted_quantity", detail.countedQuantity);
                    ERP::DAOHelpers::getPlainValue(data, "difference", detail.difference);
                    ERP::DAOHelpers::getOptionalStringValue(data, "lot_number", detail.lotNumber);
                    ERP::DAOHelpers::getOptionalStringValue(data, "serial_number", detail.serialNumber);
                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", detail.notes);
                    ERP::DAOHelpers::getOptionalStringValue(data, "adjustment_transaction_id", detail.adjustmentTransactionId);

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("StocktakeRequestDAO: stocktakeDetailFromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "StocktakeRequestDAO: Data type mismatch in stocktakeDetailFromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("StocktakeRequestDAO: stocktakeDetailFromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "StocktakeRequestDAO: Unexpected error in stocktakeDetailFromMap: " + std::string(e.what()));
                }
                return detail;
            }

            bool StocktakeRequestDAO::createStocktakeDetail(const ERP::Warehouse::DTO::StocktakeDetailDTO& detail) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("StocktakeRequestDAO::createStocktakeDetail: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "INSERT INTO " + detailsTableName_ + " (id, stocktake_request_id, product_id, warehouse_id, location_id, system_quantity, counted_quantity, difference, lot_number, serial_number, notes, adjustment_transaction_id, status, created_at, created_by, updated_at, updated_by) VALUES (:id, :stocktake_request_id, :product_id, :warehouse_id, :location_id, :system_quantity, :counted_quantity, :difference, :lot_number, :serial_number, :notes, :adjustment_transaction_id, :status, :created_at, :created_by, :updated_at, :updated_by);";

                std::map<std::string, std::any> params = stocktakeDetailToMap(detail);
                // Remove updated_at/by as they are not used in create
                params.erase("updated_at");
                params.erase("updated_by");

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("StocktakeRequestDAO::createStocktakeDetail: Failed to create stocktake detail. Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to create stocktake detail.", "Không thể tạo chi tiết kiểm kê.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

            std::optional<ERP::Warehouse::DTO::StocktakeDetailDTO> StocktakeRequestDAO::getStocktakeDetailById(const std::string& id) {
                std::map<std::string, std::any> filters;
                filters["id"] = id;
                std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> results = getStocktakeDetails(filters);
                if (!results.empty()) {
                    return results[0];
                }
                return std::nullopt;
            }

            std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> StocktakeRequestDAO::getStocktakeDetailsByRequestId(const std::string& stocktakeRequestId) {
                std::map<std::string, std::any> filters;
                filters["stocktake_request_id"] = stocktakeRequestId;
                return getStocktakeDetails(filters);
            }

            std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> StocktakeRequestDAO::getStocktakeDetails(const std::map<std::string, std::any>& filters) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("StocktakeRequestDAO::getStocktakeDetails: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return {};
                }

                std::string sql = "SELECT * FROM " + detailsTableName_;
                std::string whereClause = buildWhereClause(filters);
                sql += whereClause;

                std::vector<std::map<std::string, std::any>> results = conn->query(sql, filters);
                connectionPool_->releaseConnection(conn);

                std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> details;
                for (const auto& row : results) {
                    details.push_back(stocktakeDetailFromMap(row));
                }
                return details;
            }

            int StocktakeRequestDAO::countStocktakeDetails(const std::map<std::string, std::any>& filters) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("StocktakeRequestDAO::countStocktakeDetails: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return 0;
                }

                std::string sql = "SELECT COUNT(*) FROM " + detailsTableName_;
                std::string whereClause = buildWhereClause(filters);
                sql += whereClause;

                std::vector<std::map<std::string, std::any>> results = conn->query(sql, filters);
                connectionPool_->releaseConnection(conn);

                if (!results.empty() && results[0].count("COUNT(*)")) {
                    return static_cast<int>(std::any_cast<long long>(results[0].at("COUNT(*)"))); // SQLite COUNT(*) returns long long
                }
                return 0;
            }

            bool StocktakeRequestDAO::updateStocktakeDetail(const ERP::Warehouse::DTO::StocktakeDetailDTO& detail) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("StocktakeRequestDAO::updateStocktakeDetail: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "UPDATE " + detailsTableName_ + " SET "
                    "stocktake_request_id = :stocktake_request_id, "
                    "product_id = :product_id, "
                    "warehouse_id = :warehouse_id, "
                    "location_id = :location_id, "
                    "system_quantity = :system_quantity, "
                    "counted_quantity = :counted_quantity, "
                    "difference = :difference, "
                    "lot_number = :lot_number, "
                    "serial_number = :serial_number, "
                    "notes = :notes, "
                    "adjustment_transaction_id = :adjustment_transaction_id, "
                    "status = :status, "
                    "created_at = :created_at, "
                    "created_by = :created_by, "
                    "updated_at = :updated_at, "
                    "updated_by = :updated_by "
                    "WHERE id = :id;";

                std::map<std::string, std::any> params = stocktakeDetailToMap(detail);

                // Explicitly set updated_at/by for the update operation
                params["updated_at"] = ERP::Utils::DateUtils::formatDateTime(ERP::Utils::DateUtils::now(), ERP::Common::DATETIME_FORMAT);
                params["updated_by"] = detail.updatedBy.value_or("");

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("StocktakeRequestDAO::updateStocktakeDetail: Failed to update stocktake detail " + detail.id + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to update stocktake detail.", "Không thể cập nhật chi tiết kiểm kê.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

            bool StocktakeRequestDAO::removeStocktakeDetail(const std::string& id) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("StocktakeRequestDAO::removeStocktakeDetail: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + detailsTableName_ + " WHERE id = :id;";
                std::map<std::string, std::any> params;
                params["id"] = id;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("StocktakeRequestDAO::removeStocktakeDetail: Failed to remove stocktake detail " + id + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove stocktake detail.", "Không thể xóa chi tiết kiểm kê.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

            bool StocktakeRequestDAO::removeStocktakeDetailsByRequestId(const std::string& stocktakeRequestId) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("StocktakeRequestDAO::removeStocktakeDetailsByRequestId: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + detailsTableName_ + " WHERE stocktake_request_id = :stocktake_request_id;";
                std::map<std::string, std::any> params;
                params["stocktake_request_id"] = stocktakeRequestId;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("StocktakeRequestDAO::removeStocktakeDetailsByRequestId: Failed to remove stocktake details for request_id " + stocktakeRequestId + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove stocktake details.", "Không thể xóa các chi tiết kiểm kê.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

        } // namespace DAOs
    } // namespace Warehouse
} // namespace ERP