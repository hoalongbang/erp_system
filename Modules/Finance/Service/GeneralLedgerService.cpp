// Modules/Finance/Service/GeneralLedgerService.cpp
#include "GeneralLedgerService.h" // Standard includes
#include "GeneralLedgerAccount.h"
#include "GLAccountBalance.h"
#include "JournalEntry.h"
#include "JournalEntryDetail.h"
#include "Event.h"
#include "ConnectionPool.h"
#include "DBConnection.h"
#include "Common.h"
#include "Utils.h"
#include "DateUtils.h"
#include "AutoRelease.h"
#include "ISecurityManager.h"
#include "UserService.h"
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of if needed
// #include "DTOUtils.h" // Not needed here for QJsonObject conversions anymore

namespace ERP {
namespace Finance {
namespace Services {

GeneralLedgerService::GeneralLedgerService(
    std::shared_ptr<DAOs::GeneralLedgerDAO> glDAO,
    std::shared_ptr<ERP::Security::Service::IAuthorizationService> authorizationService,
    std::shared_ptr<ERP::Security::Service::IAuditLogService> auditLogService,
    std::shared_ptr<ERP::Database::ConnectionPool> connectionPool,
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : BaseService(authorizationService, auditLogService, connectionPool, securityManager), // Initialize BaseService
      glDAO_(glDAO) {
    if (!glDAO_) { // BaseService checks its own dependencies
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::ServerError, "GeneralLedgerService: Initialized with null DAO.", "Lỗi hệ thống trong quá trình khởi tạo dịch vụ sổ cái chung.");
        ERP::Logger::Logger::getInstance().critical("GeneralLedgerService: Injected GeneralLedgerDAO is null.");
        throw std::runtime_error("GeneralLedgerService: Null dependencies.");
    }
    ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Initialized.");
}

// Old checkUserPermission and getUserRoleIds removed as they are now in BaseService

// Helper to update GL account balance
bool GeneralLedgerService::updateGLAccountBalance(const std::string& glAccountId, double debitAmount, double creditAmount, ERP::Database::DBConnection& db_conn) {
    std::map<std::string, std::any> filter;
    filter["gl_account_id"] = glAccountId;
    std::vector<ERP::Finance::DTO::GLAccountBalanceDTO> existingBalances = glDAO_->getGLAccountBalances(filter); // Using get from DAOBase

    if (existingBalances.empty()) {
        // Create new balance record
        ERP::Finance::DTO::GLAccountBalanceDTO newBalance;
        newBalance.id = ERP::Utils::generateUUID();
        newBalance.glAccountId = glAccountId;
        newBalance.currentDebitBalance = debitAmount;
        newBalance.currentCreditBalance = creditAmount;
        newBalance.currency = "VND"; // Default currency
        newBalance.lastPostedDate = ERP::Utils::DateUtils::now();
        newBalance.createdAt = newBalance.lastPostedDate;
        newBalance.createdBy = "system"; // Or the currentUserId
        newBalance.status = ERP::Common::EntityStatus::ACTIVE;

        if (!glDAO_->createGLAccountBalance(newBalance)) { // Using create from DAOBase
            ERP::Logger::Logger::getInstance().error("GeneralLedgerService: Failed to create new GL account balance for " + glAccountId);
            return false;
        }
        ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Created new GL account balance for " + glAccountId + ". Debit: " + std::to_string(debitAmount) + ", Credit: " + std::to_string(creditAmount));
    } else {
        // Update existing balance
        ERP::Finance::DTO::GLAccountBalanceDTO existingBalance = existingBalances[0];
        existingBalance.currentDebitBalance += debitAmount;
        existingBalance.currentCreditBalance += creditAmount;
        existingBalance.lastPostedDate = ERP::Utils::DateUtils::now();
        existingBalance.updatedAt = existingBalance.lastPostedDate;
        existingBalance.updatedBy = "system"; // Or the currentUserId

        if (!glDAO_->updateGLAccountBalance(existingBalance)) { // Using update from DAOBase
            ERP::Logger::Logger::getInstance().error("GeneralLedgerService: Failed to update GL account balance for " + glAccountId);
            return false;
        }
        ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Updated GL account balance for " + glAccountId + ". New Debit: " + std::to_string(existingBalance.currentDebitBalance) + ", New Credit: " + std::to_string(existingBalance.currentCreditBalance));
    }
    return true;
}

std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> GeneralLedgerService::createGLAccount(
    const ERP::Finance::DTO::GeneralLedgerAccountDTO& glAccountDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Attempting to create GL account: " + glAccountDTO.accountNumber + " - " + glAccountDTO.accountName + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.CreateGLAccount", "Bạn không có quyền tạo tài khoản sổ cái chung.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (glAccountDTO.accountNumber.empty() || glAccountDTO.accountName.empty()) {
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: Invalid input for GL account creation (empty number or name).");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "GeneralLedgerService: Invalid input for GL account creation.", "Số hoặc tên tài khoản không được để trống.");
        return std::nullopt;
    }

