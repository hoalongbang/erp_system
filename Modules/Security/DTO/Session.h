// Modules/Security/DTO/Session.h
#ifndef MODULES_SECURITY_DTO_SESSION_H
#define MODULES_SECURITY_DTO_SESSION_H
#include <string>
#include <optional>
#include <chrono>
#include "DataObjects/BaseDTO.h"   // ĐÃ SỬA: Dùng tên tệp trực tiếp
#include "Modules/Common/Common.h"    // ĐÃ SỬA: Dùng tên tệp trực tiếp
#include "Modules/Utils/Utils.h"     // ĐÃ SỬA: Dùng tên tệp trực tiếp
namespace ERP {
    namespace Security {
        namespace DTO {
            /**
             * @brief DTO for Session entity.
             * Represents an active user session in the system.
             */
            struct SessionDTO : public BaseDTO {
                std::string userId;         // Foreign key to UserDTO
                std::string token;          // Session token (e.g., JWT)
                std::chrono::system_clock::time_point expirationTime; // Thời gian hết hạn
                std::optional<std::string> ipAddress; // Địa chỉ IP
                std::optional<std::string> userAgent; // Thông tin trình duyệt/thiết bị
                std::optional<std::string> deviceInfo; // MỚI: Thông tin chi tiết thiết bị

                SessionDTO() = default;
                virtual ~SessionDTO() = default;
            };
        } // namespace DTO
    } // namespace Security
} // namespace ERP
#endif // MODULES_SECURITY_DTO_SESSION_H