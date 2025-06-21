// Modules/Notification/Service/INotificationService.h
#ifndef MODULES_NOTIFICATION_SERVICE_INOTIFICATIONSERVICE_H
#define MODULES_NOTIFICATION_SERVICE_INOTIFICATIONSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "Notification.h"  // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

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

} // namespace Services
} // namespace Notification
} // namespace ERP
#endif // MODULES_NOTIFICATION_SERVICE_INOTIFICATIONSERVICE_H