    // Check if account number already exists
    std::map<std::string, std::any> filterByNumber;
    filterByNumber["account_number"] = glAccountDTO.accountNumber;
    if (glDAO_->countGLAccounts(filterByNumber) > 0) { // Specific DAO method
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: GL account with number " + glAccountDTO.accountNumber + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "GeneralLedgerService: GL account with number " + glAccountDTO.accountNumber + " already exists.", "Số tài khoản sổ cái chung đã tồn tại. Vui lòng chọn số khác.");
        return std::nullopt;
    }

    // Validate parent account existence if specified
    if (glAccountDTO.parentAccountId && (!getGLAccountById(*glAccountDTO.parentAccountId, userRoleIds))) {
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: Parent GL account " + *glAccountDTO.parentAccountId + " not found for GL account creation.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Tài khoản cha không tồn tại.");
        return std::nullopt;
    }

    ERP::Finance::DTO::GeneralLedgerAccountDTO newGLAccount = glAccountDTO;
    newGLAccount.id = ERP::Utils::generateUUID();
    newGLAccount.createdAt = ERP::Utils::DateUtils::now();
    newGLAccount.createdBy = currentUserId;
    newGLAccount.status = ERP::Common::EntityStatus::ACTIVE; // Default status

    std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> createdGLAccount = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!glDAO_->createGLAccount(newGLAccount)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("GeneralLedgerService: Failed to create GL account " + newGLAccount.accountNumber + " in DAO.");
                return false;
            }
            createdGLAccount = newGLAccount;
            // Optionally, create an initial balance record for this account
            // updateGLAccountBalance(newGLAccount.id, 0.0, 0.0, *db_conn); // Initialize with zero balance
            return true;
        },
        "GeneralLedgerService", "createGLAccount"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("GeneralLedgerService: GL account " + newGLAccount.accountNumber + " created successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Finance", "GLAccount", newGLAccount.id, "GLAccount", newGLAccount.accountNumber,
                       std::nullopt, newGLAccount.toMap(), "GL account created.");
        return createdGLAccount;
    }
    return std::nullopt;
}

std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> GeneralLedgerService::getGLAccountById(
    const std::string& glAccountId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("GeneralLedgerService: Retrieving GL account by ID: " + glAccountId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewGLAccounts", "Bạn không có quyền xem tài khoản sổ cái chung.")) {
        return std::nullopt;
    }

    return glDAO_->getGLAccountById(glAccountId); // Specific DAO method
}

std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> GeneralLedgerService::getGLAccountByNumber(
    const std::string& accountNumber,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().debug("GeneralLedgerService: Retrieving GL account by number: " + accountNumber + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewGLAccounts", "Bạn không có quyền xem tài khoản sổ cái chung.")) {
        return std::nullopt;
    }

    return glDAO_->getGLAccountByNumber(accountNumber); // Specific DAO method
}

std::vector<ERP::Finance::DTO::GeneralLedgerAccountDTO> GeneralLedgerService::getAllGLAccounts(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Retrieving all GL accounts with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewGLAccounts", "Bạn không có quyền xem tất cả tài khoản sổ cái chung.")) {
        return {};
    }

    return glDAO_->getGLAccounts(filter); // Specific DAO method
}

