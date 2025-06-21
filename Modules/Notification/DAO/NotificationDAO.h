// Modules/Notification/DAO/NotificationDAO.h
#ifndef MODULES_NOTIFICATION_DAO_NOTIFICATIONDAO_H
#define MODULES_NOTIFICATION_DAO_NOTIFICATIONDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Notification/DTO/Notification.h" // For DTOs
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "Modules/Utils/DTOUtils.h" // For DTO conversion helpers
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <any>
#include <optional>

namespace ERP {
namespace Notification {
namespace DAOs {

class NotificationDAO : public ERP::DAOBase::DAOBase<ERP::Notification::DTO::NotificationDTO> {
public:
    explicit NotificationDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~NotificationDAO() override = default;

    // Override toMap and fromMap for NotificationDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Notification::DTO::NotificationDTO& dto) const override;
    ERP::Notification::DTO::NotificationDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    // tableName_ is now a member of DAOBase
};

} // namespace DAOs
} // namespace Notification
} // namespace ERP
#endif // MODULES_NOTIFICATION_DAO_NOTIFICATIONDAO_H