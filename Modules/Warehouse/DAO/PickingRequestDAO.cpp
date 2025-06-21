// Modules/Warehouse/DAO/PickingRequestDAO.cpp
#include "PickingRequestDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For BaseDTO conversions

namespace ERP {
    namespace Warehouse {
        namespace DAOs {

            PickingRequestDAO::PickingRequestDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Warehouse::DTO::PickingRequestDTO>(connectionPool, "picking_requests") {
                // DAOBase constructor handles connectionPool and tableName_ initialization
                ERP::Logger::Logger::getInstance().info("PickingRequestDAO: Initialized.");
            }

            std::map<std::string, std::any> PickingRequestDAO::toMap(const ERP::Warehouse::DTO::PickingRequestDTO& request) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(request); // BaseDTO fields

                data["sales_order_id"] = request.salesOrderId;
                data["warehouse_id"] = request.warehouseId;
                data["request_number"] = request.requestNumber;
                data["request_date"] = ERP::Utils::DateUtils::formatDateTime(request.requestDate, ERP::Common::DATETIME_FORMAT);
                ERP::DAOHelpers::putOptionalString(data, "requested_by_user_id", request.requestedByUserId);
                ERP::DAOHelpers::putOptionalString(data, "assigned_to_user_id", request.assignedToUserId);
                data["status"] = static_cast<int>(request.status);
                ERP::DAOHelpers::putOptionalTime(data, "pick_start_time", request.pickStartTime);
                ERP::DAOHelpers::putOptionalTime(data, "pick_end_time", request.pickEndTime);
                ERP::DAOHelpers::putOptionalString(data, "notes", request.notes);

                return data;
            }

            ERP::Warehouse::DTO::PickingRequestDTO PickingRequestDAO::fromMap(const std::map<std::string, std::any>& data) const {
                ERP::Warehouse::DTO::PickingRequestDTO request;
                ERP::Utils::DTOUtils::fromMap(data, request); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "sales_order_id", request.salesOrderId);
                    ERP::DAOHelpers::getPlainValue(data, "warehouse_id", request.warehouseId);
                    ERP::DAOHelpers::getPlainValue(data, "request_number", request.requestNumber);
                    ERP::DAOHelpers::getPlainTimeValue(data, "request_date", request.requestDate);
                    ERP::DAOHelpers::getOptionalStringValue(data, "requested_by_user_id", request.requestedByUserId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "assigned_to_user_id", request.assignedToUserId);

                    int statusInt;
                    ERP::DAOHelpers::getPlainValue(data, "status", statusInt);
                    request.status = static_cast<ERP::Warehouse::DTO::PickingRequestStatus>(statusInt);

                    ERP::DAOHelpers::getOptionalTimeValue(data, "pick_start_time", request.pickStartTime);
                    ERP::DAOHelpers::getOptionalTimeValue(data, "pick_end_time", request.pickEndTime);
                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", request.notes);

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("PickingRequestDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "PickingRequestDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("PickingRequestDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "PickingRequestDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return request;
            }

            bool PickingRequestDAO::save(const ERP::Warehouse::DTO::PickingRequestDTO& request) {
                return create(request);
            }

            std::optional<ERP::Warehouse::DTO::PickingRequestDTO> PickingRequestDAO::findById(const std::string& id) {
                return getById(id);
            }

            bool PickingRequestDAO::update(const ERP::Warehouse::DTO::PickingRequestDTO& request) {
                return DAOBase<ERP::Warehouse::DTO::PickingRequestDTO>::update(request);
            }

            bool PickingRequestDAO::remove(const std::string& id) {
                return DAOBase<ERP::Warehouse::DTO::PickingRequestDTO>::remove(id);
            }

            std::vector<ERP::Warehouse::DTO::PickingRequestDTO> PickingRequestDAO::findAll() {
                return DAOBase<ERP::Warehouse::DTO::PickingRequestDTO>::findAll();
            }

            std::vector<ERP::Warehouse::DTO::PickingRequestDTO> PickingRequestDAO::getPickingRequests(const std::map<std::string, std::any>& filters) {
                return get(filters); // Use templated get
            }

            int PickingRequestDAO::countPickingRequests(const std::map<std::string, std::any>& filters) {
                return count(filters); // Use templated count
            }

            // --- PickingDetail operations ---

            std::map<std::string, std::any> PickingRequestDAO::pickingDetailToMap(const ERP::Warehouse::DTO::PickingDetailDTO& detail) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(detail); // BaseDTO fields

                data["picking_request_id"] = detail.pickingRequestId;
                data["product_id"] = detail.productId;
                data["warehouse_id"] = detail.warehouseId;
                data["location_id"] = detail.locationId;
                data["requested_quantity"] = detail.requestedQuantity;
                data["picked_quantity"] = detail.pickedQuantity;
                ERP::DAOHelpers::putOptionalString(data, "lot_number", detail.lotNumber);
                ERP::DAOHelpers::putOptionalString(data, "serial_number", detail.serialNumber);
                data["is_picked"] = detail.isPicked;
                ERP::DAOHelpers::putOptionalString(data, "notes", detail.notes);
                ERP::DAOHelpers::putOptionalString(data, "sales_order_detail_id", detail.salesOrderDetailId);
                ERP::DAOHelpers::putOptionalString(data, "inventory_transaction_id", detail.inventoryTransactionId);

                return data;
            }

