// Modules/Manufacturing/Service/ProductionOrderService.cpp
#include "ProductionOrderService.h" // Đã rút gọn include
#include "ProductionOrder.h" // Đã rút gọn include
#include "BillOfMaterial.h" // Đã rút gọn include
#include "ProductionLine.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "ProductService.h" // Đã rút gọn include
#include "BillOfMaterialService.h" // Đã rút gọn include
#include "ProductionLineService.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Manufacturing {
namespace Services {

ProductionOrderService::ProductionOrderService(
    std::shared_ptr<DAOs::ProductionOrderDAO> productionOrderDAO,
    std::shared_ptr<ERP::Product::Services::IProductService> productService,
    std::shared_ptr<IBillOfMaterialService> billOfMaterialService,
    std::shared_ptr<IProductionLineService> productionLineService,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      productionOrderDAO_(productionOrderDAO), productService_(productService),
      billOfMaterialService_(billOfMaterialService), productionLineService_(productionLineService) {
    if (!productionOrderDAO_ || !productService_ || !billOfMaterialService_ || !productionLineService_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "ProductionOrderService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ lệnh sản xuất.");
        ERP::Logger::Logger::getInstance().critical("ProductionOrderService: One or more injected DAOs/Services are null.");
        throw std::runtime_error("ProductionOrderService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("ProductionOrderService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> ProductionOrderService::createProductionOrder(
    const ERP::Manufacturing::DTO::ProductionOrderDTO& productionOrderDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ProductionOrderService: Attempting to create production order: " + productionOrderDTO.orderNumber + " for product: " + productionOrderDTO.productId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.CreateProductionOrder", "Bạn không có quyền tạo lệnh sản xuất.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (productionOrderDTO.orderNumber.empty() || productionOrderDTO.productId.empty() || productionOrderDTO.plannedQuantity <= 0 || productionOrderDTO.unitOfMeasureId.empty()) {
        ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Invalid input for production order creation (empty number, product, non-positive quantity, or empty unit).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ProductionOrderService: Invalid input for production order creation.", "Thông tin lệnh sản xuất không đầy đủ hoặc không hợp lệ.");
        return std::nullopt;
    }

    // Check if order number already exists
    std::map<std::string, std::any> filterByNumber;
    filterByNumber["order_number"] = productionOrderDTO.orderNumber;
    if (productionOrderDAO_->count(filterByNumber) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Production order with number " + productionOrderDTO.orderNumber + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ProductionOrderService: Production order with number " + productionOrderDTO.orderNumber + " already exists.", "Số lệnh sản xuất đã tồn tại. Vui lòng chọn số khác.");
        return std::nullopt;
    }

    // Validate Product existence
    if (!productService_->getProductById(productionOrderDTO.productId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Product " + productionOrderDTO.productId + " not found for production order.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Sản phẩm không tồn tại.");
        return std::nullopt;
    }
    // Validate Unit of Measure existence
    if (!securityManager_->getUnitOfMeasureService()->getUnitOfMeasureById(productionOrderDTO.unitOfMeasureId, userRoleIds)) { // Get UoM service via SecurityManager
        ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Unit of measure " + productionOrderDTO.unitOfMeasureId + " not found for production order.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Đơn vị đo không tồn tại.");
        return std::nullopt;
    }
    // Validate BOM existence if specified
    if (productionOrderDTO.bomId && !billOfMaterialService_->getBillOfMaterialById(*productionOrderDTO.bomId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Bill of Material " + *productionOrderDTO.bomId + " not found for production order.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Định mức nguyên vật liệu (BOM) không tồn tại.");
        return std::nullopt;
    }
    // Validate Production Line existence if specified
    if (productionOrderDTO.productionLineId && !productionLineService_->getProductionLineById(*productionOrderDTO.productionLineId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Production line " + *productionOrderDTO.productionLineId + " not found for production order.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Dây chuyền sản xuất không tồn tại.");
        return std::nullopt;
    }

    ERP::Manufacturing::DTO::ProductionOrderDTO newProductionOrder = productionOrderDTO;
    newProductionOrder.id = ERP::Utils::generateUUID();
    newProductionOrder.createdAt = ERP::Utils::DateUtils::now();
    newProductionOrder.createdBy = currentUserId;
    newProductionOrder.status = ERP::Manufacturing::DTO::ProductionOrderStatus::DRAFT; // Default status

    std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> createdProductionOrder = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!productionOrderDAO_->create(newProductionOrder)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("ProductionOrderService: Failed to create production order " + newProductionOrder.orderNumber + " in DAO.");
                return false;
            }
            createdProductionOrder = newProductionOrder;
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::ProductionOrderCreatedEvent>(newProductionOrder));
            return true;
        },
        "ProductionOrderService", "createProductionOrder"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ProductionOrderService: Production order " + newProductionOrder.orderNumber + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "ProductionOrder", newProductionOrder.id, "ProductionOrder", newProductionOrder.orderNumber,
                       std::nullopt, newProductionOrder.toMap(), "Production order created.");
        return createdProductionOrder;
    }
    return std::nullopt;
}

