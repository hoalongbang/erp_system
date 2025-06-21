// UI/Catalog/LocationManagementWidget.cpp
#include "LocationManagementWidget.h" // Đã rút gọn include
#include "Location.h"          // Đã rút gọn include
#include "Warehouse.h"         // Đã rút gọn include
#include "LocationService.h"   // Đã rút gọn include
#include "WarehouseService.h"  // Đã rút gọn include
#include "ISecurityManager.h"  // Đã rút gọn include
#include "Logger.h"            // Đã rút gọn include
#include "ErrorHandler.h"      // Đã rút gọn include
#include "Common.h"            // Đã rút gọn include
#include "DateUtils.h"         // Đã rút gọn include
#include "StringUtils.h"       // Đã rút gọn include
#include "CustomMessageBox.h"  // Đã rút gọn include
#include "UserService.h"       // For getting current user roles from SecurityManager

#include <QInputDialog> // For getting input from user

namespace ERP {
namespace UI {
namespace Catalog {

LocationManagementWidget::LocationManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::ILocationService> locationService,
    std::shared_ptr<Services::IWarehouseService> warehouseService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      locationService_(locationService),
      warehouseService_(warehouseService),
      securityManager_(securityManager) {

    if (!locationService_ || !warehouseService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ vị trí kho, kho hàng hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("LocationManagementWidget: Initialized with null dependencies.");
        // Consider disabling UI elements or exiting gracefully
        return;
    }

    // Attempt to get current user ID and roles from securityManager
    auto authService = securityManager_->getAuthenticationService();
    if (authService) {
        std::string dummySessionId = "current_session_id"; // Placeholder
        std::optional<ERP::Security::DTO::SessionDTO> currentSession = authService->validateSession(dummySessionId);
        if (currentSession) {
            currentUserId_ = currentSession->userId;
            currentUserRoleIds_ = securityManager_->getUserService()->getUserRoles(currentUserId_, {}); // Get roles for current user
        } else {
            currentUserId_ = "system_user"; // Fallback for non-logged-in operations or system actions
            currentUserRoleIds_ = {"anonymous"}; // Fallback
            ERP::Logger::Logger::getInstance().warning("LocationManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("LocationManagementWidget: Authentication Service not available. Running with limited privileges.");
    }


    setupUI();
    loadLocations();
    updateButtonsState(); // Set initial button states
}

LocationManagementWidget::~LocationManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void LocationManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Search and filter
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên vị trí...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &LocationManagementWidget::onSearchLocationClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    // Table
    locationTable_ = new QTableWidget(this);
    locationTable_->setColumnCount(7); // ID, Tên kho, Tên vị trí, Loại, Sức chứa, Đơn vị, Trạng thái, Ngày tạo
    locationTable_->setHorizontalHeaderLabels({"ID", "Kho hàng", "Tên vị trí", "Loại", "Sức chứa", "Đơn vị Sức chứa", "Trạng thái"});
    locationTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    locationTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    locationTable_->setEditTriggers(QAbstractItemView::NoEditTriggers); // Make table read-only
    locationTable_->horizontalHeader()->setStretchLastSection(true); // Stretch last column
    connect(locationTable_, &QTableWidget::itemClicked, this, &LocationManagementWidget::onLocationTableItemClicked);
    mainLayout->addWidget(locationTable_);

    // Form for Add/Edit
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this);
    idLineEdit_->setReadOnly(true); // ID is read-only
    warehouseComboBox_ = new QComboBox(this);
    nameLineEdit_ = new QLineEdit(this);
    typeLineEdit_ = new QLineEdit(this);
    capacityLineEdit_ = new QLineEdit(this);
    capacityLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this)); // Allow double
    unitOfCapacityLineEdit_ = new QLineEdit(this);
    statusComboBox_ = new QComboBox(this);
    statusComboBox_->addItem("Active", static_cast<int>(ERP::Common::EntityStatus::ACTIVE));
    statusComboBox_->addItem("Inactive", static_cast<int>(ERP::Common::EntityStatus::INACTIVE));
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Common::EntityStatus::PENDING));
    statusComboBox_->addItem("Deleted", static_cast<int>(ERP::Common::EntityStatus::DELETED));

    formLayout->addWidget(new QLabel("ID:", this), 0, 0);
    formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Kho hàng:*", this), 1, 0);
    formLayout->addWidget(warehouseComboBox_, 1, 1);
    formLayout->addWidget(new QLabel("Tên vị trí:*", this), 2, 0);
    formLayout->addWidget(nameLineEdit_, 2, 1);
    formLayout->addWidget(new QLabel("Loại:", this), 3, 0);
    formLayout->addWidget(typeLineEdit_, 3, 1);
    formLayout->addWidget(new QLabel("Sức chứa:", this), 4, 0);
    formLayout->addWidget(capacityLineEdit_, 4, 1);
    formLayout->addWidget(new QLabel("Đơn vị Sức chứa:", this), 5, 0);
    formLayout->addWidget(unitOfCapacityLineEdit_, 5, 1);
    formLayout->addWidget(new QLabel("Trạng thái:", this), 6, 0);
    formLayout->addWidget(statusComboBox_, 6, 1);

    mainLayout->addLayout(formLayout);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addLocationButton_ = new QPushButton("Thêm mới", this);
    editLocationButton_ = new QPushButton("Sửa", this);
    deleteLocationButton_ = new QPushButton("Xóa", this);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this);
    clearFormButton_ = new QPushButton("Xóa Form", this);

    connect(addLocationButton_, &QPushButton::clicked, this, &LocationManagementWidget::onAddLocationClicked);
    connect(editLocationButton_, &QPushButton::clicked, this, &LocationManagementWidget::onEditLocationClicked);
    connect(deleteLocationButton_, &QPushButton::clicked, this, &LocationManagementWidget::onDeleteLocationClicked);
    connect(updateStatusButton_, &QPushButton::clicked, this, &LocationManagementWidget::onUpdateLocationStatusClicked);
    connect(clearFormButton_, &QPushButton::clicked, this, &LocationManagementWidget::clearForm);

    buttonLayout->addWidget(addLocationButton_);
    buttonLayout->addWidget(editLocationButton_);
    buttonLayout->addWidget(deleteLocationButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void LocationManagementWidget::loadLocations() {
    ERP::Logger::Logger::getInstance().info("LocationManagementWidget: Loading locations...");
    locationTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Catalog::DTO::LocationDTO> locations = locationService_->getAllLocations({}, currentUserId_, currentUserRoleIds_);

    locationTable_->setRowCount(locations.size());
    for (int i = 0; i < locations.size(); ++i) {
        const auto& location = locations[i];
        locationTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(location.id)));
        
        // Resolve warehouse name
        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(location.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) {
            warehouseName = QString::fromStdString(warehouse->name);
        }
        locationTable_->setItem(i, 1, new QTableWidgetItem(warehouseName));

        locationTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(location.name)));
        locationTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(location.type.value_or(""))));
        locationTable_->setItem(i, 4, new QTableWidgetItem(QString::number(location.capacity.value_or(0.0))));
        locationTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(location.unitOfCapacity.value_or(""))));
        locationTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(location.status))));
    }
    locationTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("LocationManagementWidget: Locations loaded successfully.");
}

