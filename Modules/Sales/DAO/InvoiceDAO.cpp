// Modules/Sales/DAO/InvoiceDAO.cpp
#include "Modules/Sales/DAO/InvoiceDAO.h"
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

InvoiceDAO::InvoiceDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Sales::DTO::InvoiceDTO>(connectionPool, "invoices") { // Pass table name for InvoiceDTO
    Logger::Logger::getInstance().info("InvoiceDAO: Initialized.");
}

// toMap for InvoiceDTO
std::map<std::string, std::any> InvoiceDAO::toMap(const ERP::Sales::DTO::InvoiceDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["invoice_number"] = dto.invoiceNumber;
    data["customer_id"] = dto.customerId;
    data["sales_order_id"] = dto.salesOrderId;
    data["type"] = static_cast<int>(dto.type);
    data["invoice_date"] = ERP::Utils::DateUtils::formatDateTime(dto.invoiceDate, ERP::Common::DATETIME_FORMAT);
    data["due_date"] = ERP::Utils::DateUtils::formatDateTime(dto.dueDate, ERP::Common::DATETIME_FORMAT);
    data["status"] = static_cast<int>(dto.status);
    data["total_amount"] = dto.totalAmount;
    data["total_discount"] = dto.totalDiscount;
    data["total_tax"] = dto.totalTax;
    data["net_amount"] = dto.netAmount;
    data["amount_paid"] = dto.amountPaid;
    data["amount_due"] = dto.amountDue;
    data["currency"] = dto.currency;
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);

    return data;
}

// fromMap for InvoiceDTO
ERP::Sales::DTO::InvoiceDTO InvoiceDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Sales::DTO::InvoiceDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "invoice_number", dto.invoiceNumber);
        ERP::DAOHelpers::getPlainValue(data, "customer_id", dto.customerId);
        ERP::DAOHelpers::getPlainValue(data, "sales_order_id", dto.salesOrderId);
        
        int typeInt;
        if (ERP::DAOHelpers::getPlainValue(data, "type", typeInt)) dto.type = static_cast<ERP::Sales::DTO::InvoiceType>(typeInt);
        
        ERP::DAOHelpers::getPlainTimeValue(data, "invoice_date", dto.invoiceDate);
        ERP::DAOHelpers::getPlainTimeValue(data, "due_date", dto.dueDate);
        
        int statusInt;
        if (ERP::DAOHelpers::getPlainValue(data, "status", statusInt)) dto.status = static_cast<ERP::Sales::DTO::InvoiceStatus>(statusInt);
        
        ERP::DAOHelpers::getPlainValue(data, "total_amount", dto.totalAmount);
        ERP::DAOHelpers::getPlainValue(data, "total_discount", dto.totalDiscount);
        ERP::DAOHelpers::getPlainValue(data, "total_tax", dto.totalTax);
        ERP::DAOHelpers::getPlainValue(data, "net_amount", dto.netAmount);
        ERP::DAOHelpers::getPlainValue(data, "amount_paid", dto.amountPaid);
        ERP::DAOHelpers::getPlainValue(data, "amount_due", dto.amountDue);
        ERP::DAOHelpers::getPlainValue(data, "currency", dto.currency);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("InvoiceDAO: fromMap (Invoice) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "InvoiceDAO: Data type mismatch in fromMap (Invoice).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("InvoiceDAO: fromMap (Invoice) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "InvoiceDAO: Unexpected error in fromMap (Invoice).");
    }
    return dto;
}

// --- InvoiceDetailDTO specific methods and helpers ---
std::map<std::string, std::any> InvoiceDAO::toMap(const ERP::Sales::DTO::InvoiceDetailDTO& dto) {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["invoice_id"] = dto.invoiceId;
    ERP::DAOHelpers::putOptionalString(data, "sales_order_detail_id", dto.salesOrderDetailId);
    data["product_id"] = dto.productId;
    data["quantity"] = dto.quantity;
    data["unit_price"] = dto.unitPrice;
    data["discount"] = dto.discount;
    data["discount_type"] = static_cast<int>(dto.discountType);
    data["tax_rate"] = dto.taxRate;
    data["line_total"] = dto.lineTotal;
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);

    return data;
}

