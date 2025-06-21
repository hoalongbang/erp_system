cmake_minimum_required(VERSION 3.16)
project(ERP_SYSTEM LANGUAGES CXX)

# Bật C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tự động xử lý Qt (moc, uic, rcc)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ==============================================================================
# Cấu hình thư viện qua Vcpkg (KHUYẾN NGHỊ)
# Vcpkg toolchain file sẽ tự động cung cấp các đường dẫn và cấu hình cần thiết
# khi bạn build bằng Visual Studio hoặc gọi CMake với -DCMAKE_TOOLCHAIN_FILE
# ==============================================================================

# Tìm gói Qt6 - đảm bảo các COMPONENT này được cài đặt qua vcpkg
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql) 

# Tìm gói Crypto++ - vcpkg sẽ export target 'cryptopp::cryptopp'
find_package(cryptopp CONFIG REQUIRED)

# Tìm gói nlohmann/json - vcpkg sẽ export target 'nlohmann_json::nlohmann_json'
find_package(nlohmann_json CONFIG REQUIRED)

# NEW: Tìm gói cpr (C++ Requests) - vcpkg sẽ export target 'cpr::cpr'
find_package(cpr CONFIG REQUIRED)


# ==============================================================================
# Thiết lập dự án Qt (sử dụng tiện ích của CMake cho Qt)
# ==============================================================================
qt_standard_project_setup()

# ==============================================================================
# Định nghĩa các thư mục include cho dự án của bạn
# Các đường dẫn này giúp trình biên dịch tìm các file .h của bạn
# ==============================================================================
include_directories(
    ${CMAKE_SOURCE_DIR} # Cho phép include các file như main.cpp trực tiếp
    ${CMAKE_SOURCE_DIR}/DAOBase
    ${CMAKE_SOURCE_DIR}/DataObjects
    ${CMAKE_SOURCE_DIR}/DataObjects/CommonDTOs # NEW: For common DTOs
    ${CMAKE_SOURCE_DIR}/Modules/Common
    ${CMAKE_SOURCE_DIR}/Modules/Common/Service # NEW: For BaseService
    ${CMAKE_SOURCE_DIR}/Modules/Utils
    ${CMAKE_SOURCE_DIR}/Modules/Logger
    ${CMAKE_SOURCE_DIR}/Modules/EventBus
    ${CMAKE_SOURCE_DIR}/Modules/ErrorHandling
    ${CMAKE_SOURCE_DIR}/Modules/Database
    ${CMAKE_SOURCE_DIR}/Modules/Database/DTO

    # DAOs + Services
    # Cần giữ đường dẫn đến thư mục chứa các DAO và Service
    ${CMAKE_SOURCE_DIR}/Modules/Asset/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Asset/Service
    ${CMAKE_SOURCE_DIR}/Modules/Catalog/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Catalog/Service
    ${CMAKE_SOURCE_DIR}/Modules/Config/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Config/Service
    ${CMAKE_SOURCE_DIR}/Modules/Customer/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Customer/Service
    ${CMAKE_SOURCE_DIR}/Modules/Document/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Document/Service
    ${CMAKE_SOURCE_DIR}/Modules/Finance/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Finance/Service
    ${CMAKE_SOURCE_DIR}/Modules/Integration/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Integration/Service
    ${CMAKE_SOURCE_DIR}/Modules/Manufacturing/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Manufacturing/Service
    ${CMAKE_SOURCE_DIR}/Modules/Material/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Material/Service
    ${CMAKE_SOURCE_DIR}/Modules/Notification/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Notification/Service
    ${CMAKE_SOURCE_DIR}/Modules/Product/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Product/Service
    ${CMAKE_SOURCE_DIR}/Modules/Report/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Report/Service
    ${CMAKE_SOURCE_DIR}/Modules/Sales/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Sales/Service
    ${CMAKE_SOURCE_DIR}/Modules/Scheduler/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Scheduler/Service
    ${CMAKE_SOURCE_DIR}/Modules/Security/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Security/Service
    ${CMAKE_SOURCE_DIR}/Modules/Supplier/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Supplier/Service
    ${CMAKE_SOURCE_DIR}/Modules/TaskEngine
    ${CMAKE_SOURCE_DIR}/Modules/TaskEngine/DTO
    ${CMAKE_SOURCE_DIR}/Modules/TaskEngine/Service
    ${CMAKE_SOURCE_DIR}/Modules/User/DAO
    ${CMAKE_SOURCE_DIR}/Modules/User/Service
    ${CMAKE_SOURCE_DIR}/Modules/Warehouse/DAO
    ${CMAKE_SOURCE_DIR}/Modules/Warehouse/Service
    
    # UI
    ${CMAKE_SOURCE_DIR}/UI/Common
    ${CMAKE_SOURCE_DIR}/UI/Catalog
    ${CMAKE_SOURCE_DIR}/UI/Customer
    ${CMAKE_SOURCE_DIR}/UI/Finance
    ${CMAKE_SOURCE_DIR}/UI/Material
    ${CMAKE_SOURCE_DIR}/UI/Product
    ${CMAKE_SOURCE_DIR}/UI/Sales
    ${CMAKE_SOURCE_DIR}/UI/Security
    ${CMAKE_SOURCE_DIR}/UI/Supplier
    ${CMAKE_SOURCE_DIR}/UI/User
    ${CMAKE_SOURCE_DIR}/UI/Warehouse
)

# Thu thập toàn bộ mã nguồn .cpp, .h, .ui
file(GLOB_RECURSE SOURCES
    ${CMAKE_SOURCE_DIR}/main.cpp
    ${CMAKE_SOURCE_DIR}/DAOBase/*.cpp
    ${CMAKE_SOURCE_DIR}/Modules/**/*.cpp
    ${CMAKE_SOURCE_DIR}/UI/**/*.cpp
    ${CMAKE_SOURCE_DIR}/DAOBase/*.h
    ${CMAKE_SOURCE_DIR}/Modules/**/*.h
    ${CMAKE_SOURCE_DIR}/UI/**/*.h
    ${CMAKE_SOURCE_DIR}/UI/**/*.ui
)

