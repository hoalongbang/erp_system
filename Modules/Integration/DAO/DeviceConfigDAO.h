// Modules/Integration/DAO/DeviceConfigDAO.h
#ifndef MODULES_INTEGRATION_DAO_DEVICECONFIGDAO_H
#define MODULES_INTEGRATION_DAO_DEVICECONFIGDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"        // Base DAO template
#include "DeviceConfig.h"   // DeviceConfig DTO
#include "DeviceEventLog.h" // DeviceEventLog DTO (for related operations)

namespace ERP {
namespace Integration {
namespace DAOs {
/**
 * @brief DAO class for DeviceConfig entity.
 * Handles database operations for DeviceConfigDTO and related DeviceEventLogDTOs.
 */
class DeviceConfigDAO : public ERP::DAOBase::DAOBase<ERP::Integration::DTO::DeviceConfigDTO> {
public:
    DeviceConfigDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~DeviceConfigDAO() override = default;

    // Override base methods (optional, but good practice if custom logic is needed)
    bool save(const ERP::Integration::DTO::DeviceConfigDTO& config) override;
    std::optional<ERP::Integration::DTO::DeviceConfigDTO> findById(const std::string& id) override;
    bool update(const ERP::Integration::DTO::DeviceConfigDTO& config) override;
    bool remove(const std::string& id) override;
    std::vector<ERP::Integration::DTO::DeviceConfigDTO> findAll() override;

    // Specific methods for DeviceConfig and related DeviceEventLog
    std::optional<ERP::Integration::DTO::DeviceConfigDTO> getDeviceConfigByIdentifier(const std::string& identifier);
    std::vector<ERP::Integration::DTO::DeviceConfigDTO> getDeviceConfigs(const std::map<std::string, std::any>& filters);
    int countDeviceConfigs(const std::map<std::string, std::any>& filters);

    // DeviceEventLog operations (nested/related entities)
    bool createDeviceEventLog(const ERP::Integration::DTO::DeviceEventLogDTO& eventLog);
    std::vector<ERP::Integration::DTO::DeviceEventLogDTO> getDeviceEventLogsByDeviceId(const std::string& deviceId);
    std::vector<ERP::Integration::DTO::DeviceEventLogDTO> getDeviceEventLogs(const std::map<std::string, std::any>& filters);
    int countDeviceEventLogs(const std::map<std::string, std::any>& filters);
    bool removeDeviceEventLogsByDeviceId(const std::string& deviceId);

protected:
    // Required overrides for mapping between DTO and std::map<string, any>
    std::map<std::string, std::any> toMap(const ERP::Integration::DTO::DeviceConfigDTO& config) const override;
    ERP::Integration::DTO::DeviceConfigDTO fromMap(const std::map<std::string, std::any>& data) const override;

    // Mapping for DeviceEventLogDTO (internal helper, not part of base DAO template)
    std::map<std::string, std::any> deviceEventLogToMap(const ERP::Integration::DTO::DeviceEventLogDTO& eventLog) const;
    ERP::Integration::DTO::DeviceEventLogDTO deviceEventLogFromMap(const std::map<std::string, std::any>& data) const;


private:
    std::string tableName_ = "device_configs";
    std::string deviceEventLogTableName_ = "device_event_logs";
};
} // namespace DAOs
} // namespace Integration
} // namespace ERP
#endif // MODULES_INTEGRATION_DAO_DEVICECONFIGDAO_H