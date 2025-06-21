// Modules/Sales/DAO/ShipmentDetailDAO.cpp
#include "ShipmentDetailDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions (BaseDTO)

namespace ERP {
    namespace Sales {
        namespace DAOs {

            ShipmentDetailDAO::ShipmentDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Sales::DTO::ShipmentDetailDTO>(connectionPool, "shipment_details") {
                // DAOBase constructor handles connectionPool and tableName_ initialization
                ERP::Logger::Logger::getInstance().info("ShipmentDetailDAO: Initialized.");
            }

            std::map<std::string, std::any> ShipmentDetailDAO::toMap(const ERP::Sales::DTO::ShipmentDetailDTO& detail) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(detail); // BaseDTO fields

                data["shipment_id"] = detail.shipmentId;
                ERP::DAOHelpers::putOptionalString(data, "sales_order_item_id", detail.salesOrderItemId); // NEW field
                data["product_id"] = detail.productId;
                data["warehouse_id"] = detail.warehouseId;
                data["location_id"] = detail.locationId;
                data["quantity"] = detail.quantity;
                ERP::DAOHelpers::putOptionalString(data, "lot_number", detail.lotNumber);
                ERP::DAOHelpers::putOptionalString(data, "serial_number", detail.serialNumber);
                ERP::DAOHelpers::putOptionalString(data, "notes", detail.notes);

                return data;
            }

            ERP::Sales::DTO::ShipmentDetailDTO ShipmentDetailDAO::fromMap(const std::map<std::string, std::any>& data) const {
                ERP::Sales::DTO::ShipmentDetailDTO detail;
                ERP::Utils::DTOUtils::fromMap(data, detail); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "shipment_id", detail.shipmentId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "sales_order_item_id", detail.salesOrderItemId); // NEW field
                    ERP::DAOHelpers::getPlainValue(data, "product_id", detail.productId);
                    ERP::DAOHelpers::getPlainValue(data, "warehouse_id", detail.warehouseId);
                    ERP::DAOHelpers::getPlainValue(data, "location_id", detail.locationId);
                    ERP::DAOHelpers::getPlainValue(data, "quantity", detail.quantity);
                    ERP::DAOHelpers::getOptionalStringValue(data, "lot_number", detail.lotNumber);
                    ERP::DAOHelpers::getOptionalStringValue(data, "serial_number", detail.serialNumber);
                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", detail.notes);

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("ShipmentDetailDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ShipmentDetailDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("ShipmentDetailDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ShipmentDetailDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return detail;
            }

            bool ShipmentDetailDAO::save(const ERP::Sales::DTO::ShipmentDetailDTO& detail) {
                return create(detail);
            }

            std::optional<ERP::Sales::DTO::ShipmentDetailDTO> ShipmentDetailDAO::findById(const std::string& id) {
                return getById(id);
            }

            bool ShipmentDetailDAO::update(const ERP::Sales::DTO::ShipmentDetailDTO& detail) {
                return DAOBase<ERP::Sales::DTO::ShipmentDetailDTO>::update(detail);
            }

            bool ShipmentDetailDAO::remove(const std::string& id) {
                return DAOBase<ERP::Sales::DTO::ShipmentDetailDTO>::remove(id);
            }

            std::vector<ERP::Sales::DTO::ShipmentDetailDTO> ShipmentDetailDAO::findAll() {
                return DAOBase<ERP::Sales::DTO::ShipmentDetailDTO>::findAll();
            }

            std::vector<ERP::Sales::DTO::ShipmentDetailDTO> ShipmentDetailDAO::getShipmentDetailsByShipmentId(const std::string& shipmentId) {
                std::map<std::string, std::any> filters;
                filters["shipment_id"] = shipmentId;
                return getShipmentDetails(filters);
            }

            std::vector<ERP::Sales::DTO::ShipmentDetailDTO> ShipmentDetailDAO::getShipmentDetails(const std::map<std::string, std::any>& filters) {
                return get(filters); // Use templated get
            }

            int ShipmentDetailDAO::countShipmentDetails(const std::map<std::string, std::any>& filters) {
                return count(filters); // Use templated count
            }

            bool ShipmentDetailDAO::removeShipmentDetailsByShipmentId(const std::string& shipmentId) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("ShipmentDetailDAO::removeShipmentDetailsByShipmentId: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + tableName_ + " WHERE shipment_id = :shipment_id;";
                std::map<std::string, std::any> params;
                params["shipment_id"] = shipmentId;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("ShipmentDetailDAO::removeShipmentDetailsByShipmentId: Failed to remove shipment details for shipment_id " + shipmentId + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove shipment details.", "Không thể xóa các chi tiết vận chuyển.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

        } // namespace DAOs
    } // namespace Sales
} // namespace ERP