# Định nghĩa các thư viện tĩnh cho từng module (hoặc nhóm module)
# Thứ tự định nghĩa có thể quan trọng cho các phụ thuộc lẫn nhau.

# Core Libraries
add_library(ERP_Common STATIC
    Modules/Common/Common.cpp
    Modules/Utils/AutoRelease.h # Header-only
    Modules/Utils/DateUtils.cpp
    Modules/Utils/DTOUtils.cpp # New conversion utils
    Modules/Utils/StringUtils.cpp
    Modules/Utils/Utils.cpp
)
target_link_libraries(ERP_Common PUBLIC Qt6::Core cryptopp::cryptopp nlohmann_json::nlohmann_json)

add_library(ERP_Logger STATIC
    Modules/Logger/Logger.cpp
)
target_link_libraries(ERP_Logger PUBLIC Qt6::Core)

add_library(ERP_ErrorHandler STATIC
    Modules/ErrorHandling/ErrorHandler.cpp
)
target_link_libraries(ERP_ErrorHandler PUBLIC Qt6::Core ERP_Logger)

add_library(ERP_EventBus STATIC
    Modules/EventBus/EventBus.cpp
)
target_link_libraries(ERP_EventBus PUBLIC Qt6::Core)

add_library(ERP_Database STATIC
    Modules/Database/ConnectionPool.cpp
    Modules/Database/DatabaseConnectionManager.cpp
    Modules/Database/DatabaseInitializer.cpp
    Modules/Database/SQLiteConnection.cpp
    Modules/Database/DBConnection.cpp # For vtable/destructor out-of-line
    Modules/Database/DBConnection.h # Header-only
    Modules/Database/DTO/DatabaseConfig.h # Header-only
)
target_link_libraries(ERP_Database PUBLIC Qt6::Core Qt6::Sql ERP_Logger ERP_ErrorHandler ERP_Common)

add_library(ERP_DAOBase STATIC
    DAOBase/DAOBase.cpp
    DAOBase/DAOHelpers.h # Header-only, now includes Qt stuff if DTOUtils uses it.
)
target_link_libraries(ERP_DAOBase PUBLIC ERP_Database ERP_Logger ERP_ErrorHandler ERP_Common cryptopp::cryptopp nlohmann_json::nlohmann_json) # Add nlohmann/json as DTOUtils uses it

# DAO Libraries (Each DAO needs to link to ERP_DAOBase and its DTOs)
add_library(ERP_Asset_DAO STATIC Modules/Asset/DAO/AssetDAO.cpp)
target_link_libraries(ERP_Asset_DAO PUBLIC ERP_DAOBase ERP_Asset_DTO Qt6::Core) # Asset_DTO might be header only or defined later

add_library(ERP_Catalog_DAO STATIC
    Modules/Catalog/DAO/CategoryDAO.cpp
    Modules/Catalog/DAO/LocationDAO.cpp
    Modules/Catalog/DAO/PermissionDAO.cpp
    Modules/Catalog/DAO/RoleDAO.cpp
    Modules/Catalog/DAO/UnitOfMeasureDAO.cpp
    Modules/Catalog/DAO/WarehouseDAO.cpp
)
target_link_libraries(ERP_Catalog_DAO PUBLIC ERP_DAOBase ERP_Catalog_DTO Qt6::Core)

add_library(ERP_Config_DAO STATIC Modules/Config/DAO/ConfigDAO.cpp)
target_link_libraries(ERP_Config_DAO PUBLIC ERP_DAOBase ERP_Config_DTO Qt6::Core)

add_library(ERP_Customer_DAO STATIC Modules/Customer/DAO/CustomerDAO.cpp)
target_link_libraries(ERP_Customer_DAO PUBLIC ERP_DAOBase ERP_Customer_DTO Qt6::Core)

add_library(ERP_Document_DAO STATIC Modules/Document/DAO/DocumentDAO.cpp)
target_link_libraries(ERP_Document_DAO PUBLIC ERP_DAOBase ERP_Document_DTO Qt6::Core)

add_library(ERP_Finance_DAO STATIC
    Modules/Finance/DAO/AccountReceivableDAO.cpp
    Modules/Finance/DAO/GLAccountBalanceDAO.cpp
    Modules/Finance/DAO/GeneralLedgerDAO.cpp
    Modules/Finance/DAO/JournalEntryDAO.cpp
    Modules/Finance/DAO/TaxRateDAO.cpp
    Modules/Finance/DAO/AccountReceivableTransactionDAO.cpp
)
target_link_libraries(ERP_Finance_DAO PUBLIC ERP_DAOBase ERP_Finance_DTO Qt6::Core)

add_library(ERP_Integration_DAO STATIC
    Modules/Integration/DAO/DeviceConfigDAO.cpp
    Modules/Integration/DAO/APIEndpointDAO.cpp
    Modules/Integration/DAO/IntegrationConfigDAO.cpp
)
target_link_libraries(ERP_Integration_DAO PUBLIC ERP_DAOBase ERP_Integration_DTO Qt6::Core)

add_library(ERP_Manufacturing_DAO STATIC
    Modules/Manufacturing/DAO/BillOfMaterialDAO.cpp
    Modules/Manufacturing/DAO/MaintenanceManagementDAO.cpp
    Modules/Manufacturing/DAO/ProductionLineDAO.cpp
    Modules/Manufacturing/DAO/ProductionOrderDAO.cpp
)
target_link_libraries(ERP_Manufacturing_DAO PUBLIC ERP_DAOBase ERP_Manufacturing_DTO Qt6::Core)

add_library(ERP_Material_DAO STATIC
    Modules/Material/DAO/IssueSlipDAO.cpp
    Modules/Material/DAO/IssueSlipDetailDAO.cpp
    Modules/Material/DAO/MaterialIssueSlipDAO.cpp
    Modules/Material/DAO/MaterialIssueSlipDetailDAO.cpp
    Modules/Material/DAO/MaterialRequestSlipDAO.cpp
    Modules/Material/DAO/MaterialRequestSlipDetailDAO.cpp
    Modules/Material/DAO/ReceiptSlipDAO.cpp
    Modules/Material/DAO/ReceiptSlipDetailDAO.cpp
)
target_link_libraries(ERP_Material_DAO PUBLIC ERP_DAOBase ERP_Material_DTO Qt6::Core)

