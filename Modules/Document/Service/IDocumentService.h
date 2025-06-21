// Modules/Document/Service/IDocumentService.h
#ifndef MODULES_DOCUMENT_SERVICE_IDOCUMENTSERVICE_H
#define MODULES_DOCUMENT_SERVICE_IDOCUMENTSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>

// Rút gọn các include paths
#include "Document.h"      // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

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

} // namespace Services
} // namespace Document
} // namespace ERP
#endif // MODULES_DOCUMENT_SERVICE_IDOCUMENTSERVICE_H