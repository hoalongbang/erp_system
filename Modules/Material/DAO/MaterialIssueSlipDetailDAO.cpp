// Modules/Material/DAO/MaterialIssueSlipDetailDAO.cpp
#include "MaterialIssueSlipDetailDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions (BaseDTO)

namespace ERP {
    namespace Material {
        namespace DAOs {

            MaterialIssueSlipDetailDAO::MaterialIssueSlipDetailDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Material::DTO::MaterialIssueSlipDetailDTO>(connectionPool, "material_issue_slip_details") {
                // DAOBase constructor handles connectionPool and tableName_ initialization
                ERP::Logger::Logger::getInstance().info("MaterialIssueSlipDetailDAO: Initialized.");
            }

            std::map<std::string, std::any> MaterialIssueSlipDetailDAO::toMap(const ERP::Material::DTO::MaterialIssueSlipDetailDTO& detail) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(detail); // BaseDTO fields

                data["material_issue_slip_id"] = detail.materialIssueSlipId;
                ERP::DAOHelpers::putOptionalString(data, "material_request_slip_detail_id", detail.materialRequestSlipDetailId); // MỚI: Thêm field này nếu có
                data["product_id"] = detail.productId;
                ERP::DAOHelpers::putOptionalString(data, "location_id", detail.locationId); // Location is optional in DTO
                data["requested_quantity"] = detail.requestedQuantity; // MỚI: Thêm field này nếu có
                data["issued_quantity"] = detail.issuedQuantity;
                ERP::DAOHelpers::putOptionalString(data, "lot_number", detail.lotNumber);
                ERP::DAOHelpers::putOptionalString(data, "serial_number", detail.serialNumber);
                ERP::DAOHelpers::putOptionalString(data, "inventory_transaction_id", detail.inventoryTransactionId);
                ERP::DAOHelpers::putOptionalString(data, "notes", detail.notes);
                data["is_fully_issued"] = detail.isFullyIssued;

                return data;
            }

            ERP::Material::DTO::MaterialIssueSlipDetailDTO MaterialIssueSlipDetailDAO::fromMap(const std::map<std::string, std::any>& data) const {
                ERP::Material::DTO::MaterialIssueSlipDetailDTO detail;
                ERP::Utils::DTOUtils::fromMap(data, detail); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "material_issue_slip_id", detail.materialIssueSlipId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "material_request_slip_detail_id", detail.materialRequestSlipDetailId); // MỚI
                    ERP::DAOHelpers::getPlainValue(data, "product_id", detail.productId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "location_id", detail.locationId); // Optional
                    ERP::DAOHelpers::getPlainValue(data, "requested_quantity", detail.requestedQuantity); // MỚI
                    ERP::DAOHelpers::getPlainValue(data, "issued_quantity", detail.issuedQuantity);
                    ERP::DAOHelpers::getOptionalStringValue(data, "lot_number", detail.lotNumber);
                    ERP::DAOHelpers::getOptionalStringValue(data, "serial_number", detail.serialNumber);
                    ERP::DAOHelpers::getOptionalStringValue(data, "inventory_transaction_id", detail.inventoryTransactionId);
                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", detail.notes);
                    ERP::DAOHelpers::getPlainValue(data, "is_fully_issued", detail.isFullyIssued); // Should be bool/int

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("MaterialIssueSlipDetailDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "MaterialIssueSlipDetailDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("MaterialIssueSlipDetailDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "MaterialIssueSlipDetailDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return detail;
            }

            bool MaterialIssueSlipDetailDAO::save(const ERP::Material::DTO::MaterialIssueSlipDetailDTO& detail) {
                return create(detail);
            }

            std::optional<ERP::Material::DTO::MaterialIssueSlipDetailDTO> MaterialIssueSlipDetailDAO::findById(const std::string& id) {
                return getById(id);
            }

            bool MaterialIssueSlipDetailDAO::update(const ERP::Material::DTO::MaterialIssueSlipDetailDTO& detail) {
                return DAOBase<ERP::Material::DTO::MaterialIssueSlipDetailDTO>::update(detail);
            }

            bool MaterialIssueSlipDetailDAO::remove(const std::string& id) {
                return DAOBase<ERP::Material::DTO::MaterialIssueSlipDetailDTO>::remove(id);
            }

            std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> MaterialIssueSlipDetailDAO::findAll() {
                return DAOBase<ERP::Material::DTO::MaterialIssueSlipDetailDTO>::findAll();
            }

            std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> MaterialIssueSlipDetailDAO::getMaterialIssueSlipDetailsBySlipId(const std::string& materialIssueSlipId) {
                std::map<std::string, std::any> filters;
                filters["material_issue_slip_id"] = materialIssueSlipId;
                return getMaterialIssueSlipDetails(filters);
            }

            std::vector<ERP::Material::DTO::MaterialIssueSlipDetailDTO> MaterialIssueSlipDetailDAO::getMaterialIssueSlipDetails(const std::map<std::string, std::any>& filters) {
                return get(filters); // Use templated get
            }

            int MaterialIssueSlipDetailDAO::countMaterialIssueSlipDetails(const std::map<std::string, std::any>& filters) {
                return count(filters); // Use templated count
            }

            bool MaterialIssueSlipDetailDAO::removeMaterialIssueSlipDetailsBySlipId(const std::string& materialIssueSlipId) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("MaterialIssueSlipDetailDAO::removeMaterialIssueSlipDetailsBySlipId: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + tableName_ + " WHERE material_issue_slip_id = :material_issue_slip_id;";
                std::map<std::string, std::any> params;
                params["material_issue_slip_id"] = materialIssueSlipId;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("MaterialIssueSlipDetailDAO::removeMaterialIssueSlipDetailsBySlipId: Failed to remove material issue slip details for material_issue_slip_id " + materialIssueSlipId + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove material issue slip details.", "Không thể xóa chi tiết phiếu xuất vật tư sản xuất.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

        } // namespace DAOs
    } // namespace Material
} // namespace ERP