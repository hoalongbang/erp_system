// UI/Catalog/LocationManagementWidget.h
#ifndef UI_CATALOG_LOCATIONMANAGEMENTWIDGET_H
#define UI_CATALOG_LOCATIONMANAGEMENTWIDGET_H
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
#include <QDialog> // For location input dialog
#include <QDoubleValidator> // For capacity input

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "LocationService.h"   // Dịch vụ quản lý vị trí kho
#include "WarehouseService.h" // Dịch vụ quản lý kho hàng
#include "ISecurityManager.h"  // Dịch vụ bảo mật
#include "Logger.h"          // Logging
#include "ErrorHandler.h"    // Xử lý lỗi
#include "Common.h"          // Các enum chung
#include "DateUtils.h"       // Xử lý ngày tháng
#include "StringUtils.h"     // Xử lý chuỗi
#include "CustomMessageBox.h" // Hộp thoại thông báo tùy chỉnh
#include "Location.h"        // Location DTO
#include "Warehouse.h"       // Warehouse DTO (for combobox population)

namespace ERP {
    namespace UI {
        namespace Catalog {

            /**
             * @brief LocationManagementWidget class provides a UI for managing Locations.
             * This widget allows viewing, creating, updating, deleting, and changing location status.
             */
            class LocationManagementWidget : public QWidget {
                Q_OBJECT

            public:
                /**
                 * @brief Constructor for LocationManagementWidget.
                 * @param parent Parent widget.
                 * @param locationService Shared pointer to ILocationService.
                 * @param warehouseService Shared pointer to IWarehouseService.
                 * @param securityManager Shared pointer to ISecurityManager.
                 */
                explicit LocationManagementWidget(
                    QWidget* parent = nullptr,
                    std::shared_ptr<Services::ILocationService> locationService = nullptr,
                    std::shared_ptr<Services::IWarehouseService> warehouseService = nullptr,
                    std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

                ~LocationManagementWidget();

            private slots:
                void loadLocations();
                void onAddLocationClicked();
                void onEditLocationClicked();
                void onDeleteLocationClicked();
                void onUpdateLocationStatusClicked();
                void onSearchLocationClicked();
                void onLocationTableItemClicked(int row, int column);
                void clearForm();

            private:
                std::shared_ptr<Services::ILocationService> locationService_;
                std::shared_ptr<Services::IWarehouseService> warehouseService_;
                std::shared_ptr<Security::ISecurityManager> securityManager_;
                // Current user context
                std::string currentUserId_;
                std::vector<std::string> currentUserRoleIds_;

                QTableWidget* locationTable_;
                QPushButton* addLocationButton_;
                QPushButton* editLocationButton_;
                QPushButton* deleteLocationButton_;
                QPushButton* updateStatusButton_;
                QPushButton* searchButton_;
                QLineEdit* searchLineEdit_;
                QPushButton* clearFormButton_;

                // Form elements for editing/adding
                QLineEdit* idLineEdit_;
                QComboBox* warehouseComboBox_; // For selecting parent warehouse
                QLineEdit* nameLineEdit_;
                QLineEdit* typeLineEdit_;
                QLineEdit* capacityLineEdit_;
                QLineEdit* unitOfCapacityLineEdit_;
                QComboBox* statusComboBox_; // For status selection (EntityStatus)

                // Helper functions
                void setupUI();
                void populateWarehouseComboBox(QComboBox* comboBox);
                void showLocationInputDialog(ERP::Catalog::DTO::LocationDTO* location = nullptr);
                void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
                void updateButtonsState(); // Control button enable/disable based on selection and permissions

                // Permission checking helper
                bool hasPermission(const std::string& permission);
            };

        } // namespace Catalog
    } // namespace UI
} // namespace ERP

#endif // UI_CATALOG_LOCATIONMANAGEMENTWIDGET_H