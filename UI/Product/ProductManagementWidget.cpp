// UI/Product/ProductManagementWidget.cpp
#include "ProductManagementWidget.h" // Đã rút gọn include
#include "Product.h"           // Đã rút gọn include
#include "Category.h"          // Đã rút gọn include
#include "UnitOfMeasure.h"     // Đã rút gọn include
#include "ProductService.h"    // Đã rút gọn include
#include "CategoryService.h"   // Đã rút gọn include
#include "UnitOfMeasureService.h" // Đã rút gọn include
#include "ISecurityManager.h"  // Đã rút gọn include
#include "Logger.h"            // Đã rút gọn include
#include "ErrorHandler.h"      // Đã rút gọn include
#include "Common.h"            // Đã rút gọn include
#include "DateUtils.h"         // Đã rút gọn include
#include "StringUtils.h"       // Đã rút gọn include
#include "CustomMessageBox.h"  // Đã rút gọn include
#include "UserService.h"       // For getting current user roles from SecurityManager
#include "SupplierService.h"   // For Supplier validation (needed for Product)
#include "DTOUtils.h"          // For converting std::map<string,any> to/from QJson

#include <QInputDialog>
#include <QDoubleValidator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace ERP {
namespace UI {
namespace Product {

ProductManagementWidget::ProductManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::IProductService> productService,
    std::shared_ptr<Catalog::Services::ICategoryService> categoryService,
    std::shared_ptr<Catalog::Services::IUnitOfMeasureService> unitOfMeasureService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      productService_(productService),
      categoryService_(categoryService),
      unitOfMeasureService_(unitOfMeasureService),
      securityManager_(securityManager) {

    if (!productService_ || !categoryService_ || !unitOfMeasureService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ sản phẩm, danh mục, đơn vị đo hoặc bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("ProductManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("ProductManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("ProductManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadProducts();
    updateButtonsState();
}

ProductManagementWidget::~ProductManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void ProductManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên hoặc mã sản phẩm...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &ProductManagementWidget::onSearchProductClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    productTable_ = new QTableWidget(this);
    productTable_->setColumnCount(11); // ID, Tên, Mã SP, Danh mục, Đơn vị, Giá mua, Tiền tệ mua, Giá bán, Tiền tệ bán, Trạng thái, Ngày tạo
    productTable_->setHorizontalHeaderLabels({"ID", "Tên", "Mã SP", "Danh mục", "Đơn vị cơ sở", "Giá mua", "Tiền tệ mua", "Giá bán", "Tiền tệ bán", "Loại", "Trạng thái"});
    productTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    productTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    productTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    productTable_->horizontalHeader()->setStretchLastSection(true);
    connect(productTable_, &QTableWidget::itemClicked, this, &ProductManagementWidget::onProductTableItemClicked);
    mainLayout->addWidget(productTable_);

    // Form elements for editing/adding
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    nameLineEdit_ = new QLineEdit(this);
    productCodeLineEdit_ = new QLineEdit(this);
    categoryComboBox_ = new QComboBox(this);
    baseUnitOfMeasureComboBox_ = new QComboBox(this);
    descriptionLineEdit_ = new QLineEdit(this);
    purchasePriceLineEdit_ = new QLineEdit(this); purchasePriceLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    purchaseCurrencyLineEdit_ = new QLineEdit(this);
    salePriceLineEdit_ = new QLineEdit(this); salePriceLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    saleCurrencyLineEdit_ = new QLineEdit(this);
    imageUrlLineEdit_ = new QLineEdit(this);
    weightLineEdit_ = new QLineEdit(this); weightLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    weightUnitLineEdit_ = new QLineEdit(this);
    typeComboBox_ = new QComboBox(this);
    manufacturerLineEdit_ = new QLineEdit(this);
    supplierIdLineEdit_ = new QLineEdit(this); // Assuming this will be a searchable combo or dialog
    barcodeLineEdit_ = new QLineEdit(this);
    statusComboBox_ = new QComboBox(this);
    statusComboBox_->addItem("Active", static_cast<int>(ERP::Common::EntityStatus::ACTIVE));
    statusComboBox_->addItem("Inactive", static_cast<int>(ERP::Common::EntityStatus::INACTIVE));
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Common::EntityStatus::PENDING));
    statusComboBox_->addItem("Deleted", static_cast<int>(ERP::Common::EntityStatus::DELETED));
    isActiveCheckBox_ = new QCheckBox("Hoạt động", this);

    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Tên:*", this), 1, 0); formLayout->addWidget(nameLineEdit_, 1, 1);
    formLayout->addWidget(new QLabel("Mã SP:*", this), 2, 0); formLayout->addWidget(productCodeLineEdit_, 2, 1);
    formLayout->addWidget(new QLabel("Danh mục:*", this), 3, 0); formLayout->addWidget(categoryComboBox_, 3, 1);
    formLayout->addWidget(new QLabel("Đơn vị cơ sở:*", this), 4, 0); formLayout->addWidget(baseUnitOfMeasureComboBox_, 4, 1);
    formLayout->addWidget(new QLabel("Mô tả:", this), 5, 0); formLayout->addWidget(descriptionLineEdit_, 5, 1);
    formLayout->addWidget(new QLabel("Giá mua:", this), 6, 0); formLayout->addWidget(purchasePriceLineEdit_, 6, 1);
    formLayout->addWidget(new QLabel("Tiền tệ mua:", this), 7, 0); formLayout->addWidget(purchaseCurrencyLineEdit_, 7, 1);
    formLayout->addWidget(new QLabel("Giá bán:", this), 8, 0); formLayout->addWidget(salePriceLineEdit_, 8, 1);
    formLayout->addWidget(new QLabel("Tiền tệ bán:", this), 9, 0); formLayout->addWidget(saleCurrencyLineEdit_, 9, 1);
    formLayout->addWidget(new QLabel("URL Hình ảnh:", this), 10, 0); formLayout->addWidget(imageUrlLineEdit_, 10, 1);
    formLayout->addWidget(new QLabel("Cân nặng:", this), 11, 0); formLayout->addWidget(weightLineEdit_, 11, 1);
    formLayout->addWidget(new QLabel("Đơn vị cân nặng:", this), 12, 0); formLayout->addWidget(weightUnitLineEdit_, 12, 1);
    formLayout->addWidget(new QLabel("Loại SP:", this), 13, 0); formLayout->addWidget(typeComboBox_, 13, 1);
    formLayout->addWidget(new QLabel("Nhà sản xuất:", this), 14, 0); formLayout->addWidget(manufacturerLineEdit_, 14, 1);
    formLayout->addWidget(new QLabel("ID NCC:", this), 15, 0); formLayout->addWidget(supplierIdLineEdit_, 15, 1);
    formLayout->addWidget(new QLabel("Mã vạch:", this), 16, 0); formLayout->addWidget(barcodeLineEdit_, 16, 1);
    formLayout->addWidget(new QLabel("Trạng thái:", this), 17, 0); formLayout->addWidget(statusComboBox_, 17, 1);
    formLayout->addWidget(isActiveCheckBox_, 18, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addCategoryButton_ = new QPushButton("Thêm mới", this); connect(addCategoryButton_, &QPushButton::clicked, this, &ProductManagementWidget::onAddProductClicked);
    editProductButton_ = new QPushButton("Sửa", this); connect(editProductButton_, &QPushButton::clicked, this, &ProductManagementWidget::onEditProductClicked);
    deleteProductButton_ = new QPushButton("Xóa", this); connect(deleteProductButton_, &QPushButton::clicked, this, &ProductManagementWidget::onDeleteProductClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &ProductManagementWidget::onUpdateProductStatusClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &ProductManagementWidget::clearForm);
    buttonLayout->addWidget(addCategoryButton_);
    buttonLayout->addWidget(editProductButton_);
    buttonLayout->addWidget(deleteProductButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void ProductManagementWidget::loadProducts() {
    ERP::Logger::Logger::getInstance().info("ProductManagementWidget: Loading products...");
    productTable_->setRowCount(0);

    std::vector<ERP::Product::DTO::ProductDTO> products = productService_->getAllProducts({}, currentUserId_, currentUserRoleIds_);

    productTable_->setRowCount(products.size());
    for (int i = 0; i < products.size(); ++i) {
        const auto& product = products[i];
        productTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(product.id)));
        productTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(product.name)));
        productTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(product.productCode)));
        
