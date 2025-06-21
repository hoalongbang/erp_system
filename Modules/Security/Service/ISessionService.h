// Modules/Security/Service/ISessionService.h
#ifndef MODULES_SECURITY_SERVICE_ISESSIONSERVICE_H
#define MODULES_SECURITY_SERVICE_ISESSIONSERVICE_H
#include <string>
#include <vector>
#include <optional>
#include <map>    // For std::map<std::string, std::any>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "Session.h"       // DTO
#include "Common.h"        // Enum Common
#include "BaseService.h"   // Base Service

namespace ERP {
namespace Security {
namespace Services {

/**
 * @brief ISessionService interface defines operations for managing user sessions.
 */
class ISessionService {
public:
    virtual ~ISessionService() = default;
    /**
     * @brief Creates a new session.
     * @param sessionDTO DTO containing new session information.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional SessionDTO if creation is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Security::DTO::SessionDTO> createSession(
        const ERP::Security::DTO::SessionDTO& sessionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Retrieves session information by ID.
     * @param sessionId ID of the session to retrieve.
     * @param userRoleIds Roles of the user performing the operation.
     * @return An optional SessionDTO if found, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Security::DTO::SessionDTO> getSessionById(
        const std::string& sessionId,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves all sessions or sessions matching a filter.
     * @param filter Map of filter conditions.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching SessionDTOs.
     */
    virtual std::vector<ERP::Security::DTO::SessionDTO> getAllSessions(
        const std::map<std::string, std::any>& filter = {},
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Retrieves sessions for a specific user.
     * @param userIdToRetrieve ID of the user whose sessions are to be retrieved.
     * @param userRoleIds Roles of the user performing the operation.
     * @return Vector of matching SessionDTOs.
     */
    virtual std::vector<ERP::Security::DTO::SessionDTO> getSessionsForUser(
        const std::string& userIdToRetrieve,
        const std::vector<std::string>& userRoleIds = {}) = 0;
    /**
     * @brief Updates session information.
     * @param sessionDTO DTO containing updated session information (must have ID).
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if update is successful, false otherwise.
     */
    virtual bool updateSession(
        const ERP::Security::DTO::SessionDTO& sessionDTO,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deletes a session record by ID.
     * @param sessionId ID of the session to delete.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deletion is successful, false otherwise.
     */
    virtual bool deleteSession(
        const std::string& sessionId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
    /**
     * @brief Deactivates a session (marks as inactive without deleting).
     * @param sessionId ID of the session to deactivate.
     * @param currentUserId ID of the user performing the operation.
     * @param userRoleIds Roles of the user performing the operation.
     * @return true if deactivation is successful, false otherwise.
     */
    virtual bool deactivateSession(
        const std::string& sessionId,
        const std::string& currentUserId,
        const std::vector<std::string>& userRoleIds) = 0;
};

} // namespace Services
} // namespace Security
} // namespace ERP
#endif // MODULES_SECURITY_SERVICE_ISESSIONSERVICE_H