add_library(ERP_Notification_DAO STATIC Modules/Notification/DAO/NotificationDAO.cpp)
target_link_libraries(ERP_Notification_DAO PUBLIC ERP_DAOBase ERP_Notification_DTO Qt6::Core)

add_library(ERP_Product_DAO STATIC
    Modules/Product/DAO/ProductDAO.cpp
    Modules/Product/DAO/ProductUnitConversionDAO.cpp
)
target_link_libraries(ERP_Product_DAO PUBLIC ERP_DAOBase ERP_Product_DTO Qt6::Core)

add_library(ERP_Report_DAO STATIC
    Modules/Report/DAO/ReportDAO.cpp
    Modules/Report/DAO/ReportExecutionLogDAO.cpp
)
target_link_libraries(ERP_Report_DAO PUBLIC ERP_DAOBase ERP_Report_DTO Qt6::Core)

add_library(ERP_Sales_DAO STATIC
    Modules/Sales/DAO/InvoiceDAO.cpp
    Modules/Sales/DAO/InvoiceDetailDAO.cpp
    Modules/Sales/DAO/PaymentDAO.cpp
    Modules/Sales/DAO/QuotationDAO.cpp
    Modules/Sales/DAO/ReturnDAO.cpp
    Modules/Sales/DAO/SalesOrderDAO.cpp
    Modules/Sales/DAO/SalesOrderDetailDAO.cpp
    Modules/Sales/DAO/ShipmentDAO.cpp
    Modules/Sales/DAO/ShipmentDetailDAO.cpp
)
target_link_libraries(ERP_Sales_DAO PUBLIC ERP_DAOBase ERP_Sales_DTO Qt6::Core)

add_library(ERP_Scheduler_DAO STATIC
    Modules/Scheduler/DAO/ScheduledTaskDAO.cpp
    Modules/Scheduler/DAO/TaskExecutionLogDAO.cpp
)
target_link_libraries(ERP_Scheduler_DAO PUBLIC ERP_DAOBase ERP_Scheduler_DTO Qt6::Core)

add_library(ERP_Security_DAO STATIC
    Modules/Security/DAO/AuditLogDAO.cpp
    Modules/Security/DAO/SessionDAO.cpp
    Modules/Security/DAO/UserRoleDAO.cpp
)
target_link_libraries(ERP_Security_DAO PUBLIC ERP_DAOBase ERP_Security_DTO Qt6::Core)

add_library(ERP_Supplier_DAO STATIC Modules/Supplier/DAO/SupplierDAO.cpp)
target_link_libraries(ERP_Supplier_DAO PUBLIC ERP_DAOBase ERP_Supplier_DTO Qt6::Core)

add_library(ERP_TaskEngine_DAO STATIC Modules/TaskEngine/DAO/TaskLogDAO.cpp)
target_link_libraries(ERP_TaskEngine_DAO PUBLIC ERP_DAOBase ERP_TaskEngine_DTO Qt6::Core)

add_library(ERP_User_DAO STATIC Modules/User/DAO/UserDAO.cpp)
target_link_libraries(ERP_User_DAO PUBLIC ERP_DAOBase ERP_User_DTO Qt6::Core)

add_library(ERP_Warehouse_DAO STATIC
    Modules/Warehouse/DAO/InventoryDAO.cpp
    Modules/Warehouse/DAO/InventoryCostLayerDAO.cpp
    Modules/Warehouse/DAO/InventoryTransactionDAO.cpp
    Modules/Warehouse/DAO/PickingRequestDAO.cpp
    Modules/Warehouse/DAO/PickingDetailDAO.cpp
    Modules/Warehouse/DAO/StocktakeRequestDAO.cpp
    Modules/Warehouse/DAO/StocktakeDetailDAO.cpp
)
target_link_libraries(ERP_Warehouse_DAO PUBLIC ERP_DAOBase ERP_Warehouse_DTO Qt6::Core)


# DTO-only libraries (for clarity, although they are mostly header-only includes)
add_library(ERP_Asset_DTO INTERFACE)
target_sources(ERP_Asset_DTO INTERFACE
    Modules/Asset/DTO/Asset.h
)

add_library(ERP_Catalog_DTO INTERFACE)
target_sources(ERP_Catalog_DTO INTERFACE
    Modules/Catalog/DTO/Category.h
    Modules/Catalog/DTO/Location.h
    Modules/Catalog/DTO/Permission.h
    Modules/Catalog/DTO/Role.h
    Modules/Catalog/DTO/UnitOfMeasure.h
    Modules/Catalog/DTO/Warehouse.h
)

add_library(ERP_Config_DTO INTERFACE)
target_sources(ERP_Config_DTO INTERFACE
    Modules/Config/DTO/Config.h
)

add_library(ERP_Customer_DTO INTERFACE)
target_sources(ERP_Customer_DTO INTERFACE
    Modules/Customer/DTO/Customer.h
)

add_library(ERP_Document_DTO INTERFACE)
target_sources(ERP_Document_DTO INTERFACE
    Modules/Document/DTO/Document.h
)

add_library(ERP_Finance_DTO INTERFACE)
target_sources(ERP_Finance_DTO INTERFACE
    Modules/Finance/DTO/AccountReceivableBalance.h
    Modules/Finance/DTO/AccountReceivableTransaction.h
    Modules/Finance/DTO/GeneralLedgerAccount.h
    Modules/Finance/DTO/GLAccountBalance.h
    Modules/Finance/DTO/JournalEntry.h
    Modules/Finance/DTO/JournalEntryDetail.h
    Modules/Finance/DTO/TaxRate.h
)

add_library(ERP_Integration_DTO INTERFACE)
target_sources(ERP_Integration_DTO INTERFACE
    Modules/Integration/DTO/DeviceConfig.h
    Modules/Integration/DTO/DeviceEventLog.h
    Modules/Integration/DTO/APIEndpoint.h
    Modules/Integration/DTO/IntegrationConfig.h
)

