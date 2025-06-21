// Modules/Catalog/DAO/PermissionDAO.h
#ifndef MODULES_CATALOG_DAO_PERMISSIONDAO_H
#define MODULES_CATALOG_DAO_PERMISSIONDAO_H
#include "DAOBase.h"     // Đã thêm 
#include <map>
#include <string>
#include <any>
#include <vector>
#include <optional>
#include <memory>

#include "DateUtils.h"   // Date conversion
// ĐÃ SỬA: Thay đổi tên tệp include từ "PermissionDTO.h" thành "Permission.h"
// Do CMakeLists.txt đã thêm Modules/Catalog/DTO vào include_directories,
// nên chỉ cần tên tệp là đủ.
#include "Permission.h" // DTO (File này chứa struct PermissionDTO)

namespace ERP {
    namespace Catalog {
        namespace DAOs {
            class PermissionDAO : public ERP::DAOBase::DAOBase {
            public:
                PermissionDAO();
                bool create(const std::map<std::string, std::any>& data) override;
                std::vector<std::map<std::string, std::any>> get(const std::map<std::string, std::any>& filter) override;
                bool update(const std::map<std::string, std::any>& filter, const std::map<std::string, std::any>& data) override;
                bool remove(const std::string& id) override;
                bool remove(const std::map<std::string, std::any>& filter) override;
                std::optional<std::map<std::string, std::any>> getById(const std::string& id) override;
                int count(const std::map<std::string, std::any>& filter) override;
                std::optional<std::map<std::string, std::any>> getByName(const std::string& name);
                // Các phương thức toMap và fromMap đã đúng khi tham chiếu đến ERP::Catalog::DTO::PermissionDTO,
                // vì đây là tên của struct được định nghĩa trong Permission.h.
                std::map<std::string, std::any> toMap(const ERP::Catalog::DTO::PermissionDTO& permission);
                ERP::Catalog::DTO::PermissionDTO fromMap(const std::map<std::string, std::any>& data);
            private:
                std::string tableName_ = "permissions";
            };
        } // namespace DAOs
    } // namespace Catalog
} // namespace ERP
#endif // MODULES_CATALOG_DAO_PERMISSIONDAO_H