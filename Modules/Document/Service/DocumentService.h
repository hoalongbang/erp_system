// Modules/Document/Service/DocumentService.h
#ifndef MODULES_DOCUMENT_SERVICE_DOCUMENTSERVICE_H
#define MODULES_DOCUMENT_SERVICE_DOCUMENTSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"      // NEW: Kế thừa từ BaseService
#include "Document.h"         // Đã rút gọn include
#include "DocumentDAO.h"      // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "EventBus.h"         // Đã rút gọn include
#include "Logger.h"           // Đã rút gọn include
#include "ErrorHandler.h"     // Đã rút gọn include
#include "Common.h"           // Đã rút gọn include
#include "Utils.h"            // Đã rút gọn include
#include "DateUtils.h"        // Đã rút gọn include

namespace ERP {
namespace Document {
namespace Services {

/**
 * @brief IDocumentService interface defines operations for managing documents.
 */
class IDocumentService {
public:
    virtual ~IDocumentService() = default;
    /**
     * @brief Uploads a new document and records its metadata.
     * @param documentDTO DTO containing new document metadata.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional DocumentDTO if upload is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Document::DTO::DocumentDTO> uploadDocument(
        const ERP::Document::DTO::DocumentDTO& documentDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves document metadata by ID.
     * @param documentId ID of the document to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional DocumentDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Document::DTO::DocumentDTO> getDocumentById(
        const std::string& documentId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all documents or documents matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching DocumentDTOs.
     */
    virtual std::vector<ERP::Document::DTO::DocumentDTO> getAllDocuments(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves documents related to a specific business entity.
     * @param entityId ID of the related business entity.
     * @param entityType Type of the related business entity (e.g., "SalesOrder", "Product").
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching DocumentDTOs.
     */
    virtual std::vector<ERP::Document::DTO::DocumentDTO> getDocumentsByRelatedEntity(
        const std::string& entityId,
        const std::string& entityType,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates document metadata.
     * @param documentDTO DTO containing updated document metadata (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateDocument(
        const ERP::Document::DTO::DocumentDTO& documentDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a document record by ID (soft delete).
     * @param documentId ID of the document to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteDocument(
        const std::string& documentId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves documents of a specific type.
     * @param documentType The type of document to retrieve.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching DocumentDTOs.
     */
    virtual std::vector<ERP::Document::DTO::DocumentDTO> getDocumentsByType(
        ERP::Document::DTO::DocumentType documentType,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of IDocumentService.
 * This class uses DocumentDAO and ISecurityManager.
 */
class DocumentService : public IDocumentService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for DocumentService.
     * @param documentDAO Shared pointer to DocumentDAO.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    DocumentService(std::shared_ptr<DAOs::DocumentDAO> documentDAO,
                    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                    std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Document::DTO::DocumentDTO> uploadDocument(
        const ERP::Document::DTO::DocumentDTO& documentDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Document::DTO::DocumentDTO> getDocumentById(
        const std::string& documentId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Document::DTO::DocumentDTO> getAllDocuments(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Document::DTO::DocumentDTO> getDocumentsByRelatedEntity(
        const std::string& entityId,
        const std::string& entityType,
        const std::vector<std::string>& userRoleIds = {}) override;
    bool updateDocument(
        const ERP::Document::DTO::DocumentDTO& documentDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteDocument(
        const std::string& documentId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::vector<ERP::Document::DTO::DocumentDTO> getDocumentsByType(
        ERP::Document::DTO::DocumentType documentType,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::DocumentDAO> documentDAO_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // EventBus is typically accessed as a singleton.
    ERP::EventBus::EventBus& eventBus_ = ERP::EventBus::EventBus::getInstance();
};
} // namespace Services
} // namespace Document
} // namespace ERP
#endif // MODULES_DOCUMENT_SERVICE_DOCUMENTSERVICE_H