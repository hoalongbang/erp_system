// Modules/Material/Service/MaterialRequestService.cpp
#include "MaterialRequestService.h" // Standard includes
#include "MaterialRequestSlip.h"    // MaterialRequestSlip DTO
#include "MaterialRequestSlipDetail.h" // MaterialRequestSlipDetail DTO
#include "Product.h"                // Product DTO
#include "Event.h"                  // Event DTO
#include "ConnectionPool.h"         // ConnectionPool
#include "DBConnection.h"           // DBConnection
#include "Common.h"                 // Common Enums/Constants
#include "Utils.h"                  // Utility functions
#include "DateUtils.h"              // Date utility functions
#include "AutoRelease.h"            // RAII helper
#include "ISecurityManager.h"       // Security Manager interface
#include "UserService.h"            // User Service (for audit logging)
#include "ProductService.h"         // ProductService (for product validation)

#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed

namespace ERP {
    namespace Material {
        namespace Services {

            MaterialRequestService::MaterialRequestService(
                std::shared_ptr<DAOs::MaterialRequestSlipDAO> materialRequestSlipDAO,
                std::shared_ptr<ERP::Product::Services::IProductService> productService, // Dependency
                std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
                : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Initialize BaseService
                materialRequestSlipDAO_(materialRequestSlipDAO),
                productService_(productService) { // Initialize specific dependencies

                if (!materialRequestSlipDAO_ || !productService_ || !securityManager_) { // BaseService checks its own dependencies
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "MaterialRequestService: Initialized with null DAO or dependent services.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ yêu cầu vật tư.");
                    ERP::Logger::Logger::getInstance().critical("MaterialRequestService: One or more injected DAOs/Services are null.");
                    throw std::runtime_error("MaterialRequestService: Null dependencies.");
                }
                ERP::Logger::Logger::getInstance().info("MaterialRequestService: Initialized.");
            }

            std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> MaterialRequestService::createMaterialRequest(
                const ERP::Material::DTO::MaterialRequestSlipDTO& requestDTO,
                const std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO>& requestDetails,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("MaterialRequestService: Attempting to create material request: " + requestDTO.requestNumber + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Material.CreateMaterialRequest", "Bạn không có quyền tạo phiếu yêu cầu vật tư.")) {
                    return std::nullopt;
                }

                // 1. Validate input DTO
                if (requestDTO.requestNumber.empty() || requestDTO.requestingDepartment.empty() || requestDetails.empty()) {
                    ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Invalid input for request creation (missing number, department, or details).");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin phiếu yêu cầu vật tư không đầy đủ.");
                    return std::nullopt;
                }

                // Check if request number already exists
                std::map<std::string, std::any> filterByNumber;
                filterByNumber["request_number"] = requestDTO.requestNumber;
                if (materialRequestSlipDAO_->countMaterialRequestSlips(filterByNumber) > 0) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Material request with number " + requestDTO.requestNumber + " already exists.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số phiếu yêu cầu vật tư đã tồn tại. Vui lòng chọn số khác.");
                    return std::nullopt;
                }

