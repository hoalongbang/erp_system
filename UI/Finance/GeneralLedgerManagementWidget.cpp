// Modules/UI/Finance/GeneralLedgerManagementWidget.cpp
#include "GeneralLedgerManagementWidget.h" // Standard includes
#include "GeneralLedgerAccount.h"          // GL Account DTO
#include "GLAccountBalance.h"              // GL Account Balance DTO
#include "JournalEntry.h"                  // Journal Entry DTO
#include "JournalEntryDetail.h"            // Journal Entry Detail DTO
#include "User.h"                          // User DTO
#include "GeneralLedgerService.h"          // GL Service
#include "ISecurityManager.h"              // Security Manager
#include "Logger.h"                        // Logging
#include "ErrorHandler.h"                  // Error Handling
#include "Common.h"                        // Common Enums/Constants
#include "DateUtils.h"                     // Date Utilities
#include "StringUtils.h"                   // String Utilities
#include "CustomMessageBox.h"              // Custom Message Box
#include "UserService.h"                   // For getting user names
#include "DTOUtils.h"                      // For JSON conversions

#include <QTabWidget> // For tabs in UI
#include <QInputDialog>
#include <QDoubleValidator>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QTextEdit>

namespace ERP {
namespace UI {
namespace Finance {

GeneralLedgerManagementWidget::GeneralLedgerManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IGeneralLedgerService> glService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      glService_(glService),
      securityManager_(securityManager) {

    if (!glService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ sổ cái chung hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("GeneralLedgerManagementWidget: Initialized with null dependencies.");
        return;
    }

    auto authService = securityManager_->getAuthenticationService();
    if (authService) {
        std::string dummySessionId = "current_session_id"; // Placeholder
        std::optional<ERP::Security::DTO::SessionDTO> currentSession = authService->validateSession(dummySessionId);
        if (currentSession) {
            currentUserId_ = currentSession->userId;
            currentUserRoleIds_ = securityManager_->getUserService()->getUserRoles(currentUserId_, {});
        } else {
            currentUserId_ = "system_user";
            currentUserRoleIds_ = {"anonymous"};
            ERP::Logger::Logger::getInstance().warning("GeneralLedgerManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("GeneralLedgerManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadGLAccounts();
    loadJournalEntries();
    updateButtonsState();
}

GeneralLedgerManagementWidget::~GeneralLedgerManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void GeneralLedgerManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    tabWidget_ = new QTabWidget(this);
    mainLayout->addWidget(tabWidget_);

    // --- GL Accounts Tab ---
    QWidget *glAccountsTab = new QWidget(this);
    QVBoxLayout *glAccountsLayout = new QVBoxLayout(glAccountsTab);
    tabWidget_->addTab(glAccountsTab, "Tài khoản Sổ cái");

    QHBoxLayout *searchGLAccountLayout = new QHBoxLayout();
    searchGLAccountLineEdit_ = new QLineEdit(this);
    searchGLAccountLineEdit_->setPlaceholderText("Tìm kiếm theo số tài khoản hoặc tên...");
    searchGLAccountButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchGLAccountButton_, &QPushButton::clicked, this, &GeneralLedgerManagementWidget::onSearchGLAccountClicked);
    searchGLAccountLayout->addWidget(searchGLAccountLineEdit_);
    searchGLAccountLayout->addWidget(searchGLAccountButton_);
    glAccountsLayout->addLayout(searchGLAccountLayout);

    glAccountTable_ = new QTableWidget(this);
    glAccountTable_->setColumnCount(6); // ID, Account Number, Name, Type, Normal Balance, Status
    glAccountTable_->setHorizontalHeaderLabels({"ID", "Số TK", "Tên TK", "Loại", "Số dư Thông thường", "Trạng thái"});
    glAccountTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    glAccountTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    glAccountTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    glAccountTable_->horizontalHeader()->setStretchLastSection(true);
    connect(glAccountTable_, &QTableWidget::itemClicked, this, &GeneralLedgerManagementWidget::onGLAccountTableItemClicked);
    glAccountsLayout->addWidget(glAccountTable_);

    QGridLayout *glAccountFormLayout = new QGridLayout();
    glAccountIdLineEdit_ = new QLineEdit(this); glAccountIdLineEdit_->setReadOnly(true);
    accountNumberLineEdit_ = new QLineEdit(this);
    accountNameLineEdit_ = new QLineEdit(this);
    accountTypeComboBox_ = new QComboBox(this); populateAccountTypeComboBox(accountTypeComboBox_);
    normalBalanceComboBox_ = new QComboBox(this); populateNormalBalanceComboBox(normalBalanceComboBox_);
    parentAccountComboBox_ = new QComboBox(this); populateParentAccountComboBox(parentAccountComboBox_);
    glAccountStatusComboBox_ = new QComboBox(this); populateGLAccountStatusComboBox(glAccountStatusComboBox_);
    descriptionLineEdit_ = new QLineEdit(this);

    glAccountFormLayout->addWidget(new QLabel("ID:", this), 0, 0); glAccountFormLayout->addWidget(glAccountIdLineEdit_, 0, 1);
    glAccountFormLayout->addRow("Số TK:*", &accountNumberLineEdit_);
    glAccountFormLayout->addRow("Tên TK:*", &accountNameLineEdit_);
    glAccountFormLayout->addRow("Loại TK:*", &accountTypeComboBox_);
    glAccountFormLayout->addRow("Số dư Thông thường:*", &normalBalanceComboBox_);
    glAccountFormLayout->addRow("TK cha:", &parentAccountComboBox_);
    glAccountFormLayout->addRow("Trạng thái:", &glAccountStatusComboBox_);
    glAccountFormLayout->addRow("Mô tả:", &descriptionLineEdit_);
    glAccountsLayout->addLayout(glAccountFormLayout);

    QHBoxLayout *glAccountButtonLayout = new QHBoxLayout();
    addGLAccountButton_ = new QPushButton("Thêm mới", this); connect(addGLAccountButton_, &QPushButton::clicked, this, &GeneralLedgerManagementWidget::onAddGLAccountClicked);
    editGLAccountButton_ = new QPushButton("Sửa", this); connect(editGLAccountButton_, &QPushButton::clicked, this, &GeneralLedgerManagementWidget::onEditGLAccountClicked);
    deleteGLAccountButton_ = new QPushButton("Xóa", this); connect(deleteGLAccountButton_, &QPushButton::clicked, this, &GeneralLedgerManagementWidget::onDeleteGLAccountClicked);
    updateGLAccountStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateGLAccountStatusButton_, &QPushButton::clicked, this, &GeneralLedgerManagementWidget::onUpdateGLAccountStatusClicked);
    clearGLAccountFormButton_ = new QPushButton("Xóa Form", this); connect(clearGLAccountFormButton_, &QPushButton::clicked, this, &GeneralLedgerManagementWidget::clearGLAccountForm);
    glAccountButtonLayout->addWidget(addGLAccountButton_);
    glAccountButtonLayout->addWidget(editGLAccountButton_);
    glAccountButtonLayout->addWidget(deleteGLAccountButton_);
    glAccountButtonLayout->addWidget(updateGLAccountStatusButton_);
    glAccountButtonLayout->addWidget(clearGLAccountFormButton_);
    glAccountsLayout->addLayout(glAccountButtonLayout);

    // --- Journal Entries Tab ---
    QWidget *journalEntriesTab = new QWidget(this);
    QVBoxLayout *journalEntriesLayout = new QVBoxLayout(journalEntriesTab);
    tabWidget_->addTab(journalEntriesTab, "Bút toán Nhật ký");

    QHBoxLayout *searchJELayout = new QHBoxLayout();
    searchJournalEntryLineEdit_ = new QLineEdit(this);
    searchJournalEntryLineEdit_->setPlaceholderText("Tìm kiếm theo số bút toán hoặc mô tả...");
    searchJournalEntryButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchJournalEntryButton_, &QPushButton::clicked, this, &GeneralLedgerManagementWidget::onSearchJournalEntryClicked);
    searchJELayout->addWidget(searchJournalEntryLineEdit_);
    searchJELayout->addWidget(searchJournalEntryButton_);
    journalEntriesLayout->addLayout(searchJELayout);