bool GeneralLedgerService::updateGLAccount(
    const ERP::Finance::DTO::GeneralLedgerAccountDTO& glAccountDTO,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Attempting to update GL account: " + glAccountDTO.id + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.UpdateGLAccount", "Bạn không có quyền cập nhật tài khoản sổ cái chung.")) {
        return false;
    }

    std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> oldGLAccountOpt = glDAO_->getGLAccountById(glAccountDTO.id, userRoleIds); // Specific DAO method
    if (!oldGLAccountOpt) {
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: GL account with ID " + glAccountDTO.id + " not found for update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy tài khoản sổ cái chung cần cập nhật.");
        return false;
    }

    // If account number is changed, check for uniqueness
    if (glAccountDTO.accountNumber != oldGLAccountOpt->accountNumber) {
        std::map<std::string, std::any> filterByNumber;
        filterByNumber["account_number"] = glAccountDTO.accountNumber;
        if (glDAO_->countGLAccounts(filterByNumber) > 0) { // Specific DAO method
            ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: New account number " + glAccountDTO.accountNumber + " already exists.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "GeneralLedgerService: New account number " + glAccountDTO.accountNumber + " already exists.", "Số tài khoản sổ cái chung mới đã tồn tại. Vui lòng chọn số khác.");
            return false;
        }
    }

    // Validate parent account existence if specified or changed
    if (glAccountDTO.parentAccountId != oldGLAccountOpt->parentAccountId) { // Only check if changed
        if (glAccountDTO.parentAccountId && (!getGLAccountById(*glAccountDTO.parentAccountId, userRoleIds))) {
            ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: Parent GL account " + *glAccountDTO.parentAccountId + " not found for GL account update.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Tài khoản cha không tồn tại.");
            return false;
        }
    }
    // Prevent setting self as parent account
    if (glAccountDTO.parentAccountId && *glAccountDTO.parentAccountId == glAccountDTO.id) {
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: Cannot set GL account " + glAccountDTO.id + " as its own parent.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Không thể đặt tài khoản làm tài khoản cha của chính nó.");
        return false;
    }

    ERP::Finance::DTO::GeneralLedgerAccountDTO updatedGLAccount = glAccountDTO;
    updatedGLAccount.updatedAt = ERP::Utils::DateUtils::now();
    updatedGLAccount.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!glDAO_->updateGLAccount(updatedGLAccount)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("GeneralLedgerService: Failed to update GL account " + updatedGLAccount.id + " in DAO.");
                return false;
            }
            // Optionally, publish event for GL account update
            // eventBus_.publish(std::make_shared<EventBus::GLAccountUpdatedEvent>(updatedGLAccount));
            return true;
        },
        "GeneralLedgerService", "updateGLAccount"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("GeneralLedgerService: GL account " + updatedGLAccount.id + " updated successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Finance", "GLAccount", updatedGLAccount.id, "GLAccount", updatedGLAccount.accountNumber,
                       oldGLAccountOpt->toMap(), updatedGLAccount.toMap(), "GL account updated.");
        return true;
    }
    return false;
}

bool GeneralLedgerService::updateGLAccountStatus(
    const std::string& glAccountId,
    ERP::Common::EntityStatus newStatus,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Attempting to update status for GL account: " + glAccountId + " to " + ERP::Common::entityStatusToString(newStatus) + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.UpdateGLAccount", "Bạn không có quyền cập nhật trạng thái tài khoản sổ cái chung.")) {
        return false;
    }

    std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> glAccountOpt = glDAO_->getGLAccountById(glAccountId, userRoleIds); // Specific DAO method
    if (!glAccountOpt) {
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: GL account with ID " + glAccountId + " not found for status update.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy tài khoản sổ cái chung để cập nhật trạng thái.");
        return false;
    }
    
    ERP::Finance::DTO::GeneralLedgerAccountDTO oldGLAccount = *glAccountOpt;
    if (oldGLAccount.status == newStatus) {
        ERP::Logger::Logger::getInstance().info("GeneralLedgerService: GL account " + glAccountId + " is already in status " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true; // Already in desired status
    }

    // Add state transition validation logic here
    // For example: Cannot change from COMPLETED to IN_PROGRESS.

    ERP::Finance::DTO::GeneralLedgerAccountDTO updatedGLAccount = oldGLAccount;
    updatedGLAccount.status = newStatus;
    updatedGLAccount.updatedAt = ERP::Utils::DateUtils::now();
    updatedGLAccount.updatedBy = currentUserId;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!glDAO_->updateGLAccount(updatedGLAccount)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("GeneralLedgerService: Failed to update status for GL account " + glAccountId + " in DAO.");
                return false;
            }
            // Optionally, publish event for status change
            // eventBus_.publish(std::make_shared<EventBus::GLAccountStatusChangedEvent>(glAccountId, newStatus));
            return true;
        },
        "GeneralLedgerService", "updateGLAccountStatus"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Status for GL account " + glAccountId + " updated successfully to " + ERP::Common::entityStatusToString(newStatus) + ".");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::UPDATE, ERP::Common::LogSeverity::INFO,
                       "Finance", "GLAccountStatus", glAccountId, "GLAccount", oldGLAccount.accountNumber,
                       oldGLAccount.toMap(), updatedGLAccount.toMap(), "GL account status changed to " + ERP::Common::entityStatusToString(newStatus) + ".");
        return true;
    }
    return false;
}

