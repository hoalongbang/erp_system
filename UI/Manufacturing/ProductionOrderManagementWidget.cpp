// UI/Manufacturing/ProductionOrderManagementWidget.cpp
#include "ProductionOrderManagementWidget.h" // Đã rút gọn include
#include "ProductionOrder.h"        // Đã rút gọn include
#include "Product.h"                // Đã rút gọn include
#include "BillOfMaterial.h"         // Đã rút gọn include
#include "ProductionLine.h"         // Đã rút gọn include
#include "UnitOfMeasure.h"          // For resolving UoM name
#include "ProductionOrderService.h" // Đã rút gọn include
#include "ProductService.h"         // Đã rút gọn include
#include "BillOfMaterialService.h"  // Đã rút gọn include
#include "ProductionLineService.h"  // Đã rút gọn include
#include "ISecurityManager.h"       // Đã rút gọn include
#include "Logger.h"                 // Đã rút gọn include
#include "ErrorHandler.h"           // Đã rút gọn include
#include "Common.h"                 // Đã rút gọn include
#include "DateUtils.h"              // Đã rút gọn include
#include "StringUtils.h"            // Đã rút gọn include
#include "CustomMessageBox.h"       // Đã rút gọn include
#include "UserService.h"            // For getting current user roles from SecurityManager

#include <QInputDialog>
#include <QDoubleValidator>

