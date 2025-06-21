// Modules/Warehouse/DAO/PickingDetailDAO.cpp
#include "PickingDetailDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For BaseDTO conversions

namespace ERP {
    namespace Warehouse {
        namespace DAOs {

            PickingDetailDAO::PickingDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Warehouse::DTO::PickingDetailDTO>(connectionPool, "picking_details") {
                // DAOBase constructor handles connectionPool and tableName_ initialization
                ERP::Logger::Logger::getInstance().info("PickingDetailDAO: Initialized.");
            }

            std::map<std::string, std::any> PickingDetailDAO::toMap(const ERP::Warehouse::DTO::PickingDetailDTO& detail) const {
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

            ERP::Warehouse::DTO::PickingDetailDTO PickingDetailDAO::fromMap(const std::map<std::string, std::any>& data) const {
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
                    ERP::Logger::Logger::getInstance().error("PickingDetailDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "PickingDetailDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("PickingDetailDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "PickingDetailDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return detail;
            }

            bool PickingDetailDAO::save(const ERP::Warehouse::DTO::PickingDetailDTO& detail) {
                return create(detail);
            }

            std::optional<ERP::Warehouse::DTO::PickingDetailDTO> PickingDetailDAO::findById(const std::string& id) {
                return getById(id);
            }

            bool PickingDetailDAO::update(const ERP::Warehouse::DTO::PickingDetailDTO& detail) {
                return DAOBase<ERP::Warehouse::DTO::PickingDetailDTO>::update(detail);
            }

            bool PickingDetailDAO::remove(const std::string& id) {
                return DAOBase<ERP::Warehouse::DTO::PickingDetailDTO>::remove(id);
            }

            std::vector<ERP::Warehouse::DTO::PickingDetailDTO> PickingDetailDAO::findAll() {
                return DAOBase<ERP::Warehouse::DTO::PickingDetailDTO>::findAll();
            }

            std::vector<ERP::Warehouse::DTO::PickingDetailDTO> PickingDetailDAO::getPickingDetailsByRequestId(const std::string& pickingRequestId) {
                std::map<std::string, std::any> filters;
                filters["picking_request_id"] = pickingRequestId;
                return getPickingDetails(filters);
            }

            std::vector<ERP::Warehouse::DTO::PickingDetailDTO> PickingDetailDAO::getPickingDetails(const std::map<std::string, std::any>& filters) {
                return get(filters); // Use templated get
            }

            int PickingDetailDAO::countPickingDetails(const std::map<std::string, std::any>& filters) {
                return count(filters); // Use templated count
            }

            bool PickingDetailDAO::removePickingDetailsByRequestId(const std::string& pickingRequestId) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("PickingDetailDAO::removePickingDetailsByRequestId: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + tableName_ + " WHERE picking_request_id = :picking_request_id;";
                std::map<std::string, std::any> params;
                params["picking_request_id"] = pickingRequestId;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("PickingDetailDAO::removePickingDetailsByRequestId: Failed to remove picking details for request_id " + pickingRequestId + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove picking details.", "Không thể xóa chi tiết yêu cầu lấy hàng.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

        } // namespace DAOs
    } // namespace Warehouse
} // namespace ERP