bool GeneralLedgerService::deleteGLAccount(
    const std::string& glAccountId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Attempting to delete GL account: " + glAccountId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.DeleteGLAccount", "Bạn không có quyền xóa tài khoản sổ cái chung.")) {
        return false;
    }

    std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> glAccountOpt = glDAO_->getGLAccountById(glAccountId, userRoleIds); // Specific DAO method
    if (!glAccountOpt) {
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: GL account with ID " + glAccountId + " not found for deletion.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy tài khoản sổ cái chung cần xóa.");
        return false;
    }

    ERP::Finance::DTO::GeneralLedgerAccountDTO glAccountToDelete = *glAccountOpt;

    // Additional checks: Prevent deletion if GL account has associated balances or journal entries
    std::map<std::string, std::any> balanceFilter;
    balanceFilter["gl_account_id"] = glAccountId;
    if (glDAO_->countGLAccountBalances(balanceFilter) > 0) { // Using count from DAOBase for balances
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: Cannot delete GL account " + glAccountId + " as it has associated balances.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Không thể xóa tài khoản sổ cái chung có số dư liên quan.");
        return false;
    }
    // Check for journal entries that use this GL account (will require iterating details)
    // This could be a complex query or service call, omitted for brevity.

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!glDAO_->removeGLAccount(glAccountId)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("GeneralLedgerService: Failed to delete GL account " + glAccountId + " in DAO.");
                return false;
            }
            return true;
        },
        "GeneralLedgerService", "deleteGLAccount"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("GeneralLedgerService: GL account " + glAccountId + " deleted successfully.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::DELETE, ERP::Common::LogSeverity::INFO,
                       "Finance", "GLAccount", glAccountId, "GLAccount", glAccountToDelete.accountNumber,
                       glAccountToDelete.toMap(), std::nullopt, "GL account deleted.");
        return true;
    }
    return false;
}

std::optional<ERP::Finance::DTO::JournalEntryDTO> GeneralLedgerService::createJournalEntry(
    const ERP::Finance::DTO::JournalEntryDTO& journalEntryDTO,
    const std::vector<ERP::Finance::DTO::JournalEntryDetailDTO>& journalEntryDetails,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Attempting to create journal entry: " + journalEntryDTO.journalNumber + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.CreateJournalEntry", "Bạn không có quyền tạo bút toán nhật ký.")) {
        return std::nullopt;
    }

    // 1. Validate input DTO
    if (journalEntryDTO.journalNumber.empty() || journalEntryDTO.description.empty() || journalEntryDetails.empty()) {
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: Invalid input for journal entry creation.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Thông tin bút toán nhật ký không đầy đủ.");
        return std::nullopt;
    }

    // Check if journal number already exists
    std::map<std::string, std::any> filterByNumber;
    filterByNumber["journal_number"] = journalEntryDTO.journalNumber;
    if (glDAO_->getJournalEntries(filterByNumber).size() > 0) { // Specific DAO method
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: Journal entry with number " + journalEntryDTO.journalNumber + " already exists.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Số bút toán nhật ký đã tồn tại.");
        return std::nullopt;
    }

    // Validate details: total debits must equal total credits
    double totalDebits = 0.0;
    double totalCredits = 0.0;
    for (const auto& detail : journalEntryDetails) {
        totalDebits += detail.debitAmount;
        totalCredits += detail.creditAmount;
        // Validate GL account existence for each detail
        if (!getGLAccountById(detail.glAccountId, userRoleIds)) {
            ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: GL Account " + detail.glAccountId + " not found for journal entry detail.");
            ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Tài khoản sổ cái chung không tồn tại trong chi tiết bút toán.");
            return std::nullopt;
        }
    }
    if (std::abs(totalDebits - totalCredits) > 0.001) { // Allow for small floating point inaccuracies
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: Total debits (" + std::to_string(totalDebits) + ") do not equal total credits (" + std::to_string(totalCredits) + ") for journal entry " + journalEntryDTO.journalNumber + ".");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Tổng nợ phải bằng tổng có trong bút toán nhật ký.");
        return std::nullopt;
    }

    ERP::Finance::DTO::JournalEntryDTO newJournalEntry = journalEntryDTO;
    newJournalEntry.id = ERP::Utils::generateUUID();
    newJournalEntry.createdAt = ERP::Utils::DateUtils::now();
    newJournalEntry.createdBy = currentUserId;
    newJournalEntry.status = ERP::Common::EntityStatus::ACTIVE; // Or PENDING if approval is needed
    newJournalEntry.isPosted = false; // Not posted initially

    std::optional<ERP::Finance::DTO::JournalEntryDTO> createdJournalEntry = std::nullopt;

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            if (!glDAO_->createJournalEntry(newJournalEntry)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("GeneralLedgerService: Failed to create journal entry " + newJournalEntry.journalNumber + " in DAO.");
                return false;
            }
            // Save details
            for (auto detail : journalEntryDetails) {
                detail.id = ERP::Utils::generateUUID();
                detail.journalEntryId = newJournalEntry.id;
                detail.createdAt = newJournalEntry.createdAt;
                detail.createdBy = newJournalEntry.createdBy;
                detail.status = ERP::Common::EntityStatus::ACTIVE;
                if (!glDAO_->createJournalEntryDetail(detail)) { // Specific DAO method
                    ERP::Logger::Logger::getInstance().error("GeneralLedgerService: Failed to create journal entry detail for GL account " + detail.glAccountId + ".");
                    return false;
                }
            }
            createdJournalEntry = newJournalEntry;
            return true;
        },
        "GeneralLedgerService", "createJournalEntry"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Journal entry " + newJournalEntry.journalNumber + " created successfully with " + std::to_string(journalEntryDetails.size()) + " details.");
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::CREATE, ERP::Common::LogSeverity::INFO,
                       "Finance", "JournalEntry", newJournalEntry.id, "JournalEntry", newJournalEntry.journalNumber,
                       std::nullopt, newJournalEntry.toMap(), "Journal entry created.");
        return createdJournalEntry;
    }
    return std::nullopt;
}