            ERP::Warehouse::DTO::PickingDetailDTO PickingRequestDAO::pickingDetailFromMap(const std::map<std::string, std::any>& data) const {
                ERP::Warehouse::DTO::PickingDetailDTO detail;
                ERP::Utils::DTOUtils::fromMap(data, detail); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "picking_request_id", detail.pickingRequestId);
                    ERP::DAOHelpers::getPlainValue(data, "product_id", detail.productId);
                    ERP::DAOHelpers::getPlainValue(data, "warehouse_id", detail.warehouseId);
                    ERP::DAOHelpers::getPlainValue(data, "location_id", detail.locationId);
                    ERP::DAOHelpers::getPlainValue(data, "requested_quantity", detail.requestedQuantity);
                    ERP::DAOHelpers::getPlainValue(data, "picked_quantity", detail.pickedQuantity);
                    ERP::DAOHelpers::getOptionalStringValue(data, "lot_number", detail.lotNumber);
                    ERP::DAOHelpers::getOptionalStringValue(data, "serial_number", detail.serialNumber);
                    ERP::DAOHelpers::getPlainValue(data, "is_picked", detail.isPicked); // Should be bool/int
                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", detail.notes);
                    ERP::DAOHelpers::getOptionalStringValue(data, "sales_order_detail_id", detail.salesOrderDetailId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "inventory_transaction_id", detail.inventoryTransactionId);

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("PickingRequestDAO: pickingDetailFromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "PickingRequestDAO: Data type mismatch in pickingDetailFromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("PickingRequestDAO: pickingDetailFromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "PickingRequestDAO: Unexpected error in pickingDetailFromMap: " + std::string(e.what()));
                }
                return detail;
            }