namespace ERP {
namespace UI {
namespace Manufacturing {

ProductionOrderManagementWidget::ProductionOrderManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IProductionOrderService> productionOrderService,
    std::shared_ptr<Product::Services::IProductService> productService,
    std::shared_ptr<Services::IBillOfMaterialService> bomService,
    std::shared_ptr<Services::IProductionLineService> productionLineService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      productionOrderService_(productionOrderService),
      productService_(productService),
      bomService_(bomService),
      productionLineService_(productionLineService),
      securityManager_(securityManager) {

    if (!productionOrderService_ || !productService_ || !bomService_ || !productionLineService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ lệnh sản xuất, sản phẩm, BOM, dây chuyền sản xuất hoặc bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("ProductionOrderManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("ProductionOrderManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("ProductionOrderManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadProductionOrders();
    updateButtonsState();
}

ProductionOrderManagementWidget::~ProductionOrderManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void ProductionOrderManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo số lệnh sản xuất...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &ProductionOrderManagementWidget::onSearchOrderClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    orderTable_ = new QTableWidget(this);
    orderTable_->setColumnCount(10); // ID, Số lệnh, Sản phẩm, SL kế hoạch, Đơn vị, Trạng thái, BOM, Dây chuyền, Ngày bắt đầu KH, Ngày kết thúc KH
    orderTable_->setHorizontalHeaderLabels({"ID", "Số Lệnh", "Sản phẩm", "SL kế hoạch", "Đơn vị", "Trạng thái", "BOM", "Dây chuyền", "Ngày bắt đầu KH", "Ngày kết thúc KH"});
    orderTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    orderTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    orderTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    orderTable_->horizontalHeader()->setStretchLastSection(true);
    connect(orderTable_, &QTableWidget::itemClicked, this, &ProductionOrderManagementWidget::onOrderTableItemClicked);
    mainLayout->addWidget(orderTable_);

    // Form elements for editing/adding orders
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    orderNumberLineEdit_ = new QLineEdit(this);
    productComboBox_ = new QComboBox(this); populateProductComboBox();
    plannedQuantityLineEdit_ = new QLineEdit(this); plannedQuantityLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    unitOfMeasureComboBox_ = new QComboBox(this); populateUnitOfMeasureComboBox();
    statusComboBox_ = new QComboBox(this); populateStatusComboBox();
    bomComboBox_ = new QComboBox(this); populateBOMComboBox();
    productionLineComboBox_ = new QComboBox(this); populateProductionLineComboBox();
    plannedStartDateEdit_ = new QDateTimeEdit(this); plannedStartDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    plannedEndDateEdit_ = new QDateTimeEdit(this); plannedEndDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    actualStartDateEdit_ = new QDateTimeEdit(this); actualStartDateEdit_->setReadOnly(true); actualStartDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    actualEndDateEdit_ = new QDateTimeEdit(this); actualEndDateEdit_->setReadOnly(true); actualEndDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    actualQuantityProducedLineEdit_ = new QLineEdit(this); actualQuantityProducedLineEdit_->setReadOnly(true); actualQuantityProducedLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    notesLineEdit_ = new QLineEdit(this);

    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Số Lệnh:*", this), 1, 0); formLayout->addWidget(orderNumberLineEdit_, 1, 1);
    formLayout->addWidget(new QLabel("Sản phẩm:*", this), 2, 0); formLayout->addWidget(productComboBox_, 2, 1);
    formLayout->addWidget(new QLabel("SL kế hoạch:*", this), 3, 0); formLayout->addWidget(plannedQuantityLineEdit_, 3, 1);
    formLayout->addWidget(new QLabel("Đơn vị:*", this), 4, 0); formLayout->addWidget(unitOfMeasureComboBox_, 4, 1);
    formLayout->addWidget(new QLabel("Trạng thái:*", this), 5, 0); formLayout->addWidget(statusComboBox_, 5, 1);
    formLayout->addWidget(new QLabel("BOM:", this), 6, 0); formLayout->addWidget(bomComboBox_, 6, 1);
    formLayout->addWidget(new QLabel("Dây chuyền:", this), 7, 0); formLayout->addWidget(productionLineComboBox_, 7, 1);
    formLayout->addWidget(new QLabel("Ngày bắt đầu KH:*", this), 8, 0); formLayout->addWidget(plannedStartDateEdit_, 8, 1);
    formLayout->addWidget(new QLabel("Ngày kết thúc KH:*", this), 9, 0); formLayout->addWidget(plannedEndDateEdit_, 9, 1);
    formLayout->addWidget(new QLabel("Ngày bắt đầu TT:", this), 10, 0); formLayout->addWidget(actualStartDateEdit_, 10, 1);
    formLayout->addWidget(new QLabel("Ngày kết thúc TT:", this), 11, 0); formLayout->addWidget(actualEndDateEdit_, 11, 1);
    formLayout->addWidget(new QLabel("SL thực tế:", this), 12, 0); formLayout->addWidget(actualQuantityProducedLineEdit_, 12, 1);
    formLayout->addWidget(new QLabel("Ghi chú:", this), 13, 0); formLayout->addWidget(notesLineEdit_, 13, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addOrderButton_ = new QPushButton("Thêm mới", this); connect(addOrderButton_, &QPushButton::clicked, this, &ProductionOrderManagementWidget::onAddOrderClicked);
    editOrderButton_ = new QPushButton("Sửa", this); connect(editOrderButton_, &QPushButton::clicked, this, &ProductionOrderManagementWidget::onEditOrderClicked);
    deleteOrderButton_ = new QPushButton("Xóa", this); connect(deleteOrderButton_, &QPushButton::clicked, this, &ProductionOrderManagementWidget::onDeleteOrderClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &ProductionOrderManagementWidget::onUpdateOrderStatusClicked);
    recordActualQuantityButton_ = new QPushButton("Ghi nhận SL thực tế", this); connect(recordActualQuantityButton_, &QPushButton::clicked, this, &ProductionOrderManagementWidget::onRecordActualQuantityClicked);
    searchButton_ = new QPushButton("Tìm kiếm", this); connect(searchButton_, &QPushButton::clicked, this, &ProductionOrderManagementWidget::onSearchOrderClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &ProductionOrderManagementWidget::clearForm);
    
    buttonLayout->addWidget(addOrderButton_);
    buttonLayout->addWidget(editOrderButton_);
    buttonLayout->addWidget(deleteOrderButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(recordActualQuantityButton_);
    buttonLayout->addWidget(searchButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void ProductionOrderManagementWidget::loadProductionOrders() {
    ERP::Logger::Logger::getInstance().info("ProductionOrderManagementWidget: Loading production orders...");
    orderTable_->setRowCount(0); // Clear existing rows

    std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> orders = productionOrderService_->getAllProductionOrders({}, currentUserId_, currentUserRoleIds_);

    orderTable_->setRowCount(orders.size());
    for (int i = 0; i < orders.size(); ++i) {
        const auto& order = orders[i];
        orderTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(order.id)));
        orderTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(order.orderNumber)));
        
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(order.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);
        orderTable_->setItem(i, 2, new QTableWidgetItem(productName));

        orderTable_->setItem(i, 3, new QTableWidgetItem(QString::number(order.plannedQuantity)));
        
        QString unitName = "N/A";
        std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> unit = securityManager_->getUnitOfMeasureService()->getUnitOfMeasureById(order.unitOfMeasureId, currentUserId_, currentUserRoleIds_);
        if (unit) unitName = QString::fromStdString(unit->name);
        orderTable_->setItem(i, 4, new QTableWidgetItem(unitName));

        orderTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(order.getStatusString())));
        
        QString bomName = "N/A";
        if (order.bomId) {
            std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> bom = bomService_->getBillOfMaterialById(*order.bomId, currentUserId_, currentUserRoleIds_);
            if (bom) bomName = QString::fromStdString(bom->bomName);
        }
        orderTable_->setItem(i, 6, new QTableWidgetItem(bomName));
        
        QString productionLineName = "N/A";
        if (order.productionLineId) {
            std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> line = productionLineService_->getProductionLineById(*order.productionLineId, currentUserId_, currentUserRoleIds_);
            if (line) productionLineName = QString::fromStdString(line->lineName);
        }
        orderTable_->setItem(i, 7, new QTableWidgetItem(productionLineName));

        orderTable_->setItem(i, 8, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(order.plannedStartDate, ERP::Common::DATETIME_FORMAT))));
        orderTable_->setItem(i, 9, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(order.plannedEndDate, ERP::Common::DATETIME_FORMAT))));
    }
    orderTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ProductionOrderManagementWidget: Production orders loaded successfully.");
}

