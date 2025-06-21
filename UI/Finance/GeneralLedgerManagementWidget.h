// UI/Finance/GeneralLedgerManagementWidget.h
#ifndef UI_FINANCE_GENERALLEDGERMANAGEMENTWIDGET_H
#define UI_FINANCE_GENERALLEDGERMANAGEMENTWIDGET_H
#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QDialog>
#include <QDateTimeEdit>
#include <QDoubleValidator>
#include <QTabWidget> // For tabs in UI
#include <QCheckBox>
#include <QTextEdit> // For displaying notes in dialogs


#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "GeneralLedgerService.h" // Dịch vụ sổ cái chung
#include "ISecurityManager.h"     // Dịch vụ bảo mật
#include "Logger.h"               // Logging
#include "ErrorHandler.h"         // Xử lý lỗi
#include "Common.h"               // Các enum chung
#include "DateUtils.h"            // Xử lý ngày tháng
#include "StringUtils.h"          // Xử lý chuỗi
#include "CustomMessageBox.h"     // Hộp thoại thông báo tùy chỉnh
#include "GeneralLedgerAccount.h" // GL Account DTO
#include "GLAccountBalance.h"     // GL Account Balance DTO
#include "JournalEntry.h"         // Journal Entry DTO
#include "JournalEntryDetail.h"   // Journal Entry Detail DTO
#include "User.h"                 // User DTO (for display)

namespace ERP {
namespace UI {
namespace Finance {

/**
 * @brief GeneralLedgerManagementWidget class provides a UI for managing the General Ledger.
 * This widget handles GL Accounts, Journal Entries, and GL Account Balances.
 */
class GeneralLedgerManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for GeneralLedgerManagementWidget.
     * @param glService Shared pointer to IGeneralLedgerService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit GeneralLedgerManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IGeneralLedgerService> glService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~GeneralLedgerManagementWidget();

private slots:
    // GL Accounts Tab
    void loadGLAccounts();
    void onAddGLAccountClicked();
    void onEditGLAccountClicked();
    void onDeleteGLAccountClicked();
    void onUpdateGLAccountStatusClicked();
    void onSearchGLAccountClicked();
    void onGLAccountTableItemClicked(int row, int column);
    void clearGLAccountForm();

    // Journal Entries Tab
    void loadJournalEntries();
    void onAddJournalEntryClicked();
    void onPostJournalEntryClicked();
    void onDeleteJournalEntryClicked();
    void onSearchJournalEntryClicked();
    void onJournalEntryTableItemClicked(int row, int column);
    void clearJournalEntryForm();
    void onViewJournalEntryDetailsClicked();

private:
    std::shared_ptr<Services::IGeneralLedgerService> glService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTabWidget *tabWidget_;

    // GL Accounts UI elements
    QTableWidget *glAccountTable_;
    QPushButton *addGLAccountButton_;
    QPushButton *editGLAccountButton_;
    QPushButton *deleteGLAccountButton_;
    QPushButton *updateGLAccountStatusButton_;
    QLineEdit *searchGLAccountLineEdit_;
    QPushButton *searchGLAccountButton_;
    QPushButton *clearGLAccountFormButton_;

    QLineEdit *glAccountIdLineEdit_;
    QLineEdit *accountNumberLineEdit_;
    QLineEdit *accountNameLineEdit_;
    QComboBox *accountTypeComboBox_;
    QComboBox *normalBalanceComboBox_;
    QComboBox *parentAccountComboBox_; // For hierarchical accounts
    QComboBox *glAccountStatusComboBox_;
    QLineEdit *descriptionLineEdit_;

    // Journal Entries UI elements
    QTableWidget *journalEntryTable_;
    QPushButton *addJournalEntryButton_;
    QPushButton *postJournalEntryButton_;
    QPushButton *deleteJournalEntryButton_;
    QLineEdit *searchJournalEntryLineEdit_;
    QPushButton *searchJournalEntryButton_;
    QPushButton *clearJournalEntryFormButton_;
    QPushButton *viewJournalEntryDetailsButton_;

    QLineEdit *journalEntryIdLineEdit_;
    QLineEdit *journalNumberLineEdit_;
    QLineEdit *descriptionJELineEdit_;
    QDateTimeEdit *entryDateEdit_;
    QDateTimeEdit *postingDateEdit_;
    QLineEdit *referenceLineEdit_;
    QLineEdit *totalDebitLineEdit_;
    QLineEdit *totalCreditLineEdit_;
    QLineEdit *postedByLineEdit_;
    QComboBox *journalEntryStatusComboBox_;
    QCheckBox *isPostedCheckBox_;

    // Helper functions
    void setupUI();
    void populateAccountTypeComboBox(QComboBox* comboBox);
    void populateNormalBalanceComboBox(QComboBox* comboBox);
    void populateParentAccountComboBox(QComboBox* comboBox);
    void populateGLAccountStatusComboBox(QComboBox* comboBox);
    void populateJournalEntryStatusComboBox(QComboBox* comboBox);
    void populateUserComboBox(QComboBox* comboBox);

    void showGLAccountInputDialog(ERP::Finance::DTO::GeneralLedgerAccountDTO* account = nullptr);
    void showJournalEntryInputDialog(ERP::Finance::DTO::JournalEntryDTO* entry = nullptr, std::vector<ERP::Finance::DTO::JournalEntryDetailDTO>* details = nullptr);
    void showJournalEntryDetailsDialog(ERP::Finance::DTO::JournalEntryDTO* entry);

    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Finance
} // namespace UI
} // namespace ERP

#endif // UI_FINANCE_GENERALLEDGERMANAGEMENTWIDGET_H