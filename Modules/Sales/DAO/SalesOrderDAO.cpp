// Modules/Sales/DAO/SalesOrderDAO.cpp
#include "Modules/Sales/DAO/SalesOrderDAO.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "DAOHelpers.h"
#include "Modules/Utils/DTOUtils.h" // For common DTO to map conversions
#include <sstream>
#include <stdexcept>
#include <typeinfo> // For std::bad_any_cast

namespace ERP {
namespace Sales {
namespace DAOs {

SalesOrderDAO::SalesOrderDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Sales::DTO::SalesOrderDTO>(connectionPool, "sales_orders") { // Pass table name for SalesOrderDTO
    Logger::Logger::getInstance().info("SalesOrderDAO: Initialized.");
}

// toMap for SalesOrderDTO
std::map<std::string, std::any> SalesOrderDAO::toMap(const ERP::Sales::DTO::SalesOrderDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["order_number"] = dto.orderNumber;
    data["customer_id"] = dto.customerId;
    data["requested_by_user_id"] = dto.requestedByUserId;
    ERP::DAOHelpers::putOptionalString(data, "approved_by_user_id", dto.approvedByUserId);
    data["order_date"] = ERP::Utils::DateUtils::formatDateTime(dto.orderDate, ERP::Common::DATETIME_FORMAT);
    ERP::DAOHelpers::putOptionalTime(data, "required_delivery_date", dto.requiredDeliveryDate);
    data["status"] = static_cast<int>(dto.status);
    data["total_amount"] = dto.totalAmount;
    data["total_discount"] = dto.totalDiscount;
    data["total_tax"] = dto.totalTax;
    data["net_amount"] = dto.netAmount;
    data["amount_paid"] = dto.amountPaid;
    data["amount_due"] = dto.amountDue;
    data["currency"] = dto.currency;
    ERP::DAOHelpers::putOptionalString(data, "payment_terms", dto.paymentTerms);
    ERP::DAOHelpers::putOptionalString(data, "delivery_address", dto.deliveryAddress);
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);
    data["warehouse_id"] = dto.warehouseId;
    ERP::DAOHelpers::putOptionalString(data, "quotation_id", dto.quotationId);

    return data;
}

// fromMap for SalesOrderDTO
ERP::Sales::DTO::SalesOrderDTO SalesOrderDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Sales::DTO::SalesOrderDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "order_number", dto.orderNumber);
        ERP::DAOHelpers::getPlainValue(data, "customer_id", dto.customerId);
        ERP::DAOHelpers::getPlainValue(data, "requested_by_user_id", dto.requestedByUserId);
        ERP::DAOHelpers::getOptionalStringValue(data, "approved_by_user_id", dto.approvedByUserId);
        ERP::DAOHelpers::getPlainTimeValue(data, "order_date", dto.orderDate);
        ERP::DAOHelpers::getOptionalTimeValue(data, "required_delivery_date", dto.requiredDeliveryDate);
        
        int statusInt;
        if (ERP::DAOHelpers::getPlainValue(data, "status", statusInt)) dto.status = static_cast<ERP::Sales::DTO::SalesOrderStatus>(statusInt);
        
        ERP::DAOHelpers::getPlainValue(data, "total_amount", dto.totalAmount);
        ERP::DAOHelpers::getPlainValue(data, "total_discount", dto.totalDiscount);
        ERP::DAOHelpers::getPlainValue(data, "total_tax", dto.totalTax);
        ERP::DAOHelpers::getPlainValue(data, "net_amount", dto.netAmount);
        ERP::DAOHelpers::getPlainValue(data, "amount_paid", dto.amountPaid);
        ERP::DAOHelpers::getPlainValue(data, "amount_due", dto.amountDue);
        ERP::DAOHelpers::getPlainValue(data, "currency", dto.currency);
        ERP::DAOHelpers::getOptionalStringValue(data, "payment_terms", dto.paymentTerms);
        ERP::DAOHelpers::getOptionalStringValue(data, "delivery_address", dto.deliveryAddress);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
        ERP::DAOHelpers::getPlainValue(data, "warehouse_id", dto.warehouseId);
        ERP::DAOHelpers::getOptionalStringValue(data, "quotation_id", dto.quotationId);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("SalesOrderDAO: fromMap (Order) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "SalesOrderDAO: Data type mismatch in fromMap (Order).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("SalesOrderDAO: fromMap (Order) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "SalesOrderDAO: Unexpected error in fromMap (Order).");
    }
    return dto;
}

// --- SalesOrderDetailDTO specific methods and helpers ---
std::map<std::string, std::any> SalesOrderDAO::toMap(const ERP::Sales::DTO::SalesOrderDetailDTO& dto) {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["sales_order_id"] = dto.salesOrderId;
    data["product_id"] = dto.productId;
    data["quantity"] = dto.quantity;
    data["unit_price"] = dto.unitPrice;
    data["discount"] = dto.discount;
    data["discount_type"] = static_cast<int>(dto.discountType);
    data["tax_rate"] = dto.taxRate;
    data["line_total"] = dto.lineTotal;
    data["delivered_quantity"] = dto.deliveredQuantity;
    data["invoiced_quantity"] = dto.invoicedQuantity;
    data["is_fully_delivered"] = dto.isFullyDelivered;
    data["is_fully_invoiced"] = dto.isFullyInvoiced;
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);

    return data;
}

