// UI/mainwindow.cpp
#include "mainwindow.h"
#include "./ui_mainwindow.h" // Generated UI header (from .ui file)
#include "loginform.h"
#include "registerform.h"
#include "Session.h"       // For SessionDTO
#include "User.h"          // For UserDTO
#include "Logger.h"        // For logging
#include "ErrorHandler.h"  // For error handling
#include "Common.h"        // For common enums/constants
#include "DateUtils.h"     // For date/time conversions
#include "StringUtils.h"   // For string utilities
#include "CustomMessageBox.h" // For custom message boxes

// Services Interfaces (needed by MainWindow to interact with core logic)
#include "ISecurityManager.h"
#include "IAuthenticationService.h"
#include "IAuthorizationService.h"
#include "IUserService.h"
#include "ICategoryService.h"
#include "ILocationService.h"
#include "IUnitOfMeasureService.h"
#include "IWarehouseService.h"
#include "IRoleService.h"
#include "IPermissionService.h"
#include "IProductService.h"
#include "ICustomerService.h"
#include "ISupplierService.h"
#include "IInventoryManagementService.h"
#include "IPickingService.h"
#include "IStocktakeService.h"
#include "IInventoryTransactionService.h" // NEW
#include "IReceiptSlipService.h"
#include "IIssueSlipService.h"
#include "IMaterialRequestService.h"
#include "IMaterialIssueSlipService.h"
#include "ISalesOrderService.h"
#include "IInvoiceService.h"
#include "IPaymentService.h"
#include "IShipmentService.h"
#include "IQuotationService.h"
#include "IReturnService.h"
#include "IAccountReceivableService.h"
#include "IGeneralLedgerService.h"
#include "ITaxService.h"
#include "IDocumentService.h"
#include "IAssetManagementService.h"
#include "IConfigService.h"
#include "IDeviceManagerService.h"
#include "IExternalSystemService.h"
#include "INotificationService.h"
#include "IBillOfMaterialService.h"
#include "IMaintenanceManagementService.h"
#include "IProductionLineService.h"
#include "IProductionOrderService.h"
#include "IReportService.h"
#include "IScheduledTaskService.h"
#include "ITaskExecutionLogService.h"
#include "ISessionService.h"

// Specific UI Widgets (to be dynamically loaded)
#include "CategoryManagementWidget.h"
#include "LocationManagementWidget.h"
#include "UnitOfMeasureManagementWidget.h"
#include "WarehouseManagementWidget.h"
#include "RoleManagementWidget.h"
#include "PermissionManagementWidget.h"
#include "ProductManagementWidget.h"
#include "CustomerManagementWidget.h"
#include "SupplierManagementWidget.h"
#include "SessionManagementWidget.h"
#include "AuditLogViewerWidget.h"
#include "InvoiceManagementWidget.h"
#include "PaymentManagementWidget.h"
#include "QuotationManagementWidget.h"
#include "SalesOrderManagementWidget.h"
#include "ShipmentManagementWidget.h"
#include "BillOfMaterialManagementWidget.h"
#include "MaintenanceManagementWidget.h"
#include "ProductionLineManagementWidget.h"
#include "ProductionOrderManagementWidget.h"
#include "ReceiptSlipManagementWidget.h"
#include "IssueSlipManagementWidget.h"
#include "MaterialRequestSlipManagementWidget.h"
#include "MaterialIssueSlipManagementWidget.h"
#include "InventoryManagementWidget.h"
#include "PickingRequestManagementWidget.h"
#include "StocktakeRequestManagementWidget.h"
#include "InventoryTransactionManagementWidget.h" // NEW
#include "AccountReceivableManagementWidget.h"
#include "GeneralLedgerManagementWidget.h"
#include "TaxRateManagementWidget.h"
#include "FinancialReportsWidget.h"
#include "NotificationManagementWidget.h"
#include "ReportManagementWidget.h"
#include "ScheduledTaskManagementWidget.h"
#include "TaskExecutionLogManagementWidget.h"
#include "DeviceManagementWidget.h"
#include "ExternalSystemManagementWidget.h"
#include "ReturnManagementWidget.h"

#include <QVBoxLayout> // For layouts
#include <QHBoxLayout>
#include <QAbstractButton> // For connecting multiple buttons to one slot

