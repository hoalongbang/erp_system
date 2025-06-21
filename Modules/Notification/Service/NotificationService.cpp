// Modules/Notification/Service/NotificationService.cpp
#include "NotificationService.h" // Đã rút gọn include
#include "Notification.h" // Đã rút gọn include
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
namespace Notification {
namespace Services {

NotificationService::NotificationService(
    std::shared_ptr<DAOs::NotificationDAO> notificationDAO,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      notificationDAO_(notificationDAO) {
    if (!notificationDAO_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "NotificationService: Initialized with null DAO.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ thông báo.");
        ERP::Logger::Logger::getInstance().critical("NotificationService: Injected NotificationDAO is null.");
        throw std::runtime_error("NotificationService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("NotificationService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Notification::DTO::NotificationDTO> NotificationService::createNotification(
    const ERP::Notification::DTO::NotificationDTO& notificationDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("NotificationService: Attempting to create notification for user: " + notificationDTO.userId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Notification.CreateNotification", "Bạn không có quyền tạo thông báo.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (notificationDTO.userId.empty() || notificationDTO.title.empty() || notificationDTO.message.empty()) {
        ERP::Logger::Logger::getInstance().warning("NotificationService: Invalid input for notification creation (empty userId, title, or message).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "NotificationService: Invalid input for notification creation.", "Thông tin thông báo không đầy đủ.");
        return std::nullopt;
    }

    ERP::Notification::DTO::NotificationDTO newNotification = notificationDTO;
    newNotification.id = ERP::Utils::generateUUID();
    newNotification.createdAt = ERP::Utils::DateUtils::now();
    newNotification.createdBy = currentUserId;
    newNotification.sentTime = newNotification.createdAt; // Set sentTime as creation time
    newNotification.isRead = false; // Default to unread

    std::optional<ERP::Notification::DTO::NotificationDTO> createdNotification = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!notificationDAO_->create(newNotification)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("NotificationService: Failed to create notification for user " + newNotification.userId + " in DAO.");
                return false;
            }
            createdNotification = newNotification;
            // Optionally, publish an event for new notification
            // eventBus_.publish(std::make_shared<EventBus::NotificationCreatedEvent>(newNotification));
            return true;
        },
        "NotificationService", "createNotification"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("NotificationService: Notification created successfully for user: " + newNotification.userId + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Notification", "Notification", newNotification.id, "Notification", newNotification.title,
                       std::nullopt, newNotification.toMap(), "Notification created for user: " + newNotification.userId + ".");
        return createdNotification;
    }
    return std::nullopt;
}

std::optional<ERP::Notification::DTO::NotificationDTO> NotificationService::getNotificationById(
    const std::string& notificationId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("NotificationService: Retrieving notification by ID: " + notificationId + ".");

    // Permission check: User can view their own notifications, or Admin can view any
    // This requires knowing the owner of the notification. Let's fetch it first.
    std::optional<ERP::Notification::DTO::NotificationDTO> notificationOpt = notificationDAO_->getById(notificationId);
    if (!notificationOpt) {
        ERP::Logger::Logger::getInstance().warning("NotificationService: Notification with ID " + notificationId + " not found.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy thông báo.");
        return std::nullopt;
    }

    if (notificationOpt->userId != currentUserId && !checkPermission(currentUserId, userRoleIds, "Notification.ViewAnyNotification", "Bạn không có quyền xem thông báo này.")) {
        return std::nullopt;
    }

    return notificationOpt; // Already fetched
}

std::vector<ERP::Notification::DTO::NotificationDTO> NotificationService::getAllNotifications(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("NotificationService: Retrieving all notifications with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Notification.ViewAllNotifications", "Bạn không có quyền xem tất cả thông báo.")) {
        return {};
    }

    return notificationDAO_->get(filter); // Using get from DAOBase template
}

std::vector<ERP::Notification::DTO::NotificationDTO> NotificationService::getNotificationsForUser(
    const std::string& userIdToRetrieve,
    bool includeRead,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("NotificationService: Retrieving notifications for user: " + userIdToRetrieve + " (Include Read: " + (includeRead ? "Yes" : "No") + ").");

    // Permission check: User can view their own notifications, or Admin/specific role can view others'
    if (userIdToRetrieve != currentUserId && !checkPermission(currentUserId, userRoleIds, "Notification.ViewUserNotifications", "Bạn không có quyền xem thông báo của người dùng khác.")) {
        return {};
    }

    std::map<std::string, std::any> filter;
    filter["user_id"] = userIdToRetrieve;
    if (!includeRead) {
        filter["is_read"] = false;
    }

    return notificationDAO_->get(filter); // Using get from DAOBase template
}

bool NotificationService::markNotificationAsRead(
    const std::string& notificationId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("NotificationService: Attempting to mark notification " + notificationId + " as read by " + currentUserId + ".");

    std::optional<ERP::Notification::DTO::NotificationDTO> notificationOpt = notificationDAO_->getById(notificationId);
    if (!notificationOpt) {
        ERP::Logger::Logger::getInstance().warning("NotificationService: Notification with ID " + notificationId + " not found to mark as read.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy thông báo để đánh dấu đã đọc.");
        return false;
    }

    ERP::Notification::DTO::NotificationDTO oldNotification = *notificationOpt;

    // Permission check: User can mark their own notifications as read, or Admin can mark any
    if (oldNotification.userId != currentUserId && !checkPermission(currentUserId, userRoleIds, "Notification.MarkAnyNotificationAsRead", "Bạn không có quyền đánh dấu thông báo này đã đọc.")) {
        return false;
    }

    if (oldNotification.isRead) {
        ERP::Logger::Logger::getInstance().info("NotificationService: Notification " + notificationId + " is already marked as read.");
        return true; // Already read
    }

    ERP::Notification::DTO::NotificationDTO updatedNotification = oldNotification;
    updatedNotification.isRead = true;
    updatedNotification.updatedAt = ERP::Utils::DateUtils::now();
    updatedNotification.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!notificationDAO_->update(updatedNotification)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("NotificationService: Failed to mark notification " + notificationId + " as read in DAO.");
                return false;
            }
            return true;
        },
        "NotificationService", "markNotificationAsRead"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("NotificationService: Notification " + notificationId + " marked as read successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Notification", "MarkAsRead", notificationId, "Notification", oldNotification.title,
                       oldNotification.toMap(), updatedNotification.toMap(), "Notification marked as read.");
        return true;
    }
    return false;
}

bool NotificationService::deleteNotification(
    const std::string& notificationId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("NotificationService: Attempting to delete notification: " + notificationId + " by " + currentUserId + ".");

    std::optional<ERP::Notification::DTO::NotificationDTO> notificationOpt = notificationDAO_->getById(notificationId);
    if (!notificationOpt) {
        ERP::Logger::Logger::getInstance().warning("NotificationService: Notification with ID " + notificationId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy thông báo cần xóa.");
        return false;
    }

    ERP::Notification::DTO::NotificationDTO notificationToDelete = *notificationOpt;

    // Permission check: User can delete their own notifications, or Admin can delete any
    if (notificationToDelete.userId != currentUserId && !checkPermission(currentUserId, userRoleIds, "Notification.DeleteAnyNotification", "Bạn không có quyền xóa thông báo này.")) {
        return false;
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!notificationDAO_->remove(notificationId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("NotificationService: Failed to delete notification " + notificationId + " in DAO.");
                return false;
            }
            return true;
        },
        "NotificationService", "deleteNotification"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("NotificationService: Notification " + notificationId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Notification", "Notification", notificationId, "Notification", notificationToDelete.title,
                       notificationToDelete.toMap(), std::nullopt, "Notification deleted.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Notification
} // namespace ERP