bool GeneralLedgerService::postJournalEntry(
    const std::string& journalEntryId,
    const std::string& currentUserId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Attempting to post journal entry: " + journalEntryId + " by " + currentUserId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.PostJournalEntry", "Bạn không có quyền hạch toán bút toán nhật ký.")) {
        return false;
    }

    std::optional<ERP::Finance::DTO::JournalEntryDTO> journalEntryOpt = glDAO_->getJournalEntryById(journalEntryId, userRoleIds); // Specific DAO method
    if (!journalEntryOpt) {
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: Journal entry with ID " + journalEntryId + " not found for posting.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::NotFound, "Không tìm thấy bút toán nhật ký cần hạch toán.");
        return false;
    }
    
    ERP::Finance::DTO::JournalEntryDTO journalEntry = *journalEntryOpt;
    if (journalEntry.isPosted) {
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerService: Journal entry " + journalEntryId + " is already posted.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::InvalidInput, "Bút toán nhật ký đã được hạch toán.");
        return true; // Already posted
    }
    // Check if journal entry is balanced (should have been validated at creation)
    std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> details = glDAO_->getJournalEntryDetailsByEntryId(journalEntryId, userRoleIds); // Specific DAO method
    double totalDebits = 0.0;
    double totalCredits = 0.0;
    for (const auto& detail : details) {
        totalDebits += detail.debitAmount;
        totalCredits += detail.creditAmount;
    }
    if (std::abs(totalDebits - totalCredits) > 0.001) {
        ERP::Logger::Logger::getInstance().error("GeneralLedgerService: Unbalanced journal entry " + journalEntryId + ". Cannot post.");
        ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::OperationFailed, "Bút toán nhật ký không cân bằng. Không thể hạch toán.");
        return false;
    }

    bool success = executeTransaction(
        [&](std::shared_ptr<ERP::Database::DBConnection> db_conn) {
            // Update each GL account balance based on journal entry details
            for (const auto& detail : details) {
                if (!updateGLAccountBalance(detail.glAccountId, detail.debitAmount, detail.creditAmount, *db_conn)) {
                    ERP::Logger::Logger::getInstance().error("GeneralLedgerService: Failed to update GL account balance for detail " + detail.id + " during posting.");
                    return false;
                }
            }
            // Mark journal entry as posted
            journalEntry.isPosted = true;
            journalEntry.postingDate = ERP::Utils::DateUtils::now();
            journalEntry.postedByUserId = currentUserId;
            journalEntry.updatedAt = ERP::Utils::DateUtils::now();
            journalEntry.updatedBy = currentUserId;
            if (!glDAO_->updateJournalEntry(journalEntry)) { // Specific DAO method
                ERP::Logger::Logger::getInstance().error("GeneralLedgerService: Failed to update journal entry status to posted in DAO.");
                return false;
            }
            return true;
        },
        "GeneralLedgerService", "postJournalEntry"
    );

    if (success) {
        ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Journal entry " + journalEntryId + " posted successfully.");
        eventBus_.publish(std::make_shared<EventBus::JournalEntryPostedEvent>(journalEntryId));
        recordAuditLog(currentUserId, securityManager_->getUserService()->getUserName(currentUserId), getCurrentSessionId(),
                       ERP::Security::DTO::AuditActionType::PROCESS_END, ERP::Common::LogSeverity::INFO, // Could be POST
                       "Finance", "JournalEntryPosting", journalEntryId, "JournalEntry", journalEntry.journalNumber,
                       journalEntryOpt->toMap(), journalEntry.toMap(), "Journal entry posted.");
        return true;
    }
    return false;
}

std::vector<ERP::Finance::DTO::JournalEntryDTO> GeneralLedgerService::getAllJournalEntries(
    const std::map<std::string, std::any>& filter,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Retrieving all journal entries with filter.");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewJournalEntries", "Bạn không có quyền xem bút toán nhật ký.")) {
        return {};
    }

    return glDAO_->getJournalEntries(filter); // Specific DAO method
}

