// Modules/Document/Service/DocumentService.cpp
#include "DocumentService.h" // Đã rút gọn include
#include "Document.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Document {
namespace Services {

DocumentService::DocumentService(
    std::shared_ptr<DAOs::DocumentDAO> documentDAO,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      documentDAO_(documentDAO) {
    if (!documentDAO_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "DocumentService: Initialized with null DAO.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ tài liệu.");
        ERP::Logger::Logger::getInstance().critical("DocumentService: Injected DocumentDAO is null.");
        throw std::runtime_error("DocumentService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("DocumentService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Document::DTO::DocumentDTO> DocumentService::uploadDocument(
    const ERP::Document::DTO::DocumentDTO& documentDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("DocumentService: Attempting to upload document: " + documentDTO.fileName + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Document.UploadDocument", "Bạn không có quyền tải lên tài liệu.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (documentDTO.fileName.empty() || documentDTO.internalFilePath.empty() || documentDTO.mimeType.empty()) {
        ERP::Logger::Logger::getInstance().warning("DocumentService: Invalid input for document upload (empty filename, path, or mime type).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "DocumentService: Invalid input for document upload.", "Thông tin tài liệu không đầy đủ.");
        return std::nullopt;
    }

    // Validate related entity if specified
    // (This would require specific service calls, e.g., productService->getProductById, customerService->getCustomerById)
    // For brevity, skipping cross-service validation here.

    ERP::Document::DTO::DocumentDTO newDocument = documentDTO;
    newDocument.id = ERP::Utils::generateUUID();
    newDocument.createdAt = ERP::Utils::DateUtils::now();
    newDocument.createdBy = currentUserId;
    newDocument.uploadTime = newDocument.createdAt; // Set uploadTime as creation time
    newDocument.uploadedByUserId = currentUserId;
    newDocument.status = ERP::Common::EntityStatus::ACTIVE; // Default status

    std::optional<ERP::Document::DTO::DocumentDTO> uploadedDocument = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Note: This service only records metadata. Actual file storage (e.g., to disk, cloud)
            // would be handled by a separate storage service.
            if (!documentDAO_->create(newDocument)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("DocumentService: Failed to upload document metadata " + newDocument.fileName + " in DAO.");
                return false;
            }
            uploadedDocument = newDocument;
            eventBus_.publish(std::make_shared<EventBus::DocumentUploadedEvent>(newDocument.id, newDocument.fileName, newDocument.relatedEntityId.value_or("")));
            return true;
        },
        "DocumentService", "uploadDocument"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("DocumentService: Document " + newDocument.fileName + " uploaded successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::FILE_UPLOAD, ERP::Common::LogSeverity::INFO,
                       "Document", "Document", newDocument.id, "Document", newDocument.fileName,
                       std::nullopt, newDocument.toMap(), "Document uploaded.");
        return uploadedDocument;
    }
    return std::nullopt;
}

std::optional<ERP::Document::DTO::DocumentDTO> DocumentService::getDocumentById(
    const std::string& documentId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("DocumentService: Retrieving document by ID: " + documentId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Document.ViewDocument", "Bạn không có quyền xem tài liệu.")) {
        return std::nullopt;
    }

    return documentDAO_->getById(documentId); // Using getById from DAOBase template
}

std::vector<ERP::Document::DTO::DocumentDTO> DocumentService::getAllDocuments(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("DocumentService: Retrieving all documents with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Document.ViewDocument", "Bạn không có quyền xem tất cả tài liệu.")) {
        return {};
    }

    return documentDAO_->get(filter); // Using get from DAOBase template
}

std::vector<ERP::Document::DTO::DocumentDTO> DocumentService::getDocumentsByRelatedEntity(
    const std::string& entityId,
    const std::string& entityType,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("DocumentService: Retrieving documents for related entity ID: " + entityId + " of type: " + entityType + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Document.ViewDocument", "Bạn không có quyền xem tài liệu liên quan.")) {
        return {};
    }

    std::map<std::string, std::any> filter;
    filter["related_entity_id"] = entityId;
    filter["related_entity_type"] = entityType;

    return documentDAO_->get(filter); // Using get from DAOBase template
}

bool DocumentService::updateDocument(
    const ERP::Document::DTO::DocumentDTO& documentDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("DocumentService: Attempting to update document: " + documentDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Document.UpdateDocument", "Bạn không có quyền cập nhật tài liệu.")) {
        return false;
    }

    std::optional<ERP::Document::DTO::DocumentDTO> oldDocumentOpt = documentDAO_->getById(documentDTO.id); // Using getById from DAOBase
    if (!oldDocumentOpt) {
        ERP::Logger::Logger::getInstance().warning("DocumentService: Document with ID " + documentDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy tài liệu cần cập nhật.");
        return false;
    }

    // If file name is changed, check for uniqueness (if applicable, e.g., in a specific folder)
    // For simplicity, not checking unique filename across all documents.

    ERP::Document::DTO::DocumentDTO updatedDocument = documentDTO;
    updatedDocument.updatedAt = ERP::Utils::DateUtils::now();
    updatedDocument.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!documentDAO_->update(updatedDocument)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("DocumentService: Failed to update document " + updatedDocument.id + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::DocumentUpdatedEvent>(updatedDocument.id, updatedDocument.fileName));
            return true;
        },
        "DocumentService", "updateDocument"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("DocumentService: Document " + updatedDocument.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Document", "Document", updatedDocument.id, "Document", updatedDocument.fileName,
                       oldDocumentOpt->toMap(), updatedDocument.toMap(), "Document metadata updated.");
        return true;
    }
    return false;
}

bool DocumentService::deleteDocument(
    const std::string& documentId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("DocumentService: Attempting to delete document: " + documentId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Document.DeleteDocument", "Bạn không có quyền xóa tài liệu.")) {
        return false;
    }

    std::optional<ERP::Document::DTO::DocumentDTO> documentOpt = documentDAO_->getById(documentId); // Using getById from DAOBase
    if (!documentOpt) {
        ERP::Logger::Logger::getInstance().warning("DocumentService: Document with ID " + documentId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy tài liệu cần xóa.");
        return false;
    }

    ERP::Document::DTO::DocumentDTO documentToDelete = *documentOpt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!documentDAO_->remove(documentId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("DocumentService: Failed to delete document " + documentId + " in DAO.");
                return false;
            }
            // In a real system, also delete the physical file from storage via a StorageService.
            return true;
        },
        "DocumentService", "deleteDocument"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("DocumentService: Document " + documentId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Document", "Document", documentId, "Document", documentToDelete.fileName,
                       documentToDelete.toMap(), std::nullopt, "Document deleted.");
        return true;
    }
    return false;
}

std::vector<ERP::Document::DTO::DocumentDTO> DocumentService::getDocumentsByType(
    ERP::Document::DTO::DocumentType documentType,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("DocumentService: Retrieving documents by type: " + ERP::Document::DTO::DocumentDTO().getTypeString(documentType) + " by user: " + currentUserId);

    if (!checkPermission(currentUserId, userRoleIds, "Document.ViewDocument", "Bạn không có quyền xem tài liệu theo loại.")) {
        return {};
    }
    
    std::map<std::string, std::any> filter;
    filter["type"] = static_cast<int>(documentType);

    return documentDAO_->get(filter); // Using get from DAOBase template
}

} // namespace Services
} // namespace Document
} // namespace ERP