// Modules/Sales/DAO/PaymentDAO.cpp
#include "Modules/Sales/DAO/PaymentDAO.h"
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

PaymentDAO::PaymentDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Sales::DTO::PaymentDTO>(connectionPool, "payments") { // Pass table name for PaymentDTO
    Logger::Logger::getInstance().info("PaymentDAO: Initialized.");
}

// toMap for PaymentDTO
std::map<std::string, std::any> PaymentDAO::toMap(const ERP::Sales::DTO::PaymentDTO& dto) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(dto); // Populate BaseDTO fields

    data["customer_id"] = dto.customerId;
    data["invoice_id"] = dto.invoiceId;
    data["payment_number"] = dto.paymentNumber;
    data["amount"] = dto.amount;
    data["payment_date"] = ERP::Utils::DateUtils::formatDateTime(dto.paymentDate, ERP::Common::DATETIME_FORMAT);
    data["method"] = static_cast<int>(dto.method);
    data["status"] = static_cast<int>(dto.status);
    ERP::DAOHelpers::putOptionalString(data, "transaction_id", dto.transactionId);
    ERP::DAOHelpers::putOptionalString(data, "notes", dto.notes);
    data["currency"] = dto.currency;

    return data;
}

// fromMap for PaymentDTO
ERP::Sales::DTO::PaymentDTO PaymentDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Sales::DTO::PaymentDTO dto;
    ERP::Utils::DTOUtils::fromMap(data, dto); // Populate BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "customer_id", dto.customerId);
        ERP::DAOHelpers::getPlainValue(data, "invoice_id", dto.invoiceId);
        ERP::DAOHelpers::getPlainValue(data, "payment_number", dto.paymentNumber);
        ERP::DAOHelpers::getPlainValue(data, "amount", dto.amount);
        ERP::DAOHelpers::getPlainTimeValue(data, "payment_date", dto.paymentDate);
        
        int methodInt;
        if (ERP::DAOHelpers::getPlainValue(data, "method", methodInt)) dto.method = static_cast<ERP::Sales::DTO::PaymentMethod>(methodInt);
        
        int statusInt;
        if (ERP::DAOHelpers::getPlainValue(data, "status", statusInt)) dto.status = static_cast<ERP::Sales::DTO::PaymentStatus>(statusInt);
        
        ERP::DAOHelpers::getOptionalStringValue(data, "transaction_id", dto.transactionId);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", dto.notes);
        ERP::DAOHelpers::getPlainValue(data, "currency", dto.currency);
    } catch (const std::bad_any_cast& e) {
        Logger::Logger::getInstance().error("PaymentDAO: fromMap - Data type mismatch: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "PaymentDAO: Data type mismatch in fromMap.");
    } catch (const std::exception& e) {
        Logger::Logger::getInstance().error("PaymentDAO: fromMap - Unexpected error: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "PaymentDAO: Unexpected error in fromMap.");
    }
    return dto;
}

} // namespace DAOs
} // namespace Sales
} // namespace ERP