std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> GeneralLedgerService::getJournalEntryDetails(
    const std::string& journalEntryId,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Retrieving journal entry details for entry ID: " + journalEntryId + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewJournalEntries", "Bạn không có quyền xem chi tiết bút toán nhật ký.")) {
        return {};
    }

    return glDAO_->getJournalEntryDetailsByEntryId(journalEntryId, userRoleIds); // Specific DAO method
}

// NEW: Financial Report Generation Implementations (Simplified for now)

// Helper to calculate balances for a given period
std::map<std::string, double> GeneralLedgerService::calculateAccountBalances(
    const std::chrono::system_clock::time_point& startDate,
    const std::chrono::system_clock::time_point& endDate,
    bool includeOpeningBalances, // If true, sums all transactions up to end date from beginning
    const std::vector<std::string>& userRoleIds
) {
    std::map<std::string, double> balances; // Map of GL Account ID to net balance (Debit - Credit)

    // Get all GL Accounts (for account details like type and normal balance)
    std::vector<ERP::Finance::DTO::GeneralLedgerAccountDTO> allAccounts = glDAO_->getAllGLAccounts({}, userRoleIds);

    // Get all journal entry details within the period (or up to endDate if includeOpeningBalances)
    std::map<std::string, std::any> journalFilter;
    // For simplicity, we're fetching all and filtering in memory, but a real system
    // would filter at DB level or use a specialized balance table for efficiency.
    std::vector<ERP::Finance::DTO::JournalEntryDTO> allPostedEntries = glDAO_->getAllJournalEntries({{"is_posted", true}}, userRoleIds);

    for (const auto& entry : allPostedEntries) {
        if ((entry.postingDate >= startDate && entry.postingDate <= endDate) || (includeOpeningBalances && entry.postingDate <= endDate)) {
            std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> details = glDAO_->getJournalEntryDetails(entry.id, userRoleIds);
            for (const auto& detail : details) {
                // Sum up debits and credits per account
                if (balances.find(detail.glAccountId) == balances.end()) {
                    balances[detail.glAccountId] = 0.0;
                }
                balances[detail.glAccountId] += detail.debitAmount - detail.creditAmount;
            }
        }
    }
    return balances;
}

std::map<std::string, double> GeneralLedgerService::generateTrialBalance(
    const std::chrono::system_clock::time_point& startDate,
    const std::chrono::system_clock::time_point& endDate,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Generating Trial Balance from " + ERP::Utils::DateUtils::formatDateTime(startDate, ERP::Common::DATETIME_FORMAT) + " to " + ERP::Utils::DateUtils::formatDateTime(endDate, ERP::Common::DATETIME_FORMAT) + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewTrialBalance", "Bạn không có quyền tạo bảng cân đối thử.")) {
        return {};
    }

    std::map<std::string, double> trialBalanceReport; // Account Name -> Net Balance

    // Get all accounts
    std::vector<ERP::Finance::DTO::GeneralLedgerAccountDTO> allAccounts = glDAO_->getAllGLAccounts({}, userRoleIds);
    // Get all posted journal entries within the period
    std::map<std::string, std::any> journalFilter;
    journalFilter["posting_date_ge"] = ERP::Utils::DateUtils::formatDateTime(startDate, ERP::Common::DATETIME_FORMAT);
    journalFilter["posting_date_le"] = ERP::Utils::DateUtils::formatDateTime(endDate, ERP::Common::DATETIME_FORMAT);
    journalFilter["is_posted"] = true;
    std::vector<ERP::Finance::DTO::JournalEntryDTO> entries = glDAO_->getAllJournalEntries(journalFilter, userRoleIds);

    std::map<std::string, double> accountNetChanges; // GL Account ID -> Net Change in period

    for (const auto& entry : entries) {
        std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> details = glDAO_->getJournalEntryDetails(entry.id, userRoleIds);
        for (const auto& detail : details) {
            accountNetChanges[detail.glAccountId] += detail.debitAmount - detail.creditAmount;
        }
    }

    // Now, map account IDs to account names and compile report
    for (const auto& account : allAccounts) {
        double netBalance = accountNetChanges.count(account.id) ? accountNetChanges[account.id] : 0.0;
        trialBalanceReport[account.accountNumber + " - " + account.accountName] = netBalance;
    }

    return trialBalanceReport;
}

