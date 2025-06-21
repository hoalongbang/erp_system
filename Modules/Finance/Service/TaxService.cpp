// Modules/Finance/Service/TaxService.cpp
#include "TaxService.h" // Đã rút gọn include
#include "TaxRate.h" // Đã rút gọn include
#include "Event.h" // Đã rút gọn include
#include "ConnectionPool.h" // Đã rút gọn include
#include "DBConnection.h" // Đã rút gọn include
#include "Common.h" // Đã rút gọn include
#include "Utils.h" // Đã rút gọn include
#include "DateUtils.h" // Đã rút gọn include
#include "AutoRelease.h" // Đã rút gọn include
#include "ISecurityManager.h" // Đã rút gọn include
#include "UserService.h" // Đã rút gọn include
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
#include "DTOUtils.h" // For mapToQJsonObject etc.

namespace ERP {
namespace Finance {
namespace Services {

TaxService::TaxService(
    std::shared_ptr<DAOs::TaxRateDAO> taxRateDAO,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Khởi tạo BaseService
      taxRateDAO_(taxRateDAO) {
    if (!taxRateDAO_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "TaxService: Initialized with null DAO.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ thuế.");
        ERP::Logger::Logger::getInstance().critical("TaxService: Injected TaxRateDAO is null.");
        throw std::runtime_error("TaxService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("TaxService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

std::optional<ERP::Finance::DTO::TaxRateDTO> TaxService::createTaxRate(
    const ERP::Finance::DTO::TaxRateDTO& taxRateDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("TaxService: Attempting to create tax rate: " + taxRateDTO.name + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.CreateTaxRate", "Bạn không có quyền tạo thuế suất.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (taxRateDTO.name.empty() || taxRateDTO.rate < 0) {
        ERP::Logger::Logger::getInstance().warning("TaxService: Invalid input for tax rate creation (empty name or negative rate).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "TaxService: Invalid input for tax rate creation.", "Tên hoặc thuế suất không hợp lệ.");
        return std::nullopt;
    }

    // Check if tax rate name already exists
    std::map<std::string, std::any> filterByName;
    filterByName["name"] = taxRateDTO.name;
    if (taxRateDAO_->count(filterByName) > 0) { // Using count from DAOBase template
        ERP::Logger::Logger::getInstance().warning("TaxService: Tax rate with name " + taxRateDTO.name + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "TaxService: Tax rate with name " + taxRateDTO.name + " already exists.", "Tên thuế suất đã tồn tại. Vui lòng chọn tên khác.");
        return std::nullopt;
    }

    ERP::Finance::DTO::TaxRateDTO newTaxRate = taxRateDTO;
    newTaxRate.id = ERP::Utils::generateUUID();
    newTaxRate.createdAt = ERP::Utils::DateUtils::now();
    newTaxRate.createdBy = currentUserId;
    newTaxRate.status = ERP::Common::EntityStatus::ACTIVE; // Default status

    std::optional<ERP::Finance::DTO::TaxRateDTO> createdTaxRate = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!taxRateDAO_->create(newTaxRate)) { // Using create from DAOBase template
                ERP::Logger::Logger::getInstance().error("TaxService: Failed to create tax rate " + newTaxRate.name + " in DAO.");
                return false;
            }
            createdTaxRate = newTaxRate;
            eventBus_.publish(std::make_shared<EventBus::TaxRateCreatedEvent>(newTaxRate.id, newTaxRate.name, newTaxRate.rate));
            return true;
        },
        "TaxService", "createTaxRate"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("TaxService: Tax rate " + newTaxRate.name + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Finance", "TaxRate", newTaxRate.id, "TaxRate", newTaxRate.name,
                       std::nullopt, newTaxRate.toMap(), "Tax rate created.");
        return createdTaxRate;
    }
    return std::nullopt;
}

std::optional<ERP::Finance::DTO::TaxRateDTO> TaxService::getTaxRateById(
    const std::string& taxRateId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("TaxService: Retrieving tax rate by ID: " + taxRateId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewTaxRates", "Bạn không có quyền xem thuế suất.")) {
        return std::nullopt;
    }

    return taxRateDAO_->getById(taxRateId); // Using getById from DAOBase template
}

std::optional<ERP::Finance::DTO::TaxRateDTO> TaxService::getTaxRateByName(
    const std::string& taxRateName,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("TaxService: Retrieving tax rate by name: " + taxRateName + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewTaxRates", "Bạn không có quyền xem thuế suất.")) {
        return std::nullopt;
    }

    std::map<std::string, std::any> filter;
    filter["name"] = taxRateName;
    std::vector<ERP::Finance::DTO::TaxRateDTO> taxRates = taxRateDAO_->get(filter); // Using get from DAOBase template
    if (!taxRates.empty()) {
        return taxRates[0];
    }
    ERP::Logger::Logger::getInstance().debug("TaxService: Tax rate with name " + taxRateName + " not found.");
    return std::nullopt;
}

std::vector<ERP::Finance::DTO::TaxRateDTO> TaxService::getAllTaxRates(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("TaxService: Retrieving all tax rates with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewTaxRates", "Bạn không có quyền xem tất cả thuế suất.")) {
        return {};
    }

    return taxRateDAO_->get(filter); // Using get from DAOBase template
}

bool TaxService::updateTaxRate(
    const ERP::Finance::DTO::TaxRateDTO& taxRateDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("TaxService: Attempting to update tax rate: " + taxRateDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.UpdateTaxRate", "Bạn không có quyền cập nhật thuế suất.")) {
        return false;
    }

    std::optional<ERP::Finance::DTO::TaxRateDTO> oldTaxRateOpt = taxRateDAO_->getById(taxRateDTO.id); // Using getById from DAOBase
    if (!oldTaxRateOpt) {
        ERP::Logger::Logger::getInstance().warning("TaxService: Tax rate with ID " + taxRateDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy thuế suất cần cập nhật.");
        return false;
    }

    // If tax rate name is changed, check for uniqueness
    if (taxRateDTO.name != oldTaxRateOpt->name) {
        std::map<std::string, std::any> filterByName;
        filterByName["name"] = taxRateDTO.name;
        if (taxRateDAO_->count(filterByName) > 0) { // Using count from DAOBase
            ERP::Logger::Logger::getInstance().warning("TaxService: New tax rate name " + taxRateDTO.name + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "TaxService: New tax rate name " + taxRateDTO.name + " already exists.", "Tên thuế suất mới đã tồn tại. Vui lòng chọn tên khác.");
            return false;
        }
    }

    ERP::Finance::DTO::TaxRateDTO updatedTaxRate = taxRateDTO;
    updatedTaxRate.updatedAt = ERP::Utils::DateUtils::now();
    updatedTaxRate.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!taxRateDAO_->update(updatedTaxRate)) { // Using update from DAOBase template
                ERP::Logger::Logger::getInstance().error("TaxService: Failed to update tax rate " + updatedTaxRate.id + " in DAO.");
                return false;
            }
            eventBus_.publish(std::make_shared<EventBus::TaxRateUpdatedEvent>(updatedTaxRate.id, updatedTaxRate.name, updatedTaxRate.rate));
            return true;
        },
        "TaxService", "updateTaxRate"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("TaxService: Tax rate " + updatedTaxRate.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Finance", "TaxRate", updatedTaxRate.id, "TaxRate", updatedTaxRate.name,
                       oldTaxRateOpt->toMap(), updatedTaxRate.toMap(), "Tax rate updated.");
        return true;
    }
    return false;
}

bool TaxService::deleteTaxRate(
    const std::string& taxRateId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("TaxService: Attempting to delete tax rate: " + taxRateId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.DeleteTaxRate", "Bạn không có quyền xóa thuế suất.")) {
        return false;
    }

    std::optional<ERP::Finance::DTO::TaxRateDTO> taxRateOpt = taxRateDAO_->getById(taxRateId); // Using getById from DAOBase
    if (!taxRateOpt) {
        ERP::Logger::Logger::getInstance().warning("TaxService: Tax rate with ID " + taxRateId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy thuế suất cần xóa.");
        return false;
    }

    ERP::Finance::DTO::TaxRateDTO taxRateToDelete = *taxRateOpt;

    // Additional checks: Prevent deletion if tax rate is currently in use by any active sales orders, invoices, products, etc.
    // This requires dependencies on SalesOrderService, SalesInvoiceService, ProductService.
    // For simplicity, this check is omitted here, but would be crucial in a real system.

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!taxRateDAO_->remove(taxRateId)) { // Using remove from DAOBase template
                ERP::Logger::Logger::getInstance().error("TaxService: Failed to delete tax rate " + taxRateId + " in DAO.");
                return false;
            }
            return true;
        },
        "TaxService", "deleteTaxRate"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("TaxService: Tax rate " + taxRateId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Finance", "TaxRate", taxRateId, "TaxRate", taxRateToDelete.name,
                       taxRateToDelete.toMap(), std::nullopt, "Tax rate deleted.");
        return true;
    }
    return false;
}

} // namespace Services
} // namespace Finance
} // namespace ERP