// Modules/Sales/DAO/QuotationDAO.cpp
#include "Modules/Sales/DAO/QuotationDAO.h"
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

QuotationDAO::QuotationDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Sales::DTO::QuotationDTO>(connectionPool, "quotations") { // Pass table name for QuotationDTO
    Logger::Logger::getInstance().info("QuotationDAO: Initialized.");
}

// toMap for QuotationDTO
std::map<std::string, std::any> QuotationDAO::toMap(const ERP::Sales::DTO::QuotationDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["quotation_number"] = dto.quotationNumber;
    data["customer_id"] = dto.customerId;
    data["requested_by_user_id"] = dto.requestedByUserId;
    data["quotation_date"] = ERP::Utils::DateUtils::formatDateTime(dto.quotationDate, ERP::Common::DATETIME_FORMAT);
    data["valid_until_date"] = ERP::Utils::DateUtils::formatDateTime(dto.validUntilDate, ERP::Common::DATETIME_FORMAT);
    data["status"] = static_cast<int>(dto.status);
    data["total_amount"] = dto.totalAmount;
    data["total_discount"] = dto.totalDiscount;
    data["total_tax"] = dto.totalTax;
    data["net_amount"] = dto.netAmount;
    data["currency"] = dto.currency;
    ERP::DAOHelpers::putOptionalString(data, "payment_terms", dto.paymentTerms);
    ERP::DAOHelpers::putOptionalString(data, "delivery_terms", dto.deliveryTerms);
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);

    return data;
}

// fromMap for QuotationDTO
ERP::Sales::DTO::QuotationDTO QuotationDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Sales::DTO::QuotationDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "quotation_number", dto.quotationNumber);
        ERP::DAOHelpers::getPlainValue(data, "customer_id", dto.customerId);
        ERP::DAOHelpers::getPlainValue(data, "requested_by_user_id", dto.requestedByUserId);
        ERP::DAOHelpers::getPlainTimeValue(data, "quotation_date", dto.quotationDate);
        ERP::DAOHelpers::getPlainTimeValue(data, "valid_until_date", dto.validUntilDate);
        
        int statusInt;
        if (ERP::DAOHelpers::getPlainValue(data, "status", statusInt)) dto.status = static_cast<ERP::Sales::DTO::QuotationStatus>(statusInt);
        
        ERP::DAOHelpers::getPlainValue(data, "total_amount", dto.totalAmount);
        ERP::DAOHelpers::getPlainValue(data, "total_discount", dto.totalDiscount);
        ERP::DAOHelpers::getPlainValue(data, "total_tax", dto.totalTax);
        ERP::DAOHelpers::getPlainValue(data, "net_amount", dto.netAmount);
        ERP::DAOHelpers::getPlainValue(data, "currency", dto.currency);
        ERP::DAOHelpers::getOptionalStringValue(data, "payment_terms", dto.paymentTerms);
        ERP::DAOHelpers::getOptionalStringValue(data, "delivery_terms", dto.deliveryTerms);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("QuotationDAO: fromMap (Quotation) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "QuotationDAO: Data type mismatch in fromMap (Quotation).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("QuotationDAO: fromMap (Quotation) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "QuotationDAO: Unexpected error in fromMap (Quotation).");
    }
    return dto;
}

// --- QuotationDetailDTO specific methods and helpers ---
std::map<std::string, std::any> QuotationDAO::toMap(const ERP::Sales::DTO::QuotationDetailDTO& dto) {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["quotation_id"] = dto.quotationId;
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

ERP::Sales::DTO::QuotationDetailDTO QuotationDAO::fromMap(const std::map<std::string, std::any>& data) {
    ERP::Sales::DTO::QuotationDetailDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "quotation_id", dto.quotationId);
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
        Logger::Logger::getInstance().error("QuotationDAO: fromMap (Detail) - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "QuotationDAO: Data type mismatch in fromMap (Detail).");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("QuotationDAO: fromMap (Detail) - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "QuotationDAO: Unexpected error in fromMap (Detail).");
    }
    return dto;
}