add_library(ERP_Manufacturing_DTO INTERFACE)
target_sources(ERP_Manufacturing_DTO INTERFACE
    Modules/Manufacturing/DTO/BillOfMaterial.h
    Modules/Manufacturing/DTO/BillOfMaterialItem.h
    Modules/Manufacturing/DTO/MaintenanceManagement.h
    Modules/Manufacturing/DTO/ProductionLine.h
    Modules/Manufacturing/DTO/ProductionOrder.h
)

add_library(ERP_Material_DTO INTERFACE)
target_sources(ERP_Material_DTO INTERFACE
    Modules/Material/DTO/IssueSlip.h
    Modules/Material/DTO/IssueSlipDetail.h
    Modules/Material/DTO/MaterialIssueSlip.h
    Modules/Material/DTO/MaterialIssueSlipDetail.h
    Modules/Material/DTO/MaterialRequestSlip.h
    Modules/Material/DTO/MaterialRequestSlipDetail.h
    Modules/Material/DTO/ReceiptSlip.h
    Modules/Material/DTO/ReceiptSlipDetail.h
)

add_library(ERP_Notification_DTO INTERFACE)
target_sources(ERP_Notification_DTO INTERFACE
    Modules/Notification/DTO/Notification.h
)

add_library(ERP_Product_DTO INTERFACE)
target_sources(ERP_Product_DTO INTERFACE
    Modules/Product/DTO/Product.h
    Modules/Product/DTO/ProductUnitConversion.h
)

add_library(ERP_Report_DTO INTERFACE)
target_sources(ERP_Report_DTO INTERFACE
    Modules/Report/DTO/Report.h
    Modules/Report/DTO/ReportExecutionLog.h
)

add_library(ERP_Sales_DTO INTERFACE)
target_sources(ERP_Sales_DTO INTERFACE
    Modules/Sales/DTO/Invoice.h
    Modules/Sales/DTO/InvoiceDetail.h
    Modules/Sales/DTO/Payment.h
    Modules/Sales/DTO/Quotation.h
    Modules/Sales/DTO/QuotationDetail.h
    Modules/Sales/DTO/Return.h
    Modules/Sales/DTO/ReturnDetail.h
    Modules/Sales/DTO/SalesOrder.h
    Modules/Sales/DTO/SalesOrderDetail.h
    Modules/Sales/DTO/Shipment.h
    Modules/Sales/DTO/ShipmentDetail.h
)

add_library(ERP_Scheduler_DTO INTERFACE)
target_sources(ERP_Scheduler_DTO INTERFACE
    Modules/Scheduler/DTO/ScheduledTask.h
    Modules/Scheduler/DTO/TaskExecutionLog.h
)

add_library(ERP_Security_DTO INTERFACE)
target_sources(ERP_Security_DTO INTERFACE
    Modules/Security/DTO/AuditLog.h
    Modules/Security/DTO/Session.h
)

add_library(ERP_Supplier_DTO INTERFACE)
target_sources(ERP_Supplier_DTO INTERFACE
    Modules/Supplier/DTO/Supplier.h
)

add_library(ERP_TaskEngine_DTO INTERFACE)
target_sources(ERP_TaskEngine_DTO INTERFACE
    Modules/TaskEngine/DTO/TaskLog.h
)

add_library(ERP_User_DTO INTERFACE)
target_sources(ERP_User_DTO INTERFACE
    Modules/User/DTO/User.h
    Modules/User/DTO/UserProfile.h
)

add_library(ERP_Warehouse_DTO INTERFACE)
target_sources(ERP_Warehouse_DTO INTERFACE
    Modules/Warehouse/DTO/Inventory.h
    Modules/Warehouse/DTO/InventoryCostLayer.h
    Modules/Warehouse/DTO/InventoryTransaction.h
    Modules/Warehouse/DTO/PickingRequest.h
    Modules/Warehouse/DTO/PickingDetail.h
    Modules/Warehouse/DTO/StocktakeDetail.h
    Modules/Warehouse/DTO/StocktakeRequest.h
)

# Service Libraries
add_library(ERP_Common_Service_BaseService STATIC Modules/Common/Service/BaseService.cpp)
target_link_libraries(ERP_Common_Service_BaseService PUBLIC Qt6::Core ERP_Logger ERP_ErrorHandler ERP_Common ERP_Database ERP_Security_Service_Interfaces)

add_library(ERP_Security_Service_Interfaces INTERFACE
    Modules/Security/Service/IAuthenticationService.h
    Modules/Security/Service/IAuthorizationService.h
    Modules/Security/Service/IAuditLogService.h
    Modules/Security/Service/ISecurityManager.h
    Modules/Security/Service/ISessionService.h
    Modules/Security/Service/EncryptionService.h # Header-only singleton, but good to group
)

add_library(ERP_Security_Services STATIC
    Modules/Security/Service/AuthenticationService.cpp
    Modules/Security/Service/AuthorizationService.cpp
    Modules/Security/Service/AuditLogService.cpp
    Modules/Security/Service/SessionService.cpp
    Modules/Security/Service/SecurityManager.cpp
    Modules/Security/Service/EncryptionService.cpp
    Modules/Security/Utils/PasswordHasher.cpp
)
target_link_libraries(ERP_Security_Services PUBLIC
    ERP_Security_Service_Interfaces
    ERP_Security_DAO
    ERP_User_DAO # For UserService (User authentication/authorization)
    ERP_Catalog_DAO # For RoleService, PermissionService
    ERP_Common_Service_BaseService # Required for BaseService in SecurityManager constructor even if not directly inheriting
    ERP_Database ERP_Logger ERP_ErrorHandler ERP_Common ERP_EventBus # Core dependencies
    cryptopp::cryptopp nlohmann_json::nlohmann_json # For encryption and DTOUtils
    # Link to all *other* service interface libraries (if defined as INTERFACE libraries)
    ERP_User_Service_Interfaces
    ERP_Catalog_Service_Interfaces
    ERP_Asset_Service_Interfaces
    ERP_Config_Service_Interfaces
    ERP_Customer_Service_Interfaces
    ERP_Document_Service_Interfaces
    ERP_Finance_Service_Interfaces
    ERP_Integration_Service_Interfaces
    ERP_Manufacturing_Service_Interfaces
    ERP_Material_Service_Interfaces
    ERP_Notification_Service_Interfaces
    ERP_Product_Service_Interfaces
    ERP_Report_Service_Interfaces
    ERP_Sales_Service_Interfaces
    ERP_Scheduler_Service_Interfaces
    ERP_Supplier_Service_Interfaces
    ERP_TaskEngine_Service_Interfaces
    ERP_Warehouse_Service_Interfaces
)