void LocationManagementWidget::populateWarehouseComboBox() {
    warehouseComboBox_->clear();
    std::vector<ERP::Catalog::DTO::WarehouseDTO> allWarehouses = warehouseService_->getAllWarehouses({}, currentUserId_, currentUserRoleIds_);
    for (const auto& warehouse : allWarehouses) {
        warehouseComboBox_->addItem(QString::fromStdString(warehouse.name), QString::fromStdString(warehouse.id));
    }
}

void LocationManagementWidget::onAddLocationClicked() {
    if (!hasPermission("Catalog.CreateLocation")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm vị trí kho.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateWarehouseComboBox(); // Populate before showing dialog
    showLocationInputDialog();
}

void LocationManagementWidget::onEditLocationClicked() {
    if (!hasPermission("Catalog.UpdateLocation")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa vị trí kho.", QMessageBox::Warning);
        return;
    }

    int selectedRow = locationTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Vị Trí Kho", "Vui lòng chọn một vị trí kho để sửa.", QMessageBox::Information);
        return;
    }

    QString locationId = locationTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Catalog::DTO::LocationDTO> locationOpt = locationService_->getLocationById(locationId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (locationOpt) {
        populateWarehouseComboBox();
        showLocationInputDialog(&(*locationOpt)); // Pass by address to modify
    } else {
        showMessageBox("Sửa Vị Trí Kho", "Không tìm thấy vị trí kho để sửa.", QMessageBox::Critical);
    }
}

void LocationManagementWidget::onDeleteLocationClicked() {
    if (!hasPermission("Catalog.DeleteLocation")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa vị trí kho.", QMessageBox::Warning);
        return;
    }

    int selectedRow = locationTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Vị Trí Kho", "Vui lòng chọn một vị trí kho để xóa.", QMessageBox::Information);
        return;
    }

    QString locationId = locationTable_->item(selectedRow, 0)->text();
    QString locationName = locationTable_->item(selectedRow, 2)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Vị Trí Kho");
    confirmBox.setText("Bạn có chắc chắn muốn xóa vị trí kho '" + locationName + "' (ID: " + locationId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (locationService_->deleteLocation(locationId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Vị Trí Kho", "Vị trí kho đã được xóa thành công.", QMessageBox::Information);
            loadLocations();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa vị trí kho. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void LocationManagementWidget::onUpdateLocationStatusClicked() {
    if (!hasPermission("Catalog.ChangeLocationStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái vị trí kho.", QMessageBox::Warning);
        return;
    }

    int selectedRow = locationTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một vị trí kho để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString locationId = locationTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Catalog::DTO::LocationDTO> locationOpt = locationService_->getLocationById(locationId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!locationOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy vị trí kho để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Catalog::DTO::LocationDTO currentLocation = *locationOpt;
    // Toggle status between ACTIVE and INACTIVE
    ERP::Common::EntityStatus newStatus = (currentLocation.status == ERP::Common::EntityStatus::ACTIVE) ?
                                          ERP::Common::EntityStatus::INACTIVE :
                                          ERP::Common::EntityStatus::ACTIVE;

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Cập nhật trạng thái vị trí kho");
    confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái vị trí kho '" + QString::fromStdString(currentLocation.name) + "' thành " + QString::fromStdString(ERP::Common::entityStatusToString(newStatus)) + "?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (locationService_->updateLocationStatus(locationId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Cập nhật trạng thái", "Trạng thái vị trí kho đã được cập nhật thành công.", QMessageBox::Information);
            loadLocations();
            clearForm();
        } else {
            showMessageBox("Lỗi", "Không thể cập nhật trạng thái vị trí kho. Vui lòng kiểm tra log.", QMessageBox::Critical);
        }
    }
}


void LocationManagementWidget::onSearchLocationClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["name_contains"] = searchText.toStdString(); // Assuming service supports "contains" filter
    }
    locationTable_->setRowCount(0);
    std::vector<ERP::Catalog::DTO::LocationDTO> locations = locationService_->getAllLocations(filter, currentUserId_, currentUserRoleIds_);

    locationTable_->setRowCount(locations.size());
    for (int i = 0; i < locations.size(); ++i) {
        const auto& location = locations[i];
        locationTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(location.id)));
        
        QString warehouseName = "N/A";
        std::optional<ERP::Catalog::DTO::WarehouseDTO> warehouse = warehouseService_->getWarehouseById(location.warehouseId, currentUserId_, currentUserRoleIds_);
        if (warehouse) {
            warehouseName = QString::fromStdString(warehouse->name);
        }
        locationTable_->setItem(i, 1, new QTableWidgetItem(warehouseName));

        locationTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(location.name)));
        locationTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(location.type.value_or(""))));
        locationTable_->setItem(i, 4, new QTableWidgetItem(QString::number(location.capacity.value_or(0.0))));
        locationTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(location.unitOfCapacity.value_or(""))));
        locationTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(location.status))));
    }
    locationTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("LocationManagementWidget: Search completed.");
}

