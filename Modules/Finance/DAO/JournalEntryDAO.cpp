// Modules/Finance/DAO/JournalEntryDAO.cpp
#include "JournalEntryDAO.h"
#include "DAOHelpers.h" // Standard includes
#include "Logger.h"     // Standard includes
#include "ErrorHandler.h" // Standard includes
#include "Common.h"     // Standard includes
#include "DateUtils.h"  // Standard includes
#include "DTOUtils.h"   // For JSON conversions

namespace ERP {
    namespace Finance {
        namespace DAOs {

            JournalEntryDAO::JournalEntryDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool)
                : DAOBase<ERP::Finance::DTO::JournalEntryDTO>(connectionPool, "journal_entries") {
                ERP::Logger::Logger::getInstance().info("JournalEntryDAO: Initialized.");
            }

            std::map<std::string, std::any> JournalEntryDAO::toMap(const ERP::Finance::DTO::JournalEntryDTO& entry) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(entry); // BaseDTO fields

                data["journal_number"] = entry.journalNumber;
                data["description"] = entry.description;
                data["entry_date"] = ERP::Utils::DateUtils::formatDateTime(entry.entryDate, ERP::Common::DATETIME_FORMAT);
                ERP::DAOHelpers::putOptionalTime(data, "posting_date", entry.postingDate);
                ERP::DAOHelpers::putOptionalString(data, "reference", entry.reference);
                data["total_debit"] = entry.totalDebit;
                data["total_credit"] = entry.totalCredit;
                ERP::DAOHelpers::putOptionalString(data, "posted_by_user_id", entry.postedByUserId);
                data["is_posted"] = entry.isPosted;

                return data;
            }

            ERP::Finance::DTO::JournalEntryDTO JournalEntryDAO::fromMap(const std::map<std::string, std::any>& data) const {
                ERP::Finance::DTO::JournalEntryDTO entry;
                ERP::Utils::DTOUtils::fromMap(data, entry); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "journal_number", entry.journalNumber);
                    ERP::DAOHelpers::getPlainValue(data, "description", entry.description);
                    ERP::DAOHelpers::getPlainTimeValue(data, "entry_date", entry.entryDate);
                    ERP::DAOHelpers::getOptionalTimeValue(data, "posting_date", entry.postingDate);
                    ERP::DAOHelpers::getOptionalStringValue(data, "reference", entry.reference);
                    ERP::DAOHelpers::getPlainValue(data, "total_debit", entry.totalDebit);
                    ERP::DAOHelpers::getPlainValue(data, "total_credit", entry.totalCredit);
                    ERP::DAOHelpers::getOptionalStringValue(data, "posted_by_user_id", entry.postedByUserId);
                    ERP::DAOHelpers::getPlainValue(data, "is_posted", entry.isPosted);

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("JournalEntryDAO: fromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "JournalEntryDAO: Data type mismatch in fromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("JournalEntryDAO: fromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "JournalEntryDAO: Unexpected error in fromMap: " + std::string(e.what()));
                }
                return entry;
            }

            bool JournalEntryDAO::save(const ERP::Finance::DTO::JournalEntryDTO& entry) {
                return create(entry);
            }

            std::optional<ERP::Finance::DTO::JournalEntryDTO> JournalEntryDAO::findById(const std::string& id) {
                return getById(id);
            }

            bool JournalEntryDAO::update(const ERP::Finance::DTO::JournalEntryDTO& entry) {
                return DAOBase<ERP::Finance::DTO::JournalEntryDTO>::update(entry);
            }

            bool JournalEntryDAO::remove(const std::string& id) {
                return DAOBase<ERP::Finance::DTO::JournalEntryDTO>::remove(id);
            }

            std::vector<ERP::Finance::DTO::JournalEntryDTO> JournalEntryDAO::findAll() {
                return DAOBase<ERP::Finance::DTO::JournalEntryDTO>::findAll();
            }

            std::optional<ERP::Finance::DTO::JournalEntryDTO> JournalEntryDAO::getJournalEntryByNumber(const std::string& journalNumber) {
                std::map<std::string, std::any> filters;
                filters["journal_number"] = journalNumber;
                std::vector<ERP::Finance::DTO::JournalEntryDTO> results = get(filters); // Use templated get
                if (!results.empty()) {
                    return results[0];
                }
                return std::nullopt;
            }

