// Modules/Notification/Service/NotificationService.h
#ifndef MODULES_NOTIFICATION_SERVICE_NOTIFICATIONSERVICE_H
#define MODULES_NOTIFICATION_SERVICE_NOTIFICATIONSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <set> // For permissions

#include "BaseService.h"      // NEW: Kế thừa từ BaseService
#include "Notification.h"     // Đã rút gọn include
#include "NotificationDAO.h"  // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "EventBus.h"         // Đã rút gọn include
#include "Logger.h"           // Đã rút gọn include
#include "ErrorHandler.h"     // Đã rút gọn include
#include "Common.h"           // Đã rút gọn include
#include "Utils.h"            // Đã rút gọn include
#include "DateUtils.h"        // Đã rút gọn include

namespace ERP {
namespace Notification {
namespace Services {

/**
 * @brief INotificationService interface defines operations for managing notifications.
 */
class INotificationService {
public:
    virtual ~INotificationService() = default;
    /**
     * @brief Creates a new notification.
     * @param notificationDTO DTO containing new notification information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional NotificationDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Notification::DTO::NotificationDTO> createNotification(
        const ERP::Notification::DTO::NotificationDTO& notificationDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves notification information by ID.
     * @param notificationId ID of the notification to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional NotificationDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Notification::DTO::NotificationDTO> getNotificationById(
        const std::string& notificationId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all notifications or notifications matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching NotificationDTOs.
     */
    virtual std::vector<ERP::Notification::DTO::NotificationDTO> getAllNotifications(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves notifications for a specific user.
     * @param userIdToRetrieve ID of the user whose notifications are to be retrieved.
     * @param includeRead True to include read notifications, false to only include unread.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching NotificationDTOs.
     */
    virtual std::vector<ERP::Notification::DTO::NotificationDTO> getNotificationsForUser(
        const std::string& userIdToRetrieve,
        bool includeRead,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Marks a notification as read.
     * @param notificationId ID of the notification to mark as read.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if successful, false otherwise.
     */
    virtual bool markNotificationAsRead(
        const std::string& notificationId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a notification record by ID.
     * @param notificationId ID of the notification to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteNotification(
        const std::string& notificationId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};
/**
 * @brief Default implementation of INotificationService.
 * This class uses NotificationDAO and ISecurityManager.
 */
class NotificationService : public INotificationService, public ERP::Common::Services::BaseService {
public:
    /**
     * @brief Constructor for NotificationService.
     * @param notificationDAO Shared pointer to NotificationDAO.
     * @param authorizationService Shared pointer to IAuthorizationService.
     * @param auditLogService Shared pointer to IAuditLogService.
     * @param connectionPool Shared pointer to ConnectionPool.
     * @param securityManager Shared pointer to ISecurityManager.
     */
    NotificationService(std::shared_ptr<DAOs::NotificationDAO> notificationDAO,
                        std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
                        std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
                        std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
                        std::shared_ptr<ERP::Security::ISecurityManager> securityManager);

    std::optional<ERP::Notification::DTO::NotificationDTO> createNotification(
        const ERP::Notification::DTO::NotificationDTO& notificationDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    std::optional<ERP::Notification::DTO::NotificationDTO> getNotificationById(
        const std::string& notificationId,
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Notification::DTO::NotificationDTO> getAllNotifications(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) override;
    std::vector<ERP::Notification::DTO::NotificationDTO> getNotificationsForUser(
        const std::string& userIdToRetrieve,
        bool includeRead,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool markNotificationAsRead(
        const std::string& notificationId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;
    bool deleteNotification(
        const std::string& notificationId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) override;

private:
    std::shared_ptr<DAOs::NotificationDAO> notificationDAO_;
    // Inherited: authorizationService_, auditLogService_, connectionPool_, securityManager_

    // Old private helper functions removed as they are now in BaseService
};
} // namespace Services
} // namespace Notification
} // namespace ERP
#endif // MODULES_NOTIFICATION_SERVICE_NOTIFICATIONSERVICE_H