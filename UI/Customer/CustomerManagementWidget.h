// UI/Customer/CustomerManagementWidget.h
#ifndef UI_CUSTOMER_CUSTOMERMANAGEMENTWIDGET_H
#define UI_CUSTOMER_CUSTOMERMANAGEMENTWIDGET_H
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
#include <QDialog> // For customer input dialog
#include <QDoubleValidator> // For credit limit

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "CustomerService.h" // Dịch vụ khách hàng
#include "ISecurityManager.h"  // Dịch vụ bảo mật
#include "Logger.h"          // Logging
#include "ErrorHandler.h"    // Xử lý lỗi
#include "Common.h"          // Các enum chung
#include "DateUtils.h"       // Xử lý ngày tháng
#include "StringUtils.h"     // Xử lý chuỗi
#include "CustomMessageBox.h" // Hộp thoại thông báo tùy chỉnh
#include "Customer.h"        // Customer DTO (for enums etc.)
#include "ContactPersonDTO.h" // Common DTO
#include "AddressDTO.h"       // Common DTO

namespace ERP {
namespace UI {
namespace Customer {

/**
 * @brief CustomerManagementWidget class provides a UI for managing Customer accounts.
 * This widget allows viewing, creating, updating, deleting, and changing customer status.
 */
class CustomerManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for CustomerManagementWidget.
     * @param customerService Shared pointer to ICustomerService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit CustomerManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::ICustomerService> customerService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~CustomerManagementWidget();

private slots:
    void loadCustomers();
    void onAddCustomerClicked();
    void onEditCustomerClicked();
    void onDeleteCustomerClicked();
    void onUpdateCustomerStatusClicked();
    void onSearchCustomerClicked();
    void onCustomerTableItemClicked(int row, int column);
    void clearForm();

private:
    std::shared_ptr<Services::ICustomerService> customerService_;
    std::shared_ptr<Security::ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *customerTable_;
    QPushButton *addCustomerButton_;
    QPushButton *editCustomerButton_;
    QPushButton *deleteCustomerButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;

    // Form elements for editing/adding
    QLineEdit *idLineEdit_;
    QLineEdit *nameLineEdit_;
    QLineEdit *taxIdLineEdit_;
    QLineEdit *notesLineEdit_;
    QLineEdit *defaultPaymentTermsLineEdit_;
    QLineEdit *creditLimitLineEdit_;
    QComboBox *statusComboBox_; // EntityStatus

    // New widgets for Contact Persons and Addresses (will likely need separate widgets/dialogs)
    // QTableWidget *contactPersonsTable_;
    // QPushButton *addContactPersonButton_;
    // QPushButton *editContactPersonButton_;
    // QPushButton *deleteContactPersonButton_;
    // QTableWidget *addressesTable_;
    // QPushButton *addAddressButton_;
    // QPushButton *editAddressButton_;
    // QPushButton *deleteAddressButton_;


    // Helper functions
    void setupUI();
    void showCustomerInputDialog(ERP::Customer::DTO::CustomerDTO* customer = nullptr);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState(); // Control button enable/disable based on selection and permissions
    
    // Permission checking helper
    bool hasPermission(const std::string& permission);
};

} // namespace Customer
} // namespace UI
} // namespace ERP

#endif // UI_CUSTOMER_CUSTOMERMANAGEMENTWIDGET_H