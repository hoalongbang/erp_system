// Modules/Material/DAO/MaterialRequestSlipDetailDAO.cpp
#include "MaterialRequestSlipDetailDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions (BaseDTO)

namespace ERP {
    namespace Material {
        namespace DAOs {

            MaterialRequestSlipDetailDAO::MaterialRequestSlipDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Material::DTO::MaterialRequestSlipDetailDTO>(connectionPool, "material_request_slip_details") {
                // DAOBase constructor handles connectionPool and tableName_ initialization
                ERP::Logger::Logger::getInstance().info("MaterialRequestSlipDetailDAO: Initialized.");
            }

            std::map<std::string, std::any> MaterialRequestSlipDetailDAO::toMap(const ERP::Material::DTO::MaterialRequestSlipDetailDTO& detail) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(detail); // BaseDTO fields

                data["material_request_slip_id"] = detail.materialRequestSlipId;
                data["product_id"] = detail.productId;
                data["requested_quantity"] = detail.requestedQuantity;
                data["issued_quantity"] = detail.issuedQuantity;
                ERP::DAOHelpers::putOptionalString(data, "notes", detail.notes);
                data["is_fully_issued"] = detail.isFullyIssued;

                return data;
            }

            ERP::Material::DTO::MaterialRequestSlipDetailDTO MaterialRequestSlipDetailDAO::fromMap(const std::map<std::string, std::any>& data) const {
                ERP::Material::DTO::MaterialRequestSlipDetailDTO detail;
                ERP::Utils::DTOUtils::fromMap(data, detail); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "material_request_slip_id", detail.materialRequestSlipId);
                    ERP::DAOHelpers::getPlainValue(data, "product_id", detail.productId);
                    ERP::DAOHelpers::getPlainValue(data, "requested_quantity", detail.requestedQuantity);
                    ERP::DAOHelpers::getPlainValue(data, "issued_quantity", detail.issuedQuantity);
                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", detail.notes);
                    ERP::DAOHelpers::getPlainValue(data, "is_fully_issued", detail.isFullyIssued); // Should be bool/int

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("MaterialRequestSlipDetailDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "MaterialRequestSlipDetailDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("MaterialRequestSlipDetailDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "MaterialRequestSlipDetailDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return detail;
            }

            bool MaterialRequestSlipDetailDAO::save(const ERP::Material::DTO::MaterialRequestSlipDetailDTO& detail) {
                return create(detail);
            }

            std::optional<ERP::Material::DTO::MaterialRequestSlipDetailDTO> MaterialRequestSlipDetailDAO::findById(const std::string& id) {
                return getById(id);
            }

            bool MaterialRequestSlipDetailDAO::update(const ERP::Material::DTO::MaterialRequestSlipDetailDTO& detail) {
                return DAOBase<ERP::Material::DTO::MaterialRequestSlipDetailDTO>::update(detail);
            }

            bool MaterialRequestSlipDetailDAO::remove(const std::string& id) {
                return DAOBase<ERP::Material::DTO::MaterialRequestSlipDetailDTO>::remove(id);
            }

            std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> MaterialRequestSlipDetailDAO::findAll() {
                return DAOBase<ERP::Material::DTO::MaterialRequestSlipDetailDTO>::findAll();
            }

            std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> MaterialRequestSlipDetailDAO::getMaterialRequestSlipDetailsBySlipId(const std::string& requestSlipId) {
                std::map<std::string, std::any> filters;
                filters["material_request_slip_id"] = requestSlipId;
                return getMaterialRequestSlipDetails(filters);
            }

            std::vector<ERP::Material::DTO::MaterialRequestSlipDetailDTO> MaterialRequestSlipDetailDAO::getMaterialRequestSlipDetails(const std::map<std::string, std::any>& filters) {
                return get(filters); // Use templated get
            }

            int MaterialRequestSlipDetailDAO::countMaterialRequestSlipDetails(const std::map<std::string, std::any>& filters) {
                return count(filters); // Use templated count
            }

            bool MaterialRequestSlipDetailDAO::removeMaterialRequestSlipDetailsBySlipId(const std::string& requestSlipId) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("MaterialRequestSlipDetailDAO::removeMaterialRequestSlipDetailsBySlipId: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + tableName_ + " WHERE material_request_slip_id = :material_request_slip_id;";
                std::map<std::string, std::any> params;
                params["material_request_slip_id"] = requestSlipId;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("MaterialRequestSlipDetailDAO::removeMaterialRequestSlipDetailsBySlipId: Failed to remove material request slip details for request_slip_id " + requestSlipId + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove material request slip details.", "Không thể xóa chi tiết phiếu yêu cầu vật tư.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

        } // namespace DAOs
    } // namespace Material
} // namespace ERP