    journalEntryTable_ = new QTableWidget(this);
    journalEntryTable_->setColumnCount(8); // ID, Số bút toán, Mô tả, Ngày bút toán, Ngày hạch toán, Tổng Nợ, Tổng Có, Đã hạch toán
    journalEntryTable_->setHorizontalHeaderLabels({"ID", "Số bút toán", "Mô tả", "Ngày bút toán", "Ngày hạch toán", "Tổng Nợ", "Tổng Có", "Đã hạch toán"});
    journalEntryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    journalEntryTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    journalEntryTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    journalEntryTable_->horizontalHeader()->setStretchLastSection(true);
    connect(journalEntryTable_, &QTableWidget::itemClicked, this, &GeneralLedgerManagementWidget::onJournalEntryTableItemClicked);
    journalEntriesLayout->addWidget(journalEntryTable_);

    QGridLayout *journalEntryFormLayout = new QGridLayout();
    journalEntryIdLineEdit_ = new QLineEdit(this); journalEntryIdLineEdit_->setReadOnly(true);
    journalNumberLineEdit_ = new QLineEdit(this); journalNumberLineEdit_->setReadOnly(true);
    descriptionJELineEdit_ = new QLineEdit(this); descriptionJELineEdit_->setReadOnly(true);
    entryDateEdit_ = new QDateTimeEdit(this); entryDateEdit_->setReadOnly(true); entryDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    postingDateEdit_ = new QDateTimeEdit(this); postingDateEdit_->setReadOnly(true); postingDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    referenceLineEdit_ = new QLineEdit(this); referenceLineEdit_->setReadOnly(true);
    totalDebitLineEdit_ = new QLineEdit(this); totalDebitLineEdit_->setReadOnly(true);
    totalCreditLineEdit_ = new QLineEdit(this); totalCreditLineEdit_->setReadOnly(true);
    postedByLineEdit_ = new QLineEdit(this); postedByLineEdit_->setReadOnly(true);
    journalEntryStatusComboBox_ = new QComboBox(this); populateJournalEntryStatusComboBox(journalEntryStatusComboBox_); journalEntryStatusComboBox_->setEnabled(false);
    isPostedCheckBox_ = new QCheckBox("Đã hạch toán", this); isPostedCheckBox_->setEnabled(false);

    journalEntryFormLayout->addWidget(new QLabel("ID:", this), 0, 0); journalEntryFormLayout->addWidget(journalEntryIdLineEdit_, 0, 1);
    journalEntryFormLayout->addRow("Số bút toán:", &journalNumberLineEdit_);
    journalEntryFormLayout->addRow("Mô tả:", &descriptionJELineEdit_);
    journalEntryFormLayout->addRow("Ngày bút toán:", &entryDateEdit_);
    journalEntryFormLayout->addRow("Ngày hạch toán:", &postingDateEdit_);
    journalEntryFormLayout->addRow("Tham chiếu:", &referenceLineEdit_);
    journalEntryFormLayout->addRow("Tổng Nợ:", &totalDebitLineEdit_);
    journalEntryFormLayout->addRow("Tổng Có:", &totalCreditLineEdit_);
    journalEntryFormLayout->addRow("Người hạch toán:", &postedByLineEdit_);
    journalEntryFormLayout->addRow("Trạng thái:", &journalEntryStatusComboBox_);
    journalEntryFormLayout->addWidget(isPostedCheckBox_, 10, 1);
    journalEntriesLayout->addLayout(journalEntryFormLayout);

    QHBoxLayout *journalEntryButtonLayout = new QHBoxLayout();
    addJournalEntryButton_ = new QPushButton("Thêm mới Bút toán", this); connect(addJournalEntryButton_, &QPushButton::clicked, this, &GeneralLedgerManagementWidget::onAddJournalEntryClicked);
    postJournalEntryButton_ = new QPushButton("Hạch toán Bút toán", this); connect(postJournalEntryButton_, &QPushButton::clicked, this, &GeneralLedgerManagementWidget::onPostJournalEntryClicked);
    deleteJournalEntryButton_ = new QPushButton("Xóa Bút toán", this); connect(deleteJournalEntryButton_, &QPushButton::clicked, this, &GeneralLedgerManagementWidget::onDeleteJournalEntryClicked);
    viewJournalEntryDetailsButton_ = new QPushButton("Xem Chi tiết Bút toán", this); connect(viewJournalEntryDetailsButton_, &QPushButton::clicked, this, &GeneralLedgerManagementWidget::onViewJournalEntryDetailsClicked);
    clearJournalEntryFormButton_ = new QPushButton("Xóa Form", this); connect(clearJournalEntryFormButton_, &QPushButton::clicked, this, &GeneralLedgerManagementWidget::clearJournalEntryForm);
    
    journalEntryButtonLayout->addWidget(addJournalEntryButton_);
    journalEntryButtonLayout->addWidget(postJournalEntryButton_);
    journalEntryButtonLayout->addWidget(deleteJournalEntryButton_);
    journalEntryButtonLayout->addWidget(viewJournalEntryDetailsButton_);
    journalEntryButtonLayout->addWidget(clearJournalEntryFormButton_);
    journalEntriesLayout->addLayout(journalEntryButtonLayout);
}

void GeneralLedgerManagementWidget::loadGLAccounts() {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerManagementWidget: Loading GL accounts...");
    glAccountTable_->setRowCount(0);

    std::vector<ERP::Finance::DTO::GeneralLedgerAccountDTO> accounts = glService_->getAllGLAccounts({}, currentUserId_, currentUserRoleIds_);

    glAccountTable_->setRowCount(accounts.size());
    for (int i = 0; i < accounts.size(); ++i) {
        const auto& account = accounts[i];
        glAccountTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(account.id)));
        glAccountTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(account.accountNumber)));
        glAccountTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(account.accountName)));
        glAccountTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(account.getTypeString())));
        glAccountTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(account.getNormalBalanceString())));
        glAccountTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(account.status))));
    }
    glAccountTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("GeneralLedgerManagementWidget: GL accounts loaded successfully.");
}