add_library(ERP_User_Service_Interfaces INTERFACE
    Modules/User/Service/IUserService.h
)
add_library(ERP_User_Service STATIC Modules/User/Service/UserService.cpp)
target_link_libraries(ERP_User_Service PUBLIC
    ERP_User_Service_Interfaces
    ERP_User_DAO
    ERP_Security_DAO # NEW: For UserRoleDAO (used by UserService)
    ERP_Common_Service_BaseService
    ERP_Catalog_Service_Interfaces # For RoleService
    ERP_Security_Service_Interfaces # For SecurityManager
)

add_library(ERP_Catalog_Service_Interfaces INTERFACE
    Modules/Catalog/Service/ICategoryService.h
    Modules/Catalog/Service/ILocationService.h
    Modules/Catalog/Service/IPermissionService.h
    Modules/Catalog/Service/IRoleService.h
    Modules/Catalog/Service/IUnitOfMeasureService.h
    Modules/Catalog/Service/IWarehouseService.h
)
add_library(ERP_Catalog_Services STATIC
    Modules/Catalog/Service/CategoryService.cpp
    Modules/Catalog/Service/LocationService.cpp
    Modules/Catalog/Service/PermissionService.cpp
    Modules/Catalog/Service/RoleService.cpp
    Modules/Catalog/Service/UnitOfMeasureService.cpp
    Modules/Catalog/Service/WarehouseService.cpp
)
target_link_libraries(ERP_Catalog_Services PUBLIC
    ERP_Catalog_Service_Interfaces ERP_Catalog_DAO
    ERP_Common_Service_BaseService
    ERP_Security_Service_Interfaces # For SecurityManager
    ERP_User_Service_Interfaces # For UserService
    ERP_Product_Service_Interfaces # For ProductService (Category delete check)
    ERP_Warehouse_Service_Interfaces # For WarehouseService (Location delete check)
    ERP_Material_Service_Interfaces # For IssueSlipService, ReceiptSlipService (Warehouse delete check)
    ERP_Sales_Service_Interfaces # For SalesOrderService (Warehouse delete check)
)


add_library(ERP_Asset_Service_Interfaces INTERFACE
    Modules/Asset/Service/IAssetManagementService.h
)
add_library(ERP_Asset_Services STATIC Modules/Asset/Service/AssetManagementService.cpp)
target_link_libraries(ERP_Asset_Services PUBLIC
    ERP_Asset_Service_Interfaces ERP_Asset_DAO
    ERP_Common_Service_BaseService
    ERP_Catalog_Service_Interfaces # For LocationService, ProductionLineService
    ERP_Manufacturing_Service_Interfaces # For ProductionLineService
    ERP_Security_Service_Interfaces # For SecurityManager
)

add_library(ERP_Config_Service_Interfaces INTERFACE
    Modules/Config/Service/IConfigService.h
)
add_library(ERP_Config_Services STATIC Modules/Config/Service/ConfigService.cpp)
target_link_libraries(ERP_Config_Services PUBLIC
    ERP_Config_Service_Interfaces ERP_Config_DAO
    ERP_Common_Service_BaseService
    ERP_Security_Service_Interfaces # For EncryptionService, SecurityManager
)

add_library(ERP_Customer_Service_Interfaces INTERFACE
    Modules/Customer/Service/ICustomerService.h
)
add_library(ERP_Customer_Services STATIC Modules/Customer/Service/CustomerService.cpp)
target_link_libraries(ERP_Customer_Services PUBLIC
    ERP_Customer_Service_Interfaces ERP_Customer_DAO
    ERP_Common_Service_BaseService
    ERP_Security_Service_Interfaces # For SecurityManager
    ERP_Sales_Service_Interfaces # For SalesOrderService (Customer delete check)
)

add_library(ERP_Document_Service_Interfaces INTERFACE
    Modules/Document/Service/IDocumentService.h
)
add_library(ERP_Document_Services STATIC Modules/Document/Service/DocumentService.cpp)
target_link_libraries(ERP_Document_Services PUBLIC
    ERP_Document_Service_Interfaces ERP_Document_DAO
    ERP_Common_Service_BaseService
    ERP_Security_Service_Interfaces # For SecurityManager
)

add_library(ERP_Finance_Service_Interfaces INTERFACE
    Modules/Finance/Service/IAccountReceivableService.h
    Modules/Finance/Service/IGeneralLedgerService.h
    Modules/Finance/Service/ITaxService.h
)
add_library(ERP_Finance_Services STATIC
    Modules/Finance/Service/AccountReceivableService.cpp
    Modules/Finance/Service/GeneralLedgerService.cpp
    Modules/Finance/Service/TaxService.cpp
)
target_link_libraries(ERP_Finance_Services PUBLIC
    ERP_Finance_Service_Interfaces ERP_Finance_DAO
    ERP_Common_Service_BaseService
    ERP_Customer_Service_Interfaces # For CustomerService (AR validation)
    ERP_Sales_Service_Interfaces # For InvoiceService, PaymentService (AR validation)
    ERP_Security_Service_Interfaces # For SecurityManager
)

add_library(ERP_Integration_Service_Interfaces INTERFACE
    Modules/Integration/Service/IDeviceManagerService.h
    Modules/Integration/Service/IExternalSystemService.h
)
add_library(ERP_Integration_Services STATIC
    Modules/Integration/Service/DeviceManagementService.cpp
    Modules/Integration/Service/ExternalSystemService.cpp
)
target_link_libraries(ERP_Integration_Services PUBLIC
    ERP_Integration_Service_Interfaces ERP_Integration_DAO
    ERP_Common_Service_BaseService
    ERP_Security_Service_Interfaces # For SecurityManager
    cpr::cpr # NEW: For cpr library
)

