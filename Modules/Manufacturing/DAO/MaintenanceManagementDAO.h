// Modules/Manufacturing/DAO/MaintenanceManagementDAO.h
#ifndef MODULES_MANUFACTURING_DAO_MAINTENANCEMANAGEMENTDAO_H
#define MODULES_MANUFACTURING_DAO_MAINTENANCEMANAGEMENTDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Manufacturing/DTO/MaintenanceManagement.h" // For DTOs
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
namespace Manufacturing {
namespace DAOs {

// MaintenanceManagementDAO handles two DTOs (Request and Activity).
// It will inherit from DAOBase for MaintenanceRequestDTO, and
// have specific methods for MaintenanceActivityDTO.

class MaintenanceManagementDAO : public ERP::DAOBase::DAOBase<ERP::Manufacturing::DTO::MaintenanceRequestDTO> {
public:
    explicit MaintenanceManagementDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~MaintenanceManagementDAO() override = default;

    // Override toMap and fromMap for MaintenanceRequestDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Manufacturing::DTO::MaintenanceRequestDTO& dto) const override;
    ERP::Manufacturing::DTO::MaintenanceRequestDTO fromMap(const std::map<std::string, std::any>& data) const override;

public:
    // Specific methods for MaintenanceActivityDTO
    bool createMaintenanceActivity(const ERP::Manufacturing::DTO::MaintenanceActivityDTO& activity);
    std::optional<ERP::Manufacturing::DTO::MaintenanceActivityDTO> getMaintenanceActivityById(const std::string& id);
    std::vector<ERP::Manufacturing::DTO::MaintenanceActivityDTO> getMaintenanceActivitiesByRequestId(const std::string& requestId);
    bool updateMaintenanceActivity(const ERP::Manufacturing::DTO::MaintenanceActivityDTO& activity);
    bool removeMaintenanceActivity(const std::string& id);
    bool removeMaintenanceActivitiesByRequestId(const std::string& requestId); // Remove all activities for a request

    // Helpers for MaintenanceActivityDTO conversion (static because not part of templated base)
    static std::map<std::string, std::any> toMap(const ERP::Manufacturing::DTO::MaintenanceActivityDTO& dto);
    static ERP::Manufacturing::DTO::MaintenanceActivityDTO fromMap(const std::map<std::string, std::any>& data);

private:
    std::string maintenanceActivitiesTableName_ = "maintenance_activities"; // Table for maintenance activities
};

} // namespace DAOs
} // namespace Manufacturing
} // namespace ERP
#endif // MODULES_MANUFACTURING_DAO_MAINTENANCEMANAGEMENTDAO_H