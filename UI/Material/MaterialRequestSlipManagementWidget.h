// UI/Material/MaterialRequestSlipManagementWidget.h
#ifndef UI_MATERIAL_MATERIALREQUESTSLIPMANAGEMENTWIDGET_H
#define UI_MATERIAL_MATERIALREQUESTSLIPMANAGEMENTWIDGET_H
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
#include "MaterialRequestService.h"     // Dịch vụ yêu cầu vật tư
#include "ProductService.h"             // Dịch vụ sản phẩm
#include "ISecurityManager.h"           // Dịch vụ bảo mật
#include "Logger.h"                     // Logging
#include "ErrorHandler.h"               // Xử lý lỗi
#include "Common.h"                     // Các enum chung
#include "DateUtils.h"                  // Xử lý ngày tháng
#include "StringUtils.h"                // Xử lý chuỗi
#include "CustomMessageBox.h"           // Hộp thoại thông báo tùy chỉnh
#include "MaterialRequestSlip.h"        // MaterialRequestSlip DTO
#include "MaterialRequestSlipDetail.h"  // MaterialRequestSlipDetail DTO
#include "Product.h"                    // Product DTO (for display)

namespace ERP {
namespace UI {
namespace Material {

/**
 * @brief MaterialRequestSlipManagementWidget class provides a UI for managing Material Request Slips.
 * This widget allows viewing, creating, updating, deleting, and changing slip status.
 * It also supports managing slip details.
 */
class MaterialRequestSlipManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for MaterialRequestSlipManagementWidget.
     * @param materialRequestService Shared pointer to IMaterialRequestService.
     * @param productService Shared pointer to IProductService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit MaterialRequestSlipManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IMaterialRequestService> materialRequestService = nullptr,
        std::shared_ptr<Product::Services::IProductService> productService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~MaterialRequestSlipManagementWidget();

private slots:
    void loadMaterialRequestSlips();
    void onAddSlipClicked();
    void onEditSlipClicked();
    void onDeleteSlipClicked();
    void onUpdateSlipStatusClicked();
    void onSearchSlipClicked();
    void onSlipTableItemClicked(int row, int column);
    void clearForm();

    void onManageDetailsClicked(); // New slot for managing slip details

private:
    std::shared_ptr<Services::IMaterialRequestService> materialRequestService_;
    std::shared_ptr<Product::Services::IProductService> productService_;
    std::shared_ptr<ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *slipTable_;
    QPushButton *addSlipButton_;
    QPushButton *editSlipButton_;
    QPushButton *deleteSlipButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;
    QPushButton *manageDetailsButton_;

    // Form elements for editing/adding slips
    QLineEdit *idLineEdit_;
    QLineEdit *requestNumberLineEdit_;
    QLineEdit *requestingDepartmentLineEdit_;
    QLineEdit *requestedByLineEdit_; // User ID, display name
    QDateTimeEdit *requestDateEdit_;
    QComboBox *statusComboBox_; // Slip Status
    QLineEdit *approvedByLineEdit_; // User ID, display name
    QDateTimeEdit *approvalDateEdit_;
    QLineEdit *notesLineEdit_;
    QLineEdit *referenceDocumentIdLineEdit_;
    QLineEdit *referenceDocumentTypeLineEdit_;

    // Helper functions
    void setupUI();
    void populateStatusComboBox(); // For material request slip status
    void populateUserComboBox(QComboBox* comboBox); // Helper to populate combo with users
    void showSlipInputDialog(ERP::Material::DTO::MaterialRequestSlipDTO* slip = nullptr);
    void showManageDetailsDialog(ERP::Material::DTO::MaterialRequestSlipDTO* slip);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Material
} // namespace UI
} // namespace ERP

#endif // UI_MATERIAL_MATERIALREQUESTSLIPMANAGEMENTWIDGET_H