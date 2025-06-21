// Modules/Warehouse/Service/StocktakeService.cpp
#include "StocktakeService.h" // Standard includes
#include "StocktakeRequest.h"       // StocktakeRequest DTO
#include "StocktakeDetail.h"        // StocktakeDetail DTO
#include "Product.h"                // Product DTO
#include "Warehouse.h"              // Warehouse DTO
#include "Location.h"               // Location DTO
#include "InventoryTransaction.h"   // InventoryTransaction DTO
#include "Event.h"                  // Event DTO
#include "ConnectionPool.h"         // ConnectionPool
#include "DBConnection.h"           // DBConnection
#include "Common.h"                 // Common Enums/Constants
#include "Utils.h"                  // Utility functions
#include "DateUtils.h"              // Date utility functions
#include "AutoRelease.h"            // RAII helper
#include "ISecurityManager.h"       // Security Manager interface
#include "UserService.h"            // User Service (for audit logging)
#include "InventoryManagementService.h" // InventoryManagementService (for adjustments)
#include "WarehouseService.h"       // WarehouseService (for warehouse/location validation)
#include "ProductService.h"         // ProductService (for product validation)

#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed

namespace ERP {
    namespace Warehouse {
        namespace Services {

            StocktakeService::StocktakeService(
                std::shared_ptr<DAOs::StocktakeRequestDAO> stocktakeRequestDAO,
                std::shared_ptr<DAOs::StocktakeDetailDAO> stocktakeDetailDAO, // NEW
                std::shared_ptr<ERP::Warehouse::Services::IInventoryManagementService> inventoryManagementService,
                std::shared_ptr<ERP::Catalog::Services::IWarehouseService> warehouseService,
                std::shared_ptr<ERP::Product::Services::IProductService> productService,
                std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
                : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Initialize BaseService
                stocktakeRequestDAO_(stocktakeRequestDAO),
                stocktakeDetailDAO_(stocktakeDetailDAO), // NEW
                inventoryManagementService_(inventoryManagementService),
                warehouseService_(warehouseService),
                productService_(productService) {

                if (!stocktakeRequestDAO_ || !stocktakeDetailDAO_ || !inventoryManagementService_ || !warehouseService_ || !productService_ || !securityManager_) { // BaseService checks its own dependencies
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "StocktakeService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ kiểm kê.");
                    ERP::Logger::Logger::getInstance().critical("StocktakeService: One or more injected DAOs/Services are null.");
                    throw std::runtime_error("StocktakeService: Null dependencies.");
                }
                ERP::Logger::Logger::getInstance().info("StocktakeService: Initialized.");
            }

            std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> StocktakeService::createStocktakeRequest(
                const ERP::Warehouse::DTO::StocktakeRequestDTO& stocktakeRequestDTO,
                const std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO>& stocktakeDetails,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("StocktakeService: Attempting to create stocktake request for warehouse: " + stocktakeRequestDTO.warehouseId + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.CreateStocktake", "Bạn không có quyền tạo yêu cầu kiểm kê.")) {
                    return std::nullopt;
                }

                // 1. Validate input DTO
                if (stocktakeRequestDTO.warehouseId.empty() || stocktakeRequestDTO.countDate == std::chrono::system_clock::time_point() || stocktakeRequestDTO.status == ERP::Warehouse::DTO::StocktakeRequestStatus::UNKNOWN) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Invalid input for request creation (missing warehouse ID, count date, or unknown status).");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin yêu cầu kiểm kê không đầy đủ.");
                    return std::nullopt;
                }

