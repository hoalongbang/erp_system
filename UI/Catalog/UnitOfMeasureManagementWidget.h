// UI/Catalog/UnitOfMeasureManagementWidget.h
#ifndef UI_CATALOG_UNITOFMEASUREMANAGEMENTWIDGET_H
#define UI_CATALOG_UNITOFMEASUREMANAGEMENTWIDGET_H
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
#include <QDialog> // For UoM input dialog

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "UnitOfMeasureService.h" // Dịch vụ đơn vị đo
#include "ISecurityManager.h"  // Dịch vụ bảo mật
#include "Logger.h"          // Logging
#include "ErrorHandler.h"    // Xử lý lỗi
#include "Common.h"          // Các enum chung
#include "DateUtils.h"       // Xử lý ngày tháng
#include "StringUtils.h"     // Xử lý chuỗi
#include "CustomMessageBox.h" // Hộp thoại thông báo tùy chỉnh
#include "UnitOfMeasure.h"   // UoM DTO (for enums etc.)

namespace ERP {
namespace UI {
namespace Catalog {

/**
 * @brief UnitOfMeasureManagementWidget class provides a UI for managing Units of Measure.
 * This widget allows viewing, creating, updating, deleting, and changing UoM status.
 */
class UnitOfMeasureManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for UnitOfMeasureManagementWidget.
     * @param unitOfMeasureService Shared pointer to IUnitOfMeasureService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit UnitOfMeasureManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IUnitOfMeasureService> unitOfMeasureService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~UnitOfMeasureManagementWidget();

private slots:
    void loadUnitsOfMeasure();
    void onAddUnitOfMeasureClicked();
    void onEditUnitOfMeasureClicked();
    void onDeleteUnitOfMeasureClicked();
    void onUpdateUnitOfMeasureStatusClicked();
    void onSearchUnitOfMeasureClicked();
    void onUnitOfMeasureTableItemClicked(int row, int column);
    void clearForm();

private:
    std::shared_ptr<Services::IUnitOfMeasureService> unitOfMeasureService_;
    std::shared_ptr<Security::ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *unitOfMeasureTable_;
    QPushButton *addUnitOfMeasureButton_;
    QPushButton *editUnitOfMeasureButton_;
    QPushButton *deleteUnitOfMeasureButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;

    // Form elements for editing/adding
    QLineEdit *idLineEdit_;
    QLineEdit *nameLineEdit_;
    QLineEdit *symbolLineEdit_;
    QLineEdit *descriptionLineEdit_;
    QComboBox *statusComboBox_; // For status selection

    // Helper functions
    void setupUI();
    void showUnitOfMeasureInputDialog(ERP::Catalog::DTO::UnitOfMeasureDTO* uom = nullptr);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState(); // Control button enable/disable based on selection and permissions
    
    // Permission checking helper
    bool hasPermission(const std::string& permission);
};

} // namespace Catalog
} // namespace UI
} // namespace ERP

#endif // UI_CATALOG_UNITOFMEASUREMANAGEMENTWIDGET_H