        // Resolve Category Name
        QString categoryName = "N/A";
        std::optional<ERP::Catalog::DTO::CategoryDTO> category = categoryService_->getCategoryById(product.categoryId, currentUserId_, currentUserRoleIds_);
        if (category) categoryName = QString::fromStdString(category->name);
        productTable_->setItem(i, 3, new QTableWidgetItem(categoryName));

        // Resolve Base Unit of Measure Name
        QString uomName = "N/A";
        std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> uom = unitOfMeasureService_->getUnitOfMeasureById(product.baseUnitOfMeasureId, currentUserId_, currentUserRoleIds_);
        if (uom) uomName = QString::fromStdString(uom->name);
        productTable_->setItem(i, 4, new QTableWidgetItem(uomName));

        productTable_->setItem(i, 5, new QTableWidgetItem(QString::number(product.purchasePrice.value_or(0.0), 'f', 2)));
        productTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(product.purchaseCurrency.value_or(""))));
        productTable_->setItem(i, 7, new QTableWidgetItem(QString::number(product.salePrice.value_or(0.0), 'f', 2)));
        productTable_->setItem(i, 8, new QTableWidgetItem(QString::fromStdString(product.saleCurrency.value_or(""))));
        productTable_->setItem(i, 9, new QTableWidgetItem(QString::fromStdString(product.getTypeString()))); // ProductType string
        productTable_->setItem(i, 10, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(product.status))));
    }
    productTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ProductManagementWidget: Products loaded successfully.");
}