                // Validate Warehouse existence
                std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(stocktakeRequestDTO.warehouseId, userRoleIds);
                if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Invalid Warehouse ID provided or warehouse is not active: " + stocktakeRequestDTO.warehouseId);
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc không hoạt động.");
                    return std::nullopt;
                }
                // Validate Location existence if provided
                if (stocktakeRequestDTO.locationId) {
                    std::optional<ERP::Catalog::DTO::LocationDTO> location = warehouseService_->getLocationById(*stocktakeRequestDTO.locationId, userRoleIds);
                    if (!location || location->status != ERP::Common::EntityStatus::ACTIVE || location->warehouseId != stocktakeRequestDTO.warehouseId) {
                        ERP::Logger::Logger::getInstance().warning("StocktakeService: Invalid Location ID provided or location is not active or does not belong to warehouse " + stocktakeRequestDTO.warehouseId + ".");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID vị trí không hợp lệ hoặc không hoạt động.");
                        return std::nullopt;
                    }
                }
                // Validate user existence for requestedBy/countedBy
                if (!securityManager_->getUserService()->getUserById(stocktakeRequestDTO.requestedByUserId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Requested by user " + stocktakeRequestDTO.requestedByUserId + " not found.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Người yêu cầu không tồn tại.");
                    return std::nullopt;
                }
                if (stocktakeRequestDTO.countedByUserId && !securityManager_->getUserService()->getUserById(*stocktakeRequestDTO.countedByUserId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Counted by user " + *stocktakeRequestDTO.countedByUserId + " not found.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Người kiểm kê không tồn tại.");
                    return std::nullopt;
                }

                ERP::Warehouse::DTO::StocktakeRequestDTO newRequest = stocktakeRequestDTO;
                newRequest.id = ERP::Utils::generateUUID();
                newRequest.createdAt = ERP::Utils::DateUtils::now();
                newRequest.createdBy = currentUserId;
                newRequest.status = ERP::Warehouse::DTO::StocktakeRequestStatus::PENDING; // Always pending on creation
                // newRequest.countDate is assumed to be provided by DTO

                std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> createdRequest = std::nullopt;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!stocktakeRequestDAO_->create(newRequest)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("StocktakeService: Failed to create stocktake request in DAO.");
                            return false;
                        }
                        // If details are provided at creation, save them.
                        // Also, capture system quantities for these details.
                        for (auto detail : stocktakeDetails) {
                            detail.id = ERP::Utils::generateUUID();
                            detail.stocktakeRequestId = newRequest.id;
                            detail.createdAt = newRequest.createdAt;
                            detail.createdBy = newRequest.createdBy;
                            detail.status = ERP::Common::EntityStatus::ACTIVE; // Details are active
                            // Capture system quantity
                            std::optional<ERP::Warehouse::DTO::InventoryDTO> currentInv = inventoryManagementService_->getInventoryByProductLocation(detail.productId, detail.warehouseId, detail.locationId, userRoleIds);
                            detail.systemQuantity = currentInv ? currentInv->quantity : 0.0;
                            // Initially, countedQuantity is typically 0, or user provided it.
                            // difference is calculated on reconciliation.
                            detail.difference = detail.systemQuantity - detail.countedQuantity; // Calculate initial difference

                            if (!stocktakeDetailDAO_->create(detail)) { // NEW: Use StocktakeDetailDAO
                                ERP::Logger::Logger::getInstance().error("StocktakeService: Failed to create stocktake detail for product " + detail.productId + ".");
                                return false;
                            }
                        }
                        createdRequest = newRequest;
                        // Optionally, publish event
                        // eventBus_.publish(std::make_shared<EventBus::StocktakeRequestCreatedEvent>(newRequest.id, newRequest.warehouseId)); // Assuming such an event
                        return true;
                    },
                    "StocktakeService", "createStocktakeRequest"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("StocktakeService: Stocktake request " + newRequest.id + " created successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                        "Warehouse", "StocktakeRequest", newRequest.id, "StocktakeRequest", newRequest.warehouseId + "/" + newRequest.locationId.value_or("All"),
                        std::nullopt, newRequest.toMap(), "Stocktake request created.");
                    return createdRequest;
                }
                return std::nullopt;
            }

            std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> StocktakeService::getStocktakeRequestById(
                const std::string& requestId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().debug("StocktakeService: Retrieving stocktake request by ID: " + requestId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewStocktakes", "Bạn không có quyền xem yêu cầu kiểm kê.")) {
                    return std::nullopt;
                }

                std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> requestOpt = stocktakeRequestDAO_->findById(requestId);
                if (requestOpt) {
                    // Load details for the DTO
                    requestOpt->details = stocktakeDetailDAO_->getStocktakeDetailsByRequestId(requestOpt->id);
                }
                return requestOpt;
            }

            std::vector<ERP::Warehouse::DTO::StocktakeRequestDTO> StocktakeService::getAllStocktakeRequests(
                const std::map<std::string, std::any>& filter,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("StocktakeService: Retrieving all stocktake requests with filter.");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewStocktakes", "Bạn không có quyền xem tất cả yêu cầu kiểm kê.")) {
                    return {};
                }

                std::vector<ERP::Warehouse::DTO::StocktakeRequestDTO> requests = stocktakeRequestDAO_->getStocktakeRequests(filter);
                // Load details for each request
                for (auto& request : requests) {
                    request.details = stocktakeDetailDAO_->getStocktakeDetailsByRequestId(request.id);
                }
                return requests;
            }

            std::vector<ERP::Warehouse::DTO::StocktakeRequestDTO> StocktakeService::getStocktakeRequestsByWarehouseLocation(
                const std::string& warehouseId,
                const std::optional<std::string>& locationId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("StocktakeService: Retrieving stocktake requests for warehouse: " + warehouseId + (locationId ? (" and location: " + *locationId) : "") + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewStocktakes", "Bạn không có quyền xem yêu cầu kiểm kê theo kho/vị trí.")) {
                    return {};
                }

                std::map<std::string, std::any> filter;
                filter["warehouse_id"] = warehouseId;
                if (locationId) {
                    filter["location_id"] = *locationId;
                }
                std::vector<ERP::Warehouse::DTO::StocktakeRequestDTO> requests = stocktakeRequestDAO_->getStocktakeRequests(filter);
                for (auto& request : requests) {
                    request.details = stocktakeDetailDAO_->getStocktakeDetailsByRequestId(request.id);
                }
                return requests;
            }

            bool StocktakeService::updateStocktakeRequest(
                const ERP::Warehouse::DTO::StocktakeRequestDTO& stocktakeRequestDTO,
                const std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO>& stocktakeDetails,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("StocktakeService: Attempting to update stocktake request: " + stocktakeRequestDTO.id + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.UpdateStocktake", "Bạn không có quyền cập nhật yêu cầu kiểm kê.")) {
                    return false;
                }

                std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> oldRequestOpt = stocktakeRequestDAO_->findById(stocktakeRequestDTO.id);
                if (!oldRequestOpt) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Stocktake request with ID " + stocktakeRequestDTO.id + " not found for update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu kiểm kê cần cập nhật.");
                    return false;
                }

                // Prevent update if already reconciled or cancelled
                if (oldRequestOpt->status == ERP::Warehouse::DTO::StocktakeRequestStatus::RECONCILED ||
                    oldRequestOpt->status == ERP::Warehouse::DTO::StocktakeRequestStatus::COMPLETED ||
                    oldRequestOpt->status == ERP::Warehouse::DTO::StocktakeRequestStatus::CANCELLED) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Cannot update stocktake request " + stocktakeRequestDTO.id + " as it's already " + oldRequestOpt->getStatusString() + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể cập nhật yêu cầu kiểm kê đã hoàn thành hoặc bị hủy.");
                    return false;
                }

                // Validate Warehouse existence if changed
                if (stocktakeRequestDTO.warehouseId != oldRequestOpt->warehouseId) {
                    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(stocktakeRequestDTO.warehouseId, userRoleIds);
                    if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("StocktakeService: Invalid Warehouse ID provided for update or warehouse is not active: " + stocktakeRequestDTO.warehouseId);
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID kho hàng không hợp lệ hoặc không hoạt động.");
                        return std::nullopt;
                    }
                }
                // Validate Location existence if changed
                if (stocktakeRequestDTO.locationId != oldRequestOpt->locationId) {
                    if (stocktakeRequestDTO.locationId) {
                        std::optional<ERP::Catalog::DTO::LocationDTO> location = warehouseService_->getLocationById(*stocktakeRequestDTO.locationId, userRoleIds);
                        if (!location || location->status != ERP::Common::EntityStatus::ACTIVE || location->warehouseId != stocktakeRequestDTO.warehouseId) {
                            ERP::Logger::Logger::getInstance().warning("StocktakeService: Invalid Location ID provided for update or location is not active or does not belong to warehouse " + stocktakeRequestDTO.warehouseId + ".");
                            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID vị trí không hợp lệ hoặc không hoạt động.");
                            return std::nullopt;
                        }
                    }
                }
                // Validate user existence for requestedBy/countedBy
                if (stocktakeRequestDTO.requestedByUserId != oldRequestOpt->requestedByUserId && !securityManager_->getUserService()->getUserById(stocktakeRequestDTO.requestedByUserId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Requested by user " + stocktakeRequestDTO.requestedByUserId + " not found.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Người yêu cầu không tồn tại.");
                    return std::nullopt;
                }
                if (stocktakeRequestDTO.countedByUserId && (oldRequestOpt->countedByUserId != stocktakeRequestDTO.countedByUserId) && !securityManager_->getUserService()->getUserById(*stocktakeRequestDTO.countedByUserId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Counted by user " + *stocktakeRequestDTO.countedByUserId + " not found.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Người kiểm kê không tồn tại.");
                    return std::nullopt;
                }

                // Validate details: Product existence, quantities, consistency
                for (const auto& detail : stocktakeDetails) {
                    std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(detail.productId, userRoleIds);
                    if (!product || product->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("StocktakeService: Product " + detail.productId + " not found or not active in stocktake detail.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Sản phẩm trong chi tiết kiểm kê không hợp lệ.");
                        return std::nullopt;
                    }
                    // Quantity validation
                    if (detail.countedQuantity < 0) {
                        ERP::Logger::Logger::getInstance().warning("StocktakeService: Counted quantity must be non-negative for product " + detail.productId);
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng đã đếm không hợp lệ.");
                        return std::nullopt;
                    }
                }

                ERP::Warehouse::DTO::StocktakeRequestDTO updatedRequest = stocktakeRequestDTO;
                updatedRequest.updatedAt = ERP::Utils::DateUtils::now();
                updatedRequest.updatedBy = currentUserId;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!stocktakeRequestDAO_->update(updatedRequest)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("StocktakeService: Failed to update stocktake request " + updatedRequest.id + " in DAO.");
                            return false;
                        }

                        // Handle details replacement: Remove all old details then add new ones
                        // This is a full replacement strategy for simplicity.
                        if (!stocktakeDetailDAO_->removeStocktakeDetailsByRequestId(updatedRequest.id)) { // NEW: Use StocktakeDetailDAO
                            ERP::Logger::Logger::getInstance().error("StocktakeService: Failed to remove old stocktake details for request " + updatedRequest.id + ".");
                            return false;
                        }
                        for (auto detail : stocktakeDetails) {
                            detail.id = ERP::Utils::generateUUID(); // Assign new ID for new details
                            detail.stocktakeRequestId = updatedRequest.id;
                            detail.createdAt = updatedRequest.createdAt; // Re-use parent creation time
                            detail.createdBy = updatedRequest.createdBy; // Re-use parent created by
                            detail.status = ERP::Common::EntityStatus::ACTIVE;
                            // Recalculate difference for new details if system_quantity isn't explicitly passed for new
                            // For existing details, `detail.systemQuantity` should be loaded from DB.
                            // Here, we assume `detail.systemQuantity` is correctly populated by the caller if it's an existing item.
                            // If it's a new item, we might need to fetch `systemQuantity` from current inventory here.
                            if (detail.systemQuantity == 0.0 && !stocktakeDetails.empty()) { // If system quantity not provided, fetch from current inventory
                                std::optional<ERP::Warehouse::DTO::InventoryDTO> currentInv = inventoryManagementService_->getInventoryByProductLocation(detail.productId, detail.warehouseId, detail.locationId, userRoleIds);
                                detail.systemQuantity = currentInv ? currentInv->quantity : 0.0;
                            }
                            detail.difference = detail.systemQuantity - detail.countedQuantity;

                            if (!stocktakeDetailDAO_->create(detail)) { // NEW: Use StocktakeDetailDAO
                                ERP::Logger::Logger::getInstance().error("StocktakeService: Failed to create new stocktake detail for product " + detail.productId + " during update.");
                                return false;
                            }
                        }

                        // Optionally, publish event
                        // eventBus_.publish(std::make_shared<EventBus::StocktakeRequestUpdatedEvent>(updatedRequest.id, updatedRequest.warehouseId)); // Assuming such an event
                        return true;
                    },
                    "StocktakeService", "updateStocktakeRequest"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("StocktakeService: Stocktake request " + updatedRequest.id + " updated successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                        "Warehouse", "StocktakeRequest", updatedRequest.id, "StocktakeRequest", updatedRequest.warehouseId + "/" + updatedRequest.locationId.value_or("All"),
                        oldRequestOpt->toMap(), updatedRequest.toMap(), "Stocktake request updated.");
                    return true;
                }
                return false;
            }

            bool StocktakeService::updateStocktakeRequestStatus(
                const std::string& requestId,
                ERP::Warehouse::DTO::StocktakeRequestStatus newStatus,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("StocktakeService: Attempting to update status for stocktake request: " + requestId + " to " + ERP::Warehouse::DTO::StocktakeRequestDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.UpdateStocktakeStatus", "Bạn không có quyền cập nhật trạng thái yêu cầu kiểm kê.")) {
                    return false;
                }

                std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> requestOpt = stocktakeRequestDAO_->findById(requestId);
                if (!requestOpt) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Stocktake request with ID " + requestId + " not found for status update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu kiểm kê để cập nhật trạng thái.");
                    return false;
                }

                ERP::Warehouse::DTO::StocktakeRequestDTO oldRequest = *requestOpt;
                if (oldRequest.status == newStatus) {
                    ERP::Logger::Logger::getInstance().info("StocktakeService: Stocktake request " + requestId + " is already in status " + ERP::Warehouse::DTO::StocktakeRequestDTO().getStatusString(newStatus) + ".");
                    return true; // Already in desired status
                }

                // Add state transition validation logic here
                // For example: Cannot change from COMPLETED to PENDING.

                ERP::Warehouse::DTO::StocktakeRequestDTO updatedRequest = oldRequest;
                updatedRequest.status = newStatus;
                updatedRequest.updatedAt = ERP::Utils::DateUtils::now();
                updatedRequest.updatedBy = currentUserId;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!stocktakeRequestDAO_->update(updatedRequest)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("StocktakeService: Failed to update status for stocktake request " + requestId + " in DAO.");
                            return false;
                        }
                        // Optionally, publish event for status change
                        // eventBus_.publish(std::make_shared<EventBus::StocktakeRequestStatusChangedEvent>(requestId, newStatus)); // Assuming such an event
                        return true;
                    },
                    "StocktakeService", "updateStocktakeRequestStatus"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("StocktakeService: Status for stocktake request " + requestId + " updated successfully to " + updatedRequest.getStatusString() + ".");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                        "Warehouse", "StocktakeRequestStatus", requestId, "StocktakeRequest", oldRequest.warehouseId + "/" + oldRequest.locationId.value_or("All"),
                        oldRequest.toMap(), updatedRequest.toMap(), "Stocktake request status changed to " + updatedRequest.getStatusString() + ".");
                    return true;
                }
                return false;
            }

            bool StocktakeService::deleteStocktakeRequest(
                const std::string& requestId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("StocktakeService: Attempting to delete stocktake request: " + requestId + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.DeleteStocktake", "Bạn không có quyền xóa yêu cầu kiểm kê.")) {
                    return false;
                }

                std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> requestOpt = stocktakeRequestDAO_->findById(requestId);
                if (!requestOpt) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Stocktake request with ID " + requestId + " not found for deletion.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu kiểm kê cần xóa.");
                    return false;
                }

                ERP::Warehouse::DTO::StocktakeRequestDTO requestToDelete = *requestOpt;

                // Additional checks: Prevent deletion if request has been reconciled or has pending adjustments
                if (requestToDelete.status == ERP::Warehouse::DTO::StocktakeRequestStatus::RECONCILED ||
                    requestToDelete.status == ERP::Warehouse::DTO::StocktakeRequestStatus::COMPLETED) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Cannot delete reconciled or completed stocktake request " + requestId + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa yêu cầu kiểm kê đã đối chiếu hoặc hoàn thành.");
                    return false;
                }

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        // Remove associated details first
                        if (!stocktakeDetailDAO_->removeStocktakeDetailsByRequestId(requestId)) { // NEW: Use StocktakeDetailDAO
                            ERP::Logger::Logger::getInstance().error("StocktakeService: Failed to remove associated stocktake details for request " + requestId + ".");
                            return false;
                        }
                        if (!stocktakeRequestDAO_->remove(requestId)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("StocktakeService: Failed to delete stocktake request " + requestId + " in DAO.");
                            return false;
                        }
                        return true;
                    },
                    "StocktakeService", "deleteStocktakeRequest"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("StocktakeService: Stocktake request " + requestId + " deleted successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                        "Warehouse", "StocktakeRequest", requestId, "StocktakeRequest", requestToDelete.warehouseId + "/" + requestToDelete.locationId.value_or("All"),
                        requestToDelete.toMap(), std::nullopt, "Stocktake request deleted.");
                    return true;
                }
                return false;
            }

            std::optional<ERP::Warehouse::DTO::StocktakeDetailDTO> StocktakeService::getStocktakeDetailById(
                const std::string& detailId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().debug("StocktakeService: Retrieving stocktake detail by ID: " + detailId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewStocktakes", "Bạn không có quyền xem chi tiết kiểm kê.")) { // Reusing main view perm
                    return std::nullopt;
                }

                return stocktakeDetailDAO_->findById(detailId); // Specific DAO method
            }

            std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> StocktakeService::getStocktakeDetails(
                const std::string& requestId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("StocktakeService: Retrieving stocktake details for request ID: " + requestId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ViewStocktakes", "Bạn không có quyền xem chi tiết kiểm kê.")) {
                    return {};
                }

                // Validate parent Stocktake Request existence
                if (!stocktakeRequestDAO_->findById(requestId)) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Stocktake Request " + requestId + " not found when getting details.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Yêu cầu kiểm kê không tồn tại.");
                    return {};
                }

                return stocktakeDetailDAO_->getStocktakeDetailsByRequestId(requestId); // Specific DAO method
            }

            bool StocktakeService::recordCountedQuantity(
                const std::string& detailId,
                double countedQuantity,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("StocktakeService: Attempting to record counted quantity for detail: " + detailId + ", quantity: " + std::to_string(countedQuantity) + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.RecordCountedQuantity", "Bạn không có quyền ghi nhận số lượng đã đếm.")) {
                    return false;
                }

                std::optional<ERP::Warehouse::DTO::StocktakeDetailDTO> detailOpt = stocktakeDetailDAO_->findById(detailId);
                if (!detailOpt) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Stocktake detail with ID " + detailId + " not found for recording counted quantity.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy chi tiết kiểm kê để ghi nhận số lượng.");
                    return false;
                }

                ERP::Warehouse::DTO::StocktakeDetailDTO oldDetail = *detailOpt;
                // Prevent updating counted quantity if stocktake is already reconciled or completed
                std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> parentRequest = stocktakeRequestDAO_->findById(oldDetail.stocktakeRequestId);
                if (parentRequest && (parentRequest->status == ERP::Warehouse::DTO::StocktakeRequestStatus::RECONCILED ||
                    parentRequest->status == ERP::Warehouse::DTO::StocktakeRequestStatus::COMPLETED ||
                    parentRequest->status == ERP::Warehouse::DTO::StocktakeRequestStatus::CANCELLED)) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Cannot record counted quantity for detail " + detailId + " as parent stocktake is " + parentRequest->getStatusString() + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể ghi nhận số lượng đã đếm khi yêu cầu kiểm kê đã hoàn thành.");
                    return false;
                }

                if (countedQuantity < 0) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Counted quantity must be non-negative for detail " + detailId + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng đã đếm phải là số không âm.");
                    return false;
                }

                ERP::Warehouse::DTO::StocktakeDetailDTO updatedDetail = oldDetail;
                updatedDetail.countedQuantity = countedQuantity; // Overwrite with new counted quantity
                updatedDetail.difference = updatedDetail.systemQuantity - updatedDetail.countedQuantity; // Recalculate difference
                updatedDetail.updatedAt = ERP::Utils::DateUtils::now();
                updatedDetail.updatedBy = currentUserId;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!stocktakeDetailDAO_->update(updatedDetail)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("StocktakeService: Failed to update stocktake detail " + detailId + " in DAO.");
                            return false;
                        }

                        // Update parent StocktakeRequest status if needed (e.g., from PENDING to IN_PROGRESS or COUNTED)
                        if (parentRequest) {
                            if (parentRequest->status == ERP::Warehouse::DTO::StocktakeRequestStatus::PENDING) {
                                updateStocktakeRequestStatus(parentRequest->id, ERP::Warehouse::DTO::StocktakeRequestStatus::IN_PROGRESS, currentUserId, userRoleIds);
                            }
                            // Check if all details have been counted (i.e., countedQuantity > 0 or actively set by user)
                            std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> allDetails = stocktakeDetailDAO_->getStocktakeDetailsByRequestId(parentRequest->id);
                            bool allCounted = true;
                            for (const auto& d : allDetails) {
                                // Assuming a detail is "counted" if its countedQuantity is non-zero
                                // or if it was created with a specific value. A more robust flag 'is_counted' might be needed.
                                // For now, let's assume 'countedQuantity > 0' implies it has been counted.
                                if (d.countedQuantity == 0.0) { // Simple check, might need refine
                                    allCounted = false;
                                    break;
                                }
                            }
                            if (allCounted && parentRequest->status != ERP::Warehouse::DTO::StocktakeRequestStatus::COUNTED) {
                                updateStocktakeRequestStatus(parentRequest->id, ERP::Warehouse::DTO::StocktakeRequestStatus::COUNTED, currentUserId, userRoleIds);
                            }
                        }
                        return true;
                    },
                    "StocktakeService", "recordCountedQuantity"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("StocktakeService: Counted quantity for detail " + detailId + " recorded successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::PROCESS_END, ERP::Common::LogSeverity::INFO, // Could be specialized activity type
                        "Warehouse", "RecordCountedQuantity", detailId, "StocktakeDetail", updatedDetail.productId,
                        oldDetail.toMap(), updatedDetail.toMap(), "Counted quantity recorded.");
                    return true;
                }
                return false;
            }

            bool StocktakeService::reconcileStocktake(
                const std::string& requestId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("StocktakeService: Attempting to reconcile stocktake request: " + requestId + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Warehouse.ReconcileStocktake", "Bạn không có quyền đối chiếu kiểm kê.")) {
                    return false;
                }

                std::optional<ERP::Warehouse::DTO::StocktakeRequestDTO> requestOpt = stocktakeRequestDAO_->findById(requestId);
                if (!requestOpt) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Stocktake request with ID " + requestId + " not found for reconciliation.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu kiểm kê để đối chiếu.");
                    return false;
                }

                ERP::Warehouse::DTO::StocktakeRequestDTO stocktakeRequest = *requestOpt;
                if (stocktakeRequest.status != ERP::Warehouse::DTO::StocktakeRequestStatus::COUNTED) {
                    ERP::Logger::Logger::getInstance().warning("StocktakeService: Stocktake request " + requestId + " is not in COUNTED status. Current status: " + stocktakeRequest.getStatusString());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Yêu cầu kiểm kê chưa được đếm xong hoặc không ở trạng thái 'Đã đếm'.");
                    return false;
                }

                // Get all details for this stocktake request
                std::vector<ERP::Warehouse::DTO::StocktakeDetailDTO> details = stocktakeDetailDAO_->getStocktakeDetailsByRequestId(requestId);

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        for (auto& detail : details) {
                            if (detail.difference != 0.0) {
                                ERP::Logger::Logger::getInstance().info("StocktakeService: Reconciling difference for product " + detail.productId + " at " + detail.warehouseId + "/" + detail.locationId + ". Difference: " + std::to_string(detail.difference));

                                ERP::Warehouse::DTO::InventoryTransactionDTO invTxn;
                                invTxn.id = ERP::Utils::generateUUID();
                                invTxn.productId = detail.productId;
                                invTxn.warehouseId = detail.warehouseId;
                                invTxn.locationId = detail.locationId;
                                invTxn.transactionDate = ERP::Utils::DateUtils::now();
                                invTxn.referenceDocumentId = stocktakeRequest.id;
                                invTxn.referenceDocumentType = "Stocktake";
                                invTxn.notes = "Inventory adjustment from Stocktake " + stocktakeRequest.id;
                                invTxn.status = ERP::Common::EntityStatus::ACTIVE;
                                invTxn.createdAt = ERP::Utils::DateUtils::now();
                                invTxn.createdBy = currentUserId;

                                if (detail.difference > 0) { // System quantity > Counted quantity (Shortage) -> Adjustment Out
                                    invTxn.type = ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_OUT;
                                    invTxn.quantity = std::abs(detail.difference);
                                    // Get current unit cost from inventory for adjustment
                                    std::optional<ERP::Warehouse::DTO::InventoryDTO> currentInv = inventoryManagementService_->getInventoryByProductLocation(detail.productId, detail.warehouseId, detail.locationId, userRoleIds);
                                    invTxn.unitCost = currentInv ? currentInv->unitCost : 0.0;

                                    if (!inventoryManagementService_->adjustInventory(invTxn, currentUserId, userRoleIds)) {
                                        ERP::Logger::Logger::getInstance().error("StocktakeService: Failed to post ADJUSTMENT_OUT for detail " + detail.id + ".");
                                        return false;
                                    }
                                }
                                else if (detail.difference < 0) { // System quantity < Counted quantity (Overage) -> Adjustment In
                                    invTxn.type = ERP::Warehouse::DTO::InventoryTransactionType::ADJUSTMENT_IN;
                                    invTxn.quantity = std::abs(detail.difference);
                                    // For adjustment in, cost might be standard cost or zero, depending on policy.
                                    // For simplicity, using 0.0 or product's last purchase cost.
                                    std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(detail.productId, userRoleIds);
                                    invTxn.unitCost = product ? product->purchasePrice.value_or(0.0) : 0.0;

                                    if (!inventoryManagementService_->adjustInventory(invTxn, currentUserId, userRoleIds)) {
                                        ERP::Logger::Logger::getInstance().error("StocktakeService: Failed to post ADJUSTMENT_IN for detail " + detail.id + ".");
                                        return false;
                                    }
                                }
                                // Link transaction ID to stocktake detail
                                detail.adjustmentTransactionId = invTxn.id;
                                if (!stocktakeDetailDAO_->update(detail)) { // Update the detail with transaction ID
                                    ERP::Logger::Logger::getInstance().error("StocktakeService: Failed to update stocktake detail with adjustment transaction ID.");
                                    return false;
                                }
                            }
                        }

                        // Update Stocktake Request status to RECONCILED
                        stocktakeRequest.status = ERP::Warehouse::DTO::StocktakeRequestStatus::RECONCILED;
                        stocktakeRequest.updatedAt = ERP::Utils::DateUtils::now();
                        stocktakeRequest.updatedBy = currentUserId;
                        if (!stocktakeRequestDAO_->update(stocktakeRequest)) {
                            ERP::Logger::Logger::getInstance().error("StocktakeService: Failed to update stocktake request status to RECONCILED in DAO.");
                            return false;
                        }

                        return true;
                    },
                    "StocktakeService", "reconcileStocktake"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("StocktakeService: Stocktake request " + requestId + " reconciled successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::PROCESS_END, ERP::Common::LogSeverity::INFO, // Could be specialized activity type
                        "Warehouse", "StocktakeReconciliation", requestId, "StocktakeRequest", stocktakeRequest.warehouseId + "/" + stocktakeRequest.locationId.value_or("All"),
                        stocktakeRequest.toMap(), stocktakeRequest.toMap(), "Stocktake reconciled. Adjustments posted.");
                    return true;
                }
                return false;
            }

        } // namespace Services
    } // namespace Warehouse
} // namespace ERP