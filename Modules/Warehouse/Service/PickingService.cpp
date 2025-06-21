// Modules/Warehouse/Service/PickingService.cpp
#include "PickingService.h" // Standard includes
#include "PickingRequest.h"         // PickingRequest DTO
#include "PickingDetail.h"          // PickingDetail DTO
#include "SalesOrder.h"             // SalesOrder DTO
#include "Product.h"                // Product DTO
#include "Warehouse.h"              // Warehouse DTO
#include "Location.h"               // Location DTO
#include "InventoryTransaction.h"   // InventoryTransaction DTO for goods issue
#include "Event.h"                  // Event DTO
#include "ConnectionPool.h"         // ConnectionPool
#include "DBConnection.h"           // DBConnection
#include "Common.h"                 // Common Enums/Constants
#include "Utils.h"                  // Utility functions
#include "DateUtils.h"              // Date utility functions
#include "AutoRelease.h"            // RAII helper
#include "ISecurityManager.h"       // Security Manager interface
#include "UserService.h"            // User Service (for audit logging)
#include "SalesOrderService.h"      // SalesOrderService (for SO validation/updates)
#include "CustomerService.h"        // CustomerService (for customer validation)
#include "WarehouseService.h"       // WarehouseService (for warehouse validation)
#include "ProductService.h"         // ProductService (for product validation)
#include "InventoryManagementService.h" // InventoryManagementService (for goods issue)

#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed

namespace ERP {
    namespace Warehouse {
        namespace Services {

            PickingService::PickingService(
                std::shared_ptr<DAOs::PickingRequestDAO> pickingRequestDAO,
                std::shared_ptr<DAOs::PickingDetailDAO> pickingDetailDAO, // NEW
                std::shared_ptr<ERP::Sales::Services::ISalesOrderService> salesOrderService,
                std::shared_ptr<ERP::Customer::Services::ICustomerService> customerService,
                std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
                std::shared_ptr<ERP::Product::Services::IProductService> productService,
                std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService,
                std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
                : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Initialize BaseService
                pickingRequestDAO_(pickingRequestDAO),
                pickingDetailDAO_(pickingDetailDAO), // NEW
                salesOrderService_(salesOrderService),
                customerService_(customerService),
                warehouseService_(warehouseService),
                productService_(productService),
                inventoryManagementService_(inventoryManagementService) {

                if (!pickingRequestDAO_ || !pickingDetailDAO_ || !salesOrderService_ || !customerService_ || !warehouseService_ || !productService_ || !inventoryManagementService_ || !securityManager_) { // BaseService checks its own dependencies
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "PickingService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ lấy hàng.");
                    ERP::Logger::Logger::getInstance().critical("PickingService: One or more injected DAOs/Services are null.");
                    throw std::runtime_error("PickingService: Null dependencies.");
                }
                ERP::Logger::Logger::getInstance().info("PickingService: Initialized.");
            }

            std::optional<ERP::Warehouse::DTO::PickingRequestDTO> PickingService::createPickingRequest(
                const ERP::Warehouse::DTO::PickingRequestDTO& pickingRequestDTO,
                const std::vector<ERP::Warehouse::DTO::PickingDetailDTO>& pickingDetails,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("PickingService: Attempting to create picking request for sales order: " + pickingRequestDTO.salesOrderId + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.CreatePickingRequest", "Bạn không có quyền tạo yêu cầu lấy hàng.")) {
                    return std::nullopt;
                }

                // 1. Validate input DTO
                if (pickingRequestDTO.salesOrderId.empty() || pickingRequestDTO.warehouseId.empty() || pickingDetails.empty()) {
                    ERP::Logger::Logger::getInstance().warning("PickingService: Invalid input for request creation (missing sales order ID, warehouse ID, or details).");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin yêu cầu lấy hàng không đầy đủ.");
                    return std::nullopt;
                }

                // Validate Sales Order existence
                std::optional<ERP::Sales::DTO::SalesOrderDTO> salesOrder = salesOrderService_->getSalesOrderById(pickingRequestDTO.salesOrderId, userRoleIds);
                if (!salesOrder || (salesOrder->status != ERP::Sales::DTO::SalesOrderStatus::APPROVED && salesOrder->status != ERP::Sales::DTO::SalesOrderStatus::IN_PROGRESS && salesOrder->status != ERP::Sales::DTO::SalesOrderStatus::PARTIALLY_DELIVERED)) {
                    ERP::Logger::Logger::getInstance().warning("PickingService: Invalid Sales Order ID provided or sales order not in valid status: " + pickingRequestDTO.salesOrderId);
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Đơn hàng bán không hợp lệ hoặc không ở trạng thái đủ điều kiện lấy hàng.");
                    return std::nullopt;
                }

                // Validate Warehouse existence
                std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(pickingRequestDTO.warehouseId, userRoleIds);
                if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
                    ERP::Logger::Logger::getInstance().warning("PickingService: Invalid Warehouse ID provided or warehouse is not active: " + pickingRequestDTO.warehouseId);
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc không hoạt động.");
                    return std::nullopt;
                }

