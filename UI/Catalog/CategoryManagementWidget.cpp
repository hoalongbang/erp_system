// UI/Catalog/CategoryManagementWidget.cpp
#include "CategoryManagementWidget.h" // Standard includes
#include "Category.h"          // Category DTO
#include "CategoryService.h"   // Category Service
#include "ISecurityManager.h"  // Security Manager
#include "Logger.h"            // Logging
#include "ErrorHandler.h"      // Error Handling
#include "Common.h"            // Common Enums/Constants
#include "DateUtils.h"         // Date Utilities
#include "StringUtils.h"       // String Utilities
#include "CustomMessageBox.h"  // Custom Message Box
#include "UserService.h"       // For getting current user roles from SecurityManager

#include <QInputDialog> // For getting input from user

namespace ERP {
    namespace UI {
        namespace Catalog {

            CategoryManagementWidget::CategoryManagementWidget(
                QWidget* parent,
                std::shared_ptr<Services::ICategoryService> categoryService,
                std::shared_ptr<Security::ISecurityManager> securityManager)
                : QWidget(parent),
                categoryService_(categoryService),
                securityManager_(securityManager) {

                if (!categoryService_ || !securityManager_) {
                    showMessageBox("Lỗi Khởi Tạo", "Dịch vụ danh mục hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
                    ERP::Logger::Logger::getInstance().critical("CategoryManagementWidget: Initialized with null dependencies.");
                    return;
                }

                // Attempt to get current user ID and roles from securityManager
                // This is a simplified approach; in a real app, user context would be passed securely.
                auto authService = securityManager_->getAuthenticationService();
                if (authService) {
                    std::string dummySessionId = "current_session_id"; // Placeholder
                    std::optional<ERP::Security::DTO::SessionDTO> currentSession = authService->validateSession(dummySessionId);
                    if (currentSession) {
                        currentUserId_ = currentSession->userId;
                        currentUserRoleIds_ = securityManager_->getUserService()->getUserRoles(currentUserId_, {});
                    }
                    else {
                        currentUserId_ = "system_user"; // Fallback for non-logged-in operations or system actions
                        currentUserRoleIds_ = { "anonymous" }; // Fallback
                        ERP::Logger::Logger::getInstance().warning("CategoryManagementWidget: No active session found. Running with limited privileges.");
                    }
                }
                else {
                    currentUserId_ = "system_user";
                    currentUserRoleIds_ = { "anonymous" };
                    ERP::Logger::Logger::getInstance().warning("CategoryManagementWidget: Authentication Service not available. Running with limited privileges.");
                }


                setupUI();
                loadCategories();
                updateButtonsState(); // Set initial button states
            }

            CategoryManagementWidget::~CategoryManagementWidget() {
                // Layout and widgets are children of this, so they are deleted automatically
            }

            void CategoryManagementWidget::setupUI() {
                QVBoxLayout* mainLayout = new QVBoxLayout(this);

                // Search and filter
                QHBoxLayout* searchLayout = new QHBoxLayout();
                searchLineEdit_ = new QLineEdit(this);
                searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên danh mục...");
                searchButton_ = new QPushButton("Tìm kiếm", this);
                connect(searchButton_, &QPushButton::clicked, this, &CategoryManagementWidget::onSearchCategoryClicked);
                searchLayout->addWidget(searchLineEdit_);
                searchLayout->addWidget(searchButton_);
                mainLayout->addLayout(searchLayout);

                // Table
                categoryTable_ = new QTableWidget(this);
                categoryTable_->setColumnCount(8); // ID, Tên, Mô tả, Danh mục cha, Trạng thái, Sort Order, Is Active, Created At
                categoryTable_->setHorizontalHeaderLabels({ "ID", "Tên", "Mô tả", "Danh mục cha", "Trạng thái", "Thứ tự sắp xếp", "Hoạt động", "Ngày tạo" });
                categoryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
                categoryTable_->setSelectionMode(QAbstractItemView::SingleSelection);
                categoryTable_->setEditTriggers(QAbstractItemView::NoEditTriggers); // Make table read-only
                categoryTable_->horizontalHeader()->setStretchLastSection(true); // Stretch last column
                connect(categoryTable_, &QTableWidget::itemClicked, this, &CategoryManagementWidget::onCategoryTableItemClicked);
                mainLayout->addWidget(categoryTable_);

                // Form for Add/Edit
                QGridLayout* formLayout = new QGridLayout();
                idLineEdit_ = new QLineEdit(this);
                idLineEdit_->setReadOnly(true); // ID is read-only
                nameLineEdit_ = new QLineEdit(this);
                descriptionLineEdit_ = new QLineEdit(this);
                parentCategoryComboBox_ = new QComboBox(this);
                statusComboBox_ = new QComboBox(this);
                statusComboBox_->addItem("Active", static_cast<int>(ERP::Common::EntityStatus::ACTIVE));
                statusComboBox_->addItem("Inactive", static_cast<int>(ERP::Common::EntityStatus::INACTIVE));
                statusComboBox_->addItem("Pending", static_cast<int>(ERP::Common::EntityStatus::PENDING));
                statusComboBox_->addItem("Deleted", static_cast<int>(ERP::Common::EntityStatus::DELETED));
                sortOrderLineEdit_ = new QLineEdit(this);
                sortOrderLineEdit_->setValidator(new QIntValidator(0, 99999, this)); // Only allow integers
                isActiveCheckBox_ = new QCheckBox("Hoạt động", this);

                formLayout->addWidget(new QLabel("ID:", this), 0, 0);
                formLayout->addWidget(idLineEdit_, 0, 1);
                formLayout->addWidget(new QLabel("Tên:", this), 1, 0);
                formLayout->addWidget(nameLineEdit_, 1, 1);
                formLayout->addWidget(new QLabel("Mô tả:", this), 2, 0);
                formLayout->addWidget(descriptionLineEdit_, 2, 1);
                formLayout->addWidget(new QLabel("Danh mục cha:", this), 3, 0);
                formLayout->addWidget(parentCategoryComboBox_, 3, 1);
                formLayout->addWidget(new QLabel("Trạng thái:", this), 4, 0);
                formLayout->addWidget(statusComboBox_, 4, 1);
                formLayout->addWidget(new QLabel("Thứ tự sắp xếp:", this), 5, 0);
                formLayout->addWidget(sortOrderLineEdit_, 5, 1);
                formLayout->addWidget(isActiveCheckBox_, 6, 1);

                mainLayout->addLayout(formLayout);

                // Buttons
                QHBoxLayout* buttonLayout = new QHBoxLayout();
                addCategoryButton_ = new QPushButton("Thêm mới", this);
                editCategoryButton_ = new QPushButton("Sửa", this);
                deleteCategoryButton_ = new QPushButton("Xóa", this);
                updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this);
                clearFormButton_ = new QPushButton("Xóa Form", this);

                connect(addCategoryButton_, &QPushButton::clicked, this, &CategoryManagementWidget::onAddCategoryClicked);
                connect(editCategoryButton_, &QPushButton::clicked, this, &CategoryManagementWidget::onEditCategoryClicked);
                connect(deleteCategoryButton_, &QPushButton::clicked, this, &CategoryManagementWidget::onDeleteCategoryClicked);
                connect(updateStatusButton_, &QPushButton::clicked, this, &CategoryManagementWidget::onUpdateCategoryStatusClicked);
                connect(clearFormButton_, &QPushButton::clicked, this, &CategoryManagementWidget::clearForm);

                buttonLayout->addWidget(addCategoryButton_);
                buttonLayout->addWidget(editCategoryButton_);
                buttonLayout->addWidget(deleteCategoryButton_);
                buttonLayout->addWidget(updateStatusButton_);
                buttonLayout->addWidget(clearFormButton_);
                mainLayout->addLayout(buttonLayout);
            }

