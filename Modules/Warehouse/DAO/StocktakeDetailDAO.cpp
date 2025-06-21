// Modules/Warehouse/DAO/StocktakeDetailDAO.cpp
#include "StocktakeDetailDAO.h"
#include "DAOHelpers.h"     // Standard includes
#include "Logger.h"         // Standard includes
#include "ErrorHandler.h"   // Standard includes
#include "Common.h"         // Standard includes
#include "DateUtils.h"      // Standard includes
#include "DTOUtils.h"       // For BaseDTO conversions

namespace ERP {
    namespace Warehouse {
        namespace DAOs {

            StocktakeDetailDAO::StocktakeDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Warehouse::DTO::StocktakeDetailDTO>(connectionPool, "stocktake_details") {
                // DAOBase constructor handles connectionPool and tableName_ initialization
                ERP::Logger::Logger::getInstance().info("StocktakeDetailDAO: Initialized.");
            }

            std::map<std::string, std::any> StocktakeDetailDAO::toMap(const ERP::Warehouse::DTO::StocktakeDetailDTO& detail) const {
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

            ERP::Warehouse::DTO::StocktakeDetailDTO StocktakeDetailDAO::fromMap(const std::map<std::string, std::any>& data) const {
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
                    ERP::Logger::Logger::getInstance().error("StocktakeDetailDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "StocktakeDetailDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("StocktakeDetailDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "StocktakeDetailDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return detail;
            }

            bool StocktakeDetailDAO::save(const ERP::Warehouse::DTO::StocktakeDetailDTO& detail) {
                return create(detail);
            }

            std::optional<ERP::Warehouse::DTO::StocktakeDetailDTO> StocktakeDetailDAO::findById(const std::string& id) {
                return getById(id);
            }

            bool StocktakeDetailDAO::update(const ERP::Warehouse::DTO::StocktakeDetailDTO& detail) {
                return DAOBase<ERP::Warehouse::DTO::StocktakeDetailDTO>::update(detail);
            }

            bool StocktakeDetailDAO::remove(const std::string& id) {
                return DAOBase<ERP::Warehouse::DTO::StocktakeDetailDTO>::remove(id);
            }

            std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> StocktakeDetailDAO::findAll() {
                return DAOBase<ERP::Warehouse::DTO::StocktakeDetailDTO>::findAll();
            }

            std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> StocktakeDetailDAO::getStocktakeDetailsByRequestId(const std::string& stocktakeRequestId) {
                std::map<std::string, std::any> filters;
                filters["stocktake_request_id"] = stocktakeRequestId;
                return getStocktakeDetails(filters);
            }

            std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> StocktakeDetailDAO::getStocktakeDetails(const std::map<std::string, std::any>& filters) {
                return get(filters); // Use templated get
            }

            int StocktakeDetailDAO::countStocktakeDetails(const std::map<std::string, std::any>& filters) {
                return count(filters); // Use templated count
            }

            bool StocktakeDetailDAO::removeStocktakeDetailsByRequestId(const std::string& stocktakeRequestId) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("StocktakeDetailDAO::removeStocktakeDetailsByRequestId: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + tableName_ + " WHERE stocktake_request_id = :stocktake_request_id;";
                std::map<std::string, std::any> params;
                params["stocktake_request_id"] = stocktakeRequestId;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("StocktakeDetailDAO::removeStocktakeDetailsByRequestId: Failed to remove stocktake details for request_id " + stocktakeRequestId + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove stocktake details.", "Không thể xóa chi tiết kiểm kê.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

        } // namespace DAOs
    } // namespace Warehouse
} // namespace ERP