                // Validate details: Product existence, Location existence, quantities
                for (const auto& detail : pickingDetails) {
                    std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(detail.productId, userRoleIds);
                    if (!product || product->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("PickingService: Product " + detail.productId + " not found or not active in picking detail.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Sản phẩm trong chi tiết lấy hàng không hợp lệ.");
                        return std::nullopt;
                    }
                    std::optional<ERP::Catalog::DTO::LocationDTO> location = warehouseService_->getLocationById(detail.locationId, userRoleIds);
                    if (!location || location->status != ERP::Common::EntityStatus::ACTIVE || location->warehouseId != detail.warehouseId) {
                        ERP::Logger::Logger::getInstance().warning("PickingService: Location " + detail.locationId + " not found or not active or does not belong to warehouse " + detail.warehouseId + " in picking detail.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Vị trí lấy hàng không hợp lệ.");
                        return std::nullopt;
                    }
                    if (detail.requestedQuantity <= 0) {
                        ERP::Logger::Logger::getInstance().warning("PickingService: Invalid requested quantity in picking detail for product " + detail.productId);
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng yêu cầu trong chi tiết lấy hàng không hợp lệ.");
                        return std::nullopt;
                    }
                    // Check if enough available inventory for picking
                    std::optional<ERP::Warehouse::DTO::InventoryDTO> inventory = inventoryManagementService_->getInventoryByProductLocation(detail.productId, detail.warehouseId, detail.locationId, userRoleIds);
                    if (!inventory || inventory->availableQuantity.value_or(0.0) < detail.requestedQuantity) {
                        ERP::Logger::Logger::getInstance().warning("PickingService: Insufficient available stock for product " + detail.productId + " at " + detail.warehouseId + "/" + detail.locationId + ". Requested: " + std::to_string(detail.requestedQuantity) + ", Available: " + std::to_string(inventory->availableQuantity.value_or(0.0)));
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InsufficientStock, "Không đủ tồn kho khả dụng để tạo yêu cầu lấy hàng.");
                        return std::nullopt;
                    }
                }

                ERP::Warehouse::DTO::PickingRequestDTO newRequest = pickingRequestDTO;
                newRequest.id = ERP::Utils::generateUUID();
                newRequest.requestNumber = "PR-" + ERP::Utils::generateUUID().substr(0, 8); // Auto-generate request number
                newRequest.createdAt = ERP::Utils::DateUtils::now();
                newRequest.createdBy = currentUserId;
                newRequest.status = ERP::Warehouse::DTO::PickingRequestStatus::PENDING; // Always pending on creation
                newRequest.requestDate = ERP::Utils::DateUtils::now(); // Set actual request date

