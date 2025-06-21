// Modules/Manufacturing/DAO/ProductionOrderDAO.cpp
#include "ProductionOrderDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes

namespace ERP {
namespace Manufacturing {
namespace DAOs {

ProductionOrderDAO::ProductionOrderDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
    : DAOBase<ERP::Manufacturing::DTO::ProductionOrderDTO>(connectionPool, "production_orders") {
    // DAOBase constructor handles connectionPool and tableName_ initialization
    ERP::Logger::Logger::getInstance().info("ProductionOrderDAO: Initialized.");
}

std::map<std::string, std::any> ProductionOrderDAO::toMap(const ERP::Manufacturing::DTO::ProductionOrderDTO& order) const {
    std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(order); // BaseDTO fields

    data["order_number"] = order.orderNumber;
    data["product_id"] = order.productId;
    data["planned_quantity"] = order.plannedQuantity;
    data["unit_of_measure_id"] = order.unitOfMeasureId;
    ERP::DAOHelpers::putOptionalString(data, "bom_id", order.bomId);
    ERP::DAOHelpers::putOptionalString(data, "production_line_id", order.productionLineId);
    data["status"] = static_cast<int>(order.status);
    data["planned_start_date"] = ERP::Utils::DateUtils::formatDateTime(order.plannedStartDate, ERP::Common::DATETIME_FORMAT);
    data["planned_end_date"] = ERP::Utils::DateUtils::formatDateTime(order.plannedEndDate, ERP::Common::DATETIME_FORMAT);
    ERP::DAOHelpers::putOptionalTime(data, "actual_start_date", order.actualStartDate);
    ERP::DAOHelpers::putOptionalTime(data, "actual_end_date", order.actualEndDate);
    data["actual_quantity_produced"] = order.actualQuantityProduced;
    ERP::DAOHelpers::putOptionalString(data, "notes", order.notes);

    return data;
}

ERP::Manufacturing::DTO::ProductionOrderDTO ProductionOrderDAO::fromMap(const std::map<std::string, std::any>& data) const {
    ERP::Manufacturing::DTO::ProductionOrderDTO order;
    ERP::Utils::DTOUtils::fromMap(data, order); // BaseDTO fields

    try {
        ERP::DAOHelpers::getPlainValue(data, "order_number", order.orderNumber);
        ERP::DAOHelpers::getPlainValue(data, "product_id", order.productId);
        ERP::DAOHelpers::getPlainValue(data, "planned_quantity", order.plannedQuantity);
        ERP::DAOHelpers::getPlainValue(data, "unit_of_measure_id", order.unitOfMeasureId);
        ERP::DAOHelpers::getOptionalStringValue(data, "bom_id", order.bomId);
        ERP::DAOHelpers::getOptionalStringValue(data, "production_line_id", order.productionLineId);
        
        int statusInt;
        ERP::DAOHelpers::getPlainValue(data, "status", statusInt);
        order.status = static_cast<ERP::Manufacturing::DTO::ProductionOrderStatus>(statusInt);

        ERP::DAOHelpers::getPlainTimeValue(data, "planned_start_date", order.plannedStartDate);
        ERP::DAOHelpers::getPlainTimeValue(data, "planned_end_date", order.plannedEndDate);
        ERP::DAOHelpers::getOptionalTimeValue(data, "actual_start_date", order.actualStartDate);
        ERP::DAOHelpers::getOptionalTimeValue(data, "actual_end_date", order.actualEndDate);
        ERP::DAOHelpers::getPlainValue(data, "actual_quantity_produced", order.actualQuantityProduced);
        ERP::DAOHelpers::getOptionalStringValue(data, "notes", order.notes);

    } catch (const std::bad_any_cast& e) {
        ERP::Logger::Logger::getInstance().error("ProductionOrderDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "ProductionOrderDAO: Data type mismatch in fromMap: " + std::string(e.what()));
    } catch (const std::exception& e) {
        ERP::Logger::Logger::getInstance().error("ProductionOrderDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
        ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "ProductionOrderDAO: Unexpected error in fromMap: " + std::string(e.what()));
    }
    return order;
}

bool ProductionOrderDAO::save(const ERP::Manufacturing::DTO::ProductionOrderDTO& order) {
    return create(order);
}

std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> ProductionOrderDAO::findById(const std::string& id) {
    return getById(id);
}

bool ProductionOrderDAO::update(const ERP::Manufacturing::DTO::ProductionOrderDTO& order) {
    return DAOBase<ERP::Manufacturing::DTO::ProductionOrderDTO>::update(order);
}

bool ProductionOrderDAO::remove(const std::string& id) {
    return DAOBase<ERP::Manufacturing::DTO::ProductionOrderDTO>::remove(id);
}

std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> ProductionOrderDAO::findAll() {
    return DAOBase<ERP::Manufacturing::DTO::ProductionOrderDTO>::findAll();
}

std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> ProductionOrderDAO::getProductionOrderByNumber(const std::string& orderNumber) {
    std::map<std::string, std::any> filters;
    filters["order_number"] = orderNumber;
    std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> results = get(filters); // Use templated get
    if (!results.empty()) {
        return results[0];
    }
    return std::nullopt;
}

std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> ProductionOrderDAO::getProductionOrders(const std::map<std::string, std::any>& filters) {
    return get(filters); // Use templated get
}

int ProductionOrderDAO::countProductionOrders(const std::map<std::string, std::any>& filters) {
    return count(filters); // Use templated count
}

} // namespace DAOs
} // namespace Manufacturing
} // namespace ERP