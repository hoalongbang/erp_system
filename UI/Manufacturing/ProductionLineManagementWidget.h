// UI/Manufacturing/ProductionLineManagementWidget.h
#ifndef UI_MANUFACTURING_PRODUCTIONLINEMANAGEMENTWIDGET_H
#define UI_MANUFACTURING_PRODUCTIONLINEMANAGEMENTWIDGET_H
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
#include <QListWidget> // For associated assets

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "ProductionLineService.h"  // Dịch vụ dây chuyền sản xuất
#include "LocationService.h"        // Dịch vụ vị trí
#include "AssetManagementService.h" // Dịch vụ tài sản
#include "ISecurityManager.h"       // Dịch vụ bảo mật
#include "Logger.h"                 // Logging
#include "ErrorHandler.h"           // Xử lý lỗi
#include "Common.h"                 // Các enum chung
#include "DateUtils.h"              // Xử lý ngày tháng
#include "StringUtils.h"            // Xử lý chuỗi
#include "CustomMessageBox.h"       // Hộp thoại thông báo tùy chỉnh
#include "ProductionLine.h"         // ProductionLine DTO
#include "Location.h"               // Location DTO
#include "Asset.h"                  // Asset DTO

namespace ERP {
namespace UI {
namespace Manufacturing {

/**
 * @brief ProductionLineManagementWidget class provides a UI for managing Production Lines.
 * This widget allows viewing, creating, updating, deleting, and changing line status.
 * It also supports managing associated assets (machines/equipment).
 */
class ProductionLineManagementWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor for ProductionLineManagementWidget.
     * @param productionLineService Shared pointer to IProductionLineService.
     * @param locationService Shared pointer to ILocationService.
     * @param assetService Shared pointer to IAssetManagementService.
     * @param securityManager Shared pointer to ISecurityManager.
     * @param parent Parent widget.
     */
    explicit ProductionLineManagementWidget(
        QWidget *parent = nullptr,
        std::shared_ptr<Services::IProductionLineService> productionLineService = nullptr,
        std::shared_ptr<Catalog::Services::ILocationService> locationService = nullptr,
        std::shared_ptr<Asset::Services::IAssetManagementService> assetService = nullptr,
        std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

    ~ProductionLineManagementWidget();

private slots:
    void loadProductionLines();
    void onAddLineClicked();
    void onEditLineClicked();
    void onDeleteLineClicked();
    void onUpdateLineStatusClicked();
    void onSearchLineClicked();
    void onLineTableItemClicked(int row, int int column);
    void clearForm();

    void onManageAssetsClicked(); // New slot for managing associated assets

private:
    std::shared_ptr<Services::IProductionLineService> productionLineService_;
    std::shared_ptr<Catalog::Services::ILocationService> locationService_;
    std::shared_ptr<Asset::Services::IAssetManagementService> assetService_;
    std::shared_ptr<Security::ISecurityManager> securityManager_;
    // Current user context
    std::string currentUserId_;
    std::vector<std::string> currentUserRoleIds_;

    QTableWidget *lineTable_;
    QPushButton *addLineButton_;
    QPushButton *editLineButton_;
    QPushButton *deleteLineButton_;
    QPushButton *updateStatusButton_;
    QPushButton *searchButton_;
    QLineEdit *searchLineEdit_;
    QPushButton *clearFormButton_;
    QPushButton *manageAssetsButton_;

    // Form elements for editing/adding
    QLineEdit *idLineEdit_;
    QLineEdit *lineNameLineEdit_;
    QLineEdit *descriptionLineEdit_;
    QComboBox *statusComboBox_; // Line Status
    QComboBox *locationComboBox_; // Physical location
    // Associated assets are managed via a separate dialog

    // Helper functions
    void setupUI();
    void populateLocationComboBox();
    void populateStatusComboBox(); // For production line status
    void showLineInputDialog(ERP::Manufacturing::DTO::ProductionLineDTO* line = nullptr);
    void showManageAssetsDialog(ERP::Manufacturing::DTO::ProductionLineDTO* line);
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
    void updateButtonsState();
    
    bool hasPermission(const std::string& permission);
};

} // namespace Manufacturing
} // namespace UI
} // namespace ERP

#endif // UI_MANUFACTURING_PRODUCTIONLINEMANAGEMENTWIDGET_H