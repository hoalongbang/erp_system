// Modules/Security/Service/IAuthenticationService.h
#ifndef MODULES_SECURITY_SERVICE_IAUTHENTICATIONSERVICE_H
#define MODULES_SECURITY_SERVICE_IAUTHENTICATIONSERVICE_H
#include <string>
#include <optional>
#include <vector>
#include <chrono> // For std::chrono::system_clock::time_point

// Rút gọn các include paths
#include "Session.h"       // DTO
#include "Common.h"        // Enum Common

namespace ERP {
namespace Security {
namespace Services {

/**
 * @brief IAuthenticationService interface defines operations for user authentication.
 */
class IAuthenticationService {
public:
    virtual ~IAuthenticationService() = default;
    /**
     * @brief Authenticates a user by username and password.
     * @param username The user's username.
     * @param password The user's plain text password.
     * @param ipAddress The IP address of the login attempt.
     * @param userAgent The user agent string (e.g., browser info).
     * @param deviceInfo Additional device information.
     * @return An optional SessionDTO if authentication is successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Security::DTO::SessionDTO> authenticate(
        const std::string& username,
        const std::string& password,
        const std::optional<std::string>& ipAddress = std::nullopt,
        const std::optional<std::string>& userAgent = std::nullopt,
        const std::optional<std::string>& deviceInfo = std::nullopt) = 0;
    /**
     * @brief Logs out a user by invalidating their session.
     * @param sessionId The ID of the session to invalidate.
     * @return true if logout is successful, false otherwise.
     */
    virtual bool logout(const std::string& sessionId) = 0;
    /**
     * @brief Validates a session token.
     * @param token The session token to validate.
     * @return An optional SessionDTO if the token is valid and active, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Security::DTO::SessionDTO> validateSession(const std::string& token) = 0;
    /**
     * @brief Refreshes an existing session, extending its expiration time.
     * @param sessionId The ID of the session to refresh.
     * @return An optional updated SessionDTO if successful, std::nullopt otherwise.
     */
    virtual std::optional<ERP::Security::DTO::SessionDTO> refreshSession(const std::string& sessionId) = 0;
};

} // namespace Services
} // namespace Security
} // namespace ERP
#endif // MODULES_SECURITY_SERVICE_IAUTHENTICATIONSERVICE_H