void ProductManagementWidget::populateCategoryComboBox() {
    categoryComboBox_->clear();
    std::vector<ERP::Catalog::DTO::CategoryDTO> allCategories = categoryService_->getAllCategories({}, currentUserId_, currentUserRoleIds_);
    for (const auto& category : allCategories) {
        categoryComboBox_->addItem(QString::fromStdString(category.name), QString::fromStdString(category.id));
    }
}

void ProductManagementWidget::populateUnitOfMeasureComboBox() {
    baseUnitOfMeasureComboBox_->clear();
    std::vector<ERP::Catalog::DTO::UnitOfMeasureDTO> allUoMs = unitOfMeasureService_->getAllUnitsOfMeasure({}, currentUserId_, currentUserRoleIds_);
    for (const auto& uom : allUoMs) {
        baseUnitOfMeasureComboBox_->addItem(QString::fromStdString(uom.name), QString::fromStdString(uom.id));
    }
}

void ProductManagementWidget::populateTypeComboBox() {
    typeComboBox_->clear();
    typeComboBox_->addItem("Finished Good", static_cast<int>(ERP::Product::DTO::ProductType::FINISHED_GOOD));
    typeComboBox_->addItem("Raw Material", static_cast<int>(ERP::Product::DTO::ProductType::RAW_MATERIAL));
    typeComboBox_->addItem("Work-in-Process", static_cast<int>(ERP::Product::DTO::ProductType::WORK_IN_PROCESS));
    typeComboBox_->addItem("Service", static_cast<int>(ERP::Product::DTO::ProductType::SERVICE));
    typeComboBox_->addItem("Assembly", static_cast<int>(ERP::Product::DTO::ProductType::ASSEMBLY));
    typeComboBox_->addItem("Kit", static_cast<int>(ERP::Product::DTO::ProductType::KIT));
}

void ProductManagementWidget::onAddProductClicked() {
    if (!hasPermission("Product.CreateProduct")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm sản phẩm.", QMessageBox::Warning);
        return;
    }
    clearForm();
    populateCategoryComboBox();
    populateUnitOfMeasureComboBox();
    populateTypeComboBox();
    showProductInputDialog();
}

void ProductManagementWidget::onEditProductClicked() {
    if (!hasPermission("Product.UpdateProduct")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa sản phẩm.", QMessageBox::Warning);
        return;
    }

    int selectedRow = productTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Sản Phẩm", "Vui lòng chọn một sản phẩm để sửa.", QMessageBox::Information);
        return;
    }

    QString productId = productTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Product::DTO::ProductDTO> productOpt = productService_->getProductById(productId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (productOpt) {
        populateCategoryComboBox();
        populateUnitOfMeasureComboBox();
        populateTypeComboBox();
        showProductInputDialog(&(*productOpt));
    } else {
        showMessageBox("Sửa Sản Phẩm", "Không tìm thấy sản phẩm để sửa.", QMessageBox::Critical);
    }
}