ERP::Sales::DTO::InvoiceDetailDTO InvoiceDAO::fromMap(const std::map<std::string, std::any>& data) {
    ERP::Sales::DTO::InvoiceDetailDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "invoice_id", dto.invoiceId);
        ERP::DAOHelpers::getOptionalStringValue(data, "sales_order_detail_id", dto.salesOrderDetailId);
        ERP::DAOHelpers::getPlainValue(data, "product_id", dto.productId);
        ERP::DAOHelpers::getPlainValue(data, "quantity", dto.quantity);
        ERP::DAOHelpers::getPlainValue(data, "unit_price", dto.unitPrice);
        ERP::DAOHelpers::getPlainValue(data, "discount", dto.discount);
        
        int discountTypeInt;
        if (ERP::DAOHelpers::getPlainValue(data, "discount_type", discountTypeInt)) dto.discountType = static_cast<ERP::Sales::DTO::DiscountType>(discountTypeInt);
        
        ERP::DAOHelpers::getPlainValue(data, "tax_rate", dto.taxRate);
        ERP::DAOHelpers::getPlainValue(data, "line_total", dto.lineTotal);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("InvoiceDAO: fromMap (Detail) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "InvoiceDAO: Data type mismatch in fromMap (Detail).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("InvoiceDAO: fromMap (Detail) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "InvoiceDAO: Unexpected error in fromMap (Detail).");
    }
    return dto;
}

bool InvoiceDAO::createInvoiceDetail(const ERP::Sales::DTO::InvoiceDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("InvoiceDAO: Attempting to create new invoice detail.");
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

    std::string sql = "INSERT INTO " + invoiceDetailsTableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "InvoiceDAO", "createInvoiceDetail", sql, params
    );
}

std::optional<ERP::Sales::DTO::InvoiceDetailDTO> InvoiceDAO::getInvoiceDetailById(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("InvoiceDAO: Attempting to get invoice detail by ID: " + id);
    std::string sql = "SELECT * FROM " + invoiceDetailsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "InvoiceDAO", "getInvoiceDetailById", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front());
    }
    return std::nullopt;
}

std::vector<ERP::Sales::DTO::InvoiceDetailDTO> InvoiceDAO::getInvoiceDetailsByInvoiceId(const std::string& invoiceId) {
    ERP::Logger::Logger::getInstance().info("InvoiceDAO: Retrieving invoice details for invoice ID: " + invoiceId);
    std::string sql = "SELECT * FROM " + invoiceDetailsTableName_ + " WHERE invoice_id = ?;";
    std::map<std::string, std::any> params;
    params["invoice_id"] = invoiceId;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "InvoiceDAO", "getInvoiceDetailsByInvoiceId", sql, params
    );

    std::vector<ERP::Sales::DTO::InvoiceDetailDTO> resultsDto;
    for (const auto& rowMap : resultsMap) {
        resultsDto.push_back(fromMap(rowMap));
    }
    return resultsDto;
}

bool InvoiceDAO::updateInvoiceDetail(const ERP::Sales::DTO::InvoiceDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("InvoiceDAO: Attempting to update invoice detail with ID: " + detail.id);
    std::map<std::string, std::any> data = toMap(detail);
    if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
        ERP::Logger::Logger::getInstance().warning("InvoiceDAO: Update invoice detail called with empty data or missing ID.");
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "InvoiceDAO: Update invoice detail called with empty data or missing ID.");
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

    std::string sql = "UPDATE " + invoiceDetailsTableName_ + " SET " + setClause + " WHERE id = ?;";
    params["id_filter"] = detail.id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "InvoiceDAO", "updateInvoiceDetail", sql, params
    );
}

bool InvoiceDAO::removeInvoiceDetail(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("InvoiceDAO: Attempting to remove invoice detail with ID: " + id);
    std::string sql = "DELETE FROM " + invoiceDetailsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "InvoiceDAO", "removeInvoiceDetail", sql, params
    );
}

bool InvoiceDAO::removeInvoiceDetailsByInvoiceId(const std::string& invoiceId) {
    ERP::Logger::Logger::getInstance().info("InvoiceDAO: Attempting to remove all details for invoice ID: " + invoiceId);
    std::string sql = "DELETE FROM " + invoiceDetailsTableName_ + " WHERE invoice_id = ?;";
    std::map<std::string, std::any> params;
    params["invoice_id"] = invoiceId;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "InvoiceDAO", "removeInvoiceDetailsByInvoiceId", sql, params
    );
}

} // namespace DAOs
} // namespace Sales
} // namespace ERP