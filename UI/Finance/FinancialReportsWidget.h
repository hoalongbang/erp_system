// UI/Finance/FinancialReportsWidget.h
#ifndef UI_FINANCE_FINANCIALREPORTSWIDGET_H
#define UI_FINANCE_FINANCIALREPORTSWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QDateEdit>
#include <QTableWidget>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>
#include <chrono>

// Rút gọn các include paths
#include "GeneralLedgerService.h"
#include "ISecurityManager.h"
#include "Logger.h"
#include "ErrorHandler.h"
#include "Common.h"
#include "DateUtils.h"
#include "StringUtils.h"
#include "CustomMessageBox.h" // For Common::CustomMessageBox

namespace ERP {
namespace UI {
namespace Finance {

/**
 * @brief FinancialReportsWidget class provides a UI for generating various financial reports.
 * This widget allows users to generate Trial Balance, Balance Sheet, Income Statement, and Cash Flow Statement.
 */
class FinancialReportsWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for FinancialReportsWidget.
     * @param generalLedgerService Shared pointer to IGeneralLedgerService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit FinancialReportsWidget(
        QWidget* parent = nullptr, // Make parent first, default nullptr
        std::shared_ptr<ERP::Finance::Services::IGeneralLedgerService> generalLedgerService = nullptr,
        std::shared_ptr<ERP::Security::ISecurityManager> securityManager = nullptr);

private slots:
    /**
     * @brief Slot to handle generating the selected report.
     */
    void generateReport();

private:
    std::shared_ptr<ERP::Finance::Services::IGeneralLedgerService> generalLedgerService_;
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager_;
    std::string currentUserId_;         // Populated from securityManager_
    std::vector<std::string> currentUserRoleIds_; // Populated from securityManager_

    // UI Elements
    QComboBox* reportTypeComboBox_;
    QDateEdit* startDateEdit_;
    QDateEdit* endDateEdit_;
    QDateEdit* asOfDateEdit_; // For Balance Sheet
    QPushButton* generateReportButton_;
    QTableWidget* reportTable_;
    QLabel* reportTitleLabel_;

    // Helper functions
    void setupUI();
    void updateDateControlsVisibility();
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
};

} // namespace Finance
} // namespace UI
} // namespace ERP

#endif // UI_FINANCE_FINANCIALREPORTSWIDGET_H