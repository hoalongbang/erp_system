// Modules/Finance/DAO/JournalEntryDAO.h
#ifndef MODULES_FINANCE_DAO_JOURNALENTRYDAO_H
#define MODULES_FINANCE_DAO_JOURNALENTRYDAO_H
#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <optional>

// Rút gọn includes
#include "DAOBase.h"        // Base DAO template
#include "JournalEntry.h"   // JournalEntry DTO
#include "JournalEntryDetail.h" // JournalEntryDetail DTO (for related operations)

namespace ERP {
    namespace Finance {
        namespace DAOs {
            /**
             * @brief DAO class for JournalEntry entity.
             * Handles database operations for JournalEntryDTO and related JournalEntryDetailDTOs.
             */
            class JournalEntryDAO : public ERP::DAOBase::DAOBase<ERP::Finance::DTO::JournalEntryDTO> {
            public:
                JournalEntryDAO(std::shared_ptr<ERP::Database::ConnectionPool> connectionPool);
                ~JournalEntryDAO() override = default;

                // Override base methods (optional, but good practice if custom logic is needed)
                bool save(const ERP::Finance::DTO::JournalEntryDTO& entry) override;
                std::optional<ERP::Finance::DTO::JournalEntryDTO> findById(const std::string& id) override;
                bool update(const ERP::Finance::DTO::JournalEntryDTO& entry) override;
                bool remove(const std::string& id) override;
                std::vector<ERP::Finance::DTO::JournalEntryDTO> findAll() override;

                // Specific methods for JournalEntry
                std::optional<ERP::Finance::DTO::JournalEntryDTO> getJournalEntryByNumber(const std::string& journalNumber);
                std::vector<ERP::Finance::DTO::JournalEntryDTO> getJournalEntries(const std::map<std::string, std::any>& filters);
                int countJournalEntries(const std::map<std::string, std::any>& filters);

                // JournalEntryDetail operations (nested/related entities)
                bool createJournalEntryDetail(const ERP::Finance::DTO::JournalEntryDetailDTO& detail);
                std::optional<ERP::Finance::DTO::JournalEntryDetailDTO> getJournalEntryDetailById(const std::string& id);
                std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> getJournalEntryDetailsByEntryId(const std::string& journalEntryId);
                std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> getJournalEntryDetails(const std::map<std::string, std::any>& filters);
                int countJournalEntryDetails(const std::map<std::string, std::any>& filters);
                bool updateJournalEntryDetail(const ERP::Finance::DTO::JournalEntryDetailDTO& detail);
                bool removeJournalEntryDetail(const std::string& id);
                bool removeJournalEntryDetailsByEntryId(const std::string& journalEntryId);


            protected:
                // Required overrides for mapping between DTO and std::map<string, any>
                std::map<std::string, std::any> toMap(const ERP::Finance::DTO::JournalEntryDTO& entry) const override;
                ERP::Finance::DTO::JournalEntryDTO fromMap(const std::map<std::string, std::any>& data) const override;

                // Mapping for JournalEntryDetailDTO (internal helper, not part of base DAO template)
                std::map<std::string, std::any> journalEntryDetailToMap(const ERP::Finance::DTO::JournalEntryDetailDTO& detail) const;
                ERP::Finance::DTO::JournalEntryDetailDTO journalEntryDetailFromMap(const std::map<std::string, std::any>& data) const;

            private:
                std::string tableName_ = "journal_entries";
                std::string detailsTableName_ = "journal_entry_details";
            };
        } // namespace DAOs
    } // namespace Finance
} // namespace ERP
#endif // MODULES_FINANCE_DAO_JOURNALENTRYDAO_H