void ProductionOrderManagementWidget::populateProductComboBox() {
    productComboBox_->clear();
    std::vector<ERP::Product::DTO::ProductDTO> allProducts = productService_->getAllProducts({}, currentUserId_, currentUserRoleIds_);
    for (const auto& product : allProducts) {
        productComboBox_->addItem(QString::fromStdString(product.name), QString::fromStdString(product.id));
    }
}

void ProductionOrderManagementWidget::populateUnitOfMeasureComboBox() {
    unitOfMeasureComboBox_->clear();
    std::vector<ERP::Catalog::DTO::UnitOfMeasureDTO> allUoMs = securityManager_->getUnitOfMeasureService()->getAllUnitsOfMeasure({}, currentUserId_, currentUserRoleIds_);
    for (const auto& uom : allUoMs) {
        unitOfMeasureComboBox_->addItem(QString::fromStdString(uom.name), QString::fromStdString(uom.id));
    }
}

void ProductionOrderManagementWidget::populateStatusComboBox() {
    statusComboBox_->clear();
    statusComboBox_->addItem("Draft", static_cast<int>(ERP::Manufacturing::DTO::ProductionOrderStatus::DRAFT));
    statusComboBox_->addItem("Planned", static_cast<int>(ERP::Manufacturing::DTO::ProductionOrderStatus::PLANNED));
    statusComboBox_->addItem("Released", static_cast<int>(ERP::Manufacturing::DTO::ProductionOrderStatus::RELEASED));
    statusComboBox_->addItem("In Progress", static_cast<int>(ERP::Manufacturing::DTO::ProductionOrderStatus::IN_PROGRESS));
    statusComboBox_->addItem("Completed", static_cast<int>(ERP::Manufacturing::DTO::ProductionOrderStatus::COMPLETED));
    statusComboBox_->addItem("Cancelled", static_cast<int>(ERP::Manufacturing::DTO::ProductionOrderStatus::CANCELLED));
    statusComboBox_->addItem("On Hold", static_cast<int>(ERP::Manufacturing::DTO::ProductionOrderStatus::ON_HOLD));
    statusComboBox_->addItem("Rejected", static_cast<int>(ERP::Manufacturing::DTO::ProductionOrderStatus::REJECTED));
}

void ProductionOrderManagementWidget::populateBOMComboBox() {
    bomComboBox_->clear();
    bomComboBox_->addItem("None", "");
    std::vector<ERP::Manufacturing::DTO::BillOfMaterialDTO> allBOMs = bomService_->getAllBillOfMaterials({}, currentUserId_, currentUserRoleIds_);
    for (const auto& bom : allBOMs) {
        bomComboBox_->addItem(QString::fromStdString(bom.bomName + " (" + bom.productId + ")"), QString::fromStdString(bom.id));
    }
}

