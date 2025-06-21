// Modules/Report/DAO/ReportDAO.h
#ifndef MODULES_REPORT_DAO_REPORTDAO_H
#define MODULES_REPORT_DAO_REPORTDAO_H
#include "DAOBase/DAOBase.h" // Include templated DAOBase
#include "Modules/Report/DTO/Report.h" // For DTOs
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
namespace Report {
namespace DAOs {

// ReportDAO handles two DTOs (ReportRequest and ReportExecutionLog).
// It will inherit from DAOBase for ReportRequestDTO, and
// have specific methods for ReportExecutionLogDTO.

class ReportDAO : public ERP::DAOBase::DAOBase<ERP::Report::DTO::ReportRequestDTO> {
public:
    explicit ReportDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~ReportDAO() override = default;

    // Override toMap and fromMap for ReportRequestDTO (handled by DAOBase template)
protected:
    std::map<std::string, std::any> toMap(const ERP::Report::DTO::ReportRequestDTO& dto) const override;
    ERP::Report::DTO::ReportRequestDTO fromMap(const std::map<std::string, std::any>& data) const override;

public:
    // Specific methods for ReportExecutionLogDTO
    bool createReportExecutionLog(const ERP::Report::DTO::ReportExecutionLogDTO& log);
    std::optional<ERP::Report::DTO::ReportExecutionLogDTO> getReportExecutionLogById(const std::string& id);
    std::vector<ERP::Report::DTO::ReportExecutionLogDTO> getReportExecutionLogsByRequestId(const std::string& requestId);
    bool updateReportExecutionLog(const ERP::Report::DTO::ReportExecutionLogDTO& log);
    bool removeReportExecutionLog(const std::string& id);
    bool removeReportExecutionLogsByRequestId(const std::string& requestId); // Remove all logs for a request

    // Helpers for ReportExecutionLogDTO conversion (static because not part of templated base)
    static std::map<std::string, std::any> toMap(const ERP::Report::DTO::ReportExecutionLogDTO& dto);
    static ERP::Report::DTO::ReportExecutionLogDTO fromMap(const std::map<std::string, std::any>& data);

private:
    std::string reportExecutionLogsTableName_ = "report_execution_logs"; // Table for execution logs
};

} // namespace DAOs
} // namespace Report
} // namespace ERP
#endif // MODULES_REPORT_DAO_REPORTDAO_H