bool QuotationDAO::createQuotationDetail(const ERP::Sales::DTO::QuotationDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("QuotationDAO: Attempting to create new quotation detail.");
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

    std::string sql = "INSERT INTO " + quotationDetailsTableName_ + " (" + columns + ") VALUES (" + placeholders + ");";
    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "QuotationDAO", "createQuotationDetail", sql, params
    );
}

std::optional<ERP::Sales::DTO::QuotationDetailDTO> QuotationDAO::getQuotationDetailById(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("QuotationDAO: Attempting to get quotation detail by ID: " + id);
    std::string sql = "SELECT * FROM " + quotationDetailsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "QuotationDAO", "getQuotationDetailById", sql, params
    );

    if (!resultsMap.empty()) {
        return fromMap(resultsMap.front());
    }
    return std::nullopt;
}

std::vector<ERP::Sales::DTO::QuotationDetailDTO> QuotationDAO::getQuotationDetailsByQuotationId(const std::string& quotationId) {
    ERP::Logger::Logger::getInstance().info("QuotationDAO: Retrieving quotation details for quotation ID: " + quotationId);
    std::string sql = "SELECT * FROM " + quotationDetailsTableName_ + " WHERE quotation_id = ?;";
    std::map<std::string, std::any> params;
    params["quotation_id"] = quotationId;

    std::vector<std::map<std::string, std::any>> resultsMap = queryDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->query(sql_l, p_l);
        },
        "QuotationDAO", "getQuotationDetailsByQuotationId", sql, params
    );

    std::vector<ERP::Sales::DTO::QuotationDetailDTO> resultsDto;
    for (const auto& rowMap : resultsMap) {
        resultsDto.push_back(fromMap(rowMap));
    }
    return resultsDto;
}

bool QuotationDAO::updateQuotationDetail(const ERP::Sales::DTO::QuotationDetailDTO& detail) {
    ERP::Logger::Logger::getInstance().info("QuotationDAO: Attempting to update quotation detail with ID: " + detail.id);
    std::map<std::string, std::any> data = toMap(detail);
    if (data.empty() || data.find("id") == data.end() || ERP::DAOHelpers::getPlainValue<std::string>(data, "id", std::string()).empty()) {
        ERP::Logger::Logger::getInstance().warning("QuotationDAO: Update detail called with empty data or missing ID.");
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "QuotationDAO: Update detail called with empty data or missing ID.");
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

    std::string sql = "UPDATE " + quotationDetailsTableName_ + " SET " + setClause + " WHERE id = ?;";
    params["id_filter"] = detail.id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "QuotationDAO", "updateQuotationDetail", sql, params
    );
}

bool QuotationDAO::removeQuotationDetail(const std::string& id) {
    ERP::Logger::Logger::getInstance().info("QuotationDAO: Attempting to remove quotation detail with ID: " + id);
    std::string sql = "DELETE FROM " + quotationDetailsTableName_ + " WHERE id = ?;";
    std::map<std::string, std::any> params;
    params["id"] = id;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "QuotationDAO", "removeQuotationDetail", sql, params
    );
}

bool QuotationDAO::removeQuotationDetailsByQuotationId(const std::string& quotationId) {
    ERP::Logger::Logger::getInstance().info("QuotationDAO: Attempting to remove all details for quotation ID: " + quotationId);
    std::string sql = "DELETE FROM " + quotationDetailsTableName_ + " WHERE quotation_id = ?;";
    std::map<std::string, std::any> params;
    params["quotation_id"] = quotationId;

    return executeDbOperation(
        [](std::shared_ptr<ERP::Database::DBConnection> conn, const std::string& sql_l, const std::map<std::string, std::any>& p_l) {
            return conn->execute(sql_l, p_l);
        },
        "QuotationDAO", "removeQuotationDetailsByQuotationId", sql, params
    );
}

} // namespace DAOs
} // namespace Sales
} // namespace ERP