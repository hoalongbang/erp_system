// Modules/Sales/Service/SalesReturnService.cpp
#include "SalesReturnService.h" // Standard includes
#include "Return.h"             // Return DTO
#include "ReturnDetail.h"       // ReturnDetail DTO
#include "SalesOrder.h"         // SalesOrder DTO
#include "Customer.h"           // Customer DTO
#include "Warehouse.h"          // Warehouse DTO
#include "Product.h"            // Product DTO
#include "InventoryTransaction.h" // InventoryTransaction DTO for goods receipt
#include "Event.h"              // Event DTO
#include "ConnectionPool.h"     // ConnectionPool
#include "DBConnection.h"       // DBConnection
#include "Common.h"             // Common Enums/Constants
#include "Utils.h"              // Utility functions
#include "DateUtils.h"          // Date utility functions
#include "AutoRelease.h"        // RAII helper
#include "ISecurityManager.h"   // Security Manager interface
#include "UserService.h"        // User Service (for audit logging)
#include "SalesOrderService.h"  // SalesOrderService (for SO validation/updates)
#include "CustomerService.h"    // CustomerService (for customer validation)
#include "WarehouseService.h"   // WarehouseService (for warehouse validation)
#include "ProductService.h"     // ProductService (for product validation)
#include "InventoryManagementService.h" // InventoryManagementService (for goods receipt)

#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed

namespace ERP {
    namespace Sales {
        namespace Services {

            SalesReturnService::SalesReturnService(
                std::shared_ptr<DAOs::ReturnDAO> returnDAO,
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
                returnDAO_(returnDAO),
                salesOrderService_(salesOrderService),
                customerService_(customerService),
                warehouseService_(warehouseService),
                productService_(productService),
                inventoryManagementService_(inventoryManagementService) {

                if (!returnDAO_ || !salesOrderService_ || !customerService_ || !warehouseService_ || !productService_ || !inventoryManagementService_ || !securityManager_) {
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "SalesReturnService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ trả hàng.");
                    ERP::Logger::Logger::getInstance().critical("SalesReturnService: One or more injected DAOs/Services are null.");
                    throw std::runtime_error("SalesReturnService: Null dependencies.");
                }
                ERP::Logger::Logger::getInstance().info("SalesReturnService: Initialized.");
            }

            std::optional<ERP::Sales::DTO::ReturnDTO> SalesReturnService::createReturn(
                const ERP::Sales::DTO::ReturnDTO& returnDTO,
                const std::vector<ERP::Sales::DTO::ReturnDetailDTO>& returnDetails,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("SalesReturnService: Attempting to create sales return for sales order: " + returnDTO.salesOrderId + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Sales.CreateReturn", "Bạn không có quyền tạo yêu cầu trả hàng.")) {
                    return std::nullopt;
                }

                // 1. Validate input DTO
                if (returnDTO.salesOrderId.empty() || returnDTO.customerId.empty() || returnDetails.empty()) {
                    ERP::Logger::Logger::getInstance().warning("SalesReturnService: Invalid input for return creation (missing sales order ID, customer ID, or details).");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin trả hàng không đầy đủ.");
                    return std::nullopt;
                }

                // Validate Sales Order existence
                std::optional<ERP::Sales::DTO::SalesOrderDTO> salesOrder = salesOrderService_->getSalesOrderById(returnDTO.salesOrderId, userRoleIds);
                if (!salesOrder || (salesOrder->status != ERP::Sales::DTO::SalesOrderStatus::COMPLETED && salesOrder->status != ERP::Sales::DTO::SalesOrderStatus::IN_PROGRESS)) {
                    ERP::Logger::Logger::getInstance().warning("SalesReturnService: Invalid Sales Order ID provided or sales order not in valid status: " + returnDTO.salesOrderId);
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Đơn hàng bán không hợp lệ hoặc không ở trạng thái đủ điều kiện trả hàng.");
                    return std::nullopt;
                }
                // Ensure customer matches sales order's customer
                if (salesOrder->customerId != returnDTO.customerId) {
                    ERP::Logger::Logger::getInstance().warning("SalesReturnService: Customer ID mismatch between return DTO and sales order.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID khách hàng không khớp với đơn hàng bán.");
                    return std::nullopt;
                }

                // Validate Customer existence
                std::optional<ERP::Customer::DTO::CustomerDTO> customer = customerService_->getCustomerById(returnDTO.customerId, userRoleIds);
                if (!customer || customer->status != ERP::Common::EntityStatus::ACTIVE) {
                    ERP::Logger::Logger::getInstance().warning("SalesReturnService: Customer " + returnDTO.customerId + " not found or not active.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Khách hàng không tồn tại hoặc không hoạt động.");
                    return std::nullopt;
                }

                // Validate Warehouse existence if provided
                if (returnDTO.warehouseId) {
                    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(*returnDTO.warehouseId, userRoleIds);
                    if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("SalesReturnService: Warehouse " + *returnDTO.warehouseId + " not found or not active.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Kho hàng không tồn tại hoặc không hoạt động.");
                        return std::nullopt;
                    }
                }

