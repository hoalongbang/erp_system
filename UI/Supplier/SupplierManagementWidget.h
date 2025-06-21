// UI/Supplier/SupplierManagementWidget.h
#ifndef UI_SUPPLIER_SUPPLIERMANAGEMENTWIDGET_H
#define UI_SUPPLIER_SUPPLIERMANAGEMENTWIDGET_H
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
#include <QDialog> // For supplier input dialog
#include <QDoubleValidator> // For numeric input

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "SupplierService.h" // Dịch vụ nhà cung cấp
#include "ISecurityManager.h"  // Dịch vụ bảo mật
#include "Logger.h"          // Logging
#include "ErrorHandler.h"    // Xử lý lỗi
#include "Common.h"          // Các enum chung
#include "DateUtils.h"       // Xử lý ngày tháng
#include "StringUtils.h"     // Xử lý chuỗi
#include "CustomMessageBox.h" // Hộp thoại thông báo tùy chỉnh
#include "Supplier.h"        // Supplier DTO (for enums etc.)
#include "ContactPersonDTO.h" // Common DTO
#include "AddressDTO.h"       // Common DTO

namespace ERP {
namespace UI {
namespace Supplier {

/**
 * @brief SupplierManagementWidget class provides a UI for managing Supplier accounts.
 * This widget allows viewing, creating, updating, deleting, and changing supplier status.
 */
class SupplierManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for SupplierManagementWidget.
     * @param supplierService Shared pointer to ISupplierService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit SupplierManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::ISupplierService> supplierService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~SupplierManagementWidget();

private slots:
    void loadSuppliers();
    void onAddSupplierClicked();
    void onEditSupplierClicked();
    void onDeleteSupplierClicked();
    void onUpdateSupplierStatusClicked();
    void onSearchSupplierClicked();
    void onSupplierTableItemClicked(int row, int column);
    void clearForm();

private:
    std::shared_ptr<Services::ISupplierService> supplierService_;
    std::shared_ptr<Security::ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *supplierTable_;
    QPushButton *addSupplierButton_;
    QPushButton *editSupplierButton_;
    QPushButton *deleteSupplierButton_;
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
    QLineEdit *defaultDeliveryTermsLineEdit_;
    QComboBox *statusComboBox_; // EntityStatus

    // Helper functions
    void setupUI();
    void showSupplierInputDialog(ERP::Supplier::DTO::SupplierDTO* supplier = nullptr);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState(); // Control button enable/disable based on selection and permissions
    
    // Permission checking helper
    bool hasPermission(const std::string& permission);
};

} // namespace Supplier
} // namespace UI
} // namespace ERP

#endif // UI_SUPPLIER_SUPPLIERMANAGEMENTWIDGET_H