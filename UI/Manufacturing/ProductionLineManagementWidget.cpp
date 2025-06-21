// UI/Manufacturing/ProductionLineManagementWidget.cpp
#include "ProductionLineManagementWidget.h" // Đã rút gọn include
#include "ProductionLine.h"         // Đã rút gọn include
#include "Location.h"               // Đã rút gọn include
#include "Asset.h"                  // Đã rút gọn include
#include "ProductionLineService.h"  // Đã rút gọn include
#include "LocationService.h"        // Đã rút gọn include
#include "AssetManagementService.h" // Đã rút gọn include
#include "ISecurityManager.h"       // Đã rút gọn include
#include "Logger.h"                 // Đã rút gọn include
#include "ErrorHandler.h"           // Đã rút gọn include
#include "Common.h"                 // Đã rút gọn include
#include "DateUtils.h"              // Đã rút gọn include
#include "StringUtils.h"            // Đã rút gọn include
#include "CustomMessageBox.h"       // Đã rút gọn include
#include "UserService.h"            // For getting current user roles from SecurityManager
#include "DTOUtils.h"               // For JSON conversion helpers for metadata

#include <QInputDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSet> // For QSet for efficient checking

namespace ERP {
namespace UI {
namespace Manufacturing {

ProductionLineManagementWidget::ProductionLineManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IProductionLineService> productionLineService,
    std::shared_ptr<Catalog::Services::ILocationService> locationService,
    std::shared_ptr<Asset::Services::IAssetManagementService> assetService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      productionLineService_(productionLineService),
      locationService_(locationService),
      assetService_(assetService),
      securityManager_(securityManager) {

    if (!productionLineService_ || !locationService_ || !assetService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ dây chuyền sản xuất, vị trí, tài sản hoặc bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("ProductionLineManagementWidget: Initialized with null dependencies.");
        return;
    }

    auto authService = securityManager_->getAuthenticationService();
    if (authService) {
        std::string dummySessionId = "current_session_id"; // Placeholder
        std::optional<ERP::Security::DTO::SessionDTO> currentSession = authService->validateSession(dummySessionId);
        if (currentSession) {
            currentUserId_ = currentSession->userId;
            currentUserRoleIds_ = securityManager_->getUserService()->getUserRoles(currentUserId_, {});
        } else {
            currentUserId_ = "system_user";
            currentUserRoleIds_ = {"anonymous"};
            ERP::Logger::Logger::getInstance().warning("ProductionLineManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("ProductionLineManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadProductionLines();
    updateButtonsState();
}

ProductionLineManagementWidget::~ProductionLineManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void ProductionLineManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên dây chuyền...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &ProductionLineManagementWidget::onSearchLineClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    lineTable_ = new QTableWidget(this);
    lineTable_->setColumnCount(5); // ID, Tên, Địa điểm, Trạng thái, Số tài sản liên kết
    lineTable_->setHorizontalHeaderLabels({"ID", "Tên Dây chuyền", "Địa điểm", "Trạng thái", "Số tài sản liên kết"});
    lineTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    lineTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    lineTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    lineTable_->horizontalHeader()->setStretchLastSection(true);
    connect(lineTable_, &QTableWidget::itemClicked, this, &ProductionLineManagementWidget::onLineTableItemClicked);
    mainLayout->addWidget(lineTable_);

    // Form elements for editing/adding
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    lineNameLineEdit_ = new QLineEdit(this);
    descriptionLineEdit_ = new QLineEdit(this);
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();
    locationComboBox_ = new QComboBox(this); populateLocationComboBox();

    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Tên Dây chuyền:*", this), 1, 0); formLayout->addWidget(lineNameLineEdit_, 1, 1);
    formLayout->addWidget(new QLabel("Mô tả:", this), 2, 0); formLayout->addWidget(descriptionLineEdit_, 2, 1);
    formLayout->addWidget(new QLabel("Trạng thái:*", this), 3, 0); formLayout->addWidget(statusComboBox_, 3, 1);
    formLayout->addWidget(new QLabel("Địa điểm:*", this), 4, 0); formLayout->addWidget(locationComboBox_, 4, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addLineButton_ = new QPushButton("Thêm mới", this); connect(addLineButton_, &QPushButton::clicked, this, &ProductionLineManagementWidget::onAddLineClicked);
    editLineButton_ = new QPushButton("Sửa", this); connect(editLineButton_, &QPushButton::clicked, this, &ProductionLineManagementWidget::onEditLineClicked);
    deleteLineButton_ = new QPushButton("Xóa", this); connect(deleteLineButton_, &QPushButton::clicked, this, &ProductionLineManagementWidget::onDeleteLineClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &ProductionLineManagementWidget::onUpdateLineStatusClicked);
    manageAssetsButton_ = new QPushButton("Quản lý Tài sản", this); connect(manageAssetsButton_, &QPushButton::clicked, this, &ProductionLineManagementWidget::onManageAssetsClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &ProductionLineManagementWidget::clearForm);
    
    buttonLayout->addWidget(addLineButton_);
    buttonLayout->addWidget(editLineButton_);
    buttonLayout->addWidget(deleteLineButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(manageAssetsButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void ProductionLineManagementWidget::loadProductionLines() {
    ERP::Logger::Logger::getInstance().info("ProductionLineManagementWidget: Loading production lines...");
    lineTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Manufacturing::DTO::ProductionLineDTO> lines = productionLineService_->getAllProductionLines({}, currentUserId_, currentUserRoleIds_);

    lineTable_->setRowCount(lines.size());
    for (int i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        lineTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(line.id)));
        lineTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(line.lineName)));
        
        QString locationName = "N/A";
        std::optional<ERP::Catalog::DTO::LocationDTO> location = securityManager_->getLocationService()->getLocationById(line.locationId, currentUserId_, currentUserRoleIds_);
        if (location) locationName = QString::fromStdString(location->name);
        lineTable_->setItem(i, 2, new QTableWidgetItem(locationName));

        lineTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(line.getStatusString())));
        lineTable_->setItem(i, 4, new QTableWidgetItem(QString::number(line.associatedAssetIds.size()))); // Number of associated assets
    }
    lineTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ProductionLineManagementWidget: Production lines loaded successfully.");
}

void ProductionLineManagementWidget::populateLocationComboBox() {
    locationComboBox_->clear();
    std::vector<ERP::Catalog::DTO::LocationDTO> allLocations = securityManager_->getLocationService()->getAllLocations({}, currentUserId_, currentUserRoleIds_);
    for (const auto& location : allLocations) {
        locationComboBox_->addItem(QString::fromStdString(location.name), QString::fromStdString(location.id));
    }
}

void ProductionLineManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Operational", static_cast<int>(ERP::Manufacturing::DTO::ProductionLineStatus::OPERATIONAL));
    statusComboBox_->addItem("Maintenance", static_cast<int>(ERP::Manufacturing::DTO::ProductionLineStatus::MAINTENANCE));
    statusComboBox_->addItem("Idle", static_cast<int>(ERP::Manufacturing::DTO::ProductionLineStatus::IDLE));
    statusComboBox_->addItem("Shutdown", static_cast<int>(ERP::Manufacturing::DTO::ProductionLineStatus::SHUTDOWN));
}

void ProductionLineManagementWidget::onAddLineClicked() {
    if (!hasPermission("Manufacturing.CreateProductionLine")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm dây chuyền sản xuất.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateLocationComboBox();
    showLineInputDialog();
}

void ProductionLineManagementWidget::onEditLineClicked() {
    if (!hasPermission("Manufacturing.UpdateProductionLine")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa dây chuyền sản xuất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = lineTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Dây chuyền", "Vui lòng chọn một dây chuyền sản xuất để sửa.", QMessageBox::Information);
        return;
    }

    QString lineId = lineTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> lineOpt = productionLineService_->getProductionLineById(lineId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (lineOpt) {
        populateLocationComboBox();
        showLineInputDialog(&(*lineOpt));
    } else {
        showMessageBox("Sửa Dây chuyền", "Không tìm thấy dây chuyền sản xuất để sửa.", QMessageBox::Critical);
    }
}

void ProductionLineManagementWidget::onDeleteLineClicked() {
    if (!hasPermission("Manufacturing.DeleteProductionLine")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa dây chuyền sản xuất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = lineTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Dây chuyền", "Vui lòng chọn một dây chuyền sản xuất để xóa.", QMessageBox::Information);
        return;
    }

    QString lineId = lineTable_->item(selectedRow, 0)->text();
    QString lineName = lineTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Dây chuyền");
    confirmBox.setText("Bạn có chắc chắn muốn xóa dây chuyền sản xuất '" + lineName + "' (ID: " + lineId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (productionLineService_->deleteProductionLine(lineId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Dây chuyền", "Dây chuyền sản xuất đã được xóa thành công.", QMessageBox::Information);
            loadProductionLines();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa dây chuyền sản xuất. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void ProductionLineManagementWidget::onUpdateLineStatusClicked() {
    if (!hasPermission("Manufacturing.UpdateProductionLineStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái dây chuyền sản xuất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = lineTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một dây chuyền sản xuất để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString lineId = lineTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> lineOpt = productionLineService_->getProductionLineById(lineId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!lineOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy dây chuyền sản xuất để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    // Use a QInputDialog or custom dialog to get the new status
    QDialog statusDialog(this);
    statusDialog.setWindowTitle("Chọn Trạng Thái Mới");
    QVBoxLayout *layout = new QVBoxLayout(&statusDialog);
    QComboBox newStatusCombo;
    populateStatusComboBox(); // Populate it
    for(int i = 0; i < statusComboBox_->count(); ++i) { // Copy items
        newStatusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));
    }
    // Set current status as default selected
    int currentStatusIndex = newStatusCombo.findData(static_cast<int>(lineOpt->status));
    if (currentStatusIndex != -1) newStatusCombo.setCurrentIndex(currentStatusIndex);

    layout->addWidget(new QLabel("Chọn trạng thái mới:", &statusDialog));
    layout->addWidget(&newStatusCombo);
    QPushButton *okButton = new QPushButton("Cập nhật", &statusDialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &statusDialog);
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(okButton);
    btnLayout->addWidget(cancelButton);
    layout->addLayout(btnLayout);

    connect(okButton, &QPushButton::clicked, &statusDialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (statusDialog.exec() == QDialog::Accepted) {
        ERP::Manufacturing::DTO::ProductionLineStatus newStatus = static_cast<ERP::Manufacturing::DTO::ProductionLineStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái dây chuyền sản xuất");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái dây chuyền sản xuất này thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (productionLineService_->updateProductionLineStatus(lineId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái dây chuyền sản xuất đã được cập nhật thành công.", QMessageBox::Information);
                loadProductionLines();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái dây chuyền sản xuất. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}


void ProductionLineManagementWidget::onSearchLineClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["line_name_contains"] = searchText.toStdString(); // Assuming service supports "contains" filter
    }
    lineTable_->setRowCount(0);
    std::vector<ERP::Manufacturing::DTO::ProductionLineDTO> lines = productionLineService_->getAllProductionLines(filter, currentUserId_, currentUserRoleIds_);

    lineTable_->setRowCount(lines.size());
    for (int i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        lineTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(line.id)));
        lineTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(line.lineName)));
        
        QString locationName = "N/A";
        std::optional<ERP::Catalog::DTO::LocationDTO> location = securityManager_->getLocationService()->getLocationById(line.locationId, currentUserId_, currentUserRoleIds_);
        if (location) locationName = QString::fromStdString(location->name);
        lineTable_->setItem(i, 2, new QTableWidgetItem(locationName));

        lineTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(line.getStatusString())));
        lineTable_->setItem(i, 4, new QTableWidgetItem(QString::number(line.associatedAssetIds.size())));
    }
    lineTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ProductionLineManagementWidget: Search completed.");
}

void ProductionLineManagementWidget::onLineTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString lineId = lineTable_->item(row, 0)->text();
    std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> lineOpt = productionLineService_->getProductionLineById(lineId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (lineOpt) {
        idLineEdit_->setText(QString::fromStdString(lineOpt->id));
        lineNameLineEdit_->setText(QString::fromStdString(lineOpt->lineName));
        descriptionLineEdit_->setText(QString::fromStdString(lineOpt->description.value_or("")));
        
        populateStatusComboBox();
        int statusIndex = statusComboBox_->findData(static_cast<int>(lineOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);

        populateLocationComboBox();
        int locationIndex = locationComboBox_->findData(QString::fromStdString(lineOpt->locationId));
        if (locationIndex != -1) locationComboBox_->setCurrentIndex(locationIndex);

    } else {
        showMessageBox("Thông tin Dây chuyền", "Không tìm thấy dây chuyền sản xuất để sửa.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void ProductionLineManagementWidget::clearForm() {
    idLineEdit_->clear();
    lineNameLineEdit_->clear();
    descriptionLineEdit_->clear();
    statusComboBox_->setCurrentIndex(0); // Default to Operational
    locationComboBox_->clear(); // Will be repopulated
    lineTable_->clearSelection();
    updateButtonsState();
}

void ProductionLineManagementWidget::onManageAssetsClicked() {
    if (!hasPermission("Manufacturing.ManageProductionLineAssets")) { // Assuming this permission
        showMessageBox("Lỗi", "Bạn không có quyền quản lý tài sản của dây chuyền sản xuất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = lineTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Quản lý Tài sản", "Vui lòng chọn một dây chuyền sản xuất để quản lý tài sản.", QMessageBox::Information);
        return;
    }

    QString lineId = lineTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> lineOpt = productionLineService_->getProductionLineById(lineId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (lineOpt) {
        showManageAssetsDialog(&(*lineOpt));
    } else {
        showMessageBox("Quản lý Tài sản", "Không tìm thấy dây chuyền sản xuất để quản lý tài sản.", QMessageBox::Critical);
    }
}


void ProductionLineManagementWidget::showLineInputDialog(ERP::Manufacturing::DTO::ProductionLineDTO* line) {
    QDialog dialog(this);
    dialog.setWindowTitle(line ? "Sửa Dây chuyền" : "Thêm Dây chuyền Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit lineNameEdit(this);
    QLineEdit descriptionEdit(this);
    QComboBox statusCombo(this); populateStatusComboBox();
    for (int i = 0; i < statusComboBox_->count(); ++i) statusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));
    QComboBox locationCombo(this); populateLocationComboBox();
    for (int i = 0; i < locationComboBox_->count(); ++i) locationCombo.addItem(locationComboBox_->itemText(i), locationComboBox_->itemData(i));

    if (line) {
        lineNameEdit.setText(QString::fromStdString(line->lineName));
        descriptionEdit.setText(QString::fromStdString(line->description.value_or("")));
        int statusIndex = statusCombo.findData(static_cast<int>(line->status));
        if (statusIndex != -1) statusCombo.setCurrentIndex(statusIndex);
        int locationIndex = locationCombo.findData(QString::fromStdString(line->locationId));
        if (locationIndex != -1) locationCombo.setCurrentIndex(locationIndex);
    }

    formLayout->addRow("Tên Dây chuyền:*", &lineNameEdit);
    formLayout->addRow("Mô tả:", &descriptionEdit);
    formLayout->addRow("Trạng thái:*", &statusCombo);
    formLayout->addRow("Địa điểm:*", &locationCombo);

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(line ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Manufacturing::DTO::ProductionLineDTO newLineData;
        if (line) {
            newLineData = *line;
        }

        newLineData.lineName = lineNameEdit.text().toStdString();
        newLineData.description = descriptionEdit.text().isEmpty() ? std::nullopt : std::make_optional(descriptionEdit.text().toStdString());
        newLineData.status = static_cast<ERP::Manufacturing::DTO::ProductionLineStatus>(statusCombo.currentData().toInt());
        newLineData.locationId = locationCombo.currentData().toString().toStdString();
        
        // Associated assets are handled in separate dialog, load existing ones if editing
        std::vector<std::string> currentAssets;
        if (line) {
            currentAssets = line->associatedAssetIds; // Keep existing assets if not modified in asset dialog
        }
        newLineData.associatedAssetIds = currentAssets;

        bool success = false;
        if (line) {
            success = productionLineService_->updateProductionLine(newLineData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Dây chuyền", "Dây chuyền sản xuất đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật dây chuyền sản xuất. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> createdLine = productionLineService_->createProductionLine(newLineData, currentUserId_, currentUserRoleIds_);
            if (createdLine) {
                showMessageBox("Thêm Dây chuyền", "Dây chuyền sản xuất mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm dây chuyền sản xuất mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadProductionLines();
            clearForm();
        }
    }
}

void ProductionLineManagementWidget::showManageAssetsDialog(ERP::Manufacturing::DTO::ProductionLineDTO* line) {
    if (!line) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Quản lý Tài sản cho Dây chuyền: " + QString::fromStdString(line->lineName));
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QListWidget *allAssetsList = new QListWidget(&dialog);
    allAssetsList->setSelectionMode(QAbstractItemView::MultiSelection);
    dialogLayout->addWidget(new QLabel("Tất cả tài sản có sẵn:", &dialog));
    dialogLayout->addWidget(allAssetsList);

    // Populate all available assets
    std::vector<ERP::Asset::DTO::AssetDTO> allAssets = assetService_->getAllAssets({}, currentUserId_, currentUserRoleIds_);
    for (const auto& asset : allAssets) {
        QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(asset.assetName + " (" + asset.assetCode + ")"), allAssetsList);
        item->setData(Qt::UserRole, QString::fromStdString(asset.id)); // Store ID in user data
        allAssetsList->addItem(item);
    }

    // Select currently associated assets
    QSet<QString> associatedAssetIds;
    for (const auto& assetId : line->associatedAssetIds) {
        associatedAssetIds.insert(QString::fromStdString(assetId));
    }

    for (int i = 0; i < allAssetsList->count(); ++i) {
        QListWidgetItem *item = allAssetsList->item(i);
        if (associatedAssetIds.contains(item->data(Qt::UserRole).toString())) {
            item->setSelected(true);
        }
    }

    QPushButton *saveButton = new QPushButton("Lưu", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(saveButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Collect newly selected assets
        std::vector<std::string> newlySelectedAssetIds;
        for (int i = 0; i < allAssetsList->count(); ++i) {
            QListWidgetItem *item = allAssetsList->item(i);
            if (item->isSelected()) {
                newlySelectedAssetIds.push_back(item->data(Qt::UserRole).toString().toStdString());
            }
        }

        // Update the production line with the new asset list
        ERP::Manufacturing::DTO::ProductionLineDTO updatedLine = *line;
        updatedLine.associatedAssetIds = newlySelectedAssetIds;
        
        if (productionLineService_->updateProductionLine(updatedLine, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Quản lý Tài sản", "Tài sản của dây chuyền sản xuất đã được cập nhật thành công.", QMessageBox::Information);
            loadProductionLines(); // Refresh the main table
        } else {
            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật tài sản của dây chuyền sản xuất. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        }
    }
}


void ProductionLineManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool ProductionLineManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void ProductionLineManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Manufacturing.CreateProductionLine");
    bool canUpdate = hasPermission("Manufacturing.UpdateProductionLine");
    bool canDelete = hasPermission("Manufacturing.DeleteProductionLine");
    bool canChangeStatus = hasPermission("Manufacturing.UpdateProductionLineStatus");
    bool canManageAssets = hasPermission("Manufacturing.ManageProductionLineAssets"); // Assuming this permission

    addLineButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Manufacturing.ViewProductionLine"));

    bool isRowSelected = lineTable_->currentRow() >= 0;
    editLineButton_->setEnabled(isRowSelected && canUpdate);
    deleteLineButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    manageAssetsButton_->setEnabled(isRowSelected && canManageAssets);

    bool enableForm = isRowSelected && canUpdate;
    lineNameLineEdit_->setEnabled(enableForm);
    descriptionLineEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    locationComboBox_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        lineNameLineEdit_->clear();
        descriptionLineEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
        locationComboBox_->clear();
    }
}


} // namespace Manufacturing
} // namespace UI
} // namespace ERP