add_library(ERP_Manufacturing_Service_Interfaces INTERFACE
    Modules/Manufacturing/Service/IBillOfMaterialService.h
    Modules/Manufacturing/Service/IMaintenanceManagementService.h
    Modules/Manufacturing/Service/IProductionLineService.h
    Modules/Manufacturing/Service/IProductionOrderService.h
)
add_library(ERP_Manufacturing_Services STATIC
    Modules/Manufacturing/Service/BillOfMaterialService.cpp
    Modules/Manufacturing/Service/MaintenanceManagementService.cpp
    Modules/Manufacturing/Service/ProductionLineService.cpp
    Modules/Manufacturing/Service/ProductionOrderService.cpp
)
target_link_libraries(ERP_Manufacturing_Services PUBLIC
    ERP_Manufacturing_Service_Interfaces ERP_Manufacturing_DAO
    ERP_Common_Service_BaseService
    ERP_Product_Service_Interfaces # For ProductService
    ERP_Catalog_Service_Interfaces # For LocationService, UnitOfMeasureService, WarehouseService
    ERP_Asset_Service_Interfaces # For AssetManagementService
    ERP_Security_Service_Interfaces # For SecurityManager
)

add_library(ERP_Material_Service_Interfaces INTERFACE
    Modules/Material/Service/IIssueSlipService.h
    Modules/Material/Service/IMaterialIssueSlipService.h
    Modules/Material/Service/IMaterialRequestService.h
    Modules/Material/Service/IReceiptSlipService.h
)
add_library(ERP_Material_Services STATIC
    Modules/Material/Service/IssueSlipService.cpp
    Modules/Material/Service/MaterialIssueSlipService.cpp
    Modules/Material/Service/MaterialRequestService.cpp
    Modules/Material/Service/ReceiptSlipService.cpp
)
target_link_libraries(ERP_Material_Services PUBLIC
    ERP_Material_Service_Interfaces ERP_Material_DAO
    ERP_Common_Service_BaseService
    ERP_Product_Service_Interfaces # For ProductService
    ERP_Catalog_Service_Interfaces # For WarehouseService
    ERP_Warehouse_Service_Interfaces # For InventoryManagementService
    ERP_Security_Service_Interfaces # For SecurityManager
)

add_library(ERP_Notification_Service_Interfaces INTERFACE
    Modules/Notification/Service/INotificationService.h
)
add_library(ERP_Notification_Services STATIC Modules/Notification/Service/NotificationService.cpp)
target_link_libraries(ERP_Notification_Services PUBLIC
    ERP_Notification_Service_Interfaces ERP_Notification_DAO
    ERP_Common_Service_BaseService
    ERP_Security_Service_Interfaces # For SecurityManager
)

add_library(ERP_Product_Service_Interfaces INTERFACE
    Modules/Product/Service/IProductService.h
)
add_library(ERP_Product_Services STATIC Modules/Product/Service/ProductService.cpp)
target_link_libraries(ERP_Product_Services PUBLIC
    ERP_Product_Service_Interfaces ERP_Product_DAO
    ERP_Common_Service_BaseService
    ERP_Catalog_Service_Interfaces # For CategoryService, UnitOfMeasureService
    ERP_Supplier_Service_Interfaces # For SupplierService (Product validation)
    ERP_Warehouse_Service_Interfaces # For InventoryManagementService (Product delete check)
    ERP_Manufacturing_Service_Interfaces # For BillOfMaterialService (Product delete check)
    ERP_Security_Service_Interfaces # For SecurityManager
)

add_library(ERP_Report_Service_Interfaces INTERFACE
    Modules/Report/Service/IReportService.h
)
add_library(ERP_Report_Services STATIC Modules/Report/Service/ReportService.cpp)
target_link_libraries(ERP_Report_Services PUBLIC
    ERP_Report_Service_Interfaces ERP_Report_DAO
    ERP_Common_Service_BaseService
    ERP_Security_Service_Interfaces # For SecurityManager
)

add_library(ERP_Sales_Service_Interfaces INTERFACE
    Modules/Sales/Service/IInvoiceService.h
    Modules/Sales/Service/IPaymentService.h
    Modules/Sales/Service/IQuotationService.h
    Modules/Sales/Service/IReturnService.h
    Modules/Sales/Service/ISalesOrderService.h
    Modules/Sales/Service/IShipmentService.h
)
add_library(ERP_Sales_Services STATIC
    Modules/Sales/Service/SalesInvoiceService.cpp
    Modules/Sales/Service/SalesPaymentService.cpp
    Modules/Sales/Service/SalesQuotationService.cpp
    Modules/Sales/Service/SalesReturnService.cpp
    Modules/Sales/Service/SalesOrderService.cpp
    Modules/Sales/Service/SalesShipmentService.cpp
)
target_link_libraries(ERP_Sales_Services PUBLIC
    ERP_Sales_Service_Interfaces ERP_Sales_DAO
    ERP_Common_Service_BaseService
    ERP_Customer_Service_Interfaces # For CustomerService
    ERP_Product_Service_Interfaces # For ProductService
    ERP_Catalog_Service_Interfaces # For WarehouseService, UnitOfMeasureService
    ERP_Finance_Service_Interfaces # For AccountReceivableService (Payment)
    ERP_Warehouse_Service_Interfaces # For InventoryManagementService (Return)
    ERP_Security_Service_Interfaces # For SecurityManager
)

add_library(ERP_Scheduler_Service_Interfaces INTERFACE
    Modules/Scheduler/Service/IScheduledTaskService.h
    Modules/Scheduler/Service/ITaskExecutionLogService.h
)
add_library(ERP_Scheduler_Services STATIC
    Modules/Scheduler/Service/ScheduledTaskService.cpp
    Modules/Scheduler/Service/TaskExecutionLogService.cpp
)
target_link_libraries(ERP_Scheduler_Services PUBLIC
    ERP_Scheduler_Service_Interfaces ERP_Scheduler_DAO
    ERP_Common_Service_BaseService
    ERP_Security_Service_Interfaces # For SecurityManager
)

