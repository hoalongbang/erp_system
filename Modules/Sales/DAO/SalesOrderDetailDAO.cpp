// Modules/Sales/DAO/SalesOrderDetailDAO.cpp
#include "SalesOrderDetailDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions (BaseDTO)

namespace ERP {
    namespace Sales {
        namespace DAOs {

            SalesOrderDetailDAO::SalesOrderDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Sales::DTO::SalesOrderDetailDTO>(connectionPool, "sales_order_details") {
                // DAOBase constructor handles connectionPool and tableName_ initialization
                ERP::Logger::Logger::getInstance().info("SalesOrderDetailDAO: Initialized.");
            }

            std::map<std::string, std::any> SalesOrderDetailDAO::toMap(const ERP::Sales::DTO::SalesOrderDetailDTO& detail) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(detail); // BaseDTO fields

                data["sales_order_id"] = detail.salesOrderId;
                ERP::DAOHelpers::putOptionalString(data, "sales_order_item_id", detail.salesOrderItemId); // NEW field
                data["product_id"] = detail.productId;
                data["quantity"] = detail.quantity;
                data["unit_of_measure_id"] = detail.unitOfMeasureId;
                data["unit_price"] = detail.unitPrice;
                data["discount"] = detail.discount;
                data["discount_type"] = static_cast<int>(detail.discountType);
                data["tax_rate"] = detail.taxRate;
                data["line_total"] = detail.lineTotal;
                data["delivered_quantity"] = detail.deliveredQuantity;
                data["invoiced_quantity"] = detail.invoicedQuantity;
                data["is_fully_delivered"] = detail.isFullyDelivered;
                data["is_fully_invoiced"] = detail.isFullyInvoiced;
                ERP::DAOHelpers::putOptionalString(data, "notes", detail.notes);

                return data;
            }

            ERP::Sales::DTO::SalesOrderDetailDTO SalesOrderDetailDAO::fromMap(const std::map<std::string, std::any>& data) const {
                ERP::Sales::DTO::SalesOrderDetailDTO detail;
                ERP::Utils::DTOUtils::fromMap(data, detail); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "sales_order_id", detail.salesOrderId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "sales_order_item_id", detail.salesOrderItemId); // NEW field
                    ERP::DAOHelpers::getPlainValue(data, "product_id", detail.productId);
                    ERP::DAOHelpers::getPlainValue(data, "quantity", detail.quantity);
                    ERP::DAOHelpers::getPlainValue(data, "unit_of_measure_id", detail.unitOfMeasureId);
                    ERP::DAOHelpers::getPlainValue(data, "unit_price", detail.unitPrice);
                    ERP::DAOHelpers::getPlainValue(data, "discount", detail.discount);

                    int discountTypeInt;
                    ERP::DAOHelpers::getPlainValue(data, "discount_type", discountTypeInt);
                    detail.discountType = static_cast<ERP::Sales::DTO::DiscountType>(discountTypeInt);

                    ERP::DAOHelpers::getPlainValue(data, "tax_rate", detail.taxRate);
                    ERP::DAOHelpers::getPlainValue(data, "line_total", detail.lineTotal);
                    ERP::DAOHelpers::getPlainValue(data, "delivered_quantity", detail.deliveredQuantity);
                    ERP::DAOHelpers::getPlainValue(data, "invoiced_quantity", detail.invoicedQuantity);
                    ERP::DAOHelpers::getPlainValue(data, "is_fully_delivered", detail.isFullyDelivered); // Should be bool/int
                    ERP::DAOHelpers::getPlainValue(data, "is_fully_invoiced", detail.isFullyInvoiced); // Should be bool/int
                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", detail.notes);

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("SalesOrderDetailDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "SalesOrderDetailDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("SalesOrderDetailDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "SalesOrderDetailDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return detail;
            }

            bool SalesOrderDetailDAO::save(const ERP::Sales::DTO::SalesOrderDetailDTO& detail) {
                return create(detail);
            }

            std::optional<ERP::Sales::DTO::SalesOrderDetailDTO> SalesOrderDetailDAO::findById(const std::string& id) {
                return getById(id);
            }

            bool SalesOrderDetailDAO::update(const ERP::Sales::DTO::SalesOrderDetailDTO& detail) {
                return DAOBase<ERP::Sales::DTO::SalesOrderDetailDTO>::update(detail);
            }

            bool SalesOrderDetailDAO::remove(const std::string& id) {
                return DAOBase<ERP::Sales::DTO::SalesOrderDetailDTO>::remove(id);
            }

            std::vector<ERP::Sales::DTO::SalesOrderDetailDTO> SalesOrderDetailDAO::findAll() {
                return DAOBase<ERP::Sales::DTO::SalesOrderDetailDTO>::findAll();
            }

            std::vector<ERP::Sales::DTO::SalesOrderDetailDTO> SalesOrderDetailDAO::getSalesOrderDetailsByOrderId(const std::string& salesOrderId) {
                std::map<std::string, std::any> filters;
                filters["sales_order_id"] = salesOrderId;
                return getSalesOrderDetails(filters);
            }

            std::vector<ERP::Sales::DTO::SalesOrderDetailDTO> SalesOrderDetailDAO::getSalesOrderDetails(const std::map<std::string, std::any>& filters) {
                return get(filters); // Use templated get
            }

            int SalesOrderDetailDAO::countSalesOrderDetails(const std::map<std::string, std::any>& filters) {
                return count(filters); // Use templated count
            }

            bool SalesOrderDetailDAO::removeSalesOrderDetailsByOrderId(const std::string& salesOrderId) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("SalesOrderDetailDAO::removeSalesOrderDetailsByOrderId: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + tableName_ + " WHERE sales_order_id = :sales_order_id;";
                std::map<std::string, std::any> params;
                params["sales_order_id"] = salesOrderId;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("SalesOrderDetailDAO::removeSalesOrderDetailsByOrderId: Failed to remove sales order details for order_id " + salesOrderId + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove sales order details.", "Không thể xóa chi tiết đơn hàng bán.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

        } // namespace DAOs
    } // namespace Sales
} // namespace ERP