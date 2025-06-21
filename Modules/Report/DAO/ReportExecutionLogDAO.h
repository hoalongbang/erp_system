// Modules/Report/DAO/ReportExecutionLogDAO.h
#ifndef MODULES_REPORT_DAO_REPORTEXECUTIONLOGDAO_H
#define MODULES_REPORT_DAO_REPORTEXECUTIONLOGDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"            // Base DAO template
#include "ReportExecutionLog.h" // ReportExecutionLog DTO

namespace ERP {
namespace Report {
namespace DAOs {
/**
 * @brief DAO class for ReportExecutionLog entity.
 * Handles database operations for ReportExecutionLogDTO.
 */
class ReportExecutionLogDAO : public ERP::DAOBase::DAOBase<ERP::Report::DTO::ReportExecutionLogDTO> {
public:
    ReportExecutionLogDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
    ~ReportExecutionLogDAO() override = default;

    // Override base methods (optional, but good practice if custom logic is needed)
    bool save(const ERP::Report::DTO::ReportExecutionLogDTO& log) override;
    std::optional<ERP::Report::DTO::ReportExecutionLogDTO> findById(const std::string& id) override;
    bool update(const ERP::Report::DTO::ReportExecutionLogDTO& log) override;
    bool remove(const std::string& id) override;
    std::vector<ERP::Report::DTO::ReportExecutionLogDTO> findAll() override;

    // Specific methods for ReportExecutionLog
    std::vector<ERP::Report::DTO::ReportExecutionLogDTO> getReportExecutionLogsByRequestId(const std::string& reportRequestId);
    std::vector<ERP::Report::DTO::ReportExecutionLogDTO> getReportExecutionLogs(const std::map<std::string, std::any>& filters);
    int countReportExecutionLogs(const std::map<std::string, std::any>& filters);
    bool removeReportExecutionLogsByRequestId(const std::string& reportRequestId);

protected:
    // Required overrides for mapping between DTO and std::map<string, any>
    std::map<std::string, std::any> toMap(const ERP::Report::DTO::ReportExecutionLogDTO& log) const override;
    ERP::Report::DTO::ReportExecutionLogDTO fromMap(const std::map<std::string, std::any>& data) const override;

private:
    std::string tableName_ = "report_execution_logs";
};
} // namespace DAOs
} // namespace Report
} // namespace ERP
#endif // MODULES_REPORT_DAO_REPORTEXECUTIONLOGDAO_H