                // Validate details: Product existence, UoM existence, quantities
                double calculatedTotalAmount = 0.0;
                for (const auto& detail : returnDetails) {
                    std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(detail.productId, userRoleIds);
                    if (!product || product->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("SalesReturnService: Product " + detail.productId + " not found or not active in return detail.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Sản phẩm trong chi tiết trả hàng không hợp lệ.");
                        return std::nullopt;
                    }
                    std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> uom = unitOfMeasureService_->getUnitOfMeasureById(detail.unitOfMeasureId, userRoleIds);
                    if (!uom || uom->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("SalesReturnService: Unit of Measure " + detail.unitOfMeasureId + " not found or not active in return detail.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Đơn vị đo trong chi tiết trả hàng không hợp lệ.");
                        return std::nullopt;
                    }
                    if (detail.quantity <= 0 || detail.unitPrice < 0) {
                        ERP::Logger::Logger::getInstance().warning("SalesReturnService: Invalid quantity or unit price in return detail for product " + detail.productId);
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng hoặc đơn giá trong chi tiết trả hàng không hợp lệ.");
                        return std::nullopt;
                    }
                    calculatedTotalAmount += detail.quantity * detail.unitPrice;
                }
                // Compare calculated total with DTO's total (allow for slight float differences)
                if (std::abs(calculatedTotalAmount - returnDTO.totalAmount) > 0.01) {
                    ERP::Logger::Logger::getInstance().warning("SalesReturnService: Calculated total amount (" + std::to_string(calculatedTotalAmount) + ") does not match DTO total (" + std::to_string(returnDTO.totalAmount) + ").");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Tổng số tiền trả hàng không khớp với chi tiết.");
                    // return std::nullopt; // Decide if this is a strict error or warning
                }

                ERP::Sales::DTO::ReturnDTO newReturn = returnDTO;
                newReturn.id = ERP::Utils::generateUUID();
                newReturn.createdAt = ERP::Utils::DateUtils::now();
                newReturn.createdBy = currentUserId;
                newReturn.status = ERP::Sales::DTO::ReturnStatus::PENDING; // Always pending on creation
                newReturn.returnDate = ERP::Utils::DateUtils::now(); // Set actual return date

