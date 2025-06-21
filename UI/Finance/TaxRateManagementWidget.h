// UI/Finance/TaxRateManagementWidget.h
#ifndef UI_FINANCE_TAXRATEMANAGEMENTWIDGET_H
#define UI_FINANCE_TAXRATEMANAGEMENTWIDGET_H
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

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "TaxService.h"            // Dịch vụ thuế
#include "ISecurityManager.h"      // Dịch vụ bảo mật
#include "Logger.h"                // Logging
#include "ErrorHandler.h"          // Xử lý lỗi
#include "Common.h"                // Các enum chung
#include "DateUtils.h"             // Xử lý ngày tháng
#include "StringUtils.h"           // Xử lý chuỗi
#include "CustomMessageBox.h"      // Hộp thoại thông báo tùy chỉnh
#include "TaxRate.h"               // TaxRate DTO

namespace ERP {
namespace UI {
namespace Finance {

/**
 * @brief TaxRateManagementWidget class provides a UI for managing Tax Rates.
 * This widget allows viewing, creating, updating, and deleting tax rates.
 */
class TaxRateManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for TaxRateManagementWidget.
     * @param taxService Shared pointer to ITaxService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit TaxRateManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::ITaxService> taxService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~TaxRateManagementWidget();

private slots:
    void loadTaxRates();
    void onAddTaxRateClicked();
    void onEditTaxRateClicked();
    void onDeleteTaxRateClicked();
    void onUpdateTaxRateStatusClicked();
    void onSearchTaxRateClicked();
    void onTaxRateTableItemClicked(int row, int column);
    void clearForm();

private:
    std::shared_ptr<Services::ITaxService> taxService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *taxRateTable_;
    QPushButton *addTaxRateButton_;
    QPushButton *editTaxRateButton_;
    QPushButton *deleteTaxRateButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;

    // Form elements for editing/adding
    QLineEdit *idLineEdit_;
    QLineEdit *nameLineEdit_;
    QLineEdit *rateLineEdit_;
    QLineEdit *descriptionLineEdit_;
    QDateTimeEdit *effectiveDateEdit_;
    QDateTimeEdit *expirationDateEdit_;
    QComboBox *statusComboBox_; // EntityStatus

    // Helper functions
    void setupUI();
    void populateStatusComboBox(); // For entity status
    void showTaxRateInputDialog(ERP::Finance::DTO::TaxRateDTO* taxRate = nullptr);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Finance
} // namespace UI
} // namespace ERP

#endif // UI_FINANCE_TAXRATEMANAGEMENTWIDGET_H