std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> ProductionOrderService::getProductionOrderById(
    const std::string& orderId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("ProductionOrderService: Retrieving production order by ID: " + orderId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.ViewProductionOrder", "Bạn không có quyền xem lệnh sản xuất.")) {
        return std::nullopt;
    }

    return productionOrderDAO_->getById(orderId); // Using getById from DAOBase template
}

std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> ProductionOrderService::getProductionOrderByNumber(
    const std::string& orderNumber,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("ProductionOrderService: Retrieving production order by number: " + orderNumber + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.ViewProductionOrder", "Bạn không có quyền xem lệnh sản xuất.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["order_number"] = orderNumber;
    std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> orders = productionOrderDAO_->get(filter); // Using get from DAOBase template
    if (!orders.empty()) {
        return orders[0];
    }
    ERP::Logger::Logger::getInstance().debug("ProductionOrderService: Production order with number " + orderNumber + " not found.");
    return std::nullopt;
}

std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> ProductionOrderService::getAllProductionOrders(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ProductionOrderService: Retrieving all production orders with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.ViewProductionOrder", "Bạn không có quyền xem tất cả lệnh sản xuất.")) {
        return {};
    }

    return productionOrderDAO_->get(filter); // Using get from DAOBase template
}

std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> ProductionOrderService::getProductionOrdersByStatus(
    ERP::Manufacturing::DTO::ProductionOrderStatus status,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ProductionOrderService: Retrieving production orders by status: " + ERP::Manufacturing::DTO::ProductionOrderDTO().getStatusString(status) + " by user: " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.ViewProductionOrder", "Bạn không có quyền xem lệnh sản xuất theo trạng thái.")) {
        return {};
    }

    std::map<std::string, std::any> filter;
    filter["status"] = static_cast<int>(status);
    return productionOrderDAO_->get(filter); // Using get from DAOBase template
}

std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> ProductionOrderService::getProductionOrdersByProductionLine(
    const std::string& productionLineId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ProductionOrderService: Retrieving production orders for production line: " + productionLineId + " by user: " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.ViewProductionOrder", "Bạn không có quyền xem lệnh sản xuất theo dây chuyền sản xuất.")) {
        return {};
    }

    // Validate Production Line existence
    if (!productionLineService_->getProductionLineById(productionLineId, userRoleIds)) {
        ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Production line " + productionLineId + " not found.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Dây chuyền sản xuất không tồn tại.");
        return {};
    }

    std::map<std::string, std::any> filter;
    filter["production_line_id"] = productionLineId;
    return productionOrderDAO_->get(filter); // Using get from DAOBase template
}