                std::optional<ERP::Sales::DTO::ReturnDTO> createdReturn = std::nullopt;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!returnDAO_->create(newReturn)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("SalesReturnService: Failed to create return in DAO.");
                            return false;
                        }
                        // Save details
                        for (auto detail : returnDetails) {
                            detail.id = ERP::Utils::generateUUID();
                            detail.returnId = newReturn.id;
                            detail.createdAt = newReturn.createdAt; // Inherit creation time
                            detail.createdBy = newReturn.createdBy; // Inherit created by
                            detail.status = ERP::Common::EntityStatus::ACTIVE; // Details are active
                            if (!returnDAO_->createReturnDetail(detail)) { // Specific DAO method
                                ERP::Logger::Logger::getInstance().error("SalesReturnService: Failed to create return detail for product " + detail.productId + ".");
                                return false;
                            }
                            // When return is created, it's typically received into inventory.
                            // This would involve creating a GOODS_RECEIPT inventory transaction.
                            // Ensure the warehouseId is provided in the ReturnDTO.
                            if (newReturn.warehouseId) {
                                ERP::Warehouse::DTO::InventoryTransactionDTO invTxn;
                                invTxn.id = ERP::Utils::generateUUID();
                                invTxn.productId = detail.productId;
                                invTxn.warehouseId = *newReturn.warehouseId;
                                // For return, location might be default or specified in detail
                                invTxn.locationId = inventoryManagementService_->getDefaultLocationForWarehouse(*newReturn.warehouseId, userRoleIds).value_or(""); // Get default location
                                if (invTxn.locationId.empty()) {
                                    ERP::Logger::Logger::getInstance().error("SalesReturnService: No default location found for warehouse " + *newReturn.warehouseId + " for return inventory transaction.");
                                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không tìm thấy vị trí mặc định cho kho hàng.");
                                    return false;
                                }
                                invTxn.type = ERP::Warehouse::DTO::InventoryTransactionType::GOODS_RECEIPT;
                                invTxn.quantity = detail.quantity;
                                invTxn.unitCost = detail.unitPrice; // Use unit price as cost for return receipt
                                invTxn.transactionDate = newReturn.returnDate;
                                invTxn.referenceDocumentId = newReturn.id;
                                invTxn.referenceDocumentType = "Return";
                                invTxn.notes = "Goods receipt from Sales Return " + newReturn.returnNumber;
                                invTxn.status = ERP::Common::EntityStatus::ACTIVE;
                                invTxn.createdAt = ERP::Utils::DateUtils::now();
                                invTxn.createdBy = currentUserId;

                                if (!inventoryManagementService_->recordGoodsReceipt(invTxn, currentUserId, userRoleIds)) {
                                    ERP::Logger::Logger::getInstance().error("SalesReturnService: Failed to record goods receipt for return detail " + detail.id + ".");
                                    return false;
                                }
                                // Update the detail with the inventory transaction ID
                                detail.inventoryTransactionId = invTxn.id;
                                // This update of detail needs to be within the same transaction.
                                // If DAOBASE::update does not support nested transactions, this needs careful handling.
                                // Assuming createReturnDetail will persist this, or update detail post-creation.
                                // For now, this is a conceptual link.
                            }
                        }
                        createdReturn = newReturn;
                        // Optionally, publish event
                        eventBus_.publish(std::make_shared<EventBus::ReturnCreatedEvent>(newReturn.id, newReturn.returnNumber));
                        return true;
                    },
                    "SalesReturnService", "createReturn"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("SalesReturnService: Sales return " + newReturn.returnNumber + " created successfully with " + std::to_string(returnDetails.size()) + " details.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                        "Sales", "SalesReturn", newReturn.id, "SalesReturn", newReturn.returnNumber,
                        std::nullopt, newReturn.toMap(), "Sales return created.");
                    return createdReturn;
                }
                return std::nullopt;
            }

            std::optional<ERP::Sales::DTO::ReturnDTO> SalesReturnService::getReturnById(
                const std::string& returnId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().debug("SalesReturnService: Retrieving sales return by ID: " + returnId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewReturns", "Bạn không có quyền xem yêu cầu trả hàng.")) {
                    return std::nullopt;
                }

                std::optional<ERP::Sales::DTO::ReturnDTO> returnOpt = returnDAO_->findById(returnId);
                if (returnOpt) {
                    // Load details for the DTO
                    returnOpt->details = returnDAO_->getReturnDetailsByReturnId(returnOpt->id);
                }
                return returnOpt;
            }

            std::vector<ERP::Sales::DTO::ReturnDTO> SalesReturnService::getAllReturns(
                const std::map<std::string, std::any>& filter,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("SalesReturnService: Retrieving all sales returns with filter.");

                if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewReturns", "Bạn không có quyền xem tất cả yêu cầu trả hàng.")) {
                    return {};
                }

                std::vector<ERP::Sales::DTO::ReturnDTO> returns = returnDAO_->getReturns(filter);
                // Load details for each return
                for (auto& ret : returns) {
                    ret.details = returnDAO_->getReturnDetailsByReturnId(ret.id);
                }
                return returns;
            }

            bool SalesReturnService::updateReturn(
                const ERP::Sales::DTO::ReturnDTO& returnDTO,
                const std::vector<ERP::Sales::DTO::ReturnDetailDTO>& returnDetails,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("SalesReturnService: Attempting to update sales return: " + returnDTO.id + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Sales.UpdateReturn", "Bạn không có quyền cập nhật yêu cầu trả hàng.")) {
                    return false;
                }

                std::optional<ERP::Sales::DTO::ReturnDTO> oldReturnOpt = returnDAO_->findById(returnDTO.id);
                if (!oldReturnOpt) {
                    ERP::Logger::Logger::getInstance().warning("SalesReturnService: Sales return with ID " + returnDTO.id + " not found for update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu trả hàng cần cập nhật.");
                    return false;
                }

                // Validate Sales Order existence
                if (returnDTO.salesOrderId != oldReturnOpt->salesOrderId) { // Only check if changed
                    std::optional<ERP::Sales::DTO::SalesOrderDTO> salesOrder = salesOrderService_->getSalesOrderById(returnDTO.salesOrderId, userRoleIds);
                    if (!salesOrder || (salesOrder->status != ERP::Sales::DTO::SalesOrderStatus::COMPLETED && salesOrder->status != ERP::Sales::DTO::SalesOrderStatus::IN_PROGRESS)) {
                        ERP::Logger::Logger::getInstance().warning("SalesReturnService: Invalid Sales Order ID provided for update or sales order not in valid status: " + returnDTO.salesOrderId);
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Đơn hàng bán không hợp lệ hoặc không ở trạng thái đủ điều kiện trả hàng.");
                        return std::nullopt;
                    }
                }
                // Ensure customer matches sales order's customer
                if (returnDTO.customerId != oldReturnOpt->customerId) { // Only check if changed
                    if (salesOrderService_->getSalesOrderById(returnDTO.salesOrderId, userRoleIds)->customerId != returnDTO.customerId) {
                        ERP::Logger::Logger::getInstance().warning("SalesReturnService: Customer ID mismatch between return DTO and sales order during update.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "ID khách hàng không khớp với đơn hàng bán.");
                        return std::nullopt;
                    }
                }

                // Validate Warehouse existence if provided
                if (returnDTO.warehouseId && (oldReturnOpt->warehouseId != returnDTO.warehouseId)) {
                    std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(*returnDTO.warehouseId, userRoleIds);
                    if (!warehouse || warehouse->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("SalesReturnService: Warehouse " + *returnDTO.warehouseId + " not found or not active for update.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Kho hàng không tồn tại hoặc không hoạt động.");
                        return std::nullopt;
                    }
                }

                // Validate details: Product existence, UoM existence, quantities
                double calculatedTotalAmount = 0.0;
                for (const auto& detail : returnDetails) {
                    std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(detail.productId, userRoleIds);
                    if (!product || product->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("SalesReturnService: Product " + detail.productId + " not found or not active in return detail for update.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Sản phẩm trong chi tiết trả hàng không hợp lệ.");
                        return std::nullopt;
                    }
                    std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> uom = unitOfMeasureService_->getUnitOfMeasureById(detail.unitOfMeasureId, userRoleIds);
                    if (!uom || uom->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("SalesReturnService: Unit of Measure " + detail.unitOfMeasureId + " not found or not active in return detail for update.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Đơn vị đo trong chi tiết trả hàng không hợp lệ.");
                        return std::nullopt;
                    }
                    if (detail.quantity <= 0 || detail.unitPrice < 0) {
                        ERP::Logger::Logger::getInstance().warning("SalesReturnService: Invalid quantity or unit price in return detail for product " + detail.productId + " during update.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng hoặc đơn giá trong chi tiết trả hàng không hợp lệ.");
                        return std::nullopt;
                    }
                    calculatedTotalAmount += detail.quantity * detail.unitPrice;
                }
                // Compare calculated total with DTO's total (allow for slight float differences)
                if (std::abs(calculatedTotalAmount - returnDTO.totalAmount) > 0.01) {
                    ERP::Logger::Logger::getInstance().warning("SalesReturnService: Calculated total amount (" + std::to_string(calculatedTotalAmount) + ") does not match DTO total (" + std::to_string(returnDTO.totalAmount) + ") during update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Tổng số tiền trả hàng không khớp với chi tiết.");
                    // return false; // Decide if this is a strict error or warning
                }


                ERP::Sales::DTO::ReturnDTO updatedReturn = returnDTO;
                updatedReturn.updatedAt = ERP::Utils::DateUtils::now();
                updatedReturn.updatedBy = currentUserId;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!returnDAO_->update(updatedReturn)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("SalesReturnService: Failed to update sales return " + updatedReturn.id + " in DAO.");
                            return false;
                        }
                        // Update details: Delete old and create new ones (simplistic full replacement)
                        if (!returnDAO_->removeReturnDetailsByReturnId(updatedReturn.id)) {
                            ERP::Logger::Logger::getInstance().error("SalesReturnService: Failed to remove old return details for return " + updatedReturn.id + ".");
                            return false;
                        }
                        for (auto detail : returnDetails) {
                            detail.id = ERP::Utils::generateUUID(); // Assign new ID for new details
                            detail.returnId = updatedReturn.id;
                            detail.createdAt = updatedReturn.createdAt; // Re-use parent creation time
                            detail.createdBy = updatedReturn.createdBy; // Re-use parent created by
                            detail.status = ERP::Common::EntityStatus::ACTIVE;
                            if (!returnDAO_->createReturnDetail(detail)) {
                                ERP::Logger::Logger::getInstance().error("SalesReturnService: Failed to create new return detail for product " + detail.productId + " during update.");
                                return false;
                            }
                        }
                        // Handle inventory adjustments for quantity changes in details
                        // This would be complex: compare old and new details, create adjustments.
                        // For now, this is omitted. A full ERP would need sophisticated inventory reconciliation here.

                        // Optionally, publish event
                        eventBus_.publish(std::make_shared<EventBus::ReturnUpdatedEvent>(updatedReturn.id, updatedReturn.returnNumber)); // Assuming such an event exists
                        return true;
                    },
                    "SalesReturnService", "updateReturn"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("SalesReturnService: Sales return " + updatedReturn.id + " updated successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                        "Sales", "SalesReturn", updatedReturn.id, "SalesReturn", updatedReturn.returnNumber,
                        oldReturnOpt->toMap(), updatedReturn.toMap(), "Sales return updated.");
                    return true;
                }
                return false;
            }

            bool SalesReturnService::updateReturnStatus(
                const std::string& returnId,
                ERP::Sales::DTO::ReturnStatus newStatus,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("SalesReturnService: Attempting to update status for sales return: " + returnId + " to " + ERP::Sales::DTO::ReturnDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Sales.UpdateReturnStatus", "Bạn không có quyền cập nhật trạng thái yêu cầu trả hàng.")) {
                    return false;
                }

                std::optional<ERP::Sales::DTO::ReturnDTO> returnOpt = returnDAO_->findById(returnId);
                if (!returnOpt) {
                    ERP::Logger::Logger::getInstance().warning("SalesReturnService: Sales return with ID " + returnId + " not found for status update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu trả hàng để cập nhật trạng thái.");
                    return false;
                }

                ERP::Sales::DTO::ReturnDTO oldReturn = *returnOpt;
                if (oldReturn.status == newStatus) {
                    ERP::Logger::Logger::getInstance().info("SalesReturnService: Sales return " + returnId + " is already in status " + ERP::Sales::DTO::ReturnDTO().getStatusString(newStatus) + ".");
                    return true; // Already in desired status
                }

                // Add state transition validation logic here
                // For example: Cannot change from PROCESSED to PENDING.

                ERP::Sales::DTO::ReturnDTO updatedReturn = oldReturn;
                updatedReturn.status = newStatus;
                updatedReturn.updatedAt = ERP::Utils::DateUtils::now();
                updatedReturn.updatedBy = currentUserId;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!returnDAO_->update(updatedReturn)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("SalesReturnService: Failed to update status for sales return " + returnId + " in DAO.");
                            return false;
                        }
                        // If status changes to PROCESSED, this is where financial transactions (credit memos) would be generated
                        // and inventory adjustments might be finalized if not done at creation.
                        // For now, this is conceptual.

                        // Optionally, publish event for status change
                        eventBus_.publish(std::make_shared<EventBus::ReturnStatusChangedEvent>(returnId, newStatus)); // Assuming such an event exists
                        return true;
                    },
                    "SalesReturnService", "updateReturnStatus"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("SalesReturnService: Status for sales return " + returnId + " updated successfully to " + updatedReturn.getStatusString() + ".");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                        "Sales", "SalesReturnStatus", returnId, "SalesReturn", oldReturn.returnNumber,
                        oldReturn.toMap(), updatedReturn.toMap(), "Sales return status changed to " + updatedReturn.getStatusString() + ".");
                    return true;
                }
                return false;
            }

            bool SalesReturnService::deleteReturn(
                const std::string& returnId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("SalesReturnService: Attempting to delete sales return: " + returnId + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Sales.DeleteReturn", "Bạn không có quyền xóa yêu cầu trả hàng.")) {
                    return false;
                }

                std::optional<ERP::Sales::DTO::ReturnDTO> returnOpt = returnDAO_->findById(returnId);
                if (!returnOpt) {
                    ERP::Logger::Logger::getInstance().warning("SalesReturnService: Sales return with ID " + returnId + " not found for deletion.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy yêu cầu trả hàng cần xóa.");
                    return false;
                }

                ERP::Sales::DTO::ReturnDTO returnToDelete = *returnOpt;

                // Additional checks: Prevent deletion if return has been processed (e.g., refunded, inventory adjusted)
                if (returnToDelete.status == ERP::Sales::DTO::ReturnStatus::PROCESSED) {
                    ERP::Logger::Logger::getInstance().warning("SalesReturnService: Cannot delete processed sales return " + returnId + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa yêu cầu trả hàng đã xử lý.");
                    return false;
                }

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        // Remove associated details first
                        if (!returnDAO_->removeReturnDetailsByReturnId(returnId)) {
                            ERP::Logger::Logger::getInstance().error("SalesReturnService: Failed to remove associated return details for return " + returnId + ".");
                            return false;
                        }
                        if (!returnDAO_->remove(returnId)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("SalesReturnService: Failed to delete sales return " + returnId + " in DAO.");
                            return false;
                        }
                        return true;
                    },
                    "SalesReturnService", "deleteReturn"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("SalesReturnService: Sales return " + returnId + " deleted successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                        "Sales", "SalesReturn", returnId, "SalesReturn", returnToDelete.returnNumber,
                        returnToDelete.toMap(), std::nullopt, "Sales return deleted.");
                    return true;
                }
                return false;
            }

            std::vector<ERP::Sales::DTO::ReturnDetailDTO> SalesReturnService::getReturnDetails(
                const std::string& returnId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("SalesReturnService: Retrieving return details for return ID: " + returnId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Sales.ViewReturns", "Bạn không có quyền xem chi tiết yêu cầu trả hàng.")) {
                    return {};
                }

                // Validate parent Return existence
                if (!returnDAO_->findById(returnId)) {
                    ERP::Logger::Logger::getInstance().warning("SalesReturnService: Return " + returnId + " not found when getting details.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Yêu cầu trả hàng không tồn tại.");
                    return {};
                }

                return returnDAO_->getReturnDetailsByReturnId(returnId); // Specific DAO method
            }

        } // namespace Services
    } // namespace Sales
} // namespace ERP