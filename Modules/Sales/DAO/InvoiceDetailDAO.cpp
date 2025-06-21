// Modules/Sales/DAO/InvoiceDetailDAO.cpp
#include "InvoiceDetailDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions (BaseDTO)

namespace ERP {
    namespace Sales {
        namespace DAOs {

            InvoiceDetailDAO::InvoiceDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Sales::DTO::InvoiceDetailDTO>(connectionPool, "invoice_details") {
                // DAOBase constructor handles connectionPool and tableName_ initialization
                ERP::Logger::Logger::getInstance().info("InvoiceDetailDAO: Initialized.");
            }

            std::map<std::string, std::any> InvoiceDetailDAO::toMap(const ERP::Sales::DTO::InvoiceDetailDTO& detail) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(detail); // BaseDTO fields

                data["invoice_id"] = detail.invoiceId;
                ERP::DAOHelpers::putOptionalString(data, "sales_order_detail_id", detail.salesOrderDetailId);
                data["product_id"] = detail.productId;
                data["quantity"] = detail.quantity;
                data["unit_price"] = detail.unitPrice;
                data["discount"] = detail.discount;
                data["discount_type"] = static_cast<int>(detail.discountType);
                data["tax_rate"] = detail.taxRate;
                data["line_total"] = detail.lineTotal;
                ERP::DAOHelpers::putOptionalString(data, "notes", detail.notes);

                return data;
            }

            ERP::Sales::DTO::InvoiceDetailDTO InvoiceDetailDAO::fromMap(const std::map<std::string, std::any>& data) const {
                ERP::Sales::DTO::InvoiceDetailDTO detail;
                ERP::Utils::DTOUtils::fromMap(data, detail); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "invoice_id", detail.invoiceId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "sales_order_detail_id", detail.salesOrderDetailId);
                    ERP::DAOHelpers::getPlainValue(data, "product_id", detail.productId);
                    ERP::DAOHelpers::getPlainValue(data, "quantity", detail.quantity);
                    ERP::DAOHelpers::getPlainValue(data, "unit_price", detail.unitPrice);
                    ERP::DAOHelpers::getPlainValue(data, "discount", detail.discount);

                    int discountTypeInt;
                    ERP::DAOHelpers::getPlainValue(data, "discount_type", discountTypeInt);
                    detail.discountType = static_cast<ERP::Sales::DTO::DiscountType>(discountTypeInt);

                    ERP::DAOHelpers::getPlainValue(data, "tax_rate", detail.taxRate);
                    ERP::DAOHelpers::getPlainValue(data, "line_total", detail.lineTotal);
                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", detail.notes);

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("InvoiceDetailDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "InvoiceDetailDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("InvoiceDetailDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "InvoiceDetailDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return detail;
            }

            bool InvoiceDetailDAO::save(const ERP::Sales::DTO::InvoiceDetailDTO& detail) {
                return create(detail);
            }

            std::optional<ERP::Sales::DTO::InvoiceDetailDTO> InvoiceDetailDAO::findById(const std::string& id) {
                return getById(id);
            }

            bool InvoiceDetailDAO::update(const ERP::Sales::DTO::InvoiceDetailDTO& detail) {
                return DAOBase<ERP::Sales::DTO::InvoiceDetailDTO>::update(detail);
            }

            bool InvoiceDetailDAO::remove(const std::string& id) {
                return DAOBase<ERP::Sales::DTO::InvoiceDetailDTO>::remove(id);
            }

            std::vector<ERP::Sales::DTO::InvoiceDetailDTO> InvoiceDetailDAO::findAll() {
                return DAOBase<ERP::Sales::DTO::InvoiceDetailDTO>::findAll();
            }

            std::vector<ERP::Sales::DTO::InvoiceDetailDTO> InvoiceDetailDAO::getInvoiceDetailsByInvoiceId(const std::string& invoiceId) {
                std::map<std::string, std::any> filters;
                filters["invoice_id"] = invoiceId;
                return getInvoiceDetails(filters);
            }

            std::vector<ERP::Sales::DTO::InvoiceDetailDTO> InvoiceDetailDAO::getInvoiceDetails(const std::map<std::string, std::any>& filters) {
                return get(filters); // Use templated get
            }

            int InvoiceDetailDAO::countInvoiceDetails(const std::map<std::string, std::any>& filters) {
                return count(filters); // Use templated count
            }

            bool InvoiceDetailDAO::removeInvoiceDetailsByInvoiceId(const std::string& invoiceId) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("InvoiceDetailDAO::removeInvoiceDetailsByInvoiceId: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + tableName_ + " WHERE invoice_id = :invoice_id;";
                std::map<std::string, std::any> params;
                params["invoice_id"] = invoiceId;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("InvoiceDetailDAO::removeInvoiceDetailsByInvoiceId: Failed to remove invoice details for invoice_id " + invoiceId + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove invoice details.", "Không thể xóa chi tiết hóa đơn.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

        } // namespace DAOs
    } // namespace Sales
} // namespace ERP