void ProductionOrderManagementWidget::populateProductionLineComboBox() {
    productionLineComboBox_->clear();
    productionLineComboBox_->addItem("None", "");
    std::vector<ERP::Manufacturing::DTO::ProductionLineDTO> allLines = productionLineService_->getAllProductionLines({}, currentUserId_, currentUserRoleIds_);
    for (const auto& line : allLines) {
        productionLineComboBox_->addItem(QString::fromStdString(line.lineName), QString::fromStdString(line.id));
    }
}


void ProductionOrderManagementWidget::onAddOrderClicked() {
    if (!hasPermission("Manufacturing.CreateProductionOrder")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm lệnh sản xuất.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateProductComboBox();
    populateUnitOfMeasureComboBox();
    populateBOMComboBox();
    populateProductionLineComboBox();
    showOrderInputDialog();
}

void ProductionOrderManagementWidget::onEditOrderClicked() {
    if (!hasPermission("Manufacturing.UpdateProductionOrder")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa lệnh sản xuất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = orderTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Lệnh Sản Xuất", "Vui lòng chọn một lệnh sản xuất để sửa.", QMessageBox::Information);
        return;
    }

    QString orderId = orderTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> orderOpt = productionOrderService_->getProductionOrderById(orderId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (orderOpt) {
        populateProductComboBox();
        populateUnitOfMeasureComboBox();
        populateBOMComboBox();
        populateProductionLineComboBox();
        showOrderInputDialog(&(*orderOpt));
    } else {
        showMessageBox("Sửa Lệnh Sản Xuất", "Không tìm thấy lệnh sản xuất để sửa.", QMessageBox::Critical);
    }
}

void ProductionOrderManagementWidget::onDeleteOrderClicked() {
    if (!hasPermission("Manufacturing.DeleteProductionOrder")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa lệnh sản xuất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = orderTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Lệnh Sản Xuất", "Vui lòng chọn một lệnh sản xuất để xóa.", QMessageBox::Information);
        return;
    }

    QString orderId = orderTable_->item(selectedRow, 0)->text();
    QString orderNumber = orderTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Lệnh Sản Xuất");
    confirmBox.setText("Bạn có chắc chắn muốn xóa lệnh sản xuất '" + orderNumber + "' (ID: " + orderId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (productionOrderService_->deleteProductionOrder(orderId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Lệnh Sản Xuất", "Lệnh sản xuất đã được xóa thành công.", QMessageBox::Information);
            loadProductionOrders();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa lệnh sản xuất. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void ProductionOrderManagementWidget::onUpdateOrderStatusClicked() {
    if (!hasPermission("Manufacturing.UpdateProductionOrderStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái lệnh sản xuất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = orderTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một lệnh sản xuất để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString orderId = orderTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> orderOpt = productionOrderService_->getProductionOrderById(orderId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!orderOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy lệnh sản xuất để cập nhật trạng thái.", QMessageBox::Critical);
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
    int currentStatusIndex = newStatusCombo.findData(static_cast<int>(orderOpt->status));
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
        ERP::Manufacturing::DTO::ProductionOrderStatus newStatus = static_cast<ERP::Manufacturing::DTO::ProductionOrderStatus>(newStatusCombo.currentData().toInt());
        
        Common::CustomMessageBox confirmBox(this);
        confirmBox.setWindowTitle("Cập nhật trạng thái lệnh sản xuất");
        confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái lệnh sản xuất này thành " + newStatusCombo.currentText() + "?");
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (confirmBox.exec() == QMessageBox::Yes) {
            if (productionOrderService_->updateProductionOrderStatus(orderId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                showMessageBox("Cập nhật trạng thái", "Trạng thái lệnh sản xuất đã được cập nhật thành công.", QMessageBox::Information);
                loadProductionOrders();
                clearForm();
            } else {
                showMessageBox("Lỗi", "Không thể cập nhật trạng thái lệnh sản xuất. Vui lòng kiểm tra log.", QMessageBox::Critical);
            }
        }
    }
}

void ProductionOrderManagementWidget::onRecordActualQuantityClicked() {
    if (!hasPermission("Manufacturing.RecordActualQuantityProduced")) {
        showMessageBox("Lỗi", "Bạn không có quyền ghi nhận số lượng thực tế sản xuất.", QMessageBox::Warning);
        return;
    }

    int selectedRow = orderTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Ghi nhận SL thực tế", "Vui lòng chọn một lệnh sản xuất để ghi nhận số lượng thực tế.", QMessageBox::Information);
        return;
    }

    QString orderId = orderTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> orderOpt = productionOrderService_->getProductionOrderById(orderId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!orderOpt) {
        showMessageBox("Ghi nhận SL thực tế", "Không tìm thấy lệnh sản xuất để ghi nhận số lượng thực tế.", QMessageBox::Critical);
        return;
    }

    ERP::Manufacturing::DTO::ProductionOrderDTO currentOrder = *orderOpt;

    // Use a QInputDialog to get the actual quantity
    bool ok;
    double actualQuantity = QInputDialog::getDouble(this, "Ghi nhận Số lượng Thực tế",
                                                     "Số lượng thực tế đã sản xuất cho lệnh '" + QString::fromStdString(currentOrder.orderNumber) + "':",
                                                     currentOrder.actualQuantityProduced, 0.0, 999999999.0, 2, &ok);
    if (ok) {
        if (productionOrderService_->recordActualQuantityProduced(orderId.toStdString(), actualQuantity, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Ghi nhận SL thực tế", "Số lượng thực tế đã được ghi nhận thành công.", QMessageBox::Information);
            loadProductionOrders();
            clearForm();
        } else {
            showMessageBox("Lỗi", "Không thể ghi nhận số lượng thực tế. Vui lòng kiểm tra log.", QMessageBox::Critical);
        }
    }
}


void ProductionOrderManagementWidget::onSearchOrderClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["order_number_contains"] = searchText.toStdString(); // Assuming service supports "contains" filter
    }
    orderTable_->setRowCount(0);
    std::vector<ERP::Manufacturing::DTO::ProductionOrderDTO> orders = productionOrderService_->getAllProductionOrders(filter, currentUserId_, currentUserRoleIds_);

    orderTable_->setRowCount(orders.size());
    for (int i = 0; i < orders.size(); ++i) {
        const auto& order = orders[i];
        orderTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(order.id)));
        orderTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(order.orderNumber)));
        
        QString productName = "N/A";
        std::optional<ERP::Product::DTO::ProductDTO> product = productService_->getProductById(order.productId, currentUserId_, currentUserRoleIds_);
        if (product) productName = QString::fromStdString(product->name);
        orderTable_->setItem(i, 2, new QTableWidgetItem(productName));

        orderTable_->setItem(i, 3, new QTableWidgetItem(QString::number(order.plannedQuantity)));
        
        QString unitName = "N/A";
        std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> unit = securityManager_->getUnitOfMeasureService()->getUnitOfMeasureById(order.unitOfMeasureId, currentUserId_, currentUserRoleIds_);
        if (unit) unitName = QString::fromStdString(unit->name);
        orderTable_->setItem(i, 4, new QTableWidgetItem(unitName));

        orderTable_->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(order.getStatusString())));
        
        QString bomName = "N/A";
        if (order.bomId) {
            std::optional<ERP::Manufacturing::DTO::BillOfMaterialDTO> bom = bomService_->getBillOfMaterialById(*order.bomId, currentUserId_, currentUserRoleIds_);
            if (bom) bomName = QString::fromStdString(bom->bomName);
        }
        orderTable_->setItem(i, 6, new QTableWidgetItem(bomName));
        
        QString productionLineName = "N/A";
        if (order.productionLineId) {
            std::optional<ERP::Manufacturing::DTO::ProductionLineDTO> line = productionLineService_->getProductionLineById(*order.productionLineId, currentUserId_, currentUserRoleIds_);
            if (line) productionLineName = QString::fromStdString(line->lineName);
        }
        orderTable_->setItem(i, 7, new QTableWidgetItem(productionLineName));

        orderTable_->setItem(i, 8, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(order.plannedStartDate, ERP::Common::DATETIME_FORMAT))));
        orderTable_->setItem(i, 9, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(order.plannedEndDate, ERP::Common::DATETIME_FORMAT))));
    }
    orderTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ProductionOrderManagementWidget: Search completed.");
}