namespace ERP {
namespace UI {

MainWindow::MainWindow(QWidget *parent, std::shared_ptr<ERP::Security::ISecurityManager> securityManager)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      securityManager_(securityManager) {
    
    // Ensure securityManager is valid, otherwise critical error.
    if (!securityManager_) {
        QMessageBox::critical(this, "Lỗi Khởi Tạo", "Dịch vụ bảo mật không khả dụng. Ứng dụng sẽ thoát.");
        ERP::Logger::Logger::getInstance().critical("MainWindow: SecurityManager is null during initialization. Exiting application.");
        QCoreApplication::quit(); // Exit application if critical dependency is missing
        return;
    }

    // Initialize UI from .ui file
    ui->setupUi(this);

    // Initialize programmatic status bar and set it
    statusBar_ = new QStatusBar(this);
    setStatusBar(statusBar_);
    statusBar_->showMessage("Vui lòng đăng nhập.");


    // Setup main QStackedWidget and assign it to centralwidget
    // The centralwidget in .ui is just a placeholder, we use it to attach our stackedWidget
    stackedWidget_ = new QStackedWidget(this);
    // Find the horizontalLayout that contains toolBoxMainNavigation and add stackedWidget_ to it.
    // Assuming the centralwidget in .ui has a QHBoxLayout named "horizontalLayout"
    if (ui->centralwidget->layout()) {
        QHBoxLayout* hLayout = qobject_cast<QHBoxLayout*>(ui->centralwidget->layout());
        if (hLayout) {
            hLayout->addWidget(stackedWidget_);
            // The horizontal spacer might need to be removed if it's there and you want stackedWidget to fill remaining space
            // Or adjust its stretch factor in .ui. For now, it's added to the right.
        } else {
            // Fallback if layout not found, this should ideally not happen if .ui is set up as expected.
            ui->centralwidget->setLayout(new QHBoxLayout());
            ui->centralwidget->layout()->addWidget(ui->toolBoxMainNavigation); // Re-add toolBox if layout was replaced
            ui->centralwidget->layout()->addWidget(stackedWidget_);
            ERP::Logger::Logger::getInstance().warning("MainWindow: Expected QHBoxLayout not found in centralwidget. Created new one.");
        }
    } else {
        // If no layout at all, set a new QHBoxLayout
        ui->centralwidget->setLayout(new QHBoxLayout());
        ui->centralwidget->layout()->addWidget(ui->toolBoxMainNavigation);
        ui->centralwidget->layout()->addWidget(stackedWidget_);
    }


    // Login and Register Forms (managed directly by MainWindow, outside the moduleWidgets_ map initially)
    loginForm_ = new LoginForm(this, securityManager_->getAuthenticationService(), securityManager_->getUserService());
    registerForm_ = new RegisterForm(this, securityManager_->getUserService());

    stackedWidget_->addWidget(loginForm_);    // Index 0: Login Form
    stackedWidget_->addWidget(registerForm_); // Index 1: Register Form

    connect(loginForm_, &LoginForm::loginSuccess, this, &MainWindow::onLoginSuccess);
    connect(loginForm_, &LoginForm::registerRequested, this, &MainWindow::onRegisterRequested);
    connect(registerForm_, &RegisterForm::backToLoginRequested, this, &MainWindow::onBackToLoginRequested);

    // Connect all navigation buttons to a single slot
    // Collect all QPushButton objects from QToolBox pages
    QList<QPushButton*> navigationButtons = ui->toolBoxMainNavigation->findChildren<QPushButton*>();
    for (QPushButton* button : navigationButtons) {
        connect(button, &QPushButton::clicked, this, &MainWindow::onNavigationButtonClicked);
    }
    
    // Initial setup
    setupLoginScreen();
    // Initially, navigation toolbox is disabled until login
    ui->toolBoxMainNavigation->setEnabled(false);
}

MainWindow::~MainWindow() {
    delete ui;
    // statusbar_ and stackedWidget_ are children of QMainWindow, deleted automatically
}

std::string MainWindow::getCurrentUserId() const {
    if (currentSession_ && !currentSession_->userId.empty()) {
        return currentSession_->userId;
    }
    return "unknown_user"; // Fallback
}

void MainWindow::setupLoginScreen() {
    stackedWidget_->setCurrentWidget(loginForm_);
    statusBar_->showMessage("Vui lòng đăng nhập.");
    ui->toolBoxMainNavigation->setEnabled(false); // Ensure navigation is disabled
}

void MainWindow::setupMainApplicationUI() {
    // This is called after successful login.
    statusBar_->showMessage("Đăng nhập thành công!");
    ui->toolBoxMainNavigation->setEnabled(true); // Enable navigation

    // Clear existing module widgets if any (e.g., after logout/re-login)
    for (QWidget* widget : moduleWidgets_.values()) {
        stackedWidget_->removeWidget(widget);
        delete widget;
    }
    moduleWidgets_.clear();

    // Create UI widgets, passing necessary service dependencies
    // Order matters here: dependencies between widgets, and passing already initialized services.

    // Catalog Module UI
    loadModuleWidget("Categories", new ERP::UI::Catalog::CategoryManagementWidget(this, securityManager_->getCategoryService(), securityManager_));
    loadModuleWidget("Locations", new ERP::UI::Catalog::LocationManagementWidget(this, securityManager_->getLocationService(), securityManager_->getWarehouseService(), securityManager_));
    loadModuleWidget("UnitsOfMeasure", new ERP::UI::Catalog::UnitOfMeasureManagementWidget(this, securityManager_->getUnitOfMeasureService(), securityManager_));
    loadModuleWidget("Warehouses", new ERP::UI::Catalog::WarehouseManagementWidget(this, securityManager_->getWarehouseService(), securityManager_->getLocationService(), securityManager_));
    loadModuleWidget("Roles", new ERP::UI::Catalog::RoleManagementWidget(this, securityManager_->getRoleService(), securityManager_->getPermissionService(), securityManager_));
    loadModuleWidget("Permissions", new ERP::UI::Catalog::PermissionManagementWidget(this, securityManager_->getPermissionService(), securityManager_));

    // Product Module UI
    loadModuleWidget("Products", new ERP::UI::Product::ProductManagementWidget(this, securityManager_->getProductService(), securityManager_->getCategoryService(), securityManager_->getUnitOfMeasureService(), securityManager_));

    // Customer Module UI
    loadModuleWidget("Customers", new ERP::UI::Customer::CustomerManagementWidget(this, securityManager_->getCustomerService(), securityManager_));

    // Supplier Module UI
    loadModuleWidget("Suppliers", new ERP::UI::Supplier::SupplierManagementWidget(this, securityManager_->getSupplierService(), securityManager_));

    // User Module UI
    loadModuleWidget("Users", new ERP::UI::User::UserManagementWidget(this, securityManager_->getUserService(), securityManager_->getRoleService(), securityManager_));

    // Sales Module UI
    loadModuleWidget("SalesOrders", new ERP::UI::Sales::SalesOrderManagementWidget(this, securityManager_->getSalesOrderService(), securityManager_->getCustomerService(), securityManager_->getWarehouseService(), securityManager_->getProductService(), securityManager_));
    loadModuleWidget("Invoices", new ERP::UI::Sales::InvoiceManagementWidget(this, securityManager_->getInvoiceService(), securityManager_->getSalesOrderService(), securityManager_));
    loadModuleWidget("Payments", new ERP::UI::Sales::PaymentManagementWidget(this, securityManager_->getPaymentService(), securityManager_->getCustomerService(), securityManager_->getInvoiceService(), securityManager_));
    loadModuleWidget("Quotations", new ERP::UI::Sales::QuotationManagementWidget(this, securityManager_->getQuotationService(), securityManager_->getCustomerService(), securityManager_->getProductService(), securityManager_->getUnitOfMeasureService(), securityManager_->getSalesOrderService(), securityManager_));
    loadModuleWidget("Shipments", new ERP::UI::Sales::ShipmentManagementWidget(this, securityManager_->getShipmentService(), securityManager_->getSalesOrderService(), securityManager_->getCustomerService(), securityManager_->getProductService(), securityManager_->getWarehouseService(), securityManager_));
    loadModuleWidget("Returns", new ERP::UI::Sales::ReturnManagementWidget(this, securityManager_->getReturnService(), securityManager_->getSalesOrderService(), securityManager_->getCustomerService(), securityManager_->getWarehouseService(), securityManager_->getProductService(), securityManager_->getInventoryManagementService(), securityManager_));

    // Manufacturing Module UI
    loadModuleWidget("BillOfMaterials", new ERP::UI::Manufacturing::BillOfMaterialManagementWidget(this, securityManager_->getBillOfMaterialService(), securityManager_->getProductService(), securityManager_->getUnitOfMeasureService(), securityManager_));
    loadModuleWidget("Maintenance", new ERP::UI::Manufacturing::MaintenanceManagementWidget(this, securityManager_->getMaintenanceManagementService(), securityManager_->getAssetManagementService(), securityManager_));
    loadModuleWidget("ProductionLines", new ERP::UI::Manufacturing::ProductionLineManagementWidget(this, securityManager_->getProductionLineService(), securityManager_->getLocationService(), securityManager_->getAssetManagementService(), securityManager_));
    loadModuleWidget("ProductionOrders", new ERP::UI::Manufacturing::ProductionOrderManagementWidget(this, securityManager_->getProductionOrderService(), securityManager_->getProductService(), securityManager_->getBillOfMaterialService(), securityManager_->getProductionLineService(), securityManager_));

    // Material Module UI
    loadModuleWidget("ReceiptSlips", new ERP::UI::Material::ReceiptSlipManagementWidget(this, securityManager_->getReceiptSlipService(), securityManager_->getProductService(), securityManager_->getWarehouseService(), securityManager_->getInventoryManagementService(), securityManager_));
    loadModuleWidget("IssueSlips", new ERP::UI::Material::IssueSlipManagementWidget(this, securityManager_->getIssueSlipService(), securityManager_->getProductService(), securityManager_->getWarehouseService(), securityManager_->getInventoryManagementService(), securityManager_->getMaterialRequestService(), securityManager_));
    loadModuleWidget("MaterialRequests", new ERP::UI::Material::MaterialRequestSlipManagementWidget(this, securityManager_->getMaterialRequestService(), securityManager_->getProductService(), securityManager_));
    loadModuleWidget("MaterialIssueSlips", new ERP::UI::Material::MaterialIssueSlipManagementWidget(this, securityManager_->getMaterialIssueSlipService(), securityManager_->getProductionOrderService(), securityManager_->getProductService(), securityManager_->getWarehouseService(), securityManager_->getInventoryManagementService(), securityManager_));

    // Warehouse Module UI
    loadModuleWidget("Inventory", new ERP::UI::Warehouse::InventoryManagementWidget(this, securityManager_->getInventoryManagementService(), securityManager_->getProductService(), securityManager_->getWarehouseService(), securityManager_));
    loadModuleWidget("PickingRequests", new ERP::UI::Warehouse::PickingRequestManagementWidget(this, securityManager_->getPickingService(), securityManager_->getSalesOrderService(), securityManager_->getInventoryManagementService(), securityManager_));
    loadModuleWidget("StocktakeRequests", new ERP::UI::Warehouse::StocktakeRequestManagementWidget(this, securityManager_->getStocktakeService(), securityManager_->getInventoryManagementService(), securityManager_->getWarehouseService(), securityManager_));
    loadModuleWidget("InventoryTransactions", new ERP::UI::Warehouse::InventoryTransactionManagementWidget(this, securityManager_->getInventoryTransactionService(), securityManager_->getProductService(), securityManager_->getWarehouseService(), securityManager_->getLocationService(), securityManager_));

    // Finance Module UI
    loadModuleWidget("AccountReceivable", new ERP::UI::Finance::AccountReceivableManagementWidget(this, securityManager_->getAccountReceivableService(), securityManager_->getCustomerService(), securityManager_->getInvoiceService(), securityManager->getPaymentService(), securityManager_));
    loadModuleWidget("GeneralLedger", new ERP::UI::Finance::GeneralLedgerManagementWidget(this, securityManager_->getGeneralLedgerService(), securityManager_));
    loadModuleWidget("TaxRates", new ERP::UI::Finance::TaxRateManagementWidget(this, securityManager_->getTaxService(), securityManager_));
    loadModuleWidget("FinancialReports", new ERP::UI::Finance::FinancialReportsWidget(this, securityManager_->getGeneralLedgerService(), securityManager_));

    // Integration Module UI
    loadModuleWidget("DeviceManagement", new ERP::UI::Integration::DeviceManagementWidget(this, securityManager_->getDeviceManagerService(), securityManager_));
    loadModuleWidget("ExternalSystems", new ERP::UI::Integration::ExternalSystemManagementWidget(this, securityManager_->getExternalSystemService(), securityManager_));

    // Notification Module UI
    loadModuleWidget("Notifications", new ERP::UI::Notification::NotificationManagementWidget(this, securityManager_->getNotificationService(), securityManager_));

    // Report Module UI
    loadModuleWidget("Reports", new ERP::UI::Report::ReportManagementWidget(this, securityManager_->getReportService(), securityManager_));

    // Scheduler Module UI
    loadModuleWidget("ScheduledTasks", new ERP::UI::Scheduler::ScheduledTaskManagementWidget(this, securityManager_->getScheduledTaskService(), securityManager_));
    loadModuleWidget("TaskExecutionLogs", new ERP::UI::Scheduler::TaskExecutionLogManagementWidget(this, securityManager_->getTaskExecutionLogService(), securityManager_->getScheduledTaskService(), securityManager_));

    // Security Module UI (viewers/managers for audit logs, sessions, etc.)
    loadModuleWidget("AuditLogs", new ERP::UI::Security::AuditLogViewerWidget(this, securityManager_->getAuditLogService(), securityManager_));
    loadModuleWidget("Sessions", new ERP::UI::Security::SessionManagementWidget(this, securityManager_->getSessionService(), securityManager_));
    
    // Set a default view, e.g., first available widget or a dashboard
    // This logic attempts to set the first visible widget as the current one.
    QWidget* defaultWidget = nullptr;
    if (!moduleWidgets_.isEmpty()) {
        for (const QString& moduleName : moduleWidgets_.keys()) {
            // Find the button associated with this module
            QPushButton* button = ui->toolBoxMainNavigation->findChild<QPushButton*>(QString("btn%1").arg(moduleName));
            // Check if the button is enabled and its parent page is enabled
            if (button && button->isEnabled() && button->parentWidget()->isEnabled()) {
                defaultWidget = moduleWidgets_.value(moduleName);
                break;
            }
        }
    }
    
    if (defaultWidget) {
        stackedWidget_->setCurrentWidget(defaultWidget);
        // Expand the QToolBox page where the default widget's button is located
        QWidget* parentPage = defaultWidget->parentWidget(); // Get the QToolBox page
        int pageIndex = ui->toolBoxMainNavigation->indexOf(parentPage);
        if (pageIndex != -1) {
            ui->toolBoxMainNavigation->setCurrentIndex(pageIndex);
        }
    } else {
        QLabel* noAccessLabel = new QLabel("Bạn không có quyền truy cập module nào. Vui lòng liên hệ quản trị viên.", this);
        noAccessLabel->setAlignment(Qt::AlignCenter);
        stackedWidget_->addWidget(noAccessLabel);
        stackedWidget_->setCurrentWidget(noAccessLabel);
    }
    
    // Update button states initially based on permissions
    updateUIForPermissions();
}

// Slot for handling clicks on navigation buttons in QToolBox
void MainWindow::onNavigationButtonClicked() {
    QPushButton* clickedButton = qobject_cast<QPushButton*>(sender());
    if (!clickedButton) return;

    QString buttonName = clickedButton->objectName(); // e.g., "btnManageCategories"
    QString moduleName;

    // Map button name to module name (simplified logic, could use a QMap for more robustness)
    if (buttonName == "btnFileLogout") moduleName = "Logout"; // Special case for logout
    else if (buttonName == "btnManageCategories") moduleName = "Categories";
    else if (buttonName == "btnManageProducts") moduleName = "Products";
    else if (buttonName == "btnManageCustomers") moduleName = "Customers";
    else if (buttonName == "btnManageSuppliers") moduleName = "Suppliers";
    else if (buttonName == "btnManageUsers") moduleName = "Users";
    else if (buttonName == "btnManageWarehouses") moduleName = "Warehouses";
    else if (buttonName == "btnManageLocations") moduleName = "Locations";
    else if (buttonName == "btnManageUnitsOfMeasure") moduleName = "UnitsOfMeasure";
    else if (buttonName == "btnManageRoles") moduleName = "Roles";
    else if (buttonName == "btnManagePermissions") moduleName = "Permissions";
    else if (buttonName == "btnManageSalesOrders") moduleName = "SalesOrders";
    else if (buttonName == "btnManageInvoices") moduleName = "Invoices";
    else if (buttonName == "btnManagePayments") moduleName = "Payments";
    else if (buttonName == "btnManageQuotations") moduleName = "Quotations";
    else if (buttonName == "btnManageShipments") moduleName = "Shipments";
    else if (buttonName == "btnManageReturns") moduleName = "Returns";
    else if (buttonName == "btnManageBillOfMaterials") moduleName = "BillOfMaterials";
    else if (buttonName == "btnManageMaintenance") moduleName = "Maintenance";
    else if (buttonName == "btnManageProductionLines") moduleName = "ProductionLines";
    else if (buttonName == "btnManageProductionOrders") moduleName = "ProductionOrders";
    else if (buttonName == "btnManageReceiptSlips") moduleName = "ReceiptSlips";
    else if (buttonName == "btnManageIssueSlips") moduleName = "IssueSlips";
    else if (buttonName == "btnManageMaterialRequests") moduleName = "MaterialRequests";
    else if (buttonName == "btnManageMaterialIssueSlips") moduleName = "MaterialIssueSlips";
    else if (buttonName == "btnManageInventory") moduleName = "Inventory";
    else if (buttonName == "btnManagePickingRequests") moduleName = "PickingRequests";
    else if (buttonName == "btnManageStocktakeRequests") moduleName = "StocktakeRequests";
    else if (buttonName == "btnViewInventoryTransactions") moduleName = "InventoryTransactions";
    else if (buttonName == "btnManageAccountReceivable") moduleName = "AccountReceivable";
    else if (buttonName == "btnManageGeneralLedger") moduleName = "GeneralLedger";
    else if (buttonName == "btnManageTaxRates") moduleName = "TaxRates";
    else if (buttonName == "btnViewFinancialReports") moduleName = "FinancialReports";
    else if (buttonName == "btnManageDeviceManagement") moduleName = "DeviceManagement";
    else if (buttonName == "btnManageExternalSystems") moduleName = "ExternalSystems";
    else if (buttonName == "btnManageNotifications") moduleName = "Notifications";
    else if (buttonName == "btnManageReports") moduleName = "Reports";
    else if (buttonName == "btnManageScheduledTasks") moduleName = "ScheduledTasks";
    else if (buttonName == "btnViewTaskExecutionLogs") moduleName = "TaskExecutionLogs";
    else if (buttonName == "btnViewAuditLogs") moduleName = "AuditLogs";
    else if (buttonName == "btnManageSessions") moduleName = "Sessions";
    else if (buttonName == "btnHelpAbout") moduleName = "About"; // Special case for About

    if (moduleName == "Logout") {
        onLogoutRequested();
    } else if (moduleName == "About") {
        showMessageBox("Về Hệ thống ERP", "Hệ thống quản lý tài nguyên doanh nghiệp (ERP) Sản xuất.\nPhiên bản 1.0", QMessageBox::Information);
    }
    else if (moduleWidgets_.contains(moduleName)) {
        stackedWidget_->setCurrentWidget(moduleWidgets_.value(moduleName));
        statusBar_->showMessage("Đã tải module: " + moduleName);
    } else {
        statusBar_->showMessage("Module không tìm thấy hoặc bạn không có quyền: " + moduleName);
        ERP::Logger::Logger::getInstance().warning("MainWindow: Attempted to load unknown or unauthorized module: " + moduleName.toStdString());
    }
}


void MainWindow::onLoginSuccess(const QString& username, const QString& userId, const QString& sessionId) {
    ERP::Logger::Logger::getInstance().info("MainWindow: Login successful for user: " + username.toStdString());
    
    // Retrieve full session DTO to store and get expiration etc.
    std::optional<ERP::Security::DTO::SessionDTO> session = securityManager_->getAuthenticationService()->validateSession(sessionId.toStdString());
    if (session) {
        currentSession_ = std::make_shared<ERP::Security::DTO::SessionDTO>(*session);
        // Get user roles for permission checking
        currentUserRoleIds_ = securityManager_->getUserService()->getUserRoles(currentSession_->userId, {currentSession_->userId}); // Pass current user's ID as dummy role for initial permission check
        ERP::Logger::Logger::getInstance().info("MainWindow: User roles loaded for user " + username.toStdString() + ": " + QString::fromStdString(ERP::Utils::StringUtils::join(currentUserRoleIds_, ", ")).toStdString());
        
        setupMainApplicationUI();
        stackedWidget_->setCurrentWidget(moduleWidgets_.values().first()); // Set initial module after login
        statusBar_->showMessage("Đăng nhập thành công với tài khoản: " + username);

    } else {
        ERP::Logger::Logger::getInstance().error("MainWindow: Failed to retrieve session details after successful login for user: " + username.toStdString());
        showMessageBox("Lỗi Đăng Nhập", "Không thể lấy thông tin phiên. Vui lòng thử lại.", QMessageBox::Critical);
        setupLoginScreen(); // Go back to login
    }
}

void MainWindow::onLogoutRequested() {
    if (currentSession_) {
        ERP::Logger::Logger::getInstance().info("MainWindow: Logout requested for user: " + QString::fromStdString(currentSession_->userId));
        if (securityManager_->getAuthenticationService()->logout(currentSession_->id)) {
            showMessageBox("Đăng Xuất", "Bạn đã đăng xuất thành công.", QMessageBox::Information);
            currentSession_ = nullptr;
            currentUserRoleIds_.clear();
            setupLoginScreen();
            // Clear all loaded module widgets
            for (QWidget* widget : moduleWidgets_.values()) {
                stackedWidget_->removeWidget(widget);
                delete widget;
            }
            moduleWidgets_.clear();
            statusBar_->showMessage("Đã đăng xuất. Vui lòng đăng nhập lại.");
        } else {
            showMessageBox("Đăng Xuất", "Đăng xuất thất bại. Vui lòng thử lại.", QMessageBox::Warning);
            ERP::Logger::Logger::getInstance().warning("MainWindow: Logout failed for session: " + QString::fromStdString(currentSession_->id));
        }
    } else {
        setupLoginScreen(); // Already logged out or no session
    }
}

void MainWindow::onRegisterRequested() {
    stackedWidget_->setCurrentWidget(registerForm_);
    statusBar_->showMessage("Đăng ký tài khoản mới.");
}

void MainWindow::onBackToLoginRequested() {
    stackedWidget_->setCurrentWidget(loginForm_);
    statusBar_->showMessage("Vui lòng đăng nhập.");
}

void MainWindow::loadModuleWidget(const QString& moduleName, QWidget* widget) {
    std::string requiredPermission = ""; // Default empty, assuming permission check in updateUIForPermissions handles it.
    // Determine permission string based on moduleName for specific checks in updateUIForPermissions later
    if (moduleName == "Categories") requiredPermission = "Catalog.ViewCategories";
    else if (moduleName == "Products") requiredPermission = "Product.ViewProducts";
    else if (moduleName == "Customers") requiredPermission = "Customer.ViewCustomers";
    else if (moduleName == "Suppliers") requiredPermission = "Supplier.ViewSuppliers";
    else if (moduleName == "Users") requiredPermission = "User.ViewUsers";
    else if (moduleName == "Warehouses") requiredPermission = "Catalog.ViewWarehouses";
    else if (moduleName == "Locations") requiredPermission = "Catalog.ViewLocations";
    else if (moduleName == "UnitsOfMeasure") requiredPermission = "Catalog.ViewUnitsOfMeasure";
    else if (moduleName == "Roles") requiredPermission = "Catalog.ViewRoles";
    else if (moduleName == "Permissions") requiredPermission = "Catalog.ViewPermissions";
    else if (moduleName == "SalesOrders") requiredPermission = "Sales.ViewSalesOrders";
    else if (moduleName == "Invoices") requiredPermission = "Sales.ViewInvoices";
    else if (moduleName == "Payments") requiredPermission = "Sales.ViewPayments";
    else if (moduleName == "Quotations") requiredPermission = "Sales.ViewQuotations";
    else if (moduleName == "Shipments") requiredPermission = "Sales.ViewShipments";
    else if (moduleName == "Returns") requiredPermission = "Sales.ViewReturns";
    else if (moduleName == "BillOfMaterials") requiredPermission = "Manufacturing.ViewBillOfMaterial";
    else if (moduleName == "Maintenance") requiredPermission = "Manufacturing.ViewMaintenanceManagement";
    else if (moduleName == "ProductionLines") requiredPermission = "Manufacturing.ViewProductionLine";
    else if (moduleName == "ProductionOrders") requiredPermission = "Manufacturing.ViewProductionOrder";
    else if (moduleName == "ReceiptSlips") requiredPermission = "Material.ViewReceiptSlips";
    else if (moduleName == "IssueSlips") requiredPermission = "Material.ViewIssueSlips";
    else if (moduleName == "MaterialRequests") requiredPermission = "Material.ViewMaterialRequests";
    else if (moduleName == "MaterialIssueSlips") requiredPermission = "Material.ViewMaterialIssueSlips";
    else if (moduleName == "Inventory") requiredPermission = "Warehouse.ViewInventory";
    else if (moduleName == "PickingRequests") requiredPermission = "Warehouse.ViewPickingRequests";
    else if (moduleName == "StocktakeRequests") requiredPermission = "Warehouse.ViewStocktakes";
    else if (moduleName == "InventoryTransactions") requiredPermission = "Warehouse.ViewInventoryTransactions";
    else if (moduleName == "AccountReceivable") requiredPermission = "Finance.ViewARBalance";
    else if (moduleName == "GeneralLedger") requiredPermission = "Finance.ViewGLAccounts";
    else if (moduleName == "TaxRates") requiredPermission = "Finance.ViewTaxRates";
    else if (moduleName == "FinancialReports") requiredPermission = "Finance.ViewFinancialReports";
    else if (moduleName == "DeviceManagement") requiredPermission = "Integration.ViewDeviceConfigs";
    else if (moduleName == "ExternalSystems") requiredPermission = "Integration.ViewIntegrationConfigs";
    else if (moduleName == "Notifications") requiredPermission = "Notification.ViewNotifications";
    else if (moduleName == "Reports") requiredPermission = "Report.ViewReportRequests";
    else if (moduleName == "ScheduledTasks") requiredPermission = "Scheduler.ViewScheduledTasks";
    else if (moduleName == "TaskExecutionLogs") requiredPermission = "Scheduler.ViewTaskExecutionLogs";
    else if (moduleName == "AuditLogs") requiredPermission = "Security.ViewAuditLogs";
    else if (moduleName == "Sessions") requiredPermission = "Security.ViewSessions";


    // We only add widget to stackedWidget and map if user has initial view permission
    // The permission check for individual buttons in toolBoxMainNavigation will handle visibility/enabled state
    if (hasPermission(requiredPermission.empty() ? "Admin.FullAccess" : requiredPermission)) { // If no specific perm, assume Admin access or general access
        if (!moduleWidgets_.contains(moduleName)) {
            stackedWidget_->addWidget(widget);
            moduleWidgets_.insert(moduleName, widget);
            ERP::Logger::Logger::getInstance().info("MainWindow: Module widget '" + moduleName.toStdString() + "' loaded.");
        }
    } else {
        // Clean up widget if not added due to permission
        widget->deleteLater();
        ERP::Logger::Logger::getInstance().warning("MainWindow: User " + currentUserId_ + " does not have permission '" + requiredPermission + "'. Widget '" + moduleName.toStdString() + "' not loaded.");
    }
}

void MainWindow::updateUIForPermissions() {
    // Collect all QPushButton objects from QToolBox pages
    QList<QPushButton*> navigationButtons = ui->toolBoxMainNavigation->findChildren<QPushButton*>();

    for (QPushButton* button : navigationButtons) {
        QString buttonName = button->objectName(); // e.g., "btnManageCategories"
        std::string requiredPermission = "";
        std::string moduleGroup = ""; // e.g., "Sales", "Finance" for top-level menu permission

        // Map button name to permission string and module group for top-level menu control
        if (buttonName == "btnFileLogout") { requiredPermission = "User.Logout"; moduleGroup = "File"; } // Special for logout
        else if (buttonName == "btnManageCategories") { requiredPermission = "Catalog.ViewCategories"; moduleGroup = "Manage"; }
        else if (buttonName == "btnManageProducts") { requiredPermission = "Product.ViewProducts"; moduleGroup = "Manage"; }
        else if (buttonName == "btnManageCustomers") { requiredPermission = "Customer.ViewCustomers"; moduleGroup = "Manage"; }
        else if (buttonName == "btnManageSuppliers") { requiredPermission = "Supplier.ViewSuppliers"; moduleGroup = "Manage"; }
        else if (buttonName == "btnManageUsers") { requiredPermission = "User.ViewUsers"; moduleGroup = "Manage"; }
        else if (buttonName == "btnManageWarehouses") { requiredPermission = "Catalog.ViewWarehouses"; moduleGroup = "Manage"; }
        else if (buttonName == "btnManageLocations") { requiredPermission = "Catalog.ViewLocations"; moduleGroup = "Manage"; }
        else if (buttonName == "btnManageUnitsOfMeasure") { requiredPermission = "Catalog.ViewUnitsOfMeasure"; moduleGroup = "Manage"; }
        else if (buttonName == "btnManageRoles") { requiredPermission = "Catalog.ViewRoles"; moduleGroup = "Manage"; }
        else if (buttonName == "btnManagePermissions") { requiredPermission = "Catalog.ViewPermissions"; moduleGroup = "Manage"; }
        else if (buttonName == "btnManageSalesOrders") { requiredPermission = "Sales.ViewSalesOrders"; moduleGroup = "Sales"; }
        else if (buttonName == "btnManageInvoices") { requiredPermission = "Sales.ViewInvoices"; moduleGroup = "Sales"; }
        else if (buttonName == "btnManagePayments") { requiredPermission = "Sales.ViewPayments"; moduleGroup = "Sales"; }
        else if (buttonName == "btnManageQuotations") { requiredPermission = "Sales.ViewQuotations"; moduleGroup = "Sales"; }
        else if (buttonName == "btnManageShipments") { requiredPermission = "Sales.ViewShipments"; moduleGroup = "Sales"; }
        else if (buttonName == "btnManageReturns") { requiredPermission = "Sales.ViewReturns"; moduleGroup = "Sales"; }
        else if (buttonName == "btnManageBillOfMaterials") { requiredPermission = "Manufacturing.ViewBillOfMaterial"; moduleGroup = "Manufacturing"; }
        else if (buttonName == "btnManageMaintenance") { requiredPermission = "Manufacturing.ViewMaintenanceManagement"; moduleGroup = "Manufacturing"; }
        else if (buttonName == "btnManageProductionLines") { requiredPermission = "Manufacturing.ViewProductionLine"; moduleGroup = "Manufacturing"; }
        else if (buttonName == "btnManageProductionOrders") { requiredPermission = "Manufacturing.ViewProductionOrder"; moduleGroup = "Manufacturing"; }
        else if (buttonName == "btnManageReceiptSlips") { requiredPermission = "Material.ViewReceiptSlips"; moduleGroup = "Material"; }
        else if (buttonName == "btnManageIssueSlips") { requiredPermission = "Material.ViewIssueSlips"; moduleGroup = "Material"; }
        else if (buttonName == "btnManageMaterialRequests") { requiredPermission = "Material.ViewMaterialRequests"; moduleGroup = "Material"; }
        else if (buttonName == "btnManageMaterialIssueSlips") { requiredPermission = "Material.ViewMaterialIssueSlips"; moduleGroup = "Material"; }
        else if (buttonName == "btnManageInventory") { requiredPermission = "Warehouse.ViewInventory"; moduleGroup = "Warehouse"; }
        else if (buttonName == "btnManagePickingRequests") { requiredPermission = "Warehouse.ViewPickingRequests"; moduleGroup = "Warehouse"; }
        else if (buttonName == "btnManageStocktakeRequests") { requiredPermission = "Warehouse.ViewStocktakes"; moduleGroup = "Warehouse"; }
        else if (buttonName == "btnViewInventoryTransactions") { requiredPermission = "Warehouse.ViewInventoryTransactions"; moduleGroup = "Warehouse"; }
        else if (buttonName == "btnManageAccountReceivable") { requiredPermission = "Finance.ViewARBalance"; moduleGroup = "Finance"; }
        else if (buttonName == "btnManageGeneralLedger") { requiredPermission = "Finance.ViewGLAccounts"; moduleGroup = "Finance"; }
        else if (buttonName == "btnManageTaxRates") { requiredPermission = "Finance.ViewTaxRates"; moduleGroup = "Finance"; }
        else if (buttonName == "btnViewFinancialReports") { requiredPermission = "Finance.ViewFinancialReports"; moduleGroup = "Finance"; }
        else if (buttonName == "btnManageDeviceManagement") { requiredPermission = "Integration.ViewDeviceConfigs"; moduleGroup = "Integration"; }
        else if (buttonName == "btnManageExternalSystems") { requiredPermission = "Integration.ViewIntegrationConfigs"; moduleGroup = "Integration"; }
        else if (buttonName == "btnManageNotifications") { requiredPermission = "Notification.ViewNotifications"; moduleGroup = "Notification"; }
        else if (buttonName == "btnManageReports") { requiredPermission = "Report.ViewReportRequests"; moduleGroup = "Report"; }
        else if (buttonName == "btnManageScheduledTasks") { requiredPermission = "Scheduler.ViewScheduledTasks"; moduleGroup = "Scheduler"; }
        else if (buttonName == "btnViewTaskExecutionLogs") { requiredPermission = "Scheduler.ViewTaskExecutionLogs"; moduleGroup = "Scheduler"; }
        else if (buttonName == "btnViewAuditLogs") { requiredPermission = "Security.ViewAuditLogs"; moduleGroup = "Security"; }
        else if (buttonName == "btnManageSessions") { requiredPermission = "Security.ViewSessions"; moduleGroup = "Security"; }
        else if (buttonName == "btnHelpAbout") { requiredPermission = "User.ViewHelp"; moduleGroup = "Help"; } // Example permission for Help/About

        bool hasButtonPermission = hasPermission(requiredPermission);
        button->setVisible(hasButtonPermission); // Hide button if no permission
        button->setEnabled(hasButtonPermission); // Disable button if no permission

        // Control QToolBox pages visibility/enabled state based on its contained buttons' permissions
        // A QToolBox page is enabled/visible if at least one of its buttons is enabled/visible.
        QWidget* parentPage = button->parentWidget();
        if (parentPage) {
            // Check if any button in this page is visible/enabled
            bool anyButtonVisibleInPage = false;
            QList<QPushButton*> buttonsInPage = parentPage->findChildren<QPushButton*>();
            for (QPushButton* pBtn : buttonsInPage) {
                if (pBtn->isVisible() && pBtn->isEnabled()) {
                    anyButtonVisibleInPage = true;
                    break;
                }
            }
            parentPage->setVisible(anyButtonVisibleInPage);
            ui->toolBoxMainNavigation->setItemEnabled(ui->toolBoxMainNavigation->indexOf(parentPage), anyButtonVisibleInPage);
        }
    }
}

void MainWindow::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool MainWindow::hasPermission(const std::string& permission) {
    if (!securityManager_ || currentUserId_.empty() || currentUserRoleIds_.empty()) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

} // namespace UI
} // namespace ERP