bool ProductionOrderService::updateProductionOrder(
    const ERP::Manufacturing::DTO::ProductionOrderDTO& productionOrderDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ProductionOrderService: Attempting to update production order: " + productionOrderDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.UpdateProductionOrder", "Bạn không có quyền cập nhật lệnh sản xuất.")) {
        return false;
    }

    std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> oldProductionOrderOpt = productionOrderDAO_->getById(productionOrderDTO.id); // Using getById from DAOBase
    if (!oldProductionOrderOpt) {
        ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Production order with ID " + productionOrderDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy lệnh sản xuất cần cập nhật.");
        return false;
    }

    // If order number is changed, check for uniqueness
    if (productionOrderDTO.orderNumber != oldProductionOrderOpt->orderNumber) {
        std::map<std::string, std::any> filterByNumber;
        filterByNumber["order_number"] = productionOrderDTO.orderNumber;
        if (productionOrderDAO_->count(filterByNumber) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("ProductionOrderService: New order number " + productionOrderDTO.orderNumber + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ProductionOrderService: New order number " + productionOrderDTO.orderNumber + " already exists.", "Số lệnh sản xuất mới đã tồn tại. Vui lòng chọn số khác.");
            return false;
        }
    }

    // Validate Product existence
    if (productionOrderDTO.productId != oldProductionOrderOpt->productId) { // Only check if changed
        if (!productService_->getProductById(productionOrderDTO.productId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Product " + productionOrderDTO.productId + " not found for production order update.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Sản phẩm không tồn tại.");
            return false;
        }
    }
    // Validate Unit of Measure existence
    if (productionOrderDTO.unitOfMeasureId != oldProductionOrderOpt->unitOfMeasureId) { // Only check if changed
        if (!securityManager_->getUnitOfMeasureService()->getUnitOfMeasureById(productionOrderDTO.unitOfMeasureId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Unit of measure " + productionOrderDTO.unitOfMeasureId + " not found for production order update.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Đơn vị đo không tồn tại.");
            return false;
        }
    }
    // Validate BOM existence if specified or changed
    if (productionOrderDTO.bomId != oldProductionOrderOpt->bomId) { // Only check if changed
        if (productionOrderDTO.bomId && !billOfMaterialService_->getBillOfMaterialById(*productionOrderDTO.bomId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Bill of Material " + *productionOrderDTO.bomId + " not found for production order update.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Định mức nguyên vật liệu (BOM) không tồn tại.");
            return false;
        }
    }
    // Validate Production Line existence if specified or changed
    if (productionOrderDTO.productionLineId != oldProductionOrderOpt->productionLineId) { // Only check if changed
        if (productionOrderDTO.productionLineId && !productionLineService_->getProductionLineById(*productionOrderDTO.productionLineId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Production line " + *productionOrderDTO.productionLineId + " not found for production order update.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Dây chuyền sản xuất không tồn tại.");
            return false;
        }
    }

    ERP::Manufacturing::DTO::ProductionOrderDTO updatedProductionOrder = productionOrderDTO;
    updatedProductionOrder.updatedAt = ERP::Utils::DateUtils::now();
    updatedProductionOrder.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!productionOrderDAO_->update(updatedProductionOrder)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("ProductionOrderService: Failed to update production order " + updatedProductionOrder.id + " in DAO.");
                return false;
            }
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::ProductionOrderUpdatedEvent>(updatedProductionOrder));
            return true;
        },
        "ProductionOrderService", "updateProductionOrder"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ProductionOrderService: Production order " + updatedProductionOrder.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "ProductionOrder", updatedProductionOrder.id, "ProductionOrder", updatedProductionOrder.orderNumber,
                       oldProductionOrderOpt->toMap(), updatedProductionOrder.toMap(), "Production order updated.");
        return true;
    }
    return false;
}