std::map<std::string, double> GeneralLedgerService::generateBalanceSheet(
    const std::chrono::system_clock::time_point& asOfDate,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Generating Balance Sheet as of " + ERP::Utils::DateUtils::formatDateTime(asOfDate, ERP::Common::DATETIME_FORMAT) + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewBalanceSheet", "Bạn không có quyền tạo bảng cân đối kế toán.")) {
        return {};
    }

    std::map<std::string, double> balanceSheetReport;
    double totalAssets = 0.0;
    double totalLiabilities = 0.0;
    double totalEquity = 0.0;

    // Calculate balances up to asOfDate for all asset, liability, equity accounts
    std::map<std::string, double> accountBalances = calculateAccountBalances(
        std::chrono::system_clock::time_point(), // Start from beginning of time
        asOfDate,
        true, // Include all transactions up to asOfDate
        userRoleIds
    );

    std::vector<ERP::Finance::DTO::GeneralLedgerAccountDTO> allAccounts = glDAO_->getAllGLAccounts({}, userRoleIds);

    for (const auto& account : allAccounts) {
        double balance = accountBalances.count(account.id) ? accountBalances[account.id] : 0.0;
        
        // For Balance Sheet, need to consider normal balance to determine if debit or credit increases/decreases balance
        if (account.normalBalance == ERP::Finance::DTO::NormalBalanceType::CREDIT) {
            balance = -balance; // Reverse sign for liability/equity accounts in trial balance context
        }

        if (account.accountType == ERP::Finance::DTO::GLAccountType::ASSET) {
            balanceSheetReport["Tài sản: " + account.accountName] = balance;
            totalAssets += balance;
        } else if (account.accountType == ERP::Finance::DTO::GLAccountType::LIABILITY) {
            balanceSheetReport["Nợ phải trả: " + account.accountName] = balance;
            totalLiabilities += balance;
        } else if (account.accountType == ERP::Finance::DTO::GLAccountType::EQUITY) {
            balanceSheetReport["Vốn chủ sở hữu: " + account.accountName] = balance;
            totalEquity += balance;
        }
    }

    balanceSheetReport["Tổng tài sản"] = totalAssets;
    balanceSheetReport["Tổng nợ phải trả"] = totalLiabilities;
    balanceSheetReport["Tổng vốn chủ sở hữu"] = totalEquity;
    balanceSheetReport["Tổng Nợ + Vốn chủ sở hữu"] = totalLiabilities + totalEquity;


    return balanceSheetReport;
}

std::map<std::string, double> GeneralLedgerService::generateIncomeStatement(
    const std::chrono::system_clock::time_point& startDate,
    const std::chrono::system_clock::time_point& endDate,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Generating Income Statement from " + ERP::Utils::DateUtils::formatDateTime(startDate, ERP::Common::DATETIME_FORMAT) + " to " + ERP::Utils::DateUtils::formatDateTime(endDate, ERP::Common::DATETIME_FORMAT) + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewIncomeStatement", "Bạn không có quyền tạo báo cáo kết quả hoạt động kinh doanh.")) {
        return {};
    }

    std::map<std::string, double> incomeStatementReport;
    double totalRevenue = 0.0;
    double totalExpenses = 0.0;

    // Calculate net changes for revenue and expense accounts within the period
    std::map<std::string, double> accountNetChanges = calculateAccountBalances(startDate, endDate, false, userRoleIds);

    std::vector<ERP::Finance::DTO::GeneralLedgerAccountDTO> allAccounts = glDAO_->getAllGLAccounts({}, userRoleIds);

    for (const auto& account : allAccounts) {
        double netChange = accountNetChanges.count(account.id) ? accountNetChanges[account.id] : 0.0;
        
        // For Income Statement, revenue accounts typically have a credit normal balance, so their net change is negative.
        // Expense accounts typically have a debit normal balance, so their net change is positive.
        // Adjust sign to reflect normal income statement presentation (Revenue as positive, Expenses as positive)
        if (account.accountType == ERP::Finance::DTO::GLAccountType::REVENUE) {
            incomeStatementReport["Doanh thu: " + account.accountName] = -netChange; // Invert sign for revenue
            totalRevenue += -netChange;
        } else if (account.accountType == ERP::Finance::DTO::GLAccountType::EXPENSE) {
            incomeStatementReport["Chi phí: " + account.accountName] = netChange; // Keep sign for expense
            totalExpenses += netChange;
        }
    }

    double netIncome = totalRevenue - totalExpenses;
    incomeStatementReport["Tổng doanh thu"] = totalRevenue;
    incomeStatementReport["Tổng chi phí"] = totalExpenses;
    incomeStatementReport["Lợi nhuận ròng"] = netIncome;

    return incomeStatementReport;
}