void ProductManagementWidget::onDeleteProductClicked() {
    if (!hasPermission("Product.DeleteProduct")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa sản phẩm.", QMessageBox::Warning);
        return;
    }

    int selectedRow = productTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Sản Phẩm", "Vui lòng chọn một sản phẩm để xóa.", QMessageBox::Information);
        return;
    }

    QString productId = productTable_->item(selectedRow, 0)->text();
    QString productName = productTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Sản Phẩm");
    confirmBox.setText("Bạn có chắc chắn muốn xóa sản phẩm '" + productName + "' (ID: " + productId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (productService_->deleteProduct(productId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Sản Phẩm", "Sản phẩm đã được xóa thành công.", QMessageBox::Information);
            loadProducts();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa sản phẩm. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void ProductManagementWidget::onUpdateProductStatusClicked() {
    if (!hasPermission("Product.UpdateProductStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái sản phẩm.", QMessageBox::Warning);
        return;
    }

    int selectedRow = productTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một sản phẩm để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString productId = productTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Product::DTO::ProductDTO> productOpt = productService_->getProductById(productId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!productOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy sản phẩm để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Product::DTO::ProductDTO currentProduct = *productOpt;
    ERP::Common::EntityStatus newStatus = (currentProduct.status == ERP::Common::EntityStatus::ACTIVE) ?
                                          ERP::Common::EntityStatus::INACTIVE :
                                          ERP::Common::EntityStatus::ACTIVE;

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Cập nhật trạng thái sản phẩm");
    confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái sản phẩm '" + QString::fromStdString(currentProduct.name) + "' thành " + QString::fromStdString(ERP::Common::entityStatusToString(newStatus)) + "?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (productService_->updateProductStatus(productId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Cập nhật trạng thái", "Trạng thái sản phẩm đã được cập nhật thành công.", QMessageBox::Information);
            loadProducts();
            clearForm();
        } else {
            showMessageBox("Lỗi", "Không thể cập nhật trạng thái sản phẩm. Vui lòng kiểm tra log.", QMessageBox::Critical);
        }
    }
}


void ProductManagementWidget::onSearchProductClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["name_or_code_contains"] = searchText.toStdString(); // Assuming service supports this
    }
    productTable_->setRowCount(0);
    std::vector<ERP::Product::DTO::ProductDTO> products = productService_->getAllProducts(filter, currentUserId_, currentUserRoleIds_);

    productTable_->setRowCount(products.size());
    for (int i = 0; i < products.size(); ++i) {
        const auto& product = products[i];
        productTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(product.id)));
        productTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(product.name)));
        productTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(product.productCode)));
        
        QString categoryName = "N/A";
        std::optional<ERP::Catalog::DTO::CategoryDTO> category = categoryService_->getCategoryById(product.categoryId, currentUserId_, currentUserRoleIds_);
        if (category) categoryName = QString::fromStdString(category->name);
        productTable_->setItem(i, 3, new QTableWidgetItem(categoryName));

        QString uomName = "N/A";
        std::optional<ERP::Catalog::DTO::UnitOfMeasureDTO> uom = unitOfMeasureService_->getUnitOfMeasureById(product.baseUnitOfMeasureId, currentUserId_, currentUserRoleIds_);
        if (uom) uomName = QString::fromStdString(uom->name);
        productTable_->setItem(i, 4, new QTableWidgetItem(uomName));

        productTable_->setItem(i, 5, new QTableWidgetItem(QString::number(product.purchasePrice.value_or(0.0), 'f', 2)));
        productTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(product.purchaseCurrency.value_or(""))));
        productTable_->setItem(i, 7, new QTableWidgetItem(QString::number(product.salePrice.value_or(0.0), 'f', 2)));
        productTable_->setItem(i, 8, new QTableWidgetItem(QString::fromStdString(product.saleCurrency.value_or(""))));
        productTable_->setItem(i, 9, new QTableWidgetItem(QString::fromStdString(product.getTypeString())));
        productTable_->setItem(i, 10, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(product.status))));
    }
    productTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("ProductManagementWidget: Search completed.");
}

