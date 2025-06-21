// Modules/Material/DAO/IssueSlipDetailDAO.cpp
#include "IssueSlipDetailDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions (BaseDTO)

namespace ERP {
    namespace Material {
        namespace DAOs {

            IssueSlipDetailDAO::IssueSlipDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Material::DTO::IssueSlipDetailDTO>(connectionPool, "issue_slip_details") {
                // DAOBase constructor handles connectionPool and tableName_ initialization
                ERP::Logger::Logger::getInstance().info("IssueSlipDetailDAO: Initialized.");
            }

            std::map<std::string, std::any> IssueSlipDetailDAO::toMap(const ERP::Material::DTO::IssueSlipDetailDTO& detail) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(detail); // BaseDTO fields

                data["issue_slip_id"] = detail.issueSlipId;
                data["product_id"] = detail.productId;
                data["location_id"] = detail.locationId;
                data["requested_quantity"] = detail.requestedQuantity;
                data["issued_quantity"] = detail.issuedQuantity;
                ERP::DAOHelpers::putOptionalString(data, "lot_number", detail.lotNumber);
                ERP::DAOHelpers::putOptionalString(data, "serial_number", detail.serialNumber);
                ERP::DAOHelpers::putOptionalString(data, "notes", detail.notes);
                data["is_fully_issued"] = detail.isFullyIssued;
                ERP::DAOHelpers::putOptionalString(data, "inventory_transaction_id", detail.inventoryTransactionId);

                return data;
            }

            ERP::Material::DTO::IssueSlipDetailDTO IssueSlipDetailDAO::fromMap(const std::map<std::string, std::any>& data) const {
                ERP::Material::DTO::IssueSlipDetailDTO detail;
                ERP::Utils::DTOUtils::fromMap(data, detail); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "issue_slip_id", detail.issueSlipId);
                    ERP::DAOHelpers::getPlainValue(data, "product_id", detail.productId);
                    ERP::DAOHelpers::getPlainValue(data, "location_id", detail.locationId);
                    ERP::DAOHelpers::getPlainValue(data, "requested_quantity", detail.requestedQuantity);
                    ERP::DAOHelpers::getPlainValue(data, "issued_quantity", detail.issuedQuantity);
                    ERP::DAOHelpers::getOptionalStringValue(data, "lot_number", detail.lotNumber);
                    ERP::DAOHelpers::getOptionalStringValue(data, "serial_number", detail.serialNumber);
                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", detail.notes);
                    ERP::DAOHelpers::getPlainValue(data, "is_fully_issued", detail.isFullyIssued); // Should be bool/int
                    ERP::DAOHelpers::getOptionalStringValue(data, "inventory_transaction_id", detail.inventoryTransactionId);

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("IssueSlipDetailDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "IssueSlipDetailDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("IssueSlipDetailDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "IssueSlipDetailDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return detail;
            }

            bool IssueSlipDetailDAO::save(const ERP::Material::DTO::IssueSlipDetailDTO& detail) {
                return create(detail);
            }

            std::optional<ERP::Material::DTO::IssueSlipDetailDTO> IssueSlipDetailDAO::findById(const std::string& id) {
                return getById(id);
            }

            bool IssueSlipDetailDAO::update(const ERP::Material::DTO::IssueSlipDetailDTO& detail) {
                return DAOBase<ERP::Material::DTO::IssueSlipDetailDTO>::update(detail);
            }

            bool IssueSlipDetailDAO::remove(const std::string& id) {
                return DAOBase<ERP::Material::DTO::IssueSlipDetailDTO>::remove(id);
            }

            std::vector<ERP::Material::DTO::IssueSlipDetailDTO> IssueSlipDetailDAO::findAll() {
                return DAOBase<ERP::Material::DTO::IssueSlipDetailDTO>::findAll();
            }

            std::vector<ERP::Material::DTO::IssueSlipDetailDTO> IssueSlipDetailDAO::getIssueSlipDetailsBySlipId(const std::string& issueSlipId) {
                std::map<std::string, std::any> filters;
                filters["issue_slip_id"] = issueSlipId;
                return getIssueSlipDetails(filters);
            }

            std::vector<ERP::Material::DTO::IssueSlipDetailDTO> IssueSlipDetailDAO::getIssueSlipDetails(const std::map<std::string, std::any>& filters) {
                return get(filters); // Use templated get
            }

            int IssueSlipDetailDAO::countIssueSlipDetails(const std::map<std::string, std::any>& filters) {
                return count(filters); // Use templated count
            }

            bool IssueSlipDetailDAO::removeIssueSlipDetailsBySlipId(const std::string& issueSlipId) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("IssueSlipDetailDAO::removeIssueSlipDetailsBySlipId: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + tableName_ + " WHERE issue_slip_id = :issue_slip_id;";
                std::map<std::string, std::any> params;
                params["issue_slip_id"] = issueSlipId;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("IssueSlipDetailDAO::removeIssueSlipDetailsBySlipId: Failed to remove issue slip details for issue_slip_id " + issueSlipId + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove issue slip details.", "Không thể xóa chi tiết phiếu xuất kho.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

        } // namespace DAOs
    } // namespace Material
} // namespace ERP