std::map<std::string, double> GeneralLedgerService::generateCashFlowStatement(
    const std::chrono::system_clock::time_point& startDate,
    const std::chrono::system_clock::time_point& endDate,
    const std::vector<std::string>& userRoleIds) {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerService: Generating Cash Flow Statement from " + ERP::Utils::DateUtils::formatDateTime(startDate, ERP::Common::DATETIME_FORMAT) + " to " + ERP::Utils::DateUtils::formatDateTime(endDate, ERP::Common::DATETIME_FORMAT) + ".");

    if (!checkPermission(currentUserId, userRoleIds, "Finance.ViewCashFlowStatement", "Bạn không có quyền tạo báo cáo lưu chuyển tiền tệ.")) {
        return {};
    }

    std::map<std::string, double> cashFlowReport;

    // This is a highly simplified Cash Flow Statement (Direct Method) focusing only on Cash/Bank accounts.
    // A true Cash Flow Statement requires detailed analysis of all transactions affecting cash,
    // and adjustments for non-cash items from the income statement and balance sheet (Indirect Method).
    // This is a placeholder for basic cash movements.

    double cashFromOperatingActivities = 0.0;
    double cashFromInvestingActivities = 0.0;
    double cashFromFinancingActivities = 0.0;

    // Get all transactions for cash/bank accounts within the period
    // Assuming you have a way to identify cash/bank GL accounts (e.g., by account type or specific account numbers)
    std::vector<ERP::Finance::DTO::GeneralLedgerAccountDTO> cashBankAccounts = glDAO_->getAllGLAccounts({{"account_type", static_cast<int>(ERP::Finance::DTO::GLAccountType::ASSET)}}, userRoleIds); // Simplistic filter for assets
    // Further filter cash/bank type accounts.
    
    std::map<std::string, std::any> journalFilter;
    journalFilter["posting_date_ge"] = ERP::Utils::DateUtils::formatDateTime(startDate, ERP::Common::DATETIME_FORMAT);
    journalFilter["posting_date_le"] = ERP::Utils::DateUtils::formatDateTime(endDate, ERP::Common::DATETIME_FORMAT);
    journalFilter["is_posted"] = true;
    std::vector<ERP::Finance::DTO::JournalEntryDTO> entries = glDAO_->getAllJournalEntries(journalFilter, userRoleIds);

    for (const auto& entry : entries) {
        std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> details = glDAO_->getJournalEntryDetails(entry.id, userRoleIds);
        for (const auto& detail : details) {
            // Very basic classification:
            // Assuming all cash inflows are "Debit" on cash account, outflows are "Credit".
            // You'd need more sophisticated logic to classify by operating, investing, financing.

            // Find the GL account for this detail
            std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> glAccount = glDAO_->getGLAccountById(detail.glAccountId, userRoleIds);
            if (glAccount && (glAccount->accountName.find("Cash") != std::string::npos || glAccount->accountName.find("Bank") != std::string::npos)) {
                // This is a cash/bank related transaction
                double cashImpact = detail.debitAmount - detail.creditAmount;

                // Simplistic categorization based on account type and description (needs real rules)
                if (glAccount->accountType == ERP::Finance::DTO::GLAccountType::REVENUE ||
                    glAccount->accountType == ERP::Finance::DTO::GLAccountType::EXPENSE) {
                    cashFromOperatingActivities += cashImpact;
                } else if (glAccount->accountType == ERP::Finance::DTO::GLAccountType::ASSET && glAccount->accountName.find("Equipment") != std::string::npos) {
                    cashFromInvestingActivities += cashImpact;
                } else if (glAccount->accountType == ERP::Finance::DTO::GLAccountType::LIABILITY || glAccount->accountType == ERP::Finance::DTO::GLAccountType::EQUITY) {
                    cashFromFinancingActivities += cashImpact;
                } else {
                    cashFromOperatingActivities += cashImpact; // Default to operating
                }
            }
        }
    }

    cashFlowReport["Dòng tiền từ hoạt động kinh doanh"] = cashFromOperatingActivities;
    cashFlowReport["Dòng tiền từ hoạt động đầu tư"] = cashFromInvestingActivities;
    cashFlowReport["Dòng tiền từ hoạt động tài chính"] = cashFromFinancingActivities;
    cashFlowReport["Lưu chuyển tiền tệ ròng"] = cashFromOperatingActivities + cashFromInvestingActivities + cashFromFinancingActivities;

    // You might also need beginning and ending cash balances from the balance sheet.
    // double beginningCash = ...;
    // double endingCash = beginningCash + cashFlowReport["Lưu chuyển tiền tệ ròng"];
    // cashFlowReport["Tiền và tương đương tiền đầu kỳ"] = beginningCash;
    // cashFlowReport["Tiền và tương đương tiền cuối kỳ"] = endingCash;

    return cashFlowReport;
}

} // namespace Services
} // namespace Finance
} // namespace ERP