void ProductManagementWidget::onProductTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString productId = productTable_->item(row, 0)->text();
    std::optional<ERP::Product::DTO::ProductDTO> productOpt = productService_->getProductById(productId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (productOpt) {
        idLineEdit_->setText(QString::fromStdString(productOpt->id));
        nameLineEdit_->setText(QString::fromStdString(productOpt->name));
        productCodeLineEdit_->setText(QString::fromStdString(productOpt->productCode));
        
        populateCategoryComboBox();
        int categoryIndex = categoryComboBox_->findData(QString::fromStdString(productOpt->categoryId));
        if (categoryIndex != -1) categoryComboBox_->setCurrentIndex(categoryIndex);

        populateUnitOfMeasureComboBox();
        int uomIndex = baseUnitOfMeasureComboBox_->findData(QString::fromStdString(productOpt->baseUnitOfMeasureId));
        if (uomIndex != -1) baseUnitOfMeasureComboBox_->setCurrentIndex(uomIndex);

        descriptionLineEdit_->setText(QString::fromStdString(productOpt->description.value_or("")));
        purchasePriceLineEdit_->setText(QString::number(productOpt->purchasePrice.value_or(0.0), 'f', 2));
        purchaseCurrencyLineEdit_->setText(QString::fromStdString(productOpt->purchaseCurrency.value_or("")));
        salePriceLineEdit_->setText(QString::number(productOpt->salePrice.value_or(0.0), 'f', 2));
        saleCurrencyLineEdit_->setText(QString::fromStdString(productOpt->saleCurrency.value_or("")));
        imageUrlLineEdit_->setText(QString::fromStdString(productOpt->imageUrl.value_or("")));
        weightLineEdit_->setText(QString::number(productOpt->weight.value_or(0.0), 'f', 2));
        weightUnitLineEdit_->setText(QString::fromStdString(productOpt->weightUnit.value_or("")));

        populateTypeComboBox();
        int typeIndex = typeComboBox_->findData(static_cast<int>(productOpt->type));
        if (typeIndex != -1) typeComboBox_->setCurrentIndex(typeIndex);
        
        manufacturerLineEdit_->setText(QString::fromStdString(productOpt->manufacturer.value_or("")));
        supplierIdLineEdit_->setText(QString::fromStdString(productOpt->supplierId.value_or("")));
        barcodeLineEdit_->setText(QString::fromStdString(productOpt->barcode.value_or("")));

        int statusIndex = statusComboBox_->findData(static_cast<int>(productOpt->status));
        if (statusIndex != -1) statusComboBox_->setCurrentIndex(statusIndex);
        isActiveCheckBox_->setChecked(productOpt->status == ERP::Common::EntityStatus::ACTIVE);

    } else {
        showMessageBox("Thông tin Sản Phẩm", "Không thể tải chi tiết sản phẩm đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void ProductManagementWidget::clearForm() {
    idLineEdit_->clear();
    nameLineEdit_->clear();
    productCodeLineEdit_->clear();
    categoryComboBox_->clear(); // Will be repopulated when needed
    baseUnitOfMeasureComboBox_->clear(); // Will be repopulated when needed
    descriptionLineEdit_->clear();
    purchasePriceLineEdit_->clear();
    purchaseCurrencyLineEdit_->clear();
    salePriceLineEdit_->clear();
    saleCurrencyLineEdit_->clear();
    imageUrlLineEdit_->clear();
    weightLineEdit_->clear();
    weightUnitLineEdit_->clear();
    typeComboBox_->clear();
    manufacturerLineEdit_->clear();
    supplierIdLineEdit_->clear();
    barcodeLineEdit_->clear();
    statusComboBox_->setCurrentIndex(0); // Default to Active
    isActiveCheckBox_->setChecked(true); // Default to active
    productTable_->clearSelection();
    updateButtonsState();
}


void ProductManagementWidget::showProductInputDialog(ERP::Product::DTO::ProductDTO* product) {
    QDialog dialog(this);
    dialog.setWindowTitle(product ? "Sửa Sản Phẩm" : "Thêm Sản Phẩm Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit nameEdit(this);
    QLineEdit productCodeEdit(this);
    QComboBox categoryCombo(this); populateCategoryComboBox();
    for (int i = 0; i < categoryComboBox_->count(); ++i) categoryCombo.addItem(categoryComboBox_->itemText(i), categoryComboBox_->itemData(i));

    QComboBox baseUnitOfMeasureCombo(this); populateUnitOfMeasureComboBox();
    for (int i = 0; i < baseUnitOfMeasureComboBox_->count(); ++i) baseUnitOfMeasureCombo.addItem(baseUnitOfMeasureComboBox_->itemText(i), baseUnitOfMeasureComboBox_->itemData(i));
    
    QLineEdit descriptionEdit(this);
    QLineEdit purchasePriceEdit(this); purchasePriceEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    QLineEdit purchaseCurrencyEdit(this);
    QLineEdit salePriceEdit(this); salePriceEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    QLineEdit saleCurrencyEdit(this);
    QLineEdit imageUrlEdit(this);
    QLineEdit weightEdit(this); weightEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    QLineEdit weightUnitEdit(this);
    QComboBox typeCombo(this); populateTypeComboBox();
    for (int i = 0; i < typeComboBox_->count(); ++i) typeCombo.addItem(typeComboBox_->itemText(i), typeComboBox_->itemData(i));
    
    QLineEdit manufacturerEdit(this);
    QLineEdit supplierIdEdit(this);
    QLineEdit barcodeEdit(this);
    QCheckBox isActiveCheck("Hoạt động", this);

    if (product) {
        nameEdit.setText(QString::fromStdString(product->name));
        productCodeEdit.setText(QString::fromStdString(product->productCode));
        
        int categoryIndex = categoryCombo.findData(QString::fromStdString(product->categoryId));
        if (categoryIndex != -1) categoryCombo.setCurrentIndex(categoryIndex);

        int uomIndex = baseUnitOfMeasureCombo.findData(QString::fromStdString(product->baseUnitOfMeasureId));
        if (uomIndex != -1) baseUnitOfMeasureCombo.setCurrentIndex(uomIndex);

        descriptionEdit.setText(QString::fromStdString(product->description.value_or("")));
        purchasePriceEdit.setText(QString::number(product->purchasePrice.value_or(0.0), 'f', 2));
        purchaseCurrencyEdit.setText(QString::fromStdString(product->purchaseCurrency.value_or("")));
        salePriceEdit.setText(QString::number(product->salePrice.value_or(0.0), 'f', 2));
        saleCurrencyEdit.setText(QString::fromStdString(product->saleCurrency.value_or("")));
        imageUrlEdit.setText(QString::fromStdString(product->imageUrl.value_or("")));
        weightEdit.setText(QString::number(product->weight.value_or(0.0), 'f', 2));
        weightUnitEdit.setText(QString::fromStdString(product->weightUnit.value_or("")));

        int typeIndex = typeCombo.findData(static_cast<int>(product->type));
        if (typeIndex != -1) typeCombo.setCurrentIndex(typeIndex);
        
        manufacturerEdit.setText(QString::fromStdString(product->manufacturer.value_or("")));
        supplierIdEdit.setText(QString::fromStdString(product->supplierId.value_or("")));
        barcodeEdit.setText(QString::fromStdString(product->barcode.value_or("")));
        isActiveCheck.setChecked(product->status == ERP::Common::EntityStatus::ACTIVE);
    } else {
        purchasePriceEdit.setText("0.00"); salePriceEdit.setText("0.00"); weightEdit.setText("0.00");
        isActiveCheck.setChecked(true);
    }

    formLayout->addRow("Tên:*", &nameEdit);
    formLayout->addRow("Mã SP:*", &productCodeEdit);
    formLayout->addRow("Danh mục:*", &categoryCombo);
    formLayout->addRow("Đơn vị cơ sở:*", &baseUnitOfMeasureCombo);
    formLayout->addRow("Mô tả:", &descriptionEdit);
    formLayout->addRow("Giá mua:", &purchasePriceEdit);
    formLayout->addRow("Tiền tệ mua:", &purchaseCurrencyEdit);
    formLayout->addRow("Giá bán:", &salePriceEdit);
    formLayout->addRow("Tiền tệ bán:", &saleCurrencyEdit);
    formLayout->addRow("URL Hình ảnh:", &imageUrlEdit);
    formLayout->addRow("Cân nặng:", &weightEdit);
    formLayout->addRow("Đơn vị cân nặng:", &weightUnitEdit);
    formLayout->addRow("Loại SP:", &typeCombo);
    formLayout->addRow("Nhà sản xuất:", &manufacturerEdit);
    formLayout->addRow("ID NCC:", &supplierIdEdit);
    formLayout->addRow("Mã vạch:", &barcodeEdit);
    formLayout->addRow("", &isActiveCheck);

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(product ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Product::DTO::ProductDTO newProductData;
        if (product) {
            newProductData = *product;
        }

        newProductData.name = nameEdit.text().toStdString();
        newProductData.productCode = productCodeEdit.text().toStdString();
        newProductData.categoryId = categoryCombo.currentData().toString().toStdString();
        newProductData.baseUnitOfMeasureId = baseUnitOfMeasureCombo.currentData().toString().toStdString();
        newProductData.description = descriptionEdit.text().isEmpty() ? std::nullopt : std::make_optional(descriptionEdit.text().toStdString());
        newProductData.purchasePrice = purchasePriceEdit.text().isEmpty() ? std::nullopt : std::make_optional(purchasePriceEdit.text().toDouble());
        newProductData.purchaseCurrency = purchaseCurrencyEdit.text().isEmpty() ? std::nullopt : std::make_optional(purchaseCurrencyEdit.text().toStdString());
        newProductData.salePrice = salePriceEdit.text().isEmpty() ? std::nullopt : std::make_optional(salePriceEdit.text().toDouble());
        newProductData.saleCurrency = saleCurrencyEdit.text().isEmpty() ? std::nullopt : std::make_optional(saleCurrencyEdit.text().toStdString());
        newProductData.imageUrl = imageUrlEdit.text().isEmpty() ? std::nullopt : std::make_optional(imageUrlEdit.text().toStdString());
        newProductData.weight = weightEdit.text().isEmpty() ? std::nullopt : std::make_optional(weightEdit.text().toDouble());
        newProductData.weightUnit = weightUnitEdit.text().isEmpty() ? std::nullopt : std::make_optional(weightUnitEdit.text().toStdString());
        newProductData.type = static_cast<ERP::Product::DTO::ProductType>(typeCombo.currentData().toInt());
        newProductData.manufacturer = manufacturerEdit.text().isEmpty() ? std::nullopt : std::make_optional(manufacturerEdit.text().toStdString());
        newProductData.supplierId = supplierIdEdit.text().isEmpty() ? std::nullopt : std::make_optional(supplierIdEdit.text().toStdString());
        newProductData.barcode = barcodeEdit.text().isEmpty() ? std::nullopt : std::make_optional(barcodeEdit.text().toStdString());
        
        newProductData.status = isActiveCheck.isChecked() ? ERP::Common::EntityStatus::ACTIVE : ERP::Common::EntityStatus::INACTIVE;

        bool success = false;
        if (product) {
            success = productService_->updateProduct(newProductData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Sản Phẩm", "Sản phẩm đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật sản phẩm. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Product::DTO::ProductDTO> createdProduct = productService_->createProduct(newProductData, currentUserId_, currentUserRoleIds_);
            if (createdProduct) {
                showMessageBox("Thêm Sản Phẩm", "Sản phẩm mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm sản phẩm mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadProducts();
            clearForm();
        }
    }
}


void ProductManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool ProductManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void ProductManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Product.CreateProduct");
    bool canUpdate = hasPermission("Product.UpdateProduct");
    bool canDelete = hasPermission("Product.DeleteProduct");
    bool canChangeStatus = hasPermission("Product.UpdateProductStatus");

    addProductButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Product.ViewProducts"));

    bool isRowSelected = productTable_->currentRow() >= 0;
    editProductButton_->setEnabled(isRowSelected && canUpdate);
    deleteProductButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);

    bool enableForm = isRowSelected && canUpdate;
    nameLineEdit_->setEnabled(enableForm);
    productCodeLineEdit_->setEnabled(enableForm);
    categoryComboBox_->setEnabled(enableForm);
    baseUnitOfMeasureComboBox_->setEnabled(enableForm);
    descriptionLineEdit_->setEnabled(enableForm);
    purchasePriceLineEdit_->setEnabled(enableForm);
    purchaseCurrencyLineEdit_->setEnabled(enableForm);
    salePriceLineEdit_->setEnabled(enableForm);
    saleCurrencyLineEdit_->setEnabled(enableForm);
    imageUrlLineEdit_->setEnabled(enableForm);
    weightLineEdit_->setEnabled(enableForm);
    weightUnitLineEdit_->setEnabled(enableForm);
    typeComboBox_->setEnabled(enableForm);
    manufacturerLineEdit_->setEnabled(enableForm);
    supplierIdLineEdit_->setEnabled(enableForm);
    barcodeLineEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);
    isActiveCheckBox_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear(); // Clear ID field if no row selected
        // Other fields will be cleared by clearForm or reset by populate methods on dialog
    }
}

} // namespace Product
} // namespace UI
} // namespace ERP