ERP::Sales::DTO::SalesOrderDetailDTO SalesOrderDAO::fromMap(const std::map<std::string, std::any>& data) {
    ERP::Sales::DTO::SalesOrderDetailDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "sales_order_id", dto.salesOrderId);
        ERP::DAOHelpers::getPlainValue(data, "product_id", dto.productId);
        ERP::DAOHelpers::getPlainValue(data, "quantity", dto.quantity);
        ERP::DAOHelpers::getPlainValue(data, "unit_price", dto.unitPrice);
        ERP::DAOHelpers::getPlainValue(data, "discount", dto.discount);
        
        int discountTypeInt;
        if (ERP::DAOHelpers::getPlainValue(data, "discount_type", discountTypeInt)) dto.discountType = static_cast<ERP::Sales::DTO::DiscountType>(discountTypeInt);
        
        ERP::DAOHelpers::getPlainValue(data, "tax_rate", dto.taxRate);
        ERP::DAOHelpers::getPlainValue(data, "line_total", dto.lineTotal);
        ERP::DAOHelpers::getPlainValue(data, "delivered_quantity", dto.deliveredQuantity);
        ERP::DAOHelpers::getPlainValue(data, "invoiced_quantity", dto.invoicedQuantity);
        ERP::DAOHelpers::getPlainValue(data, "is_fully_delivered", dto.isFullyDelivered);
        ERP::DAOHelpers::getPlainValue(data, "is_fully_invoiced", dto.isFullyInvoiced);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("SalesOrderDAO: fromMap (Detail) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "SalesOrderDAO: Data type mismatch in fromMap (Detail).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("SalesOrderDAO: fromMap (Detail) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "SalesOrderDAO: Unexpected error in fromMap (Detail).");
    }
    return dto;
}

bool SalesOrderDAO::createSalesOrderDetail(const ERP::Sales::DTO::SalesOrderDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("SalesOrderDAO: Attempting to create new sales order detail.");
    std::map<std::string, std::any> data = toMap(detail);
    
    std::string columns;
    std::string placeholders;
    std::map<std::string, std::any> params;
    bool first = true;

    for (const auto& pair : data) {
        if (!first) { columns += ", "; placeholders += ", "; }
        columns += pair.first;
        placeholders += "?";
        params[pair.first] = pair.second;
        first = false;
    }

    std::string sql = "INSERT INTO " + salesOrderDetailsTableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "SalesOrderDAO", "createSalesOrderDetail", sql, params
    );
}

std::optional<ERP::Sales::DTO::SalesOrderDetailDTO> SalesOrderDAO::getSalesOrderDetailById(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("SalesOrderDAO: Attempting to get sales order detail by ID: " + id);
    std::string sql = "SELECT * FROM " + salesOrderDetailsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "SalesOrderDAO", "getSalesOrderDetailById", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front());
    }
    return std::nullopt;
}

std::vector<ERP::Sales::DTO::SalesOrderDetailDTO> SalesOrderDAO::getSalesOrderDetailsByOrderId(const std::string& orderId) {
    ERP::Logger::Logger::getInstance().info("SalesOrderDAO: Retrieving sales order details for order ID: " + orderId);
    std::string sql = "SELECT * FROM " + salesOrderDetailsTableName_ + " WHERE sales_order_id = ?;";
    std::map<std::string, std::any> params;
    params["sales_order_id"] = orderId;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "SalesOrderDAO", "getSalesOrderDetailsByOrderId", sql, params
    );

    std::vector<ERP::Sales::DTO::SalesOrderDetailDTO> resultsDto;
    for (const auto& rowMap : resultsMap) {
        resultsDto.push_back(fromMap(rowMap));
    }
    return resultsDto;
}

bool SalesOrderDAO::updateSalesOrderDetail(const ERP::Sales::DTO::SalesOrderDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("SalesOrderDAO: Attempting to update sales order detail with ID: " + detail.id);
    std::map<std::string, std::any> data = toMap(detail);
    if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
        ERP::Logger::Logger::getInstance().warning("SalesOrderDAO: Update detail called with empty data or missing ID.");
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "SalesOrderDAO: Update detail called with empty data or missing ID.");
        return false;
    }

    std::string setClause;
    std::map<std::string, std::any> params;
    bool firstSet = true;

    for (const auto& pair : data) {
        if (pair.first == "id") continue;
        if (!firstSet) setClause += ", ";
        setClause += pair.first + " = ?";
        params[pair.first] = pair.second;
        firstSet = false;
    }

    std::string sql = "UPDATE " + salesOrderDetailsTableName_ + " SET " + setClause + " WHERE id = ?;";
    params["id_filter"] = detail.id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "SalesOrderDAO", "updateSalesOrderDetail", sql, params
    );
}

bool SalesOrderDAO::removeSalesOrderDetail(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("SalesOrderDAO: Attempting to remove sales order detail with ID: " + id);
    std::string sql = "DELETE FROM " + salesOrderDetailsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "SalesOrderDAO", "removeSalesOrderDetail", sql, params
    );
}

bool SalesOrderDAO::removeSalesOrderDetailsByOrderId(const std::string& orderId) {
    ERP::Logger::Logger::getInstance().info("SalesOrderDAO: Attempting to remove all details for sales order ID: " + orderId);
    std::string sql = "DELETE FROM " + salesOrderDetailsTableName_ + " WHERE sales_order_id = ?;";
    std::map<std::string, std::any> params;
    params["sales_order_id"] = orderId;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "SalesOrderDAO", "removeSalesOrderDetailsByOrderId", sql, params
    );
}

} // namespace DAOs
} // namespace Sales
} // namespace ERP