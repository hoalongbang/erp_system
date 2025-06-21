// Modules/TaskEngine/DAO/TaskLogDAO.h
#ifndef MODULES_TASKENGINE_DAO_TASKLOGDAO_H
#define MODULES_TASKENGINE_DAO_TASKLOGDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/TaskEngine/DTO/TaskLog.h" // For DTOs
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
namespace TaskEngine {
namespace DAOs {

class TaskLogDAO : public ERP::DAOBase::DAOBase<ERP::TaskEngine::DTO::TaskLogDTO> {
public:
    explicit TaskLogDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~TaskLogDAO() override = default;

    // Override toMap and fromMap for TaskLogDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::TaskEngine::DTO::TaskLogDTO& dto) const override;
    ERP::TaskEngine::DTO::TaskLogDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    // tableName_ is now a member of DAOBase
};

} // namespace DAOs
} // namespace TaskEngine
} // namespace ERP
#endif // MODULES_TASKENGINE_DAO_TASKLOGDAO_H