void ProductionOrderManagementWidget::onOrderTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString orderId = orderTable_->item(row, 0)->text();
    std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> orderOpt = productionOrderService_->getProductionOrderById(orderId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (orderOpt) {
        idLineEdit_->setText(QString::fromStdString(orderOpt->id));
        orderNumberLineEdit_->setText(QString::fromStdString(orderOpt->orderNumber));
        
        populateProductComboBox();
        int productIndex = productComboBox_->findData(QString::fromStdString(orderOpt->productId));
        if (productIndex != -1) productComboBox_->setCurrentIndex(productIndex);

        plannedQuantityLineEdit_->setText(QString::number(orderOpt->plannedQuantity));
        
        populateUnitOfMeasureComboBox();
        int unitIndex = unitOfMeasureComboBox_->findData(QString::fromStdString(orderOpt->unitOfMeasureId));
        if (unitIndex != -1) unitOfMeasureComboBox_->setCurrentIndex(unitIndex);

        populateStatusComboBox();
        int statusIndex = statusComboBox_->findData(static_cast<int>(orderOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);

        populateBOMComboBox();
        if (orderOpt->bomId) {
            int bomIndex = bomComboBox_->findData(QString::fromStdString(*orderOpt->bomId));
            if (bomIndex != -1) bomComboBox_->setCurrentIndex(bomIndex);
            else bomComboBox_->setCurrentIndex(0); // "None"
        } else {
            bomComboBox_->setCurrentIndex(0); // "None"
        }

        populateProductionLineComboBox();
        if (orderOpt->productionLineId) {
            int lineIndex = productionLineComboBox_->findData(QString::fromStdString(*orderOpt->productionLineId));
            if (lineIndex != -1) productionLineComboBox_->setCurrentIndex(lineIndex);
            else productionLineComboBox_->setCurrentIndex(0); // "None"
        } else {
            productionLineComboBox_->setCurrentIndex(0); // "None"
        }

        plannedStartDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(orderOpt->plannedStartDate.time_since_epoch()).count()));
        plannedEndDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(orderOpt->plannedEndDate.time_since_epoch()).count()));
        
        if (orderOpt->actualStartDate) actualStartDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(orderOpt->actualStartDate->time_since_epoch()).count()));
        else actualStartDateEdit_->clear();
        if (orderOpt->actualEndDate) actualEndDateEdit_->setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(orderOpt->actualEndDate->time_since_epoch()).count()));
        else actualEndDateEdit_->clear();
        
        actualQuantityProducedLineEdit_->setText(QString::number(orderOpt->actualQuantityProduced));
        notesLineEdit_->setText(QString::fromStdString(orderOpt->notes.value_or("")));

    } else {
        showMessageBox("Thông tin Lệnh Sản Xuất", "Không tìm thấy lệnh sản xuất đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void ProductionOrderManagementWidget::clearForm() {
    idLineEdit_->clear();
    orderNumberLineEdit_->clear();
    productComboBox_->clear();
    plannedQuantityLineEdit_->clear();
    unitOfMeasureComboBox_->clear();
    statusComboBox_->setCurrentIndex(0);
    bomComboBox_->clear();
    productionLineComboBox_->clear();
    plannedStartDateEdit_->clear();
    plannedEndDateEdit_->clear();
    actualStartDateEdit_->clear();
    actualEndDateEdit_->clear();
    actualQuantityProducedLineEdit_->clear();
    notesLineEdit_->clear();
    orderTable_->clearSelection();
    updateButtonsState();
}


void ProductionOrderManagementWidget::showOrderInputDialog(ERP::Manufacturing::DTO::ProductionOrderDTO* order) {
    QDialog dialog(this);
    dialog.setWindowTitle(order ? "Sửa Lệnh Sản Xuất" : "Thêm Lệnh Sản Xuất Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit orderNumberEdit(this);
    QComboBox productCombo(this); populateProductComboBox();
    for (int i = 0; i < productComboBox_->count(); ++i) productCombo.addItem(productComboBox_->itemText(i), productComboBox_->itemData(i));
    
    QLineEdit plannedQuantityEdit(this); plannedQuantityEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    QComboBox unitOfMeasureCombo(this); populateUnitOfMeasureComboBox();
    for (int i = 0; i < unitOfMeasureComboBox_->count(); ++i) unitOfMeasureCombo.addItem(unitOfMeasureComboBox_->itemText(i), unitOfMeasureComboBox_->itemData(i));
    
    QComboBox statusCombo(this); populateStatusComboBox();
    for (int i = 0; i < statusComboBox_->count(); ++i) statusCombo.addItem(statusComboBox_->itemText(i), statusComboBox_->itemData(i));

    QComboBox bomCombo(this); populateBOMComboBox();
    for (int i = 0; i < bomComboBox_->count(); ++i) bomCombo.addItem(bomComboBox_->itemText(i), bomComboBox_->itemData(i));

    QComboBox productionLineCombo(this); populateProductionLineComboBox();
    for (int i = 0; i < productionLineComboBox_->count(); ++i) productionLineCombo.addItem(productionLineComboBox_->itemText(i), productionLineComboBox_->itemData(i));

    QDateTimeEdit plannedStartDateEdit(this); plannedStartDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QDateTimeEdit plannedEndDateEdit(this); plannedEndDateEdit.setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    QLineEdit notesEdit(this);

    if (order) {
        orderNumberEdit.setText(QString::fromStdString(order->orderNumber));
        int productIndex = productCombo.findData(QString::fromStdString(order->productId));
        if (productIndex != -1) productCombo.setCurrentIndex(productIndex);
        plannedQuantityEdit.setText(QString::number(order->plannedQuantity));
        int unitIndex = unitOfMeasureCombo.findData(QString::fromStdString(order->unitOfMeasureId));
        if (unitIndex != -1) unitOfMeasureCombo.setCurrentIndex(unitIndex);
        int statusIndex = statusCombo.findData(static_cast<int>(order->status));
        if (statusIndex != -1) statusCombo.setCurrentIndex(statusIndex);
        if (order->bomId) {
            int bomIndex = bomCombo.findData(QString::fromStdString(*order->bomId));
            if (bomIndex != -1) bomCombo.setCurrentIndex(bomIndex);
            else bomCombo.setCurrentIndex(0);
        } else {
            bomCombo.setCurrentIndex(0);
        }
        if (order->productionLineId) {
            int lineIndex = productionLineCombo.findData(QString::fromStdString(*order->productionLineId));
            if (lineIndex != -1) productionLineCombo.setCurrentIndex(lineIndex);
            else productionLineCombo.setCurrentIndex(0);
        } else {
            productionLineCombo.setCurrentIndex(0);
        }
        plannedStartDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(order->plannedStartDate.time_since_epoch()).count()));
        plannedEndDateEdit.setDateTime(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(order->plannedEndDate.time_since_epoch()).count()));
        notesEdit.setText(QString::fromStdString(order->notes.value_or("")));

        orderNumberEdit.setReadOnly(true); // Don't allow changing order number for existing orders
    } else {
        plannedQuantityEdit.setText("0.0");
        plannedStartDateEdit.setDateTime(QDateTime::currentDateTime());
        plannedEndDateEdit.setDateTime(QDateTime::currentDateTime().addDays(7)); // Default 7 days from now
    }

    formLayout->addRow("Số Lệnh:*", &orderNumberEdit);
    formLayout->addRow("Sản phẩm:*", &productCombo);
    formLayout->addRow("SL kế hoạch:*", &plannedQuantityEdit);
    formLayout->addRow("Đơn vị:*", &unitOfMeasureCombo);
    formLayout->addRow("Trạng thái:*", &statusCombo);
    formLayout->addRow("BOM:", &bomCombo);
    formLayout->addRow("Dây chuyền:", &productionLineCombo);
    formLayout->addRow("Ngày bắt đầu KH:*", &plannedStartDateEdit);
    formLayout->addRow("Ngày kết thúc KH:*", &plannedEndDateEdit);
    formLayout->addRow("Ghi chú:", &notesEdit);

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(order ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Manufacturing::DTO::ProductionOrderDTO newOrderData;
        if (order) {
            newOrderData = *order;
        } else {
            newOrderData.id = ERP::Utils::generateUUID(); // New ID for new order
        }

        newOrderData.orderNumber = orderNumberEdit.text().toStdString();
        newOrderData.productId = productCombo.currentData().toString().toStdString();
        newOrderData.plannedQuantity = plannedQuantityEdit.text().toDouble();
        newOrderData.unitOfMeasureId = unitOfMeasureCombo.currentData().toString().toStdString();
        newOrderData.status = static_cast<ERP::Manufacturing::DTO::ProductionOrderStatus>(statusCombo.currentData().toInt());
        
        QString selectedBOMId = bomCombo.currentData().toString();
        newOrderData.bomId = selectedBOMId.isEmpty() ? std::nullopt : std::make_optional(selectedBOMId.toStdString());

        QString selectedProductionLineId = productionLineCombo.currentData().toString();
        newOrderData.productionLineId = selectedProductionLineId.isEmpty() ? std::nullopt : std::make_optional(selectedProductionLineId.toStdString());
        
        newOrderData.plannedStartDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(plannedStartDateEdit.dateTime());
        newOrderData.plannedEndDate = ERP::Utils::DateUtils::qDateTimeToTimePoint(plannedEndDateEdit.dateTime());
        newOrderData.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());

        bool success = false;
        if (order) {
            success = productionOrderService_->updateProductionOrder(newOrderData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Lệnh Sản Xuất", "Lệnh sản xuất đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật lệnh sản xuất. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Manufacturing::DTO::ProductionOrderDTO> createdOrder = productionOrderService_->createProductionOrder(newOrderData, currentUserId_, currentUserRoleIds_);
            if (createdOrder) {
                showMessageBox("Thêm Lệnh Sản Xuất", "Lệnh sản xuất mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm lệnh sản xuất mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadProductionOrders();
            clearForm();
        }
    }
}


void ProductionOrderManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool ProductionOrderManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void ProductionOrderManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Manufacturing.CreateProductionOrder");
    bool canUpdate = hasPermission("Manufacturing.UpdateProductionOrder");
    bool canDelete = hasPermission("Manufacturing.DeleteProductionOrder");
    bool canChangeStatus = hasPermission("Manufacturing.UpdateProductionOrderStatus");
    bool canRecordQuantity = hasPermission("Manufacturing.RecordActualQuantityProduced");

    addOrderButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Manufacturing.ViewProductionOrder"));

    bool isRowSelected = orderTable_->currentRow() >= 0;
    editOrderButton_->setEnabled(isRowSelected && canUpdate);
    deleteOrderButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);
    recordActualQuantityButton_->setEnabled(isRowSelected && canRecordQuantity);

    bool enableForm = isRowSelected && canUpdate;
    orderNumberLineEdit_->setEnabled(enableForm); // Will be read-only for existing anyway
    productComboBox_->setEnabled(enableForm);
    plannedQuantityLineEdit_->setEnabled(enableForm);
    unitOfMeasureComboBox_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    bomComboBox_->setEnabled(enableForm);
    productionLineComboBox_->setEnabled(enableForm);
    plannedStartDateEdit_->setEnabled(enableForm);
    plannedEndDateEdit_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);

    // Read-only fields
    idLineEdit_->setEnabled(false);
    actualStartDateEdit_->setEnabled(false);
    actualEndDateEdit_->setEnabled(false);
    actualQuantityProducedLineEdit_->setEnabled(false);

    if (!isRowSelected) {
        idLineEdit_->clear();
        orderNumberLineEdit_->clear();
        productComboBox_->clear();
        plannedQuantityLineEdit_->clear();
        unitOfMeasureComboBox_->clear();
        statusComboBox_->setCurrentIndex(0);
        bomComboBox_->clear();
        productionLineComboBox_->clear();
        plannedStartDateEdit_->clear();
        plannedEndDateEdit_->clear();
        actualStartDateEdit_->clear();
        actualEndDateEdit_->clear();
        actualQuantityProducedLineEdit_->clear();
        notesLineEdit_->clear();
    }
}


} // namespace Manufacturing
} // namespace UI
} // namespace ERP