                std::optional<ERP::Warehouse::DTO::PickingRequestDTO> createdRequest = std::nullopt;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!pickingRequestDAO_->create(newRequest)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("PickingService: Failed to create picking request in DAO.");
                            return false;
                        }
                        // Save details and reserve inventory
                        for (auto detail : pickingDetails) {
                            detail.id = ERP::Utils::generateUUID();
                            detail.pickingRequestId = newRequest.id;
                            detail.createdAt = newRequest.createdAt; // Inherit creation time
                            detail.createdBy = newRequest.createdBy; // Inherit created by
                            detail.status = ERP::Common::EntityStatus::ACTIVE; // Details are active
                            // Ensure picked quantity is 0 initially
                            detail.pickedQuantity = 0.0;
                            detail.isPicked = false;

                            if (!pickingDetailDAO_->create(detail)) { // NEW: Use PickingDetailDAO
                                ERP::Logger::Logger::getInstance().error("PickingService: Failed to create picking detail for product " + detail.productId + ".");
                                return false;
                            }
                            // Reserve inventory for this picking request
                            if (!inventoryManagementService_->reserveInventory(detail.productId, detail.warehouseId, detail.locationId, detail.requestedQuantity, currentUserId, userRoleIds)) {
                                ERP::Logger::Logger::getInstance().error("PickingService: Failed to reserve inventory for product " + detail.productId + ".");
                                return false;
                            }
                        }
                        createdRequest = newRequest;
                        // Optionally, publish event
                        eventBus_.publish(std::make_shared<EventBus::PickingRequestCreatedEvent>(newRequest.id));
                        return true;
                    },
                    "PickingService", "createPickingRequest"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("PickingService: Picking request " + newRequest.requestNumber + " created successfully with " + std::to_string(pickingDetails.size()) + " details.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                        "Warehouse", "PickingRequest", newRequest.id, "PickingRequest", newRequest.requestNumber,
                        std::nullopt, newRequest.toMap(), "Picking request created.");
                    return createdRequest;
                }
                return std::nullopt;
            }

            std::optional<ERP::Warehouse::DTO::PickingRequestDTO> PickingService::getPickingRequestById(
                const std::string& requestId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().debug("PickingService: Retrieving picking request by ID: " + requestId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewPickingRequests", "Bạn không có quyền xem yêu cầu lấy hàng.")) {
                    return std::nullopt;
                }

                std::optional<ERP::Warehouse::DTO::PickingRequestDTO> requestOpt = pickingRequestDAO_->findById(requestId);
                if (requestOpt) {
                    // Load details for the DTO
                    requestOpt->details = pickingDetailDAO_->getPickingDetailsByRequestId(requestOpt->id);
                }
                return requestOpt;
            }

            std::vector<ERP::Warehouse::DTO::PickingRequestDTO> PickingService::getAllPickingRequests(
                const std::map<std::string, std::any>& filter,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("PickingService: Retrieving all picking requests with filter.");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewPickingRequests", "Bạn không có quyền xem tất cả yêu cầu lấy hàng.")) {
                    return {};
                }

                std::vector<ERP::Warehouse::DTO::PickingRequestDTO> requests = pickingRequestDAO_->getPickingRequests(filter);
                // Load details for each request
                for (auto& request : requests) {
                    request.details = pickingDetailDAO_->getPickingDetailsByRequestId(request.id);
                }
                return requests;
            }

            std::vector<ERP::Warehouse::DTO::PickingRequestDTO> PickingService::getPickingRequestsBySalesOrderId(
                const std::string& salesOrderId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("PickingService: Retrieving picking requests for sales order ID: " + salesOrderId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewPickingRequests", "Bạn không có quyền xem yêu cầu lấy hàng theo đơn hàng bán.")) {
                    return {};
                }

                std::map<std::string, std::any> filter;
                filter["sales_order_id"] = salesOrderId;
                std::vector<ERP::Warehouse::DTO::PickingRequestDTO> requests = pickingRequestDAO_->getPickingRequests(filter);
                for (auto& request : requests) {
                    request.details = pickingDetailDAO_->getPickingDetailsByRequestId(request.id);
                }
                return requests;
            }

            bool PickingService::updatePickingRequest(
                const ERP::Warehouse::DTO::PickingRequestDTO& pickingRequestDTO,
                const std::vector<ERP::Warehouse::DTO::PickingDetailDTO>& pickingDetails,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("PickingService: Attempting to update picking request: " + pickingRequestDTO.id + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.UpdatePickingRequest", "Bạn không có quyền cập nhật yêu cầu lấy hàng.")) {
                    return false;
                }

                std::optional<ERP::Warehouse::DTO::PickingRequestDTO> oldRequestOpt = pickingRequestDAO_->findById(pickingRequestDTO.id);
                if (!oldRequestOpt) {
                    ERP::Logger::Logger::getInstance().warning("PickingService: Picking request with ID " + pickingRequestDTO.id + " not found for update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu lấy hàng cần cập nhật.");
                    return false;
                }

                // Validate Sales Order existence if changed
                if (pickingRequestDTO.salesOrderId != oldRequestOpt->salesOrderId) {
                    std::optional<ERP::Sales::DTO::SalesOrderDTO> salesOrder = salesOrderService_->getSalesOrderById(pickingRequestDTO.salesOrderId, userRoleIds);
                    if (!salesOrder || (salesOrder->status != ERP::Sales::DTO::SalesOrderStatus::APPROVED && salesOrder->status != ERP::Sales::DTO::SalesOrderStatus::IN_PROGRESS)) {
                        ERP::Logger::Logger::getInstance().warning("PickingService: Invalid Sales Order ID provided for update or sales order not in valid status: " + pickingRequestDTO.salesOrderId);
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Đơn hàng bán không hợp lệ hoặc không ở trạng thái đủ điều kiện lấy hàng.");
                        return std::nullopt;
                    }
                }
                // Validate Warehouse existence if changed
                if (pickingRequestDTO.warehouseId != oldRequestOpt->warehouseId) {
                    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(pickingRequestDTO.warehouseId, userRoleIds);
                    if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("PickingService: Invalid Warehouse ID provided for update or warehouse is not active: " + pickingRequestDTO.warehouseId);
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc không hoạt động.");
                        return std::nullopt;
                    }
                }

                // Validate details (similar to create, but check existing vs new)
                // This part can get complex if partial updates are allowed for details.
                // For simplicity, this update will assume full replacement of details: delete old, create new.
                // However, if some details are already picked, this needs more sophisticated logic (e.g., prevent deleting/changing picked items).
                // For now, only allow update if request is still PENDING or IN_PROGRESS and no item is fully picked.
                if (oldRequestOpt->status == ERP::Warehouse::DTO::PickingRequestStatus::COMPLETED ||
                    oldRequestOpt->status == ERP::Warehouse::DTO::PickingRequestStatus::CANCELLED) {
                    ERP::Logger::Logger::getInstance().warning("PickingService: Cannot update picking request " + pickingRequestDTO.id + " as it's already " + oldRequestOpt->getStatusString() + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể cập nhật yêu cầu lấy hàng đã hoàn thành hoặc bị hủy.");
                    return false;
                }

                // Validate each new detail
                for (const auto& detail : pickingDetails) {
                    std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(detail.productId, userRoleIds);
                    if (!product || product->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("PickingService: Product " + detail.productId + " not found or not active in new picking detail.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Sản phẩm trong chi tiết lấy hàng không hợp lệ.");
                        return std::nullopt;
                    }
                    std::optional<ERP::Catalog::DTO::LocationDTO> location = warehouseService_->getLocationById(detail.locationId, userRoleIds);
                    if (!location || location->status != ERP::Common::EntityStatus::ACTIVE || location->warehouseId != detail.warehouseId) {
                        ERP::Logger::Logger::getInstance().warning("PickingService: Location " + detail.locationId + " not found or not active or does not belong to warehouse " + detail.warehouseId + " in new picking detail.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Vị trí lấy hàng không hợp lệ.");
                        return std::nullopt;
                    }
                    if (detail.requestedQuantity <= 0) {
                        ERP::Logger::Logger::getInstance().warning("PickingService: Invalid requested quantity in new picking detail for product " + detail.productId);
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng yêu cầu trong chi tiết lấy hàng không hợp lệ.");
                        return std::nullopt;
                    }
                    // If updating a detail with changed quantity, we need to adjust reservations. This is complex.
                    // For simplicity, the current model assumes `recordPickedQuantity` handles inventory.
                    // If `pickingDetails` is a full replacement, need to handle inventory differences here.
                }


                ERP::Warehouse::DTO::PickingRequestDTO updatedRequest = pickingRequestDTO;
                updatedRequest.updatedAt = ERP::Utils::DateUtils::now();
                updatedRequest.updatedBy = currentUserId;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!pickingRequestDAO_->update(updatedRequest)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("PickingService: Failed to update picking request " + updatedRequest.id + " in DAO.");
                            return false;
                        }

                        // Handle details replacement: Remove all old details then add new ones
                        // NOTE: This will NOT automatically unreserve/re-reserve inventory.
                        // A more robust solution would be to compare old and new details,
                        // calculate inventory changes, and create corresponding inventory adjustments
                        // (e.g., unreserve old quantities, reserve new quantities).
                        // For this simpler implementation, we just remove and recreate detail records.
                        // This might lead to inventory discrepancies if not handled carefully at application level.
                        if (!pickingDetailDAO_->removePickingDetailsByRequestId(updatedRequest.id)) { // NEW: Use PickingDetailDAO
                            ERP::Logger::Logger::getInstance().error("PickingService: Failed to remove old picking details for request " + updatedRequest.id + ".");
                            return false;
                        }
                        for (auto detail : pickingDetails) {
                            detail.id = ERP::Utils::generateUUID(); // Assign new ID for new details
                            detail.pickingRequestId = updatedRequest.id;
                            detail.createdAt = updatedRequest.createdAt; // Re-use parent creation time
                            detail.createdBy = updatedRequest.createdBy; // Re-use parent created by
                            detail.status = ERP::Common::EntityStatus::ACTIVE;
                            // Preserve pickedQuantity if updating an existing detail. If it's a new detail, pickedQuantity starts at 0.
                            // This logic needs to be careful about what 'pickingDetails' represents (all items, or just changes).
                            // Assuming it's the *desired final state* of all details for this request.
                            if (!pickingDetailDAO_->create(detail)) { // NEW: Use PickingDetailDAO
                                ERP::Logger::Logger::getInstance().error("PickingService: Failed to create new picking detail for product " + detail.productId + " during update.");
                                return false;
                            }
                            // Re-reserve inventory based on new requested quantities if needed here.
                            // This would involve comparing `oldRequestOpt->details` with `pickingDetails`.
                        }

                        // Optionally, publish event
                        eventBus_.publish(std::make_shared<EventBus::PickingRequestUpdatedEvent>(updatedRequest.id, updatedRequest.requestNumber)); // Assuming such an event exists
                        return true;
                    },
                    "PickingService", "updatePickingRequest"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("PickingService: Picking request " + updatedRequest.id + " updated successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                        "Warehouse", "PickingRequest", updatedRequest.id, "PickingRequest", updatedRequest.requestNumber,
                        oldRequestOpt->toMap(), updatedRequest.toMap(), "Picking request updated.");
                    return true;
                }
                return false;
            }

            bool PickingService::updatePickingRequestStatus(
                const std::string& requestId,
                ERP::Warehouse::DTO::PickingRequestStatus newStatus,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("PickingService: Attempting to update status for picking request: " + requestId + " to " + ERP::Warehouse::DTO::PickingRequestDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.UpdatePickingRequestStatus", "Bạn không có quyền cập nhật trạng thái yêu cầu lấy hàng.")) {
                    return false;
                }

                std::optional<ERP::Warehouse::DTO::PickingRequestDTO> requestOpt = pickingRequestDAO_->findById(requestId);
                if (!requestOpt) {
                    ERP::Logger::Logger::getInstance().warning("PickingService: Picking request with ID " + requestId + " not found for status update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu lấy hàng để cập nhật trạng thái.");
                    return false;
                }

                ERP::Warehouse::DTO::PickingRequestDTO oldRequest = *requestOpt;
                if (oldRequest.status == newStatus) {
                    ERP::Logger::Logger::getInstance().info("PickingService: Picking request " + requestId + " is already in status " + ERP::Warehouse::DTO::PickingRequestDTO().getStatusString(newStatus) + ".");
                    return true; // Already in desired status
                }

                // Add state transition validation logic here
                // For example: Cannot change from COMPLETED to PENDING.

                ERP::Warehouse::DTO::PickingRequestDTO updatedRequest = oldRequest;
                updatedRequest.status = newStatus;
                updatedRequest.updatedAt = ERP::Utils::DateUtils::now();
                updatedRequest.updatedBy = currentUserId;

                // For statuses like COMPLETED/CANCELLED, you might need to adjust inventory (unreserve any remaining).
                // This is handled conceptually by recordPickedQuantity for actual picks, but needs explicit handling here for cancellations or full completions.

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!pickingRequestDAO_->update(updatedRequest)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("PickingService: Failed to update status for picking request " + requestId + " in DAO.");
                            return false;
                        }
                        // Optionally, publish event for status change
                        eventBus_.publish(std::make_shared<EventBus::PickingRequestStatusChangedEvent>(requestId, newStatus));
                        return true;
                    },
                    "PickingService", "updatePickingRequestStatus"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("PickingService: Status for picking request " + requestId + " updated successfully to " + updatedRequest.getStatusString() + ".");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                        "Warehouse", "PickingRequestStatus", requestId, "PickingRequest", oldRequest.requestNumber,
                        oldRequest.toMap(), updatedRequest.toMap(), "Picking request status changed to " + updatedRequest.getStatusString() + ".");
                    return true;
                }
                return false;
            }

            bool PickingService::deletePickingRequest(
                const std::string& requestId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("PickingService: Attempting to delete picking request: " + requestId + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.DeletePickingRequest", "Bạn không có quyền xóa yêu cầu lấy hàng.")) {
                    return false;
                }

                std::optional<ERP::Warehouse::DTO::PickingRequestDTO> requestOpt = pickingRequestDAO_->findById(requestId);
                if (!requestOpt) {
                    ERP::Logger::Logger::getInstance().warning("PickingService: Picking request with ID " + requestId + " not found for deletion.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu lấy hàng cần xóa.");
                    return false;
                }

                ERP::Warehouse::DTO::PickingRequestDTO requestToDelete = *requestOpt;

                // Additional checks: Prevent deletion if request is completed or has issued inventory transactions
                if (requestToDelete.status == ERP::Warehouse::DTO::PickingRequestStatus::COMPLETED) {
                    ERP::Logger::Logger::getInstance().warning("PickingService: Cannot delete completed picking request " + requestId + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa yêu cầu lấy hàng đã hoàn thành.");
                    return false;
                }

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        // Remove associated details first.
                        // Note: This does not unreserve inventory for unpicked items. That must be handled separately if needed.
                        if (!pickingDetailDAO_->removePickingDetailsByRequestId(requestId)) { // NEW: Use PickingDetailDAO
                            ERP::Logger::Logger::getInstance().error("PickingService: Failed to remove associated picking details for request " + requestId + ".");
                            return false;
                        }
                        if (!pickingRequestDAO_->remove(requestId)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("PickingService: Failed to delete picking request " + requestId + " in DAO.");
                            return false;
                        }
                        return true;
                    },
                    "PickingService", "deletePickingRequest"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("PickingService: Picking request " + requestId + " deleted successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                        "Warehouse", "PickingRequest", requestId, "PickingRequest", requestToDelete.requestNumber,
                        requestToDelete.toMap(), std::nullopt, "Picking request deleted.");
                    return true;
                }
                return false;
            }

            std::optional<ERP::Warehouse::DTO::PickingDetailDTO> PickingService::getPickingDetailById(
                const std::string& detailId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().debug("PickingService: Retrieving picking detail by ID: " + detailId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewPickingRequests", "Bạn không có quyền xem chi tiết yêu cầu lấy hàng.")) { // Reusing main view perm
                    return std::nullopt;
                }

                return pickingDetailDAO_->findById(detailId); // Specific DAO method
            }

            std::vector<ERP::Warehouse::DTO::PickingDetailDTO> PickingService::getPickingDetails(
                const std::string& requestId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("PickingService: Retrieving picking details for request ID: " + requestId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewPickingRequests", "Bạn không có quyền xem chi tiết yêu cầu lấy hàng.")) {
                    return {};
                }

                // Validate parent Picking Request existence
                if (!pickingRequestDAO_->findById(requestId)) {
                    ERP::Logger::Logger::getInstance().warning("PickingService: Picking Request " + requestId + " not found when getting details.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Yêu cầu lấy hàng không tồn tại.");
                    return {};
                }

                return pickingDetailDAO_->getPickingDetailsByRequestId(requestId); // Specific DAO method
            }

            bool PickingService::recordPickedQuantity(
                const std::string& detailId,
                double pickedQuantity,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("PickingService: Attempting to record picked quantity for detail: " + detailId + ", quantity: " + std::to_string(pickedQuantity) + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.RecordPickedQuantity", "Bạn không có quyền ghi nhận số lượng đã lấy.")) {
                    return false;
                }

                std::optional<ERP::Warehouse::DTO::PickingDetailDTO> detailOpt = pickingDetailDAO_->findById(detailId);
                if (!detailOpt) {
                    ERP::Logger::Logger::getInstance().warning("PickingService: Picking detail with ID " + detailId + " not found for recording picked quantity.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy chi tiết lấy hàng để ghi nhận số lượng.");
                    return false;
                }

                ERP::Warehouse::DTO::PickingDetailDTO oldDetail = *detailOpt;
                if (oldDetail.isPicked || oldDetail.pickedQuantity >= oldDetail.requestedQuantity) {
                    ERP::Logger::Logger::getInstance().warning("PickingService: Picking detail " + detailId + " already fully picked or marked as picked.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Chi tiết lấy hàng đã được lấy đủ hoặc đã hoàn thành.");
                    return true; // Consider as success if already picked
                }

                if (pickedQuantity <= 0) {
                    ERP::Logger::Logger::getInstance().warning("PickingService: Picked quantity must be positive for detail " + detailId + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng đã lấy phải là số dương.");
                    return false;
                }
                if (oldDetail.pickedQuantity + pickedQuantity > oldDetail.requestedQuantity) {
                    ERP::Logger::Logger::getInstance().warning("PickingService: Picked quantity exceeds requested quantity for detail " + detailId + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng đã lấy vượt quá số lượng yêu cầu.");
                    return false;
                }

                // Get current inventory to check available quantity
                std::optional<ERP::Warehouse::DTO::InventoryDTO> inventory = inventoryManagementService_->getInventoryByProductLocation(oldDetail.productId, oldDetail.warehouseId, oldDetail.locationId, userRoleIds);
                if (!inventory || inventory->availableQuantity.value_or(0.0) < pickedQuantity) {
                    ERP::Logger::Logger::getInstance().warning("PickingService: Insufficient available inventory for picking product " + oldDetail.productId + " at " + oldDetail.warehouseId + "/" + oldDetail.locationId + ". Available: " + std::to_string(inventory->availableQuantity.value_or(0.0)) + ", Trying to pick: " + std::to_string(pickedQuantity));
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InsufficientStock, "Không đủ tồn kho khả dụng để lấy hàng.");
                    return false;
                }

                ERP::Warehouse::DTO::PickingDetailDTO updatedDetail = oldDetail;
                updatedDetail.pickedQuantity += pickedQuantity;
                updatedDetail.updatedAt = ERP::Utils::DateUtils::now();
                updatedDetail.updatedBy = currentUserId;

                if (updatedDetail.pickedQuantity >= updatedDetail.requestedQuantity) {
                    updatedDetail.isPicked = true; // Mark as fully picked
                }

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        // Update PickingDetail
                        if (!pickingDetailDAO_->update(updatedDetail)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("PickingService: Failed to update picking detail " + detailId + " in DAO.");
                            return false;
                        }

                        // Create InventoryTransaction (Goods Issue)
                        ERP::Warehouse::DTO::InventoryTransactionDTO invTxn;
                        invTxn.id = ERP::Utils::generateUUID();
                        invTxn.productId = updatedDetail.productId;
                        invTxn.warehouseId = updatedDetail.warehouseId;
                        invTxn.locationId = updatedDetail.locationId;
                        invTxn.type = ERP::Warehouse::DTO::InventoryTransactionType::GOODS_ISSUE;
                        invTxn.quantity = pickedQuantity; // Amount picked
                        // Unit cost can be retrieved from inventory costing layers if available.
                        // For simplicity, take current average cost or last receipt cost.
                        invTxn.unitCost = inventory->unitCost;
                        invTxn.transactionDate = ERP::Utils::DateUtils::now();
                        invTxn.referenceDocumentId = updatedDetail.pickingRequestId; // Link to picking request
                        invTxn.referenceDocumentType = "PickingRequest";
                        invTxn.notes = "Goods issue for Picking Request " + updatedDetail.pickingRequestId + " (Detail: " + detailId + ")";
                        invTxn.status = ERP::Common::EntityStatus::ACTIVE;
                        invTxn.createdAt = ERP::Utils::DateUtils::now();
                        invTxn.createdBy = currentUserId;

                        if (!inventoryManagementService_->recordGoodsIssue(invTxn, currentUserId, userRoleIds)) {
                            ERP::Logger::Logger::getInstance().error("PickingService: Failed to record goods issue transaction for picking detail " + detailId + ".");
                            return false;
                        }
                        // Update the picking detail with the inventory transaction ID
                        updatedDetail.inventoryTransactionId = invTxn.id;
                        if (!pickingDetailDAO_->update(updatedDetail)) { // Update again with transaction ID
                            ERP::Logger::Logger::getInstance().error("PickingService: Failed to update picking detail with inventory transaction ID.");
                            return false;
                        }

                        // Update parent PickingRequest status if all details are fully picked
                        std::vector<ERP::Warehouse::DTO::PickingDetailDTO> allDetails = pickingDetailDAO_->getPickingDetailsByRequestId(oldDetail.pickingRequestId);
                        bool allFullyPicked = std::all_of(allDetails.begin(), allDetails.end(), [](const auto& d) {
                            return d.isPicked;
                            });

                        if (allFullyPicked) {
                            ERP::Logger::Logger::getInstance().info("PickingService: All items for picking request " + oldDetail.pickingRequestId + " are now fully picked. Updating status to COMPLETED.");
                            updatePickingRequestStatus(oldDetail.pickingRequestId, ERP::Warehouse::DTO::PickingRequestStatus::COMPLETED, currentUserId, userRoleIds);
                        }
                        else {
                            // If some items are picked but not all, set to PARTIALLY_PICKED (if not already IN_PROGRESS)
                            std::optional<ERP::Warehouse::DTO::PickingRequestDTO> parentRequest = pickingRequestDAO_->findById(oldDetail.pickingRequestId);
                            if (parentRequest && parentRequest->status == ERP::Warehouse::DTO::PickingRequestStatus::PENDING) {
                                updatePickingRequestStatus(oldDetail.pickingRequestId, ERP::Warehouse::DTO::PickingRequestStatus::IN_PROGRESS, currentUserId, userRoleIds);
                            }
                        }
                        // Also update sales order's delivered quantity if linked
                        if (updatedDetail.salesOrderDetailId) {
                            // This would require SalesOrderService to have a method like updateSalesOrderDetailDeliveredQuantity
                            // Omitted for now, but conceptual link exists.
                        }
                        return true;
                    },
                    "PickingService", "recordPickedQuantity"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("PickingService: Picked quantity for detail " + detailId + " recorded successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::PROCESS_END, ERP::Common::LogSeverity::INFO, // Could be specialized activity type
                        "Warehouse", "RecordPickedQuantity", detailId, "PickingDetail", updatedDetail.productId,
                        oldDetail.toMap(), updatedDetail.toMap(), "Picked quantity recorded.");
                    return true;
                }
                return false;
            }

        } // namespace Services
    } // namespace Warehouse
} // namespace ERP