add_library(ERP_Supplier_Service_Interfaces INTERFACE
    Modules/Supplier/Service/ISupplierService.h
)
add_library(ERP_Supplier_Services STATIC Modules/Supplier/Service/SupplierService.cpp)
target_link_libraries(ERP_Supplier_Services PUBLIC
    ERP_Supplier_Service_Interfaces ERP_Supplier_DAO
    ERP_Common_Service_BaseService
    ERP_Product_Service_Interfaces # For ProductService (Supplier delete check)
    ERP_Security_Service_Interfaces # For SecurityManager
)

add_library(ERP_TaskEngine_Service_Interfaces INTERFACE
    Modules/TaskEngine/Service/ITaskExecutorService.h
)
add_library(ERP_TaskEngine_Services STATIC
    Modules/TaskEngine/TaskEngine.cpp
)
target_link_libraries(ERP_TaskEngine_Services PUBLIC
    ERP_TaskEngine_Service_Interfaces ERP_TaskEngine_DTO ERP_TaskEngine_DAO # TaskEngine uses TaskLogDAO
    ERP_Logger ERP_Common Qt6::Core
)


add_library(ERP_Warehouse_Service_Interfaces INTERFACE
    Modules/Warehouse/Service/IInventoryManagementService.h
    Modules/Warehouse/Service/IPickingService.h
    Modules/Warehouse/Service/IStocktakeService.h
)
add_library(ERP_Warehouse_Services STATIC
    Modules/Warehouse/Service/InventoryManagementService.cpp
    Modules/Warehouse/Service/PickingService.cpp
    Modules/Warehouse/Service/StocktakeService.cpp
    Modules/Warehouse/Service/InventoryTransactionService.cpp
)
target_link_libraries(ERP_Warehouse_Services PUBLIC
    ERP_Warehouse_Service_Interfaces ERP_Warehouse_DAO
    ERP_Common_Service_BaseService
    ERP_Product_Service_Interfaces # For ProductService
    ERP_Catalog_Service_Interfaces # For WarehouseService, LocationService
    ERP_Sales_Service_Interfaces # For SalesOrderService (Picking)
    ERP_Security_Service_Interfaces # For SecurityManager
)

# UI Libraries
add_library(ERP_UI_Common STATIC
    UI/Common/CustomMessageBox.cpp
)
target_link_libraries(ERP_UI_Common PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core ERP_Logger)

add_library(ERP_UI_Login STATIC
    UI/loginform.cpp
    UI/registerform.cpp
)
target_link_libraries(ERP_UI_Login PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core Qt6::Network ERP_Logger ERP_ErrorHandler
    ERP_UI_Common
    ERP_Security_Services ERP_User_Service_Interfaces # For Authentication and User Services
)


add_library(ERP_UI_Catalog STATIC
    UI/Catalog/CategoryManagementWidget.cpp
    UI/Catalog/LocationManagementWidget.cpp
    UI/Catalog/PermissionManagementWidget.cpp
    UI/Catalog/RoleManagementWidget.cpp
    UI/Catalog/UnitOfMeasureManagementWidget.cpp
    UI/Catalog/WarehouseManagementWidget.cpp
)
target_link_libraries(ERP_UI_Catalog PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core ERP_UI_Common ERP_Catalog_Services ERP_Security_Services ERP_Product_Service_Interfaces ERP_Warehouse_Service_Interfaces ERP_User_Service_Interfaces)

add_library(ERP_UI_Product STATIC
    UI/Product/ProductManagementWidget.cpp
)
target_link_libraries(ERP_UI_Product PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core ERP_UI_Common ERP_Product_Services ERP_Catalog_Service_Interfaces ERP_Security_Services ERP_User_Service_Interfaces ERP_Supplier_Service_Interfaces ERP_Warehouse_Service_Interfaces ERP_Manufacturing_Service_Interfaces)

add_library(ERP_UI_Customer STATIC
    UI/Customer/CustomerManagementWidget.cpp
)
target_link_libraries(ERP_UI_Customer PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core ERP_UI_Common ERP_Customer_Services ERP_Security_Services ERP_User_Service_Interfaces ERP_Sales_Service_Interfaces)

add_library(ERP_UI_Supplier STATIC
    UI/Supplier/SupplierManagementWidget.cpp
)
target_link_libraries(ERP_UI_Supplier PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core ERP_UI_Common ERP_Supplier_Services ERP_Security_Services ERP_User_Service_Interfaces ERP_Product_Service_Interfaces)

add_library(ERP_UI_Sales STATIC
    UI/Sales/InvoiceManagementWidget.cpp
    UI/Sales/PaymentManagementWidget.cpp
    UI/Sales/QuotationManagementWidget.cpp
    UI/Sales/SalesOrderManagementWidget.cpp
    UI/Sales/ShipmentManagementWidget.cpp
    UI/Sales/ReturnManagementWidget.cpp
)
target_link_libraries(ERP_UI_Sales PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core ERP_UI_Common ERP_Sales_Services ERP_Customer_Service_Interfaces ERP_Product_Service_Interfaces ERP_Catalog_Service_Interfaces ERP_Finance_Service_Interfaces ERP_Security_Services ERP_User_Service_Interfaces ERP_Warehouse_Service_Interfaces)

add_library(ERP_UI_Manufacturing STATIC
    UI/Manufacturing/BillOfMaterialManagementWidget.cpp
    UI/Manufacturing/MaintenanceManagementWidget.cpp
    UI/Manufacturing/ProductionLineManagementWidget.cpp
    UI/Manufacturing/ProductionOrderManagementWidget.cpp
)
target_link_libraries(ERP_UI_Manufacturing PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core ERP_UI_Common ERP_Manufacturing_Services ERP_Product_Service_Interfaces ERP_Catalog_Service_Interfaces ERP_Asset_Service_Interfaces ERP_Security_Services ERP_User_Service_Interfaces)

add_library(ERP_UI_Material STATIC
    UI/Material/IssueSlipManagementWidget.cpp
    UI/Material/MaterialIssueSlipManagementWidget.cpp
    UI/Material/MaterialRequestSlipManagementWidget.cpp
    UI/Material/ReceiptSlipManagementWidget.cpp
)
target_link_libraries(ERP_UI_Material PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core ERP_UI_Common ERP_Material_Services ERP_Product_Service_Interfaces ERP_Catalog_Service_Interfaces ERP_Warehouse_Service_Interfaces ERP_Security_Services ERP_User_Service_Interfaces ERP_Manufacturing_Service_Interfaces)