            bool PickingRequestDAO::createPickingDetail(const ERP::Warehouse::DTO::PickingDetailDTO& detail) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("PickingRequestDAO::createPickingDetail: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "INSERT INTO " + detailsTableName_ + " (id, picking_request_id, product_id, warehouse_id, location_id, requested_quantity, picked_quantity, lot_number, serial_number, is_picked, notes, sales_order_detail_id, inventory_transaction_id, status, created_at, created_by, updated_at, updated_by) VALUES (:id, :picking_request_id, :product_id, :warehouse_id, :location_id, :requested_quantity, :picked_quantity, :lot_number, :serial_number, :is_picked, :notes, :sales_order_detail_id, :inventory_transaction_id, :status, :created_at, :created_by, :updated_at, :updated_by);";

                std::map<std::string, std::any> params = pickingDetailToMap(detail);
                // Remove updated_at/by as they are not used in create
                params.erase("updated_at");
                params.erase("updated_by");

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("PickingRequestDAO::createPickingDetail: Failed to create picking detail. Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to create picking detail.", "Không thể tạo chi tiết yêu cầu lấy hàng.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

            std::optional<ERP::Warehouse::DTO::PickingDetailDTO> PickingRequestDAO::getPickingDetailById(const std::string& id) {
                std::map<std::string, std::any> filters;
                filters["id"] = id;
                std::vector<ERP::Warehouse::DTO::PickingDetailDTO> results = getPickingDetails(filters);
                if (!results.empty()) {
                    return results[0];
                }
                return std::nullopt;
            }

            std::vector<ERP::Warehouse::DTO::PickingDetailDTO> PickingRequestDAO::getPickingDetailsByRequestId(const std::string& pickingRequestId) {
                std::map<std::string, std::any> filters;
                filters["picking_request_id"] = pickingRequestId;
                return getPickingDetails(filters);
            }

            std::vector<ERP::Warehouse::DTO::PickingDetailDTO> PickingRequestDAO::getPickingDetails(const std::map<std::string, std::any>& filters) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("PickingRequestDAO::getPickingDetails: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return {};
                }

                std::string sql = "SELECT * FROM " + detailsTableName_;
                std::string whereClause = buildWhereClause(filters);
                sql += whereClause;

                std::vector<std::map<std::string, std::any>> results = conn->query(sql, filters);
                connectionPool_->releaseConnection(conn);

                std::vector<ERP::Warehouse::DTO::PickingDetailDTO> details;
                for (const auto& row : results) {
                    details.push_back(pickingDetailFromMap(row));
                }
                return details;
            }

            int PickingRequestDAO::countPickingDetails(const std::map<std::string, std::any>& filters) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("PickingRequestDAO::countPickingDetails: Failed to get database connection.");
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

            bool PickingRequestDAO::updatePickingDetail(const ERP::Warehouse::DTO::PickingDetailDTO& detail) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("PickingRequestDAO::updatePickingDetail: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "UPDATE " + detailsTableName_ + " SET "
                    "picking_request_id = :picking_request_id, "
                    "product_id = :product_id, "
                    "warehouse_id = :warehouse_id, "
                    "location_id = :location_id, "
                    "requested_quantity = :requested_quantity, "
                    "picked_quantity = :picked_quantity, "
                    "lot_number = :lot_number, "
                    "serial_number = :serial_number, "
                    "is_picked = :is_picked, "
                    "notes = :notes, "
                    "sales_order_detail_id = :sales_order_detail_id, "
                    "inventory_transaction_id = :inventory_transaction_id, "
                    "status = :status, "
                    "created_at = :created_at, "
                    "created_by = :created_by, "
                    "updated_at = :updated_at, "
                    "updated_by = :updated_by "
                    "WHERE id = :id;";

                std::map<std::string, std::any> params = pickingDetailToMap(detail);

                // Explicitly set updated_at/by for the update operation
                params["updated_at"] = ERP::Utils::DateUtils::formatDateTime(ERP::Utils::DateUtils::now(), ERP::Common::DATETIME_FORMAT);
                params["updated_by"] = detail.updatedBy.value_or("");

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("PickingRequestDAO::updatePickingDetail: Failed to update picking detail " + detail.id + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to update picking detail.", "Không thể cập nhật chi tiết yêu cầu lấy hàng.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

            bool PickingRequestDAO::removePickingDetail(const std::string& id) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("PickingRequestDAO::removePickingDetail: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + detailsTableName_ + " WHERE id = :id;";
                std::map<std::string, std::any> params;
                params["id"] = id;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("PickingRequestDAO::removePickingDetail: Failed to remove picking detail " + id + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove picking detail.", "Không thể xóa chi tiết yêu cầu lấy hàng.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

            bool PickingRequestDAO::removePickingDetailsByRequestId(const std::string& pickingRequestId) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("PickingRequestDAO::removePickingDetailsByRequestId: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + detailsTableName_ + " WHERE picking_request_id = :picking_request_id;";
                std::map<std::string, std::any> params;
                params["picking_request_id"] = pickingRequestId;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("PickingRequestDAO::removePickingDetailsByRequestId: Failed to remove picking details for request_id " + pickingRequestId + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove picking details.", "Không thể xóa các chi tiết yêu cầu lấy hàng.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

        } // namespace DAOs
    } // namespace Warehouse
} // namespace ERP