            void CategoryManagementWidget::loadCategories() {
                ERP::Logger::Logger::getInstance().info("CategoryManagementWidget: Loading categories...");
                categoryTable_->setRowCount(0); // Clear existing rows

                // Pass current user and roles for permission checks within the service layer
                std::vector<ERP::Catalog::DTO::CategoryDTO> categories = categoryService_->getAllCategories({}, currentUserId_, currentUserRoleIds_);

                categoryTable_->setRowCount(categories.size());
                for (int i = 0; i < categories.size(); ++i) {
                    const auto& category = categories[i];
                    categoryTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(category.id)));
                    categoryTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(category.name)));
                    categoryTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(category.description.value_or(""))));

                    // Resolve parent category name
                    QString parentName = "N/A";
                    if (category.parentCategoryId) {
                        std::optional<ERP::Catalog::DTO::CategoryDTO> parentCategory = categoryService_->getCategoryById(*category.parentCategoryId, currentUserId_, currentUserRoleIds_);
                        if (parentCategory) {
                            parentName = QString::fromStdString(parentCategory->name);
                        }
                    }
                    categoryTable_->setItem(i, 3, new QTableWidgetItem(parentName));

                    categoryTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(category.status))));
                    categoryTable_->setItem(i, 5, new QTableWidgetItem(QString::number(category.sortOrder))); // Sort Order
                    categoryTable_->setItem(i, 6, new QTableWidgetItem(category.isActive ? "Yes" : "No")); // Is Active
                    categoryTable_->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(category.createdAt, ERP::Common::DATETIME_FORMAT))));
                }
                categoryTable_->resizeColumnsToContents();
                ERP::Logger::Logger::getInstance().info("CategoryManagementWidget: Categories loaded successfully.");
            }

            void CategoryManagementWidget::populateParentCategoryComboBox(QComboBox* comboBox) {
                comboBox->clear();
                comboBox->addItem("None", ""); // Option for no parent
                std::vector<ERP::Catalog::DTO::CategoryDTO> allCategories = categoryService_->getAllCategories({}, currentUserId_, currentUserRoleIds_);
                for (const auto& category : allCategories) {
                    // Exclude the category itself from its parent options when editing
                    QString currentCategoryId = idLineEdit_->text(); // Get ID from form
                    if (!currentCategoryId.isEmpty() && category.id == currentCategoryId.toStdString()) continue;

                    comboBox->addItem(QString::fromStdString(category.name), QString::fromStdString(category.id));
                }
            }

            void CategoryManagementWidget::onAddCategoryClicked() {
                if (!hasPermission("Catalog.CreateCategory")) {
                    showMessageBox("Lỗi", "Bạn không có quyền thêm danh mục.", QMessageBox::Warning);
                    return;
                }
                clearForm();
                // Populate combo box for the input dialog
                populateParentCategoryComboBox(parentCategoryComboBox_);
                showCategoryInputDialog();
            }

            void CategoryManagementWidget::onEditCategoryClicked() {
                if (!hasPermission("Catalog.UpdateCategory")) {
                    showMessageBox("Lỗi", "Bạn không có quyền sửa danh mục.", QMessageBox::Warning);
                    return;
                }

                int selectedRow = categoryTable_->currentRow();
                if (selectedRow < 0) {
                    showMessageBox("Sửa Danh Mục", "Vui lòng chọn một danh mục để sửa.", QMessageBox::Information);
                    return;
                }

                QString categoryId = categoryTable_->item(selectedRow, 0)->text();
                std::optional<ERP::Catalog::DTO::CategoryDTO> categoryOpt = categoryService_->getCategoryById(categoryId.toStdString(), currentUserId_, currentUserRoleIds_);

                if (categoryOpt) {
                    // Populate form fields with selected category data first
                    idLineEdit_->setText(QString::fromStdString(categoryOpt->id));
                    nameLineEdit_->setText(QString::fromStdString(categoryOpt->name));
                    descriptionLineEdit_->setText(QString::fromStdString(categoryOpt->description.value_or("")));
                    sortOrderLineEdit_->setText(QString::number(categoryOpt->sortOrder));
                    isActiveCheckBox_->setChecked(categoryOpt->isActive);
                    statusComboBox_->setCurrentIndex(statusComboBox_->findData(static_cast<int>(categoryOpt->status)));

                    // Populate combo box for the input dialog (excluding self)
                    populateParentCategoryComboBox(parentCategoryComboBox_);
                    if (categoryOpt->parentCategoryId) {
                        int index = parentCategoryComboBox_->findData(QString::fromStdString(*categoryOpt->parentCategoryId));
                        if (index != -1) {
                            parentCategoryComboBox_->setCurrentIndex(index);
                        }
                        else {
                            parentCategoryComboBox_->setCurrentIndex(0); // "None" if parent not found or invalid
                        }
                    }
                    else {
                        parentCategoryComboBox_->setCurrentIndex(0); // "None"
                    }

                    showCategoryInputDialog(&(*categoryOpt)); // Pass by address to modify
                }
                else {
                    showMessageBox("Sửa Danh Mục", "Không tìm thấy danh mục để sửa.", QMessageBox::Critical);
                }
            }

            void CategoryManagementWidget::onDeleteCategoryClicked() {
                if (!hasPermission("Catalog.DeleteCategory")) {
                    showMessageBox("Lỗi", "Bạn không có quyền xóa danh mục.", QMessageBox::Warning);
                    return;
                }

                int selectedRow = categoryTable_->currentRow();
                if (selectedRow < 0) {
                    showMessageBox("Xóa Danh Mục", "Vui lòng chọn một danh mục để xóa.", QMessageBox::Information);
                    return;
                }

                QString categoryId = categoryTable_->item(selectedRow, 0)->text();
                QString categoryName = categoryTable_->item(selectedRow, 1)->text();

                Common::CustomMessageBox confirmBox(this);
                confirmBox.setWindowTitle("Xóa Danh Mục");
                confirmBox.setText("Bạn có chắc chắn muốn xóa danh mục '" + categoryName + "' (ID: " + categoryId + ")? Thao tác này sẽ vô hiệu hóa danh mục và các danh mục con.");
                confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                confirmBox.setDefaultButton(QMessageBox::No);
                if (confirmBox.exec() == QMessageBox::Yes) {
                    if (categoryService_->deleteCategory(categoryId.toStdString(), currentUserId_, currentUserRoleIds_)) {
                        showMessageBox("Xóa Danh Mục", "Danh mục đã được xóa thành công.", QMessageBox::Information);
                        loadCategories();
                        clearForm();
                    }
                    else {
                        showMessageBox("Lỗi Xóa", "Không thể xóa danh mục. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
                    }
                }
            }

            void CategoryManagementWidget::onUpdateCategoryStatusClicked() {
                if (!hasPermission("Catalog.UpdateCategoryStatus")) { // Changed to more specific permission
                    showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái danh mục.", QMessageBox::Warning);
                    return;
                }

                int selectedRow = categoryTable_->currentRow();
                if (selectedRow < 0) {
                    showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một danh mục để cập nhật trạng thái.", QMessageBox::Information);
                    return;
                }

                QString categoryId = categoryTable_->item(selectedRow, 0)->text();
                std::optional<ERP::Catalog::DTO::CategoryDTO> categoryOpt = categoryService_->getCategoryById(categoryId.toStdString(), currentUserId_, currentUserRoleIds_);

                if (!categoryOpt) {
                    showMessageBox("Cập nhật trạng thái", "Không tìm thấy danh mục để cập nhật trạng thái.", QMessageBox::Critical);
                    return;
                }

                ERP::Catalog::DTO::CategoryDTO currentCategory = *categoryOpt;
                // Use a QInputDialog or custom dialog to get the new status
                QDialog statusDialog(this);
                statusDialog.setWindowTitle("Chọn Trạng Thái Mới");
                QVBoxLayout* layout = new QVBoxLayout(&statusDialog);
                QComboBox newStatusCombo;
                // Populate based on EntityStatus enum
                newStatusCombo.addItem("Active", static_cast<int>(ERP::Common::EntityStatus::ACTIVE));
                newStatusCombo.addItem("Inactive", static_cast<int>(ERP::Common::EntityStatus::INACTIVE));
                newStatusCombo.addItem("Pending", static_cast<int>(ERP::Common::EntityStatus::PENDING));
                newStatusCombo.addItem("Deleted", static_cast<int>(ERP::Common::EntityStatus::DELETED));
                // Set current status as default selected
                int currentStatusIndex = newStatusCombo.findData(static_cast<int>(currentCategory.status));
                if (currentStatusIndex != -1) newStatusCombo.setCurrentIndex(currentStatusIndex);

                layout->addWidget(new QLabel("Chọn trạng thái mới:", &statusDialog));
                layout->addWidget(&newStatusCombo);
                QPushButton* okButton = new QPushButton("Cập nhật", &statusDialog);
                QPushButton* cancelButton = new QPushButton("Hủy", &statusDialog);
                QHBoxLayout* btnLayout = new QHBoxLayout();
                btnLayout->addWidget(okButton);
                btnLayout->addWidget(cancelButton);
                layout->addLayout(btnLayout);

                connect(okButton, &QPushButton::clicked, &statusDialog, &QDialog::accept);
                connect(cancelButton, &QPushButton::clicked, &statusDialog, &QDialog::reject);

                if (statusDialog.exec() == QDialog::Accepted) {
                    ERP::Common::EntityStatus newStatus = static_cast<ERP::Common::EntityStatus>(newStatusCombo.currentData().toInt());

                    Common::CustomMessageBox confirmBox(this);
                    confirmBox.setWindowTitle("Cập nhật trạng thái danh mục");
                    confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái danh mục '" + QString::fromStdString(currentCategory.name) + "' thành " + newStatusCombo.currentText() + "?");
                    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    if (confirmBox.exec() == QMessageBox::Yes) {
                        if (categoryService_->updateCategoryStatus(categoryId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
                            showMessageBox("Cập nhật trạng thái", "Trạng thái danh mục đã được cập nhật thành công.", QMessageBox::Information);
                            loadCategories();
                            clearForm();
                        }
                        else {
                            showMessageBox("Lỗi", "Không thể cập nhật trạng thái danh mục. Vui lòng kiểm tra log.", QMessageBox::Critical);
                        }
                    }
                }
            }


            void CategoryManagementWidget::onSearchCategoryClicked() {
                QString searchText = searchLineEdit_->text();
                std::map<std::string, std::any> filter;
                if (!searchText.isEmpty()) {
                    filter["name_contains"] = searchText.toStdString(); // Assuming service supports "contains" filter
                }
                categoryTable_->setRowCount(0);
                std::vector<ERP::Catalog::DTO::CategoryDTO> categories = categoryService_->getAllCategories(filter, currentUserId_, currentUserRoleIds_);

                categoryTable_->setRowCount(categories.size());
                for (int i = 0; i < categories.size(); ++i) {
                    const auto& category = categories[i];
                    categoryTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(category.id)));
                    categoryTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(category.name)));
                    categoryTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(category.description.value_or(""))));

                    QString parentName = "N/A";
                    if (category.parentCategoryId) {
                        std::optional<ERP::Catalog::DTO::CategoryDTO> parentCategory = categoryService_->getCategoryById(*category.parentCategoryId, currentUserId_, currentUserRoleIds_);
                        if (parentCategory) {
                            parentName = QString::fromStdString(parentCategory->name);
                        }
                    }
                    categoryTable_->setItem(i, 3, new QTableWidgetItem(parentName));

                    categoryTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(category.status))));
                    categoryTable_->setItem(i, 5, new QTableWidgetItem(QString::number(category.sortOrder)));
                    categoryTable_->setItem(i, 6, new QTableWidgetItem(category.isActive ? "Yes" : "No"));
                    categoryTable_->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(ERP::Utils::DateUtils::formatDateTime(category.createdAt, ERP::Common::DATETIME_FORMAT))));
                }
                categoryTable_->resizeColumnsToContents();
                ERP::Logger::Logger::getInstance().info("CategoryManagementWidget: Search completed.");
            }

            void CategoryManagementWidget::onCategoryTableItemClicked(int row, int column) {
                if (row < 0) return;
                QString categoryId = categoryTable_->item(row, 0)->text();
                std::optional<ERP::Catalog::DTO::CategoryDTO> categoryOpt = categoryService_->getCategoryById(categoryId.toStdString(), currentUserId_, currentUserRoleIds_);

                if (categoryOpt) {
                    idLineEdit_->setText(QString::fromStdString(categoryOpt->id));
                    nameLineEdit_->setText(QString::fromStdString(categoryOpt->name));
                    descriptionLineEdit_->setText(QString::fromStdString(categoryOpt->description.value_or("")));

                    populateParentCategoryComboBox(parentCategoryComboBox_); // Populate for the form itself
                    if (categoryOpt->parentCategoryId) {
                        int index = parentCategoryComboBox_->findData(QString::fromStdString(*categoryOpt->parentCategoryId));
                        if (index != -1) {
                            parentCategoryComboBox_->setCurrentIndex(index);
                        }
                        else {
                            parentCategoryComboBox_->setCurrentIndex(0); // "None"
                        }
                    }
                    else {
                        parentCategoryComboBox_->setCurrentIndex(0); // "None"
                    }

                    int statusIndex = statusComboBox_->findData(static_cast<int>(categoryOpt->status));
                    if (statusIndex != -1) {
                        statusComboBox_->setCurrentIndex(statusIndex);
                    }

                    sortOrderLineEdit_->setText(QString::number(categoryOpt->sortOrder));
                    isActiveCheckBox_->setChecked(categoryOpt->isActive);

                }
                else {
                    showMessageBox("Thông tin Danh Mục", "Không thể tải chi tiết danh mục đã chọn.", QMessageBox::Warning);
                    clearForm();
                }
                updateButtonsState(); // Update button states after selection
            }

            void CategoryManagementWidget::clearForm() {
                idLineEdit_->clear();
                nameLineEdit_->clear();
                descriptionLineEdit_->clear();
                parentCategoryComboBox_->clear(); // Will be repopulated when needed by next dialog/edit
                statusComboBox_->setCurrentIndex(0); // Default to Active
                sortOrderLineEdit_->clear();
                isActiveCheckBox_->setChecked(true); // Default to active
                categoryTable_->clearSelection(); // Clear table selection
                updateButtonsState(); // Update button states after clearing
            }


            void CategoryManagementWidget::showCategoryInputDialog(ERP::Catalog::DTO::CategoryDTO* category) {
                // Create a dialog for input
                QDialog dialog(this);
                dialog.setWindowTitle(category ? "Sửa Danh Mục" : "Thêm Danh Mục Mới");
                QVBoxLayout* dialogLayout = new QVBoxLayout(&dialog);
                QFormLayout* formLayout = new QFormLayout();

                QLineEdit nameEdit(this);
                QLineEdit descriptionEdit(this);
                QComboBox parentCombo(this); populateParentCategoryComboBox(&parentCombo); // Populate for dialog's combo box
                // Ensure the dialog's combo box gets its items populated correctly (excluding self if editing)
                if (category) { // If editing, remove self from parent options
                    for (int i = 0; i < parentCombo.count(); ++i) {
                        if (parentCombo.itemData(i).toString().toStdString() == category->id) {
                            parentCombo.removeItem(i);
                            break;
                        }
                    }
                }
                for (int i = 0; i < parentCategoryComboBox_->count(); ++i) { // Copy from main form's combo box if needed
                    if (parentCategoryComboBox_->itemData(i).toString().toStdString() == category->id) continue;
                    parentCombo.addItem(parentCategoryComboBox_->itemText(i), parentCategoryComboBox_->itemData(i));
                }


                QLineEdit sortOrderEdit(this);
                sortOrderEdit.setValidator(new QIntValidator(0, 99999, this));
                QCheckBox isActiveCheck("Hoạt động", this);

                if (category) {
                    nameEdit.setText(QString::fromStdString(category->name));
                    descriptionEdit.setText(QString::fromStdString(category->description.value_or("")));
                    if (category->parentCategoryId) {
                        int index = parentCombo.findData(QString::fromStdString(*category->parentCategoryId));
                        if (index != -1) parentCombo.setCurrentIndex(index);
                    }
                    sortOrderEdit.setText(QString::number(category->sortOrder));
                    isActiveCheck.setChecked(category->isActive);
                }
                else {
                    sortOrderEdit.setText("0"); // Default for new
                    isActiveCheck.setChecked(true); // Default for new
                }

                formLayout->addRow("Tên:*", &nameEdit);
                formLayout->addRow("Mô tả:", &descriptionEdit);
                formLayout->addRow("Danh mục cha:", &parentCombo);
                formLayout->addRow("Thứ tự sắp xếp:", &sortOrderEdit);
                formLayout->addRow("", &isActiveCheck);

                dialogLayout->addLayout(formLayout);

                QPushButton* okButton = new QPushButton(category ? "Lưu" : "Thêm", &dialog);
                QPushButton* cancelButton = new QPushButton("Hủy", &dialog);
                QHBoxLayout* buttonLayout = new QHBoxLayout();
                buttonLayout->addWidget(okButton);
                buttonLayout->addWidget(cancelButton);
                dialogLayout->addLayout(buttonLayout);

                connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
                connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

                if (dialog.exec() == QDialog::Accepted) {
                    ERP::Catalog::DTO::CategoryDTO newCategoryData;
                    if (category) {
                        newCategoryData = *category; // Start with existing data for update
                    }

                    newCategoryData.name = nameEdit.text().toStdString();
                    newCategoryData.description = descriptionEdit.text().isEmpty() ? std::nullopt : std::make_optional(descriptionEdit.text().toStdString());

                    QString selectedParentId = parentCombo.currentData().toString();
                    newCategoryData.parentCategoryId = selectedParentId.isEmpty() ? std::nullopt : std::make_optional(selectedParentId.toStdString());

                    newCategoryData.sortOrder = sortOrderEdit.text().toInt();
                    newCategoryData.isActive = isActiveCheck.isChecked();
                    newCategoryData.status = isActiveCheck.isChecked() ? ERP::Common::EntityStatus::ACTIVE : ERP::Common::EntityStatus::INACTIVE; // Sync status with isActive

                    bool success = false;
                    if (category) {
                        success = categoryService_->updateCategory(newCategoryData, currentUserId_, currentUserRoleIds_);
                        if (success) showMessageBox("Sửa Danh Mục", "Danh mục đã được cập nhật thành công.", QMessageBox::Information);
                        else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật danh mục. Vui lòng kiểm tra log.")), QMessageBox::Critical);
                    }
                    else {
                        std::optional<ERP::Catalog::DTO::CategoryDTO> createdCategory = categoryService_->createCategory(newCategoryData, currentUserId_, currentUserRoleIds_);
                        if (createdCategory) {
                            showMessageBox("Thêm Danh Mục", "Danh mục mới đã được thêm thành công.", QMessageBox::Information);
                            success = true;
                        }
                        else {
                            showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm danh mục mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
                        }
                    }
                    if (success) {
                        loadCategories();
                        clearForm();
                    }
                }
            }


            void CategoryManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
                Common::CustomMessageBox msgBox(this);
                msgBox.setWindowTitle(title);
                msgBox.setText(message);
                msgBox.setIcon(icon);
                msgBox.exec();
            }

            bool CategoryManagementWidget::hasPermission(const std::string& permission) {
                if (!securityManager_) return false;
                return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
            }

            void CategoryManagementWidget::updateButtonsState() {
                bool canCreate = hasPermission("Catalog.CreateCategory");
                bool canUpdate = hasPermission("Catalog.UpdateCategory");
                bool canDelete = hasPermission("Catalog.DeleteCategory");
                bool canChangeStatus = hasPermission("Catalog.UpdateCategoryStatus"); // Specific status permission

                addCategoryButton_->setEnabled(canCreate);
                searchButton_->setEnabled(hasPermission("Catalog.ViewCategories"));

                bool isRowSelected = categoryTable_->currentRow() >= 0;
                editCategoryButton_->setEnabled(isRowSelected && canUpdate);
                deleteCategoryButton_->setEnabled(isRowSelected && canDelete);
                updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);

                bool enableForm = isRowSelected && canUpdate;
                nameLineEdit_->setEnabled(enableForm);
                descriptionLineEdit_->setEnabled(enableForm);
                parentCategoryComboBox_->setEnabled(enableForm);
                statusComboBox_->setEnabled(enableForm);
                sortOrderLineEdit_->setEnabled(enableForm);
                isActiveCheckBox_->setEnabled(enableForm);

                if (!isRowSelected) {
                    idLineEdit_->clear();
                    nameLineEdit_->clear();
                    descriptionLineEdit_->clear();
                    parentCategoryComboBox_->clear(); // Will be repopulated when needed
                    statusComboBox_->setCurrentIndex(0);
                    sortOrderLineEdit_->clear();
                    isActiveCheckBox_->setChecked(true);
                }
            }


        } // namespace Catalog
    } // namespace UI
} // namespace ERP