            std::vector<ERP::Finance::DTO::JournalEntryDTO> JournalEntryDAO::getJournalEntries(const std::map<std::string, std::any>& filters) {
                return get(filters); // Use templated get
            }

            int JournalEntryDAO::countJournalEntries(const std::map<std::string, std::any>& filters) {
                return count(filters); // Use templated count
            }

            // --- JournalEntryDetail operations ---

            std::map<std::string, std::any> JournalEntryDAO::journalEntryDetailToMap(const ERP::Finance::DTO::JournalEntryDetailDTO& detail) const {
                std::map<std::string, std::any> data = ERP::Utils::DTOUtils::toMap(detail); // BaseDTO fields

                data["journal_entry_id"] = detail.journalEntryId;
                data["gl_account_id"] = detail.glAccountId;
                data["debit_amount"] = detail.debitAmount;
                data["credit_amount"] = detail.creditAmount;
                ERP::DAOHelpers::putOptionalString(data, "notes", detail.notes);

                return data;
            }

            ERP::Finance::DTO::JournalEntryDetailDTO JournalEntryDAO::journalEntryDetailFromMap(const std::map<std::string, std::any>& data) const {
                ERP::Finance::DTO::JournalEntryDetailDTO detail;
                ERP::Utils::DTOUtils::fromMap(data, detail); // BaseDTO fields

                try {
                    ERP::DAOHelpers::getPlainValue(data, "journal_entry_id", detail.journalEntryId);
                    ERP::DAOHelpers::getPlainValue(data, "gl_account_id", detail.glAccountId);
                    ERP::DAOHelpers::getPlainValue(data, "debit_amount", detail.debitAmount);
                    ERP::DAOHelpers::getPlainValue(data, "credit_amount", detail.creditAmount);
                    ERP::DAOHelpers::getOptionalStringValue(data, "notes", detail.notes);

                }
                catch (const std::bad_any_cast& e) {
                    ERP::Logger::Logger::getInstance().error("JournalEntryDAO: journalEntryDetailFromMap - Data type mismatch during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::InvalidInput, "JournalEntryDAO: Data type mismatch in journalEntryDetailFromMap: " + std::string(e.what()));
                }
                catch (const std::exception& e) {
                    ERP::Logger::Logger::getInstance().error("JournalEntryDAO: journalEntryDetailFromMap - Unexpected error during conversion: " + std::string(e.what()));
                    ERP::ErrorHandling::ErrorHandler::logError(ERP::Common::ErrorCode::OperationFailed, "JournalEntryDAO: Unexpected error in journalEntryDetailFromMap: " + std::string(e.what()));
                }
                return detail;
            }

            bool JournalEntryDAO::createJournalEntryDetail(const ERP::Finance::DTO::JournalEntryDetailDTO& detail) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("JournalEntryDAO::createJournalEntryDetail: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "INSERT INTO " + detailsTableName_ + " (id, journal_entry_id, gl_account_id, debit_amount, credit_amount, notes, status, created_at, created_by, updated_at, updated_by) VALUES (:id, :journal_entry_id, :gl_account_id, :debit_amount, :credit_amount, :notes, :status, :created_at, :created_by, :updated_at, :updated_by);";

                std::map<std::string, std::any> params = journalEntryDetailToMap(detail);
                // Remove updated_at/by as they are not used in create
                params.erase("updated_at");
                params.erase("updated_by");

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("JournalEntryDAO::createJournalEntryDetail: Failed to create journal entry detail. Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to create journal entry detail.", "Không thể tạo chi tiết bút toán nhật ký.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

            std::optional<ERP::Finance::DTO::JournalEntryDetailDTO> JournalEntryDAO::getJournalEntryDetailById(const std::string& id) {
                std::map<std::string, std::any> filters;
                filters["id"] = id;
                std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> results = getJournalEntryDetails(filters);
                if (!results.empty()) {
                    return results[0];
                }
                return std::nullopt;
            }

            std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> JournalEntryDAO::getJournalEntryDetailsByEntryId(const std::string& journalEntryId) {
                std::map<std::string, std::any> filters;
                filters["journal_entry_id"] = journalEntryId;
                return getJournalEntryDetails(filters);
            }

            std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> JournalEntryDAO::getJournalEntryDetails(const std::map<std::string, std::any>& filters) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("JournalEntryDAO::getJournalEntryDetails: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return {};
                }

                std::string sql = "SELECT * FROM " + detailsTableName_;
                std::string whereClause = buildWhereClause(filters);
                sql += whereClause;

                std::vector<std::map<std::string, std::any>> results = conn->query(sql, filters);
                connectionPool_->releaseConnection(conn);

                std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> details;
                for (const auto& row : results) {
                    details.push_back(journalEntryDetailFromMap(row));
                }
                return details;
            }

            int JournalEntryDAO::countJournalEntryDetails(const std::map<std::string, std::any>& filters) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("JournalEntryDAO::countJournalEntryDetails: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return 0;
                }

                std::string sql = "SELECT COUNT(*) FROM " + detailsTableName_;
                std::string whereClause = buildWhereClause(filters);
                sql += whereClause;

                std::vector<std::map<std::string, std::any>> results = conn->query(sql, filters);
                connectionPool_->releaseConnection(conn);

                if (!results.empty() && results[0].count("COUNT(*)")) {
                    return static_cast<int>(std::any_cast<long long>(results[0].at("COUNT(*)"))); // SQLite COUNT(*) returns long long
                }
                return 0;
            }

            bool JournalEntryDAO::updateJournalEntryDetail(const ERP::Finance::DTO::JournalEntryDetailDTO& detail) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("JournalEntryDAO::updateJournalEntryDetail: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "UPDATE " + detailsTableName_ + " SET "
                    "journal_entry_id = :journal_entry_id, "
                    "gl_account_id = :gl_account_id, "
                    "debit_amount = :debit_amount, "
                    "credit_amount = :credit_amount, "
                    "notes = :notes, "
                    "status = :status, "
                    "created_at = :created_at, " // Should not be updated, but DTOUtils::toMap includes it
                    "created_by = :created_by, " // Should not be updated
                    "updated_at = :updated_at, "
                    "updated_by = :updated_by "
                    "WHERE id = :id;";

                std::map<std::string, std::any> params = journalEntryDetailToMap(detail);

                // Set updated_at/by for the update operation explicitly if they were not set in DTO
                // DTOUtils::toMap already sets these if DTO has them
                params["updated_at"] = ERP::Utils::DateUtils::formatDateTime(ERP::Utils::DateUtils::now(), ERP::Common::DATETIME_FORMAT);
                params["updated_by"] = detail.updatedBy.value_or(""); // Ensure this matches DTO's actual field for updatedBy

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("JournalEntryDAO::updateJournalEntryDetail: Failed to update journal entry detail " + detail.id + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to update journal entry detail.", "Không thể cập nhật chi tiết bút toán nhật ký.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }


            bool JournalEntryDAO::removeJournalEntryDetail(const std::string& id) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("JournalEntryDAO::removeJournalEntryDetail: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + detailsTableName_ + " WHERE id = :id;";
                std::map<std::string, std::any> params;
                params["id"] = id;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("JournalEntryDAO::removeJournalEntryDetail: Failed to remove journal entry detail " + id + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove journal entry detail.", "Không thể xóa chi tiết bút toán nhật ký.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }

            bool JournalEntryDAO::removeJournalEntryDetailsByEntryId(const std::string& journalEntryId) {
                std::shared_ptr<ERP::Database::DBConnection> conn = connectionPool_->getConnection();
                if (!conn) {
                    ERP::Logger::Logger::getInstance().error("JournalEntryDAO::removeJournalEntryDetailsByEntryId: Failed to get database connection.");
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to get database connection.", "Không thể kết nối cơ sở dữ liệu.");
                    return false;
                }

                std::string sql = "DELETE FROM " + detailsTableName_ + " WHERE journal_entry_id = :journal_entry_id;";
                std::map<std::string, std::any> params;
                params["journal_entry_id"] = journalEntryId;

                bool success = conn->execute(sql, params);
                if (!success) {
                    ERP::Logger::Logger::getInstance().error("JournalEntryDAO::removeJournalEntryDetailsByEntryId: Failed to remove journal entry details for entry_id " + journalEntryId + ". Error: " + conn->getLastError());
                    ERP::ErrorHandling::ErrorHandler::handle(ERP::Common::ErrorCode::DatabaseError, "Failed to remove journal entry details.", "Không thể xóa các chi tiết bút toán nhật ký.");
                }
                connectionPool_->releaseConnection(conn);
                return success;
            }


        } // namespace DAOs
    } // namespace Finance
} // namespace ERP