add_library(ERP_UI_Warehouse STATIC
    UI/Warehouse/InventoryManagementWidget.cpp
    UI/Warehouse/PickingRequestManagementWidget.cpp
    UI/Warehouse/StocktakeRequestManagementWidget.cpp
    UI/Warehouse/InventoryTransactionManagementWidget.cpp
)
target_link_libraries(ERP_UI_Warehouse PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core ERP_UI_Common ERP_Warehouse_Services ERP_Product_Service_Interfaces ERP_Catalog_Service_Interfaces ERP_Sales_Service_Interfaces ERP_Security_Services ERP_User_Service_Interfaces)

add_library(ERP_UI_Finance STATIC
    UI/Finance/AccountReceivableManagementWidget.cpp
    UI/Finance/GeneralLedgerManagementWidget.cpp
    UI/Finance/TaxRateManagementWidget.cpp
    UI/Finance/FinancialReportsWidget.cpp
)
target_link_libraries(ERP_UI_Finance PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core ERP_UI_Common ERP_Finance_Services ERP_Customer_Service_Interfaces ERP_Sales_Service_Interfaces ERP_Security_Services ERP_User_Service_Interfaces)


add_library(ERP_UI_Integration STATIC
    UI/Integration/DeviceManagementWidget.cpp
    UI/Integration/ExternalSystemManagementWidget.cpp
)
target_link_libraries(ERP_UI_Integration PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core ERP_UI_Common ERP_Integration_Services ERP_Security_Services)


add_library(ERP_UI_Notification STATIC
    UI/Notification/NotificationManagementWidget.cpp
)
target_link_libraries(ERP_UI_Notification PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core ERP_UI_Common ERP_Notification_Services ERP_Security_Service_Interfaces)

add_library(ERP_UI_Report STATIC
    UI/Report/ReportManagementWidget.cpp
)
target_link_libraries(ERP_UI_Report PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core ERP_UI_Common ERP_Report_Services ERP_Security_Service_Interfaces)

add_library(ERP_UI_Scheduler STATIC
    UI/Scheduler/ScheduledTaskManagementWidget.cpp
    UI/Scheduler/TaskExecutionLogManagementWidget.cpp
)
target_link_libraries(ERP_UI_Scheduler PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core ERP_UI_Common ERP_Scheduler_Services ERP_Security_Service_Interfaces ERP_User_Service_Interfaces)

add_library(ERP_UI_Security STATIC
    UI/Security/AuditLogViewerWidget.cpp
    UI/Security/SessionManagementWidget.cpp
)
target_link_libraries(ERP_UI_Security PUBLIC Qt6::Widgets Qt6::Gui Qt6::Core ERP_UI_Common ERP_Security_Services ERP_User_Service_Interfaces)


# Main Application Executable
add_executable(ERPSystem WIN32
    main.cpp
    UI/mainwindow.cpp
    UI/mainwindow.ui # Include UI form
)

# Link all necessary libraries to the main executable
target_link_libraries(ERPSystem PRIVATE
    Qt6::Widgets Qt6::Gui Qt6::Core Qt6::Sql # Core Qt modules
    cryptopp::cryptopp # Crypto++
    nlohmann_json::nlohmann_json # nlohmann/json
    cpr::cpr # cpr library

    ERP_Common
    ERP_Logger
    ERP_ErrorHandler
    ERP_EventBus
    ERP_Database
    ERP_DAOBase

    # DAOs
    ERP_Asset_DAO
    ERP_Catalog_DAO
    ERP_Config_DAO
    ERP_Customer_DAO
    ERP_Document_DAO
    ERP_Finance_DAO
    ERP_Integration_DAO
    ERP_Manufacturing_DAO
    ERP_Material_DAO
    ERP_Notification_DAO
    ERP_Product_DAO
    ERP_Report_DAO
    ERP_Sales_DAO
    ERP_Scheduler_DAO
    ERP_Security_DAO
    ERP_Supplier_DAO
    ERP_TaskEngine_DAO
    ERP_User_DAO
    ERP_Warehouse_DAO

    # DTO Interfaces (for proper linking and visibility, even if header-only)
    ERP_Asset_DTO
    ERP_Catalog_DTO
    ERP_Config_DTO
    ERP_Customer_DTO
    ERP_Document_DTO
    ERP_Finance_DTO
    ERP_Integration_DTO
    ERP_Manufacturing_DTO
    ERP_Material_DTO
    ERP_Notification_DTO
    ERP_Product_DTO
    ERP_Report_DTO
    ERP_Sales_DTO
    ERP_Scheduler_DTO
    ERP_Security_DTO
    ERP_Supplier_DTO
    ERP_TaskEngine_DTO
    ERP_User_DTO
    ERP_Warehouse_DTO

    # Services
    ERP_Common_Service_BaseService
    ERP_Security_Services
    ERP_User_Service
    ERP_Catalog_Services
    ERP_Asset_Services
    ERP_Config_Services
    ERP_Customer_Services
    ERP_Document_Services
    ERP_Finance_Services
    ERP_Integration_Services
    ERP_Manufacturing_Services
    ERP_Material_Services
    ERP_Notification_Services
    ERP_Product_Services
    ERP_Report_Services
    ERP_Sales_Services
    ERP_Scheduler_Services
    ERP_Supplier_Services
    ERP_TaskEngine_Services
    ERP_Warehouse_Services

    # UI Modules
    ERP_UI_Common
    ERP_UI_Login
    ERP_UI_Catalog
    ERP_UI_Product
    ERP_UI_Customer
    ERP_UI_Supplier
    ERP_UI_Sales
    ERP_UI_Manufacturing
    ERP_UI_Material
    ERP_UI_Warehouse
    ERP_UI_Finance
    ERP_UI_Integration
    ERP_UI_Notification
    ERP_UI_Report
    ERP_UI_Scheduler
    ERP_UI_Security
)