void GeneralLedgerManagementWidget::loadJournalEntries() {
    ERP::Logger::Logger::getInstance().info("GeneralLedgerManagementWidget: Loading journal entries...");
    journalEntryTable_->setRowCount(0);

    std::vector<ERP::Finance::DTO::JournalEntryDTO> entries = glService_->getAllJournalEntries({}, currentUserId_, currentUserRoleIds_);

    journalEntryTable_->setRowCount(entries.size());
    for (int i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        journalEntryTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(entry.id)));
        journalEntryTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(entry.journalNumber)));
        journalEntryTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(entry.description)));
        journalEntryTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(entry.entryDate, ERP::Common::DATETIME_FORMAT))));
        journalEntryTable_->setItem(i, 4, new QTableWidgetItem(entry.postingDate ? QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(*entry.postingDate, ERP::Common::DATETIME_FORMAT)) : "N/A"));
        journalEntryTable_->setItem(i, 5, new QTableWidgetItem(QString::number(entry.totalDebit, 'f', 2)));
        journalEntryTable_->setItem(i, 6, new QTableWidgetItem(QString::number(entry.totalCredit, 'f', 2)));
        journalEntryTable_->setItem(i, 7, new QTableWidgetItem(entry.isPosted ? "Yes" : "No"));
    }
    journalEntryTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("GeneralLedgerManagementWidget: Journal entries loaded successfully.");
}

void GeneralLedgerManagementWidget::populateAccountTypeComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("Asset", static_cast<int>(ERP::Finance::DTO::GLAccountType::ASSET));
    comboBox->addItem("Liability", static_cast<int>(ERP::Finance::DTO::GLAccountType::LIABILITY));
    comboBox->addItem("Equity", static_cast<int>(ERP::Finance::DTO::GLAccountType::EQUITY));
    comboBox->addItem("Revenue", static_cast<int>(ERP::Finance::DTO::GLAccountType::REVENUE));
    comboBox->addItem("Expense", static_cast<int>(ERP::Finance::DTO::GLAccountType::EXPENSE));
    comboBox->addItem("Other", static_cast<int>(ERP::Finance::DTO::GLAccountType::OTHER));
}

void GeneralLedgerManagementWidget::populateNormalBalanceComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("Debit", static_cast<int>(ERP::Finance::DTO::NormalBalanceType::DEBIT));
    comboBox->addItem("Credit", static_cast<int>(ERP::Finance::DTO::NormalBalanceType::CREDIT));
}

void GeneralLedgerManagementWidget::populateParentAccountComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("None", "");
    std::vector<ERP::Finance::DTO::GeneralLedgerAccountDTO> allAccounts = glService_->getAllGLAccounts({}, currentUserId_, currentUserRoleIds_);
    for (const auto& account : allAccounts) {
        comboBox->addItem(QString::fromStdString(account.accountNumber + " - " + account.accountName), QString::fromStdString(account.id));
    }
}

void GeneralLedgerManagementWidget::populateGLAccountStatusComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("Active", static_cast<int>(ERP::Common::EntityStatus::ACTIVE));
    comboBox->addItem("Inactive", static_cast<int>(ERP::Common::EntityStatus::INACTIVE));
    comboBox->addItem("Pending", static_cast<int>(ERP::Common::EntityStatus::PENDING));
    comboBox->addItem("Deleted", static_cast<int>(ERP::Common::EntityStatus::DELETED));
}

void GeneralLedgerManagementWidget::populateJournalEntryStatusComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("Draft", static_cast<int>(ERP::Common::EntityStatus::DRAFT)); // Assuming DRAFT for unposted
    comboBox->addItem("Posted", static_cast<int>(ERP::Common::EntityStatus::ACTIVE)); // Assuming ACTIVE for posted
    // Other statuses as needed
}

void GeneralLedgerManagementWidget::populateUserComboBox(QComboBox* comboBox) {
    comboBox->clear();
    comboBox->addItem("None", "");
    std::vector<ERP::User::DTO::UserDTO> allUsers = securityManager_->getUserService()->getAllUsers({}, currentUserId_, currentUserRoleIds_);
    for (const auto& user : allUsers) {
        comboBox->addItem(QString::fromStdString(user.username), QString::fromStdString(user.id));
    }
}


void GeneralLedgerManagementWidget::onAddGLAccountClicked() {
    if (!hasPermission("Finance.CreateGLAccount")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm tài khoản sổ cái.", QMessageBox::Warning);
        return;
    }
    clearGLAccountForm();
    populateParentAccountComboBox(parentAccountComboBox_);
    populateAccountTypeComboBox(accountTypeComboBox_);
    populateNormalBalanceComboBox(normalBalanceComboBox_);
    showGLAccountInputDialog();
}

void GeneralLedgerManagementWidget::onEditGLAccountClicked() {
    if (!hasPermission("Finance.UpdateGLAccount")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa tài khoản sổ cái.", QMessageBox::Warning);
        return;
    }

    int selectedRow = glAccountTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Tài khoản Sổ cái", "Vui lòng chọn một tài khoản để sửa.", QMessageBox::Information);
        return;
    }

    QString accountId = glAccountTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> accountOpt = glService_->getGLAccountById(accountId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (accountOpt) {
        populateParentAccountComboBox(parentAccountComboBox_);
        populateAccountTypeComboBox(accountTypeComboBox_);
        populateNormalBalanceComboBox(normalBalanceComboBox_);
        showGLAccountInputDialog(&(*accountOpt));
    } else {
        showMessageBox("Sửa Tài khoản Sổ cái", "Không tìm thấy tài khoản sổ cái để sửa.", QMessageBox::Critical);
    }
}