void LocationManagementWidget::onLocationTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString locationId = locationTable_->item(row, 0)->text();
    std::optional<ERP::Catalog::DTO::LocationDTO> locationOpt = locationService_->getLocationById(locationId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (locationOpt) {
        idLineEdit_->setText(QString::fromStdString(locationOpt->id));
        nameLineEdit_->setText(QString::fromStdString(locationOpt->name));
        typeLineEdit_->setText(QString::fromStdString(locationOpt->type.value_or("")));
        capacityLineEdit_->setText(QString::number(locationOpt->capacity.value_or(0.0)));
        unitOfCapacityLineEdit_->setText(QString::fromStdString(locationOpt->unitOfCapacity.value_or("")));
        
        populateWarehouseComboBox();
        int index = warehouseComboBox_->findData(QString::fromStdString(locationOpt->warehouseId));
        if (index != -1) {
            warehouseComboBox_->setCurrentIndex(index);
        }

        int statusIndex = statusComboBox_->findData(static_cast<int>(locationOpt->status));
        if (statusIndex != -1) {
            statusComboBox_->setCurrentIndex(statusIndex);
        }

    } else {
        showMessageBox("Thông tin Vị Trí Kho", "Không thể tải chi tiết vị trí kho đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState(); // Update button states after selection
}

void LocationManagementWidget::clearForm() {
    idLineEdit_->clear();
    warehouseComboBox_->clear(); // Will be repopulated when needed
    nameLineEdit_->clear();
    typeLineEdit_->clear();
    capacityLineEdit_->clear();
    unitOfCapacityLineEdit_->clear();
    statusComboBox_->setCurrentIndex(0); // Default to Active
    locationTable_->clearSelection(); // Clear table selection
    updateButtonsState(); // Update button states after clearing
}


void LocationManagementWidget::showLocationInputDialog(ERP::Catalog::DTO::LocationDTO* location) {
    // Create a dialog for input
    QDialog dialog(this);
    dialog.setWindowTitle(location ? "Sửa Vị Trí Kho" : "Thêm Vị Trí Kho Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QComboBox warehouseCombo(this);
    populateWarehouseComboBox(); // Re-populate for dialog specific combo box
    for (int i = 0; i < warehouseComboBox_->count(); ++i) { // Copy items from main combo
        warehouseCombo.addItem(warehouseComboBox_->itemText(i), warehouseComboBox_->itemData(i));
    }


    QLineEdit nameEdit(this);
    QLineEdit typeEdit(this);
    QLineEdit capacityEdit(this);
    capacityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    QLineEdit unitOfCapacityEdit(this);

    if (location) {
        int index = warehouseCombo.findData(QString::fromStdString(location->warehouseId));
        if (index != -1) warehouseCombo.setCurrentIndex(index);
        nameEdit.setText(QString::fromStdString(location->name));
        typeEdit.setText(QString::fromStdString(location->type.value_or("")));
        capacityEdit.setText(QString::number(location->capacity.value_or(0.0)));
        unitOfCapacityEdit.setText(QString::fromStdString(location->unitOfCapacity.value_or("")));
    } else {
        // Defaults for new
        capacityEdit.setText("0.0");
    }

    formLayout->addRow("Kho hàng:*", &warehouseCombo);
    formLayout->addRow("Tên vị trí:*", &nameEdit);
    formLayout->addRow("Loại:", &typeEdit);
    formLayout->addRow("Sức chứa:", &capacityEdit);
    formLayout->addRow("Đơn vị Sức chứa:", &unitOfCapacityEdit);

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(location ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Catalog::DTO::LocationDTO newLocationData;
        if (location) {
            newLocationData = *location; // Start with existing data for update
        }

        newLocationData.warehouseId = warehouseCombo.currentData().toString().toStdString();
        newLocationData.name = nameEdit.text().toStdString();
        newLocationData.type = typeEdit.text().isEmpty() ? std::nullopt : std::make_optional(typeEdit.text().toStdString());
        newLocationData.capacity = capacityEdit.text().toDouble();
        newLocationData.unitOfCapacity = unitOfCapacityEdit.text().isEmpty() ? std::nullopt : std::make_optional(unitOfCapacityEdit.text().toStdString());

        bool success = false;
        if (location) {
            // Update existing location
            success = locationService_->updateLocation(newLocationData, currentUserId_, currentUserRoleIds_);
            if (success) {
                showMessageBox("Sửa Vị Trí Kho", "Vị trí kho đã được cập nhật thành công.", QMessageBox::Information);
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật vị trí kho. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        } else {
            // Create new location
            std::optional<ERP::Catalog::DTO::LocationDTO> createdLocation = locationService_->createLocation(newLocationData, currentUserId_, currentUserRoleIds_);
            if (createdLocation) {
                showMessageBox("Thêm Vị Trí Kho", "Vị trí kho mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm vị trí kho mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadLocations();
            clearForm();
        }
    }
}


void LocationManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool LocationManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void LocationManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Catalog.CreateLocation");
    bool canUpdate = hasPermission("Catalog.UpdateLocation");
    bool canDelete = hasPermission("Catalog.DeleteLocation");
    bool canChangeStatus = hasPermission("Catalog.ChangeLocationStatus");

    addLocationButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Catalog.ViewLocations"));

    bool isRowSelected = locationTable_->currentRow() >= 0;
    editLocationButton_->setEnabled(isRowSelected && canUpdate);
    deleteLocationButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);

    // Disable form fields if no row is selected or no edit permission
    bool enableForm = isRowSelected && canUpdate;
    warehouseComboBox_->setEnabled(enableForm);
    nameLineEdit_->setEnabled(enableForm);
    typeLineEdit_->setEnabled(enableForm);
    capacityLineEdit_->setEnabled(enableForm);
    unitOfCapacityLineEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        warehouseComboBox_->clear();
        nameLineEdit_->clear();
        typeLineEdit_->clear();
        capacityLineEdit_->clear();
        unitOfCapacityLineEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
    }
}


} // namespace Catalog
} // namespace UI
} // namespace ERP