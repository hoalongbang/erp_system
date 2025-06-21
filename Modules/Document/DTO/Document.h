// Modules/Document/DTO/Document.h
#ifndef MODULES_DOCUMENT_DTO_DOCUMENT_H
#define MODULES_DOCUMENT_DTO_DOCUMENT_H
#include <string>
#include <optional>
#include <chrono>
#include <map> // For std::map<std::string, std::any> replacement of QJsonObject
#include <any> // For std::any replacement of QJsonObject
// #include <QJsonObject> // Removed
// #include <QString>     // Removed
// #include <QDateTime>   // Removed
#include "DataObjects/BaseDTO.h"
#include "Modules/Common/Common.h" // For EntityStatus
#include "Modules/Utils/Utils.h"   // For generateUUID
// No longer need Q_DECLARE_METATYPE here if DocumentDTO only uses standard C++ types
namespace ERP {
namespace Document {
namespace DTO {
/**
 * @brief Enum định nghĩa các loại tài liệu.
 */
enum class DocumentType {
    GENERAL = 0,    /**< Tài liệu chung (mặc định) */
    INVOICE = 1,    /**< Hóa đơn */
    CONTRACT = 2,   /**< Hợp đồng */
    DESIGN = 3,     /**< Bản vẽ/thiết kế kỹ thuật */
    MANUAL = 4,     /**< Sổ tay/hướng dẫn sử dụng */
    REPORT = 5,     /**< Báo cáo */
    QA_RECORD = 6,  /**< Hồ sơ QA/QC */
    PHOTO = 7,      /**< Hình ảnh */
    OTHER = 8       /**< Loại khác */
};
/**
 * @brief DTO for Document entity.
 * Represents a single digital document stored in the system.
 */
struct DocumentDTO : public BaseDTO {
    std::string fileName;          /**< Tên file gốc (ví dụ: contract_v1.pdf) */
    std::string internalFilePath;  /**< Đường dẫn lưu trữ nội bộ (có thể là đường dẫn tương đối hoặc ID trong hệ thống lưu trữ) */
    std::string mimeType;          /**< Loại MIME của file (ví dụ: application/pdf, image/jpeg) */
    long long fileSize;            /**< Kích thước file tính bằng byte (thay thế qint64) */
    DocumentType type;             /**< Loại tài liệu */
    std::optional<std::string> description; /**< Mô tả tài liệu */

    // Liên kết với bản ghi nghiệp vụ (Business Entity Linkage)
    std::optional<std::string> relatedEntityId;   /**< ID của bản ghi nghiệp vụ liên quan (ví dụ: ID SalesOrder) */
    std::optional<std::string> relatedEntityType; /**< Loại bản ghi nghiệp vụ (ví dụ: "SalesOrder", "Product", "Customer") */

    std::string uploadedByUserId;          /**< ID người dùng tải lên */
    std::chrono::system_clock::time_point uploadTime; /**< Thời gian tải lên */

    std::map<std::string, std::any> metadata; /**< Dữ liệu bổ sung dạng map<string, any> (ví dụ: version, tags, checksum) */
    bool isPublic = false;                      /**< Có công khai hay không */
    std::optional<std::string> storageLocation; /**< Vị trí lưu trữ vật lý/logic (e.g., "LocalDisk", "CloudStorage", "SharePoint") */

    // Constructors
    DocumentDTO() : BaseDTO(), fileName(""), internalFilePath(""), mimeType("application/octet-stream"),
                    fileSize(0), type(DocumentType::GENERAL), uploadTime(std::chrono::system_clock::now()),
                    isPublic(false) {}

    virtual ~DocumentDTO() = default;

    // Utility methods
    std::string getTypeString() const; // Chuyển đổi DocumentType sang chuỗi
};
} // namespace DTO
} // namespace Document
} // namespace ERP
#endif // MODULES_DOCUMENT_DTO_DOCUMENT_H