bool ProductionOrderService::updateProductionOrderStatus(
    const std::string& orderId,
    ERP::Manufacturing::DTO::ProductionOrderStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ProductionOrderService: Attempting to update status for production order: " + orderId + " to " + ERP::Manufacturing::DTO::ProductionOrderDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.UpdateProductionOrderStatus", "Bạn không có quyền cập nhật trạng thái lệnh sản xuất.")) {
        return false;
    }

    std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> orderOpt = productionOrderDAO_->getById(orderId); // Using getById from DAOBase
    if (!orderOpt) {
        ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Production order with ID " + orderId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy lệnh sản xuất để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Manufacturing::DTO::ProductionOrderDTO oldOrder = *orderOpt;
    if (oldOrder.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("ProductionOrderService: Production order " + orderId + " is already in status " + oldOrder.getStatusString() + ".");
        return true; // Already in desired status
    }

    // Add state transition validation logic here
    // For example: Cannot change from COMPLETED to IN_PROGRESS.
    // Cannot change from DRAFT directly to COMPLETED.

    ERP::Manufacturing::DTO::ProductionOrderDTO updatedOrder = oldOrder;
    updatedOrder.status = newStatus;
    updatedOrder.updatedAt = ERP::Utils::DateUtils::now();
    updatedOrder.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!productionOrderDAO_->update(updatedOrder)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("ProductionOrderService: Failed to update status for production order " + orderId + " in DAO.");
                return false;
            }
            // Optionally, publish event
            // eventBus_.publish(std::make_shared<EventBus::ProductionOrderStatusChangedEvent>(orderId, newStatus));
            return true;
        },
        "ProductionOrderService", "updateProductionOrderStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ProductionOrderService: Status for production order " + orderId + " updated successfully to " + updatedOrder.getStatusString() + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "ProductionOrderStatus", orderId, "ProductionOrder", oldOrder.orderNumber,
                       oldOrder.toMap(), updatedOrder.toMap(), "Production order status changed to " + updatedOrder.getStatusString() + ".");
        return true;
    }
    return false;
}

bool ProductionOrderService::deleteProductionOrder(
    const std::string& orderId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ProductionOrderService: Attempting to delete production order: " + orderId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.DeleteProductionOrder", "Bạn không có quyền xóa lệnh sản xuất.")) {
        return false;
    }

    std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> orderOpt = productionOrderDAO_->getById(orderId); // Using getById from DAOBase
    if (!orderOpt) {
        ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Production order with ID " + orderId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy lệnh sản xuất cần xóa.");
        return false;
    }

    ERP::Manufacturing::DTO::ProductionOrderDTO orderToDelete = *orderOpt;

    // Additional checks: Prevent deletion if order is IN_PROGRESS or COMPLETED, or has associated material issues
    if (orderToDelete.status == ERP::Manufacturing::DTO::ProductionOrderStatus::IN_PROGRESS ||
        orderToDelete.status == ERP::Manufacturing::DTO::ProductionOrderStatus::COMPLETED) {
        ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Cannot delete production order " + orderId + " as it is in progress or completed.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa lệnh sản xuất đang thực hiện hoặc đã hoàn thành.");
        return false;
    }
    // Check for associated material issue slips (via MaterialIssueSlipService or direct DAO query)
    // if (securityManager_->getMaterialIssueSlipService()->getMaterialIssueSlipsByProductionOrderId(orderId).size() > 0) {
    //     ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Cannot delete production order " + orderId + " as it has associated material issue slips.");
    //     ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa lệnh sản xuất có phiếu xuất vật tư liên quan.");
    //     return false;
    // }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // No direct details table for ProductionOrder, but if there were, they'd be removed here.
            if (!productionOrderDAO_->remove(orderId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("ProductionOrderService: Failed to delete production order " + orderId + " in DAO.");
                return false;
            }
            return true;
        },
        "ProductionOrderService", "deleteProductionOrder"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ProductionOrderService: Production order " + orderId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "ProductionOrder", orderId, "ProductionOrder", orderToDelete.orderNumber,
                       orderToDelete.toMap(), std::nullopt, "Production order deleted.");
        return true;
    }
    return false;
}

