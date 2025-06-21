// UI/mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QMap>
#include <QPushButton> // For the new navigation buttons
#include <QToolBox>    // For the new vertical navigation
#include <QStatusBar>  // For programmatic status bar
#include <QMessageBox> // For showMessageBox

#include <memory>
#include <string>
#include <vector>
#include <optional>

// Forward declarations for Services to break circular dependencies
namespace ERP { namespace Security { class ISecurityManager; }}
namespace ERP { namespace Security { namespace Service { class IAuthenticationService; }}}
namespace ERP { namespace Security { namespace DTO { struct SessionDTO; }}} // For currentSession_
namespace ERP { namespace Security { namespace Service { class IUserService; }}} // For user roles
namespace ERP { namespace Catalog { namespace Services { class ICategoryService; }}}
namespace ERP { namespace Catalog { namespace Services { class ILocationService; }}}
namespace ERP { namespace Catalog { namespace Services { class IUnitOfMeasureService; }}}
namespace ERP { namespace Catalog { namespace Services { class IWarehouseService; }}}
namespace ERP { namespace Catalog { namespace Services { class IRoleService; }}}
namespace ERP { namespace Catalog { namespace Services { class IPermissionService; }}}
namespace ERP { namespace Product { namespace Services { class IProductService; }}}
namespace ERP { namespace Customer { namespace Services { class ICustomerService; }}}
namespace ERP { namespace Supplier { namespace Services { class ISupplierService; }}}
namespace ERP { namespace Sales { namespace Services { class ISalesOrderService; }}}
namespace ERP { namespace Sales { namespace Services { class IInvoiceService; }}}
namespace ERP { namespace Sales { namespace Services { class IPaymentService; }}}
namespace ERP { namespace Sales { namespace Services { class IShipmentService; }}}
namespace ERP { namespace Sales { namespace Services { class IQuotationService; }}}
namespace ERP { namespace Sales { namespace Services { class IReturnService; }}}
namespace ERP { namespace Finance { namespace Services { class IAccountReceivableService; }}}
namespace ERP { namespace Finance { namespace Services { class IGeneralLedgerService; }}}
namespace ERP { namespace Finance { namespace Services { class ITaxService; }}}
namespace ERP { namespace Document { namespace Services { class IDocumentService; }}}
namespace ERP { namespace Asset { namespace Services { class IAssetManagementService; }}}
namespace ERP { namespace Config { namespace Services { class IConfigService; }}}
namespace ERP { namespace Integration { namespace Services { class IDeviceManagerService; }}}
namespace ERP { namespace Integration { namespace Services { class IExternalSystemService; }}}
namespace ERP { namespace Notification { namespace Services { class INotificationService; }}}
namespace ERP { namespace Manufacturing { namespace Services { class IBillOfMaterialService; }}}
namespace ERP { namespace Manufacturing { namespace Services { class IMaintenanceManagementService; }}}
namespace ERP { namespace Manufacturing { namespace Services { class IProductionLineService; }}}
namespace ERP { namespace Manufacturing { namespace Services { class IProductionOrderService; }}}
namespace ERP { namespace Material { namespace Services { class IIssueSlipService; }}}
namespace ERP { namespace Material { namespace Services { class IMaterialRequestService; }}}
namespace ERP { namespace Material { namespace Services { class IMaterialIssueSlipService; }}}
namespace ERP { namespace Material { namespace Services { class IReceiptSlipService; }}}
namespace ERP { namespace Report { namespace Services { class IReportService; }}}
namespace ERP { namespace Scheduler { namespace Services { class IScheduledTaskService; }}}
namespace ERP { namespace Scheduler { namespace Services { class ITaskExecutionLogService; }}}
namespace ERP { namespace TaskEngine { class TaskEngine; }} // For TaskEngine singleton
namespace ERP { namespace Warehouse { namespace Services { class IInventoryManagementService; }}}
namespace ERP { namespace Warehouse { namespace Services { class IPickingService; }}}
namespace ERP { namespace Warehouse { namespace Services { class IStocktakeService; }}}
namespace ERP { namespace Warehouse { namespace Services { class IInventoryTransactionService; }}} // NEW: For InventoryTransactionService


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace ERP {
namespace UI {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // Constructor takes a shared_ptr to ISecurityManager to access all services
    MainWindow(QWidget *parent = nullptr, std::shared_ptr<ERP::Security::ISecurityManager> securityManager = nullptr);
    ~MainWindow();

    // Slot to handle successful login
    void onLoginSuccess(const QString& username, const QString& userId, const QString& sessionId);
    // Slot to handle logout request
    void onLogoutRequested();
    // Slots for registration flow
    void onRegisterRequested();
    void onBackToLoginRequested();

    // Helper to load module widgets dynamically
    void loadModuleWidget(const QString& moduleName, QWidget* widget);

    // Getter for currentUserId (useful for UI widgets' constructors)
    std::string getCurrentUserId() const;

private slots:
    // Slot for handling clicks on navigation buttons in QToolBox
    void onNavigationButtonClicked();

private:
    Ui::MainWindow *ui;
    std::shared_ptr<ERP::Security::ISecurityManager> securityManager_;
    std::optional<ERP::Security::DTO::SessionDTO> currentSession_; // Stores current user session
    std::string currentUserId_; // Store current user ID
    std::vector<std::string> currentUserRoleIds_; // Store current user's role IDs

    QStackedWidget *stackedWidget_; // Manages different UI modules
    QMap<QString, QWidget*> moduleWidgets_; // Maps module names to their widgets

    // Login/Registration Forms (managed directly by MainWindow)
    QWidget *loginForm_;
    QWidget *registerForm_;

    // NEW: UI Elements for Vertical Navigation
    QToolBox *toolBoxMainNavigation_;
    QStatusBar *statusBar_; // Programmatic status bar

    // Helper functions
    void setupUI();
    void setupLoginScreen();
    void setupMainApplicationUI();
    void updateUIForPermissions(); // Dynamically enables/disables UI elements based on permissions
    void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);

    // Permission checking helper
    bool hasPermission(const std::string& permission);
};

} // namespace UI
} // namespace ERP

#endif // MAINWINDOW_H