void GeneralLedgerManagementWidget::onDeleteGLAccountClicked() {
    if (!hasPermission("Finance.DeleteGLAccount")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa tài khoản sổ cái.", QMessageBox::Warning);
        return;
    }

    int selectedRow = glAccountTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Tài khoản Sổ cái", "Vui lòng chọn một tài khoản để xóa.", QMessageBox::Information);
        return;
    }

    QString accountId = glAccountTable_->item(selectedRow, 0)->text();
    QString accountNumber = glAccountTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Tài khoản Sổ cái");
    confirmBox.setText("Bạn có chắc chắn muốn xóa tài khoản sổ cái '" + accountNumber + "' (ID: " + accountId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (glService_->deleteGLAccount(accountId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Tài khoản Sổ cái", "Tài khoản sổ cái đã được xóa thành công.", QMessageBox::Information);
            loadGLAccounts();
            clearGLAccountForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa tài khoản sổ cái. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void GeneralLedgerManagementWidget::onUpdateGLAccountStatusClicked() {
    if (!hasPermission("Finance.UpdateGLAccount")) { // Using general update permission for status change
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái tài khoản sổ cái.", QMessageBox::Warning);
        return;
    }

    int selectedRow = glAccountTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một tài khoản sổ cái để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString accountId = glAccountTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> accountOpt = glService_->getGLAccountById(accountId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!accountOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy tài khoản sổ cái để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Finance::DTO::GeneralLedgerAccountDTO currentAccount = *accountOpt;
    // Toggle status between ACTIVE and INACTIVE for simplicity, or show a dialog for specific status
    ERP::Common::EntityStatus newStatus = (currentAccount.status == ERP::Common::EntityStatus::ACTIVE) ?
                                          ERP::Common::EntityStatus::INACTIVE :
                                          ERP::Common::EntityStatus::ACTIVE;

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Cập nhật trạng thái Tài khoản Sổ cái");
    confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái tài khoản '" + QString::fromStdString(currentAccount.accountNumber) + "' thành " + QString::fromStdString(ERP::Common::entityStatusToString(newStatus)) + "?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (glService_->updateGLAccountStatus(accountId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Cập nhật trạng thái", "Trạng thái tài khoản sổ cái đã được cập nhật thành công.", QMessageBox::Information);
            loadGLAccounts();
            clearGLAccountForm();
        } else {
            showMessageBox("Lỗi", "Không thể cập nhật trạng thái tài khoản sổ cái. Vui lòng kiểm tra log.", QMessageBox::Critical);
        }
    }
}


void GeneralLedgerManagementWidget::onSearchGLAccountClicked() {
    QString searchText = searchGLAccountLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["account_number_or_name_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    loadGLAccounts(); // Reloads with current filter if any
    ERP::Logger::Logger::getInstance().info("GeneralLedgerManagementWidget: GL Account Search completed.");
}

void GeneralLedgerManagementWidget::onGLAccountTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString accountId = glAccountTable_->item(row, 0)->text();
    std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> accountOpt = glService_->getGLAccountById(accountId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (accountOpt) {
        glAccountIdLineEdit_->setText(QString::fromStdString(accountOpt->id));
        accountNumberLineEdit_->setText(QString::fromStdString(accountOpt->accountNumber));
        accountNameLineEdit_->setText(QString::fromStdString(accountOpt->accountName));
        
        populateAccountTypeComboBox(accountTypeComboBox_);
        int typeIndex = accountTypeComboBox_->findData(static_cast<int>(accountOpt->accountType));
        if (typeIndex != -1) accountTypeComboBox_->setCurrentIndex(typeIndex);

        populateNormalBalanceComboBox(normalBalanceComboBox_);
        int normalBalanceIndex = normalBalanceComboBox_->findData(static_cast<int>(accountOpt->normalBalance));
        if (normalBalanceIndex != -1) normalBalanceComboBox_->setCurrentIndex(normalBalanceIndex);

        populateParentAccountComboBox(parentAccountComboBox_);
        if (accountOpt->parentAccountId) {
            int parentIndex = parentAccountComboBox_->findData(QString::fromStdString(*accountOpt->parentAccountId));
            if (parentIndex != -1) parentAccountComboBox_->setCurrentIndex(parentIndex);
            else parentAccountComboBox_->setCurrentIndex(0); // "None"
        } else {
            parentAccountComboBox_->setCurrentIndex(0); // "None"
        }
        
        populateGLAccountStatusComboBox(glAccountStatusComboBox_);
        int statusIndex = glAccountStatusComboBox_->findData(static_cast<int>(accountOpt->status));
        if (statusIndex != -1) glAccountStatusComboBox_->setCurrentIndex(statusIndex);
        
        descriptionLineEdit_->setText(QString::fromStdString(accountOpt->description.value_or("")));

    } else {
        showMessageBox("Thông tin Tài khoản Sổ cái", "Không thể tải chi tiết tài khoản đã chọn.", QMessageBox::Warning);
        clearGLAccountForm();
    }
    updateButtonsState();
}

void GeneralLedgerManagementWidget::clearGLAccountForm() {
    glAccountIdLineEdit_->clear();
    accountNumberLineEdit_->clear();
    accountNameLineEdit_->clear();
    accountTypeComboBox_->setCurrentIndex(0);
    normalBalanceComboBox_->setCurrentIndex(0);
    parentAccountComboBox_->clear(); // Repopulated on demand
    glAccountStatusComboBox_->setCurrentIndex(0);
    descriptionLineEdit_->clear();
    glAccountTable_->clearSelection();
    updateButtonsState();
}


void GeneralLedgerManagementWidget::onAddJournalEntryClicked() {
    if (!hasPermission("Finance.CreateJournalEntry")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm bút toán nhật ký.", QMessageBox::Warning);
        return;
    }
    clearJournalEntryForm();
    // populateJournalEntryStatusComboBox(journalEntryStatusComboBox_); // Not needed in dialog for new
    showJournalEntryInputDialog();
}

void GeneralLedgerManagementWidget::onPostJournalEntryClicked() {
    if (!hasPermission("Finance.PostJournalEntry")) {
        showMessageBox("Lỗi", "Bạn không có quyền hạch toán bút toán nhật ký.", QMessageBox::Warning);
        return;
    }

    int selectedRow = journalEntryTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Hạch toán Bút toán", "Vui lòng chọn một bút toán nhật ký để hạch toán.", QMessageBox::Information);
        return;
    }

    QString entryId = journalEntryTable_->item(selectedRow, 0)->text();
    QString entryNumber = journalEntryTable_->item(selectedRow, 1)->text();
    
    // Check if already posted from UI
    if (journalEntryTable_->item(selectedRow, 7)->text() == "Yes") {
        showMessageBox("Hạch toán Bút toán", "Bút toán này đã được hạch toán rồi.", QMessageBox::Information);
        return;
    }

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Hạch toán Bút toán");
    confirmBox.setText("Bạn có chắc chắn muốn hạch toán bút toán nhật ký '" + entryNumber + "' (ID: " + entryId + ")? Thao tác này sẽ ảnh hưởng đến số dư tài khoản sổ cái.");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (glService_->postJournalEntry(entryId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Hạch toán Bút toán", "Bút toán nhật ký đã được hạch toán thành công.", QMessageBox::Information);
            loadJournalEntries(); // Refresh Journal Entries table
            loadGLAccounts();     // Refresh GL Accounts (balances might change)
            clearJournalEntryForm();
        } else {
            showMessageBox("Lỗi Hạch toán", "Không thể hạch toán bút toán nhật ký. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void GeneralLedgerManagementWidget::onDeleteJournalEntryClicked() {
    if (!hasPermission("Finance.DeleteJournalEntry")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa bút toán nhật ký.", QMessageBox::Warning);
        return;
    }

    int selectedRow = journalEntryTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Bút toán", "Vui lòng chọn một bút toán nhật ký để xóa.", QMessageBox::Information);
        return;
    }

    QString entryId = journalEntryTable_->item(selectedRow, 0)->text();
    QString entryNumber = journalEntryTable_->item(selectedRow, 1)->text();
    
    // Check if posted from UI
    if (journalEntryTable_->item(selectedRow, 7)->text() == "Yes") {
        showMessageBox("Lỗi Xóa", "Không thể xóa bút toán nhật ký đã hạch toán. Vui lòng hủy hạch toán trước.", QMessageBox::Warning);
        return;
    }

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Bút toán");
    confirmBox.setText("Bạn có chắc chắn muốn xóa bút toán nhật ký '" + entryNumber + "' (ID: " + entryId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        // Need a deleteJournalEntry method in service and DAO
        // Assuming a `deleteJournalEntry` method exists in service, if not, it should be added.
        // For now, this will call `glService_->deleteJournalEntry` (which calls glDAO_->remove).
        if (glService_->deleteJournalEntry(entryId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Bút toán", "Bút toán nhật ký đã được xóa thành công.", QMessageBox::Information);
            loadJournalEntries();
            clearJournalEntryForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa bút toán nhật ký. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void GeneralLedgerManagementWidget::onSearchJournalEntryClicked() {
    QString searchText = searchJournalEntryLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["journal_number_or_description_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    loadJournalEntries(); // Reloads with current filter if any
    ERP::Logger::Logger::getInstance().info("GeneralLedgerManagementWidget: Journal Entry Search completed.");
}

void GeneralLedgerManagementWidget::onJournalEntryTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString entryId = journalEntryTable_->item(row, 0)->text();
    std::optional<ERP::Finance::DTO::JournalEntryDTO> entryOpt = glService_->getJournalEntryById(entryId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (entryOpt) {
        journalEntryIdLineEdit_->setText(QString::fromStdString(entryOpt->id));
        journalNumberLineEdit_->setText(QString::fromStdString(entryOpt->journalNumber));
        descriptionJELineEdit_->setText(QString::fromStdString(entryOpt->description));
        entryDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(entryOpt->entryDate.time_since_epoch()).count()));
        if (entryOpt->postingDate) postingDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(entryOpt->postingDate->time_since_epoch()).count()));
        else postingDateEdit_->clear();
        referenceLineEdit_->setText(QString::fromStdString(entryOpt->reference.value_or("")));
        totalDebitLineEdit_->setText(QString::number(entryOpt->totalDebit, 'f', 2));
        totalCreditLineEdit_->setText(QString::number(entryOpt->totalCredit, 'f', 2));
        
        QString postedByName = "N/A";
        if (entryOpt->postedByUserId) {
            std::optional<ERP::User::DTO::UserDTO> postedByUser = securityManager_->getUserService()->getUserById(*entryOpt->postedByUserId, currentUserId_, currentUserRoleIds_);
            if (postedByUser) postedByName = QString::fromStdString(postedByUser->username);
        }
        postedByLineEdit_->setText(postedByName);
        
        journalEntryStatusComboBox_->setCurrentIndex(entryOpt->isPosted ? journalEntryStatusComboBox_->findData(static_cast<int>(ERP::Common::EntityStatus::ACTIVE)) : journalEntryStatusComboBox_->findData(static_cast<int>(ERP::Common::EntityStatus::DRAFT)));
        isPostedCheckBox_->setChecked(entryOpt->isPosted);

    } else {
        showMessageBox("Thông tin Bút toán", "Không thể tải chi tiết bút toán đã chọn.", QMessageBox::Warning);
        clearJournalEntryForm();
    }
    updateButtonsState();
}

void GeneralLedgerManagementWidget::clearJournalEntryForm() {
    journalEntryIdLineEdit_->clear();
    journalNumberLineEdit_->clear();
    descriptionJELineEdit_->clear();
    entryDateEdit_->clear();
    postingDateEdit_->clear();
    referenceLineEdit_->clear();
    totalDebitLineEdit_->clear();
    totalCreditLineEdit_->clear();
    postedByLineEdit_->clear();
    journalEntryStatusComboBox_->setCurrentIndex(0);
    isPostedCheckBox_->setChecked(false);
    journalEntryTable_->clearSelection();
    updateButtonsState();
}

void GeneralLedgerManagementWidget::onViewJournalEntryDetailsClicked() {
    if (!hasPermission("Finance.ViewJournalEntries")) { // Re-using general view permission
        showMessageBox("Lỗi", "Bạn không có quyền xem chi tiết bút toán.", QMessageBox::Warning);
        return;
    }

    int selectedRow = journalEntryTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xem Chi tiết Bút toán", "Vui lòng chọn một bút toán để xem chi tiết.", QMessageBox::Information);
        return;
    }

    QString entryId = journalEntryTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Finance::DTO::JournalEntryDTO> entryOpt = glService_->getJournalEntryById(entryId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (entryOpt) {
        showJournalEntryDetailsDialog(&(*entryOpt));
    } else {
        showMessageBox("Xem Chi tiết Bút toán", "Không tìm thấy bút toán để xem chi tiết.", QMessageBox::Critical);
    }
}


void GeneralLedgerManagementWidget::showGLAccountInputDialog(ERP::Finance::DTO::GeneralLedgerAccountDTO* account) {
    QDialog dialog(this);
    dialog.setWindowTitle(account ? "Sửa Tài khoản Sổ cái" : "Thêm Tài khoản Sổ cái Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit accountNumberEdit(this);
    QLineEdit accountNameEdit(this);
    QComboBox accountTypeCombo(this); populateAccountTypeComboBox(&accountTypeCombo);
    QComboBox normalBalanceCombo(this); populateNormalBalanceComboBox(&normalBalanceCombo);
    QComboBox parentAccountCombo(this); populateParentAccountComboBox(&parentAccountCombo);
    QLineEdit descriptionEdit(this);

    if (account) {
        accountNumberEdit.setText(QString::fromStdString(account->accountNumber));
        accountNameEdit.setText(QString::fromStdString(account->accountName));
        int typeIndex = accountTypeCombo.findData(static_cast<int>(account->accountType));
        if (typeIndex != -1) accountTypeCombo.setCurrentIndex(typeIndex);
        int balanceIndex = normalBalanceCombo.findData(static_cast<int>(account->normalBalance));
        if (balanceIndex != -1) normalBalanceCombo.setCurrentIndex(balanceIndex);
        if (account->parentAccountId) {
            int parentIndex = parentAccountCombo.findData(QString::fromStdString(*account->parentAccountId));
            if (parentIndex != -1) parentAccountCombo.setCurrentIndex(parentIndex);
            else parentAccountCombo.setCurrentIndex(0);
        } else {
            parentAccountCombo.setCurrentIndex(0);
        }
        descriptionEdit.setText(QString::fromStdString(account->description.value_or("")));

        accountNumberEdit.setReadOnly(true); // Don't allow changing account number for existing
    } else {
        // Defaults for new
    }

    formLayout->addRow("Số TK:*", &accountNumberEdit);
    formLayout->addRow("Tên TK:*", &accountNameEdit);
    formLayout->addRow("Loại TK:*", &accountTypeCombo);
    formLayout->addRow("Số dư Thông thường:*", &normalBalanceCombo);
    formLayout->addRow("TK cha:", &parentAccountCombo);
    formLayout->addRow("Mô tả:", &descriptionEdit);
    
    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(account ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Finance::DTO::GeneralLedgerAccountDTO newAccountData;
        if (account) {
            newAccountData = *account;
        }

        newAccountData.accountNumber = accountNumberEdit.text().toStdString();
        newAccountData.accountName = accountNameEdit.text().toStdString();
        newAccountData.accountType = static_cast<ERP::Finance::DTO::GLAccountType>(accountTypeCombo.currentData().toInt());
        newAccountData.normalBalance = static_cast<ERP::Finance::DTO::NormalBalanceType>(normalBalanceCombo.currentData().toInt());
        
        QString selectedParentId = parentAccountCombo.currentData().toString();
        newAccountData.parentAccountId = selectedParentId.isEmpty() ? std::nullopt : std::make_optional(selectedParentId.toStdString());

        newAccountData.description = descriptionEdit.text().isEmpty() ? std::nullopt : std::make_optional(descriptionEdit.text().toStdString());
        newAccountData.status = ERP::Common::EntityStatus::ACTIVE; // Always set to active on creation/update via form

        bool success = false;
        if (account) {
            success = glService_->updateGLAccount(newAccountData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Tài khoản Sổ cái", "Tài khoản sổ cái đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật tài khoản sổ cái. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> createdAccount = glService_->createGLAccount(newAccountData, currentUserId_, currentUserRoleIds_);
            if (createdAccount) {
                showMessageBox("Thêm Tài khoản Sổ cái", "Tài khoản sổ cái mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm tài khoản sổ cái mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadGLAccounts();
            clearGLAccountForm();
        }
    }
}

void GeneralLedgerManagementWidget::showJournalEntryInputDialog(ERP::Finance::DTO::JournalEntryDTO* entry, std::vector<ERP::Finance::DTO::JournalEntryDetailDTO>* details) {
    QDialog dialog(this);
    dialog.setWindowTitle(entry ? "Sửa Bút toán Nhật ký" : "Thêm Bút toán Nhật ký Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit journalNumberEdit(this);
    QLineEdit descriptionEdit(this);
    QDateTimeEdit entryDateEdit(this); entryDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QLineEdit referenceEdit(this);

    // Details Table for Journal Entry
    QTableWidget *detailsTable = new QTableWidget(this);
    detailsTable->setColumnCount(4); // GL Account, Debit, Credit, Notes
    detailsTable->setHorizontalHeaderLabels({"Tài khoản GL", "Nợ", "Có", "Ghi chú"});
    detailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    detailsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    detailsTable->horizontalHeader()->setStretchLastSection(true);
    
    // Total Debit/Credit display
    QLineEdit totalDebitDisplay("0.00"); totalDebitDisplay.setReadOnly(true);
    QLineEdit totalCreditDisplay("0.00"); totalCreditDisplay.setReadOnly(true);
    QHBoxLayout *totalsLayout = new QHBoxLayout();
    totalsLayout->addWidget(new QLabel("Tổng Nợ:"));
    totalsLayout->addWidget(&totalDebitDisplay);
    totalsLayout->addWidget(new QLabel("Tổng Có:"));
    totalsLayout->addWidget(&totalCreditDisplay);

    auto updateTotals = [&]() {
        double currentTotalDebit = 0.0;
        double currentTotalCredit = 0.0;
        for (int i = 0; i < detailsTable->rowCount(); ++i) {
            currentTotalDebit += detailsTable->item(i, 1)->text().toDouble();
            currentTotalCredit += detailsTable->item(i, 2)->text().toDouble();
        }
        totalDebitDisplay.setText(QString::number(currentTotalDebit, 'f', 2));
        totalCreditDisplay.setText(QString::number(currentTotalCredit, 'f', 2));
    };


    if (entry) {
        journalNumberEdit.setText(QString::fromStdString(entry->journalNumber));
        descriptionEdit.setText(QString::fromStdString(entry->description));
        entryDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(entry->entryDate.time_since_epoch()).count()));
        referenceEdit.setText(QString::fromStdString(entry->reference.value_or("")));
        journalNumberEdit.setReadOnly(true); // Don't allow changing for existing

        // Load existing details
        if (details) { // If details provided (e.g. from service call)
            detailsTable->setRowCount(details->size());
            for (int i = 0; i < details->size(); ++i) {
                const auto& detail = (*details)[i];
                QString glAccountName = "N/A";
                std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> glAccount = glService_->getGLAccountById(detail.glAccountId, currentUserId_, currentUserRoleIds_);
                if (glAccount) glAccountName = QString::fromStdString(glAccount->accountNumber + " - " + glAccount->accountName);

                detailsTable->setItem(i, 0, new QTableWidgetItem(glAccountName));
                detailsTable->setItem(i, 1, new QTableWidgetItem(QString::number(detail.debitAmount, 'f', 2)));
                detailsTable->setItem(i, 2, new QTableWidgetItem(QString::number(detail.creditAmount, 'f', 2)));
                detailsTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(detail.notes.value_or(""))));
                detailsTable->item(i, 0)->setData(Qt::UserRole, QString::fromStdString(detail.id)); // Store detail ID
                detailsTable->item(i, 0)->setData(Qt::UserRole+1, QString::fromStdString(detail.glAccountId)); // Store GL Account ID
            }
            updateTotals();
        }

    } else {
        journalNumberEdit.setText("JE-" + QString::fromStdString(ERP::Utils::generateUUID().substr(0, 8))); // Auto-generate
        entryDateEdit.setDateTime(QDateTime::currentDateTime());
    }

    formLayout->addRow("Số Bút toán:*", &journalNumberEdit);
    formLayout->addRow("Mô tả:*", &descriptionEdit);
    formLayout->addRow("Ngày Bút toán:*", &entryDateEdit);
    formLayout->addRow("Tham chiếu:", &referenceEdit);
    dialogLayout->addLayout(formLayout);

    dialogLayout->addWidget(new QLabel("Chi tiết Bút toán:", &dialog));
    dialogLayout->addWidget(detailsTable);
    dialogLayout->addLayout(totalsLayout);

    QHBoxLayout *itemButtonsLayout = new QHBoxLayout();
    QPushButton *addItemButton = new QPushButton("Thêm Chi tiết", &dialog);
    QPushButton *editItemButton = new QPushButton("Sửa Chi tiết", &dialog);
    QPushButton *deleteItemButton = new QPushButton("Xóa Chi tiết", &dialog);
    itemButtonsLayout->addWidget(addItemButton);
    itemButtonsLayout->addWidget(editItemButton);
    itemButtonsLayout->addWidget(deleteItemButton);
    dialogLayout->addLayout(itemButtonsLayout);

    // Connect item management buttons
    connect(addItemButton, &QPushButton::clicked, [&]() {
        QDialog itemDialog(&dialog);
        itemDialog.setWindowTitle("Thêm Chi tiết Bút toán");
        QFormLayout itemFormLayout;
        QComboBox glAccountCombo; populateParentAccountComboBox(&glAccountCombo); // Use same combo for GL Accounts
        for (int i = 0; i < parentAccountComboBox_->count(); ++i) glAccountCombo.addItem(parentAccountComboBox_->itemText(i), parentAccountComboBox_->itemData(i));
        
        QLineEdit debitEdit; debitEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit creditEdit; creditEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit notesEdit;

        itemFormLayout.addRow("Tài khoản GL:*", &glAccountCombo);
        itemFormLayout.addRow("Nợ:", &debitEdit);
        itemFormLayout.addRow("Có:", &creditEdit);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);

        QPushButton *okItemButton = new QPushButton("Thêm", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (glAccountCombo.currentData().isNull() || (debitEdit.text().isEmpty() && creditEdit.text().isEmpty())) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết (Tài khoản GL và ít nhất một giá trị Nợ/Có).", QMessageBox::Warning);
                return;
            }
            if (!debitEdit.text().isEmpty() && !creditEdit.text().isEmpty() && debitEdit.text().toDouble() > 0 && creditEdit.text().toDouble() > 0) {
                 showMessageBox("Lỗi", "Một chi tiết bút toán không thể vừa có số Nợ và số Có cùng lúc. Vui lòng nhập vào một trong hai trường.", QMessageBox::Warning);
                return;
            }

            int newRow = detailsTable->rowCount();
            detailsTable->insertRow(newRow);
            detailsTable->setItem(newRow, 0, new QTableWidgetItem(glAccountCombo.currentText()));
            detailsTable->setItem(newRow, 1, new QTableWidgetItem(debitEdit.text().isEmpty() ? "0.00" : QString::number(debitEdit.text().toDouble(), 'f', 2)));
            detailsTable->setItem(newRow, 2, new QTableWidgetItem(creditEdit.text().isEmpty() ? "0.00" : QString::number(creditEdit.text().toDouble(), 'f', 2)));
            detailsTable->setItem(newRow, 3, new QTableWidgetItem(notesEdit.text()));
            detailsTable->item(newRow, 0)->setData(Qt::UserRole+1, glAccountCombo.currentData()); // Store GL Account ID
            updateTotals();
        }
    });

    connect(editItemButton, &QPushButton::clicked, [&]() {
        int selectedItemRow = detailsTable->currentRow();
        if (selectedItemRow < 0) {
            showMessageBox("Sửa Chi tiết", "Vui lòng chọn một chi tiết để sửa.", QMessageBox::Information);
            return;
        }

        QDialog itemDialog(&dialog);
        itemDialog.setWindowTitle("Sửa Chi tiết Bút toán");
        QFormLayout itemFormLayout;
        QComboBox glAccountCombo; populateParentAccountComboBox(&glAccountCombo);
        for (int i = 0; i < parentAccountComboBox_->count(); ++i) glAccountCombo.addItem(parentAccountComboBox_->itemText(i), parentAccountComboBox_->itemData(i));
        
        QLineEdit debitEdit; debitEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit creditEdit; creditEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, &itemDialog));
        QLineEdit notesEdit;

        // Populate with current item data
        QString currentGlAccountId = detailsTable->item(selectedItemRow, 0)->data(Qt::UserRole+1).toString();
        int glAccountIndex = glAccountCombo.findData(currentGlAccountId);
        if (glAccountIndex != -1) glAccountCombo.setCurrentIndex(glAccountIndex);
        glAccountCombo.setEnabled(false); // Do not allow changing GL Account for existing detail

        debitEdit.setText(detailsTable->item(selectedItemRow, 1)->text());
        creditEdit.setText(detailsTable->item(selectedItemRow, 2)->text());
        notesEdit.setText(detailsTable->item(selectedItemRow, 3)->text());

        itemFormLayout.addRow("Tài khoản GL:*", &glAccountCombo);
        itemFormLayout.addRow("Nợ:", &debitEdit);
        itemFormLayout.addRow("Có:", &creditEdit);
        itemFormLayout.addRow("Ghi chú:", &notesEdit);

        QPushButton *okItemButton = new QPushButton("Lưu", &itemDialog);
        QPushButton *cancelItemButton = new QPushButton("Hủy", &itemDialog);
        QHBoxLayout itemButtonLayout; itemButtonLayout.addWidget(okItemButton); itemButtonLayout.addWidget(cancelItemButton);
        QVBoxLayout itemDialogLayout; itemDialogLayout.addLayout(&itemFormLayout); itemDialogLayout.addLayout(&itemButtonLayout);
        itemDialog.setLayout(&itemDialogLayout);

        connect(okItemButton, &QPushButton::clicked, &itemDialog, &QDialog::accept);
        connect(cancelItemButton, &QPushButton::clicked, &itemDialog, &QDialog::reject);

        if (itemDialog.exec() == QDialog::Accepted) {
            if (glAccountCombo.currentData().isNull() || (debitEdit.text().isEmpty() && creditEdit.text().isEmpty())) {
                showMessageBox("Lỗi", "Vui lòng điền đầy đủ thông tin chi tiết (Tài khoản GL và ít nhất một giá trị Nợ/Có).", QMessageBox::Warning);
                return;
            }
            if (!debitEdit.text().isEmpty() && !creditEdit.text().isEmpty() && debitEdit.text().toDouble() > 0 && creditEdit.text().toDouble() > 0) {
                 showMessageBox("Lỗi", "Một chi tiết bút toán không thể vừa có số Nợ và số Có cùng lúc. Vui lòng nhập vào một trong hai trường.", QMessageBox::Warning);
                return;
            }
            // Update table row
            detailsTable->setItem(selectedItemRow, 0, new QTableWidgetItem(glAccountCombo.currentText()));
            detailsTable->setItem(selectedItemRow, 1, new QTableWidgetItem(debitEdit.text().isEmpty() ? "0.00" : QString::number(debitEdit.text().toDouble(), 'f', 2)));
            detailsTable->setItem(selectedItemRow, 2, new QTableWidgetItem(creditEdit.text().isEmpty() ? "0.00" : QString::number(creditEdit.text().toDouble(), 'f', 2)));
            detailsTable->setItem(selectedItemRow, 3, new QTableWidgetItem(notesEdit.text()));
            updateTotals();
        }
    });

    connect(deleteItemButton, &QPushButton::clicked, [&]() {
        int selectedItemRow = detailsTable->currentRow();
        if (selectedItemRow < 0) {
            showMessageBox("Xóa Chi tiết", "Vui lòng chọn một chi tiết để xóa.", QMessageBox::Information);
            return;
        }
        Common::CustomMessageBox confirmDelBox(&dialog);
        confirmDelBox.setWindowTitle("Xóa Chi tiết Bút toán");
        confirmDelBox.setText("Bạn có chắc chắn muốn xóa chi tiết bút toán này?");
        confirmDelBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmDelBox.exec() == QMessageBox::Yes) {
            detailsTable->removeRow(selectedItemRow);
            updateTotals();
        }
    });


    QPushButton *okButton = new QPushButton(entry ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *actionButtonsLayout = new QHBoxLayout();
    actionButtonsLayout->addWidget(okButton);
    actionButtonsLayout->addWidget(cancelButton);
    dialogLayout->addLayout(actionButtonsLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        if (detailsTable->rowCount() == 0) {
            showMessageBox("Lỗi", "Bút toán nhật ký phải có ít nhất một chi tiết.", QMessageBox::Warning);
            return;
        }
        if (totalDebitDisplay.text() != totalCreditDisplay.text()) {
            showMessageBox("Lỗi", "Tổng Nợ phải bằng Tổng Có.", QMessageBox::Warning);
            return;
        }

        ERP::Finance::DTO::JournalEntryDTO newEntryData;
        if (entry) {
            newEntryData = *entry;
        }

        newEntryData.journalNumber = journalNumberEdit.text().toStdString();
        newEntryData.description = descriptionEdit.text().toStdString();
        newEntryData.entryDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(entryDateEdit.dateTime());
        newEntryData.reference = referenceEdit.text().isEmpty() ? std::nullopt : std::make_optional(referenceEdit.text().toStdString());
        
        // Totals calculated from details table
        newEntryData.totalDebit = totalDebitDisplay.text().toDouble();
        newEntryData.totalCredit = totalCreditDisplay.text().toDouble();

        // Collect all details from the table
        std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> updatedDetails;
        for (int i = 0; i < detailsTable->rowCount(); ++i) {
            ERP::Finance::DTO::JournalEntryDetailDTO detail;
            // If item had an ID (from existing data), reuse it, otherwise generate new
            QString existingId = detailsTable->item(i, 0)->data(Qt::UserRole).toString();
            detail.id = existingId.isEmpty() ? ERP::Utils::generateUUID() : existingId.toStdString();

            detail.journalEntryId = newEntryData.id; // Link to parent entry
            detail.glAccountId = detailsTable->item(i, 0)->data(Qt::UserRole+1).toString().toStdString();
            detail.debitAmount = detailsTable->item(i, 1)->text().toDouble();
            detail.creditAmount = detailsTable->item(i, 2)->text().toDouble();
            detail.notes = detailsTable->item(i, 3)->text().isEmpty() ? std::nullopt : std::make_optional(detailsTable->item(i, 3)->text().toStdString());
            
            updatedDetails.push_back(detail);
        }

        bool success = false;
        if (entry) {
            // Update an existing journal entry. This requires deleting old details and adding new ones.
            // Assuming glService_->updateJournalEntry handles this, or need specific update methods for details.
            // For simplicity here, the update method would typically take both header and new details.
            success = glService_->updateJournalEntry(newEntryData, updatedDetails, currentUserId_, currentUserRoleIds_); // Assuming this signature
            if (success) showMessageBox("Sửa Bút toán Nhật ký", "Bút toán nhật ký đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật bút toán nhật ký. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Finance::DTO::JournalEntryDTO> createdEntry = glService_->createJournalEntry(newEntryData, updatedDetails, currentUserId_, currentUserRoleIds_);
            if (createdEntry) {
                showMessageBox("Thêm Bút toán Nhật ký", "Bút toán nhật ký mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm bút toán nhật ký mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadJournalEntries();
            loadGLAccounts(); // Balances might change if entries were immediately posted
            clearJournalEntryForm();
        }
    }
}

void GeneralLedgerManagementWidget::showJournalEntryDetailsDialog(ERP::Finance::DTO::JournalEntryDTO* entry) {
    if (!entry) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Chi tiết Bút toán Nhật ký: " + QString::fromStdString(entry->journalNumber));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget *detailsTable = new QTableWidget(&dialog);
    detailsTable->setColumnCount(4); // GL Account, Debit, Credit, Notes
    detailsTable->setHorizontalHeaderLabels({"Tài khoản GL", "Nợ", "Có", "Ghi chú"});
    detailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    detailsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    detailsTable->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(detailsTable);

    // Load details
    std::vector<ERP::Finance::DTO::JournalEntryDetailDTO> details = glService_->getJournalEntryDetails(entry->id, currentUserId_, currentUserRoleIds_);
    detailsTable->setRowCount(details.size());
    for (int i = 0; i < details.size(); ++i) {
        const auto& detail = details[i];
        QString glAccountName = "N/A";
        std::optional<ERP::Finance::DTO::GeneralLedgerAccountDTO> glAccount = glService_->getGLAccountById(detail.glAccountId, currentUserId_, currentUserRoleIds_);
        if (glAccount) glAccountName = QString::fromStdString(glAccount->accountNumber + " - " + glAccount->accountName);

        detailsTable->setItem(i, 0, new QTableWidgetItem(glAccountName));
        detailsTable->setItem(i, 1, new QTableWidgetItem(QString::number(detail.debitAmount, 'f', 2)));
        detailsTable->setItem(i, 2, new QTableWidgetItem(QString::number(detail.creditAmount, 'f', 2)));
        detailsTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(detail.notes.value_or(""))));
    }
    detailsTable->resizeColumnsToContents();

    QPushButton *closeButton = new QPushButton("Đóng", &dialog);
    dialogLayout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}


void GeneralLedgerManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool GeneralLedgerManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void GeneralLedgerManagementWidget::updateButtonsState() {
    // GL Accounts Tab
    bool canCreateGLAccount = hasPermission("Finance.CreateGLAccount");
    bool canUpdateGLAccount = hasPermission("Finance.UpdateGLAccount");
    bool canDeleteGLAccount = hasPermission("Finance.DeleteGLAccount");
    bool canViewGLAccounts = hasPermission("Finance.ViewGLAccounts");

    addGLAccountButton_->setEnabled(canCreateGLAccount);
    searchGLAccountButton_->setEnabled(canViewGLAccounts);

    bool isGLAccountRowSelected = glAccountTable_->currentRow() >= 0;
    editGLAccountButton_->setEnabled(isGLAccountRowSelected && canUpdateGLAccount);
    deleteGLAccountButton_->setEnabled(isGLAccountRowSelected && canDeleteGLAccount);
    updateGLAccountStatusButton_->setEnabled(isGLAccountRowSelected && canUpdateGLAccount);

    // Form fields for GL Account (enabled for editing)
    bool enableGLAccountForm = isGLAccountRowSelected && canUpdateGLAccount;
    accountNumberLineEdit_->setEnabled(enableGLAccountForm); // Read-only for existing anyway
    accountNameLineEdit_->setEnabled(enableGLAccountForm);
    accountTypeComboBox_->setEnabled(enableGLAccountForm);
    normalBalanceComboBox_->setEnabled(enableGLAccountForm);
    parentAccountComboBox_->setEnabled(enableGLAccountForm);
    glAccountStatusComboBox_->setEnabled(enableGLAccountForm);
    descriptionLineEdit_->setEnabled(enableGLAccountForm);
    glAccountIdLineEdit_->setEnabled(false); // Always read-only

    // Journal Entries Tab
    bool canCreateJournalEntry = hasPermission("Finance.CreateJournalEntry");
    bool canPostJournalEntry = hasPermission("Finance.PostJournalEntry");
    bool canDeleteJournalEntry = hasPermission("Finance.DeleteJournalEntry");
    bool canViewJournalEntries = hasPermission("Finance.ViewJournalEntries");

    addJournalEntryButton_->setEnabled(canCreateJournalEntry);
    searchJournalEntryButton_->setEnabled(canViewJournalEntries);

    bool isJournalEntryRowSelected = journalEntryTable_->currentRow() >= 0;
    postJournalEntryButton_->setEnabled(isJournalEntryRowSelected && canPostJournalEntry && journalEntryTable_->item(journalEntryTable_->currentRow(), 7)->text() == "No");
    deleteJournalEntryButton_->setEnabled(isJournalEntryRowSelected && canDeleteJournalEntry && journalEntryTable_->item(journalEntryTable_->currentRow(), 7)->text() == "No");
    viewJournalEntryDetailsButton_->setEnabled(isJournalEntryRowSelected && canViewJournalEntries);

    // Form fields for Journal Entry (always read-only for details display)
    journalEntryIdLineEdit_->setEnabled(false);
    journalNumberLineEdit_->setEnabled(false);
    descriptionJELineEdit_->setEnabled(false);
    entryDateEdit_->setEnabled(false);
    postingDateEdit_->setEnabled(false);
    referenceLineEdit_->setEnabled(false);
    totalDebitLineEdit_->setEnabled(false);
    totalCreditLineEdit_->setEnabled(false);
    postedByLineEdit_->setEnabled(false);
    journalEntryStatusComboBox_->setEnabled(false);
    isPostedCheckBox_->setEnabled(false);


    // Clear buttons always enabled
    clearGLAccountFormButton_->setEnabled(true);
    clearJournalEntryFormButton_->setEnabled(true);
}


} // namespace Finance
} // namespace UI
} // namespace ERP