bool ProductionOrderService::recordActualQuantityProduced(
    const std::string& orderId,
    double actualQuantityProduced,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("ProductionOrderService: Attempting to record actual quantity produced for order: " + orderId + " to " + std::to_string(actualQuantityProduced) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Manufacturing.RecordActualQuantity", "Bạn không có quyền ghi nhận số lượng sản xuất thực tế.")) {
        return false;
    }

    std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> orderOpt = productionOrderDAO_->getById(orderId); // Using getById from DAOBase
    if (!orderOpt) {
        ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Production order with ID " + orderId + " not found for recording actual quantity.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy lệnh sản xuất để ghi nhận số lượng thực tế.");
        return false;
    }
    
    ERP::Manufacturing::DTO::ProductionOrderDTO oldOrder = *orderOpt;

    // Only allow recording for orders that are IN_PROGRESS or PLANNED
    if (oldOrder.status != ERP::Manufacturing::DTO::ProductionOrderStatus::PLANNED &&
        oldOrder.status != ERP::Manufacturing::DTO::ProductionOrderStatus::RELEASED &&
        oldOrder.status != ERP::Manufacturing::DTO::ProductionOrderStatus::IN_PROGRESS) {
        ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Cannot record actual quantity for order " + orderId + " in current status " + oldOrder.getStatusString() + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể ghi nhận số lượng thực tế cho lệnh sản xuất ở trạng thái hiện tại.");
        return false;
    }
    if (actualQuantityProduced < 0 || actualQuantityProduced < oldOrder.actualQuantityProduced) { // Cannot decrease recorded quantity
        ERP::Logger::Logger::getInstance().warning("ProductionOrderService: Invalid actual quantity produced for order " + orderId + ": " + std::to_string(actualQuantityProduced));
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng sản xuất thực tế không hợp lệ.");
        return false;
    }

    ERP::Manufacturing::DTO::ProductionOrderDTO updatedOrder = oldOrder;
    updatedOrder.actualQuantityProduced = actualQuantityProduced;
    updatedOrder.updatedAt = ERP::Utils::DateUtils::now();
    updatedOrder.updatedBy = currentUserId;

    // Automatically set status to COMPLETED if actualQuantityProduced >= plannedQuantity
    if (actualQuantityProduced >= oldOrder.plannedQuantity) {
        updatedOrder.status = ERP::Manufacturing::DTO::ProductionOrderStatus::COMPLETED;
        updatedOrder.actualEndDate = ERP::Utils::DateUtils::now();
        ERP::Logger::Logger::getInstance().info("ProductionOrderService: Production order " + orderId + " automatically set to COMPLETED.");
    } else if (oldOrder.status == ERP::Manufacturing::DTO::ProductionOrderStatus::PLANNED ||
               oldOrder.status == ERP::Manufacturing::DTO::ProductionOrderStatus::RELEASED) {
        // If not completed, and was planned/released, set to IN_PROGRESS
        updatedOrder.status = ERP::Manufacturing::DTO::ProductionOrderStatus::IN_PROGRESS;
        if (!updatedOrder.actualStartDate.has_value()) {
            updatedOrder.actualStartDate = ERP::Utils::DateUtils::now(); // Set actual start date on first production
        }
        ERP::Logger::Logger::getInstance().info("ProductionOrderService: Production order " + orderId + " automatically set to IN_PROGRESS.");
    }


    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!productionOrderDAO_->update(updatedOrder)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("ProductionOrderService: Failed to record actual quantity produced for order " + orderId + " in DAO.");
                return false;
            }
            // Optionally, create inventory transaction for goods receipt of finished product
            // and goods issue for raw materials (if not handled by separate services).
            // This is critical for inventory accuracy.
            // Example: securityManager_->getInventoryManagementService()->recordGoodsReceipt(new InventoryTransaction for finished product)
            // Example: securityManager_->getInventoryManagementService()->recordGoodsIssue(new InventoryTransaction for raw material consumption)
            return true;
        },
        "ProductionOrderService", "recordActualQuantityProduced"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("ProductionOrderService: Actual quantity produced recorded successfully for order: " + orderId);
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Manufacturing", "ActualQuantity", orderId, "ProductionOrder", oldOrder.orderNumber,
                       oldOrder.toMap(), updatedOrder.toMap(), "Actual quantity produced recorded: " + std::to_string(actualQuantityProduced) + ".");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Manufacturing
} // namespace ERP