                // Validate requestedByUserId and approvedByUserId (if provided)
                if (!securityManager_->getUserService()->getUserById(requestDTO.requestedByUserId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Requested by user " + requestDTO.requestedByUserId + " not found.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Người yêu cầu không tồn tại.");
                    return std::nullopt;
                }
                if (requestDTO.approvedByUserId && !securityManager_->getUserService()->getUserById(*requestDTO.approvedByUserId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Approved by user " + *requestDTO.approvedByUserId + " not found.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Người phê duyệt không tồn tại.");
                    return std::nullopt;
                }

                // Validate details: Product existence, quantities
                for (const auto& detail : requestDetails) {
                    std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(detail.productId, userRoleIds);
                    if (!product || product->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Product " + detail.productId + " not found or not active in request detail.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Sản phẩm trong chi tiết yêu cầu vật tư không hợp lệ.");
                        return std::nullopt;
                    }
                    if (detail.requestedQuantity <= 0) {
                        ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Invalid requested quantity in request detail for product " + detail.productId);
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng yêu cầu trong chi tiết không hợp lệ.");
                        return std::nullopt;
                    }
                }

                ERP::Material::DTO::MaterialRequestSlipDTO newRequest = requestDTO;
                newRequest.id = ERP::Utils::generateUUID();
                newRequest.createdAt = ERP::Utils::DateUtils::now();
                newRequest.createdBy = currentUserId;
                newRequest.status = ERP::Material::DTO::MaterialRequestSlipStatus::DRAFT; // Default draft
                newRequest.requestDate = ERP::Utils::DateUtils::now(); // Set actual request date

                std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> createdRequest = std::nullopt;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!materialRequestSlipDAO_->create(newRequest)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("MaterialRequestService: Failed to create material request in DAO.");
                            return false;
                        }
                        // Save details
                        for (auto detail : requestDetails) {
                            detail.id = ERP::Utils::generateUUID();
                            detail.materialRequestSlipId = newRequest.id;
                            detail.createdAt = newRequest.createdAt; // Inherit creation time
                            detail.createdBy = newRequest.createdBy; // Inherit created by
                            detail.status = ERP::Common::EntityStatus::ACTIVE; // Details are active
                            // Initially, issuedQuantity is 0
                            detail.issuedQuantity = 0.0;
                            detail.isFullyIssued = false;
                            if (!materialRequestSlipDAO_->createMaterialRequestSlipDetail(detail)) { // Specific DAO method
                                ERP::Logger::Logger::getInstance().error("MaterialRequestService: Failed to create material request detail for product " + detail.productId + ".");
                                return false;
                            }
                        }
                        createdRequest = newRequest;
                        // Optionally, publish event
                        eventBus_.publish(std::make_shared<EventBus::MaterialRequestCreatedEvent>(newRequest.id, newRequest.requestNumber)); // Assuming such an event exists
                        return true;
                    },
                    "MaterialRequestService", "createMaterialRequest"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("MaterialRequestService: Material request " + newRequest.requestNumber + " created successfully with " + std::to_string(requestDetails.size()) + " details.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                        "Material", "MaterialRequest", newRequest.id, "MaterialRequest", newRequest.requestNumber,
                        std::nullopt, newRequest.toMap(), "Material request created.");
                    return createdRequest;
                }
                return std::nullopt;
            }

            std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> MaterialRequestService::getMaterialRequestById(
                const std::string& requestId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().debug("MaterialRequestService: Retrieving material request by ID: " + requestId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Material.ViewMaterialRequests", "Bạn không có quyền xem phiếu yêu cầu vật tư.")) {
                    return std::nullopt;
                }

                std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> requestOpt = materialRequestSlipDAO_->findById(requestId);
                if (requestOpt) {
                    // Load details for the DTO
                    requestOpt->details = materialRequestSlipDAO_->getMaterialRequestSlipDetailsBySlipId(requestOpt->id);
                }
                return requestOpt;
            }

            std::vector<ERP::Material::DTO::MaterialRequestSlipDTO> MaterialRequestService::getAllMaterialRequests(
                const std::map<std::string, std::any>& filter,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("MaterialRequestService: Retrieving all material requests with filter.");

                if (!checkPermission(currentUserId, userRoleIds, "Material.ViewMaterialRequests", "Bạn không có quyền xem tất cả phiếu yêu cầu vật tư.")) {
                    return {};
                }

                std::vector<ERP::Material::DTO::MaterialRequestSlipDTO> requests = materialRequestSlipDAO_->getMaterialRequestSlips(filter);
                // Load details for each request
                for (auto& request : requests) {
                    request.details = materialRequestSlipDAO_->getMaterialRequestSlipDetailsBySlipId(request.id);
                }
                return requests;
            }

            bool MaterialRequestService::updateMaterialRequest(
                const ERP::Material::DTO::MaterialRequestSlipDTO& requestDTO,
                const std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO>& requestDetails,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("MaterialRequestService: Attempting to update material request: " + requestDTO.id + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Material.UpdateMaterialRequest", "Bạn không có quyền cập nhật phiếu yêu cầu vật tư.")) {
                    return false;
                }

                std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> oldRequestOpt = materialRequestSlipDAO_->findById(requestDTO.id);
                if (!oldRequestOpt) {
                    ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Material request with ID " + requestDTO.id + " not found for update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiếu yêu cầu vật tư cần cập nhật.");
                    return false;
                }

                // Prevent update if already approved, in progress, or cancelled
                if (oldRequestOpt->status == ERP::Material::DTO::MaterialRequestSlipStatus::APPROVED ||
                    oldRequestOpt->status == ERP::Material::DTO::MaterialRequestSlipStatus::IN_PROGRESS ||
                    oldRequestOpt->status == ERP::Material::DTO::MaterialRequestSlipStatus::COMPLETED ||
                    oldRequestOpt->status == ERP::Material::DTO::MaterialRequestSlipStatus::CANCELLED) {
                    ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Cannot update material request " + requestDTO.id + " as it's already " + oldRequestOpt->getStatusString() + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể cập nhật phiếu yêu cầu vật tư đã được phê duyệt hoặc hoàn thành.");
                    return false;
                }


                // If request number is changed, check for uniqueness
                if (requestDTO.requestNumber != oldRequestOpt->requestNumber) {
                    std::map<std::string, std::any> filterByNumber;
                    filterByNumber["request_number"] = requestDTO.requestNumber;
                    if (materialRequestSlipDAO_->countMaterialRequestSlips(filterByNumber) > 0) {
                        ERP::Logger::Logger::getInstance().warning("MaterialRequestService: New request number " + requestDTO.requestNumber + " already exists.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số phiếu yêu cầu vật tư mới đã tồn tại. Vui lòng chọn số khác.");
                        return false;
                    }
                }

                // Validate requestedByUserId and approvedByUserId (if provided/changed)
                if (requestDTO.requestedByUserId != oldRequestOpt->requestedByUserId && !securityManager_->getUserService()->getUserById(requestDTO.requestedByUserId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Requested by user " + requestDTO.requestedByUserId + " not found for update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Người yêu cầu không tồn tại.");
                    return std::nullopt;
                }
                if (requestDTO.approvedByUserId && (oldRequestOpt->approvedByUserId != requestDTO.approvedByUserId) && !securityManager_->getUserService()->getUserById(*requestDTO.approvedByUserId, userRoleIds)) {
                    ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Approved by user " + *requestDTO.approvedByUserId + " not found for update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Người phê duyệt không tồn tại.");
                    return std::nullopt;
                }

                // Validate details (similar to create)
                for (const auto& detail : requestDetails) {
                    std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(detail.productId, userRoleIds);
                    if (!product || product->status != ERP::Common::EntityStatus::ACTIVE) {
                        ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Product " + detail.productId + " not found or not active in new request detail.");
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Sản phẩm trong chi tiết yêu cầu vật tư không hợp lệ.");
                        return std::nullopt;
                    }
                    if (detail.requestedQuantity <= 0) {
                        ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Invalid requested quantity in new request detail for product " + detail.productId);
                        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số lượng yêu cầu trong chi tiết không hợp lệ.");
                        return std::nullopt;
                    }
                }

                ERP::Material::DTO::MaterialRequestSlipDTO updatedRequest = requestDTO;
                updatedRequest.updatedAt = ERP::Utils::DateUtils::now();
                updatedRequest.updatedBy = currentUserId;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!materialRequestSlipDAO_->update(updatedRequest)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("MaterialRequestService: Failed to update material request " + updatedRequest.id + " in DAO.");
                            return false;
                        }

                        // Handle details replacement: Remove all old details then add new ones
                        if (!materialRequestSlipDAO_->removeMaterialRequestSlipDetailsBySlipId(updatedRequest.id)) {
                            ERP::Logger::Logger::getInstance().error("MaterialRequestService: Failed to remove old request details for request " + updatedRequest.id + ".");
                            return false;
                        }
                        for (auto detail : requestDetails) {
                            detail.id = ERP::Utils::generateUUID(); // Assign new ID for new details
                            detail.materialRequestSlipId = updatedRequest.id;
                            detail.createdAt = updatedRequest.createdAt;
                            detail.createdBy = updatedRequest.createdBy;
                            detail.status = ERP::Common::EntityStatus::ACTIVE;
                            detail.issuedQuantity = 0.0;
                            detail.isFullyIssued = false;
                            if (!materialRequestSlipDAO_->createMaterialRequestSlipDetail(detail)) {
                                ERP::Logger::Logger::getInstance().error("MaterialRequestService: Failed to create new request detail for product " + detail.productId + " during update.");
                                return false;
                            }
                        }

                        // Optionally, publish event
                        eventBus_.publish(std::make_shared<EventBus::MaterialRequestUpdatedEvent>(updatedRequest.id, updatedRequest.requestNumber)); // Assuming such an event exists
                        return true;
                    },
                    "MaterialRequestService", "updateMaterialRequest"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("MaterialRequestService: Material request " + updatedRequest.id + " updated successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                        "Material", "MaterialRequest", updatedRequest.id, "MaterialRequest", updatedRequest.requestNumber,
                        oldRequestOpt->toMap(), updatedRequest.toMap(), "Material request updated.");
                    return true;
                }
                return false;
            }

            bool MaterialRequestService::updateMaterialRequestStatus(
                const std::string& requestId,
                ERP::Material::DTO::MaterialRequestSlipStatus newStatus,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("MaterialRequestService: Attempting to update status for material request: " + requestId + " to " + ERP::Material::DTO::MaterialRequestSlipDTO().getStatusString(newStatus) + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Material.UpdateMaterialRequest", "Bạn không có quyền cập nhật trạng thái phiếu yêu cầu vật tư.")) { // Reusing general update perm
                    return false;
                }

                std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> requestOpt = materialRequestSlipDAO_->findById(requestId);
                if (!requestOpt) {
                    ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Material request with ID " + requestId + " not found for status update.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiếu yêu cầu vật tư để cập nhật trạng thái.");
                    return false;
                }

                ERP::Material::DTO::MaterialRequestSlipDTO oldRequest = *requestOpt;
                if (oldRequest.status == newStatus) {
                    ERP::Logger::Logger::getInstance().info("MaterialRequestService: Material request " + requestId + " is already in status " + ERP::Material::DTO::MaterialRequestSlipDTO().getStatusString(newStatus) + ".");
                    return true; // Already in desired status
                }

                // Add state transition validation logic here
                // For example: Cannot change from COMPLETED to DRAFT.

                ERP::Material::DTO::MaterialRequestSlipDTO updatedRequest = oldRequest;
                updatedRequest.status = newStatus;
                updatedRequest.updatedAt = ERP::Utils::DateUtils::now();
                updatedRequest.updatedBy = currentUserId;

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        if (!materialRequestSlipDAO_->update(updatedRequest)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("MaterialRequestService: Failed to update status for material request " + requestId + " in DAO.");
                            return false;
                        }
                        // Optionally, publish event for status change
                        eventBus_.publish(std::make_shared<EventBus::MaterialRequestStatusChangedEvent>(requestId, newStatus)); // Assuming such an event exists
                        return true;
                    },
                    "MaterialRequestService", "updateMaterialRequestStatus"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("MaterialRequestService: Status for material request " + requestId + " updated successfully to " + updatedRequest.getStatusString() + ".");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                        "Material", "MaterialRequestStatus", requestId, "MaterialRequest", oldRequest.requestNumber,
                        oldRequest.toMap(), updatedRequest.toMap(), "Material request status changed to " + updatedRequest.getStatusString() + ".");
                    return true;
                }
                return false;
            }

            bool MaterialRequestService::deleteMaterialRequest(
                const std::string& requestId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("MaterialRequestService: Attempting to delete material request: " + requestId + " by " + currentUserId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Material.DeleteMaterialRequest", "Bạn không có quyền xóa phiếu yêu cầu vật tư.")) {
                    return false;
                }

                std::optional<ERP::Material::DTO::MaterialRequestSlipDTO> requestOpt = materialRequestSlipDAO_->findById(requestId);
                if (!requestOpt) {
                    ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Material request with ID " + requestId + " not found for deletion.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy phiếu yêu cầu vật tư cần xóa.");
                    return false;
                }

                ERP::Material::DTO::MaterialRequestSlipDTO requestToDelete = *requestOpt;

                // Additional checks: Prevent deletion if request is approved or has associated issue slips
                if (requestToDelete.status == ERP::Material::DTO::MaterialRequestSlipStatus::APPROVED ||
                    requestToDelete.status == ERP::Material::DTO::MaterialRequestSlipStatus::IN_PROGRESS ||
                    requestToDelete.status == ERP::Material::DTO::MaterialRequestSlipStatus::COMPLETED) {
                    ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Cannot delete material request " + requestId + " as it's already " + requestToDelete.getStatusString() + ".");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa phiếu yêu cầu vật tư đã phê duyệt hoặc đã được xử lý.");
                    return false;
                }

                bool success = executeTransaction(
                    [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
                        // Remove associated details first
                        if (!materialRequestSlipDAO_->removeMaterialRequestSlipDetailsBySlipId(requestId)) {
                            ERP::Logger::Logger::getInstance().error("MaterialRequestService: Failed to remove associated request details for request " + requestId + ".");
                            return false;
                        }
                        if (!materialRequestSlipDAO_->remove(requestId)) { // Specific DAO method
                            ERP::Logger::Logger::getInstance().error("MaterialRequestService: Failed to delete material request " + requestId + " in DAO.");
                            return false;
                        }
                        return true;
                    },
                    "MaterialRequestService", "deleteMaterialRequest"
                );

                if (success) {
                    ERP::Logger::Logger::getInstance().info("MaterialRequestService: Material request " + requestId + " deleted successfully.");
                    recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                        ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                        "Material", "MaterialRequest", requestId, "MaterialRequest", requestToDelete.requestNumber,
                        requestToDelete.toMap(), std::nullopt, "Material request deleted.");
                    return true;
                }
                return false;
            }

            std::optional<ERP::Material::DTO::MaterialRequestSlipDetailDTO> MaterialRequestService::getMaterialRequestDetailById(
                const std::string& detailId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().debug("MaterialRequestService: Retrieving material request detail by ID: " + detailId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Material.ViewMaterialRequests", "Bạn không có quyền xem chi tiết phiếu yêu cầu vật tư.")) {
                    return std::nullopt;
                }

                return materialRequestSlipDAO_->getMaterialRequestSlipDetailById(detailId); // Specific DAO method
            }

            std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> MaterialRequestService::getMaterialRequestDetails(
                const std::string& requestId,
                const std::string& currentUserId,
                const std::vector<std::string>& userRoleIds) {
                ERP::Logger::Logger::getInstance().info("MaterialRequestService: Retrieving material request details for request ID: " + requestId + ".");

                if (!checkPermission(currentUserId, userRoleIds, "Material.ViewMaterialRequests", "Bạn không có quyền xem chi tiết phiếu yêu cầu vật tư.")) {
                    return {};
                }

                // Validate parent Material Request existence
                if (!materialRequestSlipDAO_->findById(requestId)) {
                    ERP::Logger::Logger::getInstance().warning("MaterialRequestService: Material Request " + requestId + " not found when getting details.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Phiếu yêu cầu vật tư không tồn tại.");
                    return {};
                }

                return materialRequestSlipDAO_->getMaterialRequestSlipDetailsBySlipId(requestId); // Specific DAO method
            }

        } // namespace Services
    } // namespace Material
} // namespace ERP