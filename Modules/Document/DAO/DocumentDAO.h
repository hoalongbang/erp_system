// Modules/Document/DAO/DocumentDAO.h
#ifndef MODULES_DOCUMENT_DAO_DOCUMENTDAO_H
#define MODULES_DOCUMENT_DAO_DOCUMENTDAO_H
#include "DAOBase/DAOBase.h" // Include the new templated DAOBase
#include "Modules/Document/DTO/Document.h" // For DocumentDTO
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>
// #include <QJsonObject> // Removed
#include "Logger/Logger.h"
#include "Modules/ErrorHandling/ErrorHandler.h"
#include "Modules/Common/Common.h"
#include "Modules/Utils/DateUtils.h"
#include "Modules/Utils/StringUtils.h" // For enum conversions
namespace ERP {
namespace Document {
namespace DAOs {
/**
 * @brief DocumentDAO class provides data access operations for DocumentDTO objects.
 * It manages the metadata of documents stored in the system.
 */
class DocumentDAO : public DAOBase::DAOBase<ERP::Document::DTO::DocumentDTO> {
public:
    /**
     * @brief Constructs a DocumentDAO.
     * @param connectionPool Shared pointer to the ConnectionPool instance.
     */
    explicit DocumentDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~DocumentDAO() override = default;

protected:
    /**
     * @brief Converts a DocumentDTO object to a map of column_name -> value.
     * This is an override of the pure virtual method in DAOBase.
     * @param dto The DocumentDTO object to convert.
     * @return A map representing the DTO's data.
     */
    std::map<std::string, std::any> toMap(const ERP::Document::DTO::DocumentDTO& dto) const override;
    /**
     * @brief Converts a database row (map<string, any>) to a DocumentDTO object.
     * This is an override of the pure virtual method in DAOBase.
     * @param data The map representing a row from the database.
     * @return A DocumentDTO object.
     */
    ERP::Document::DTO::DocumentDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    // tableName_ is now a member of DAOBase
};
} // namespace DAOs
} // namespace Document
} // namespace ERP
#endif // MODULES_DOCUMENT_DAO_DOCUMENTDAO_H