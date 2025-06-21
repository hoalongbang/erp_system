// UI/Catalog/CategoryManagementWidget.h
#ifndef UI_CATALOG_CATEGORYMANAGEMENTWIDGET_H
#define UI_CATALOG_CATEGORYMANAGEMENTWIDGET_H
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
#include <QDialog> // For category input dialog
#include <QIntValidator> // For sortOrder input
#include <QCheckBox> // For isActive

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>

// Rút gọn các include paths
#include "CategoryService.h" // Dịch vụ quản lý danh mục
#include "ISecurityManager.h" // Dịch vụ bảo mật
#include "Logger.h"          // Logging
#include "ErrorHandler.h"    // Xử lý lỗi
#include "Common.h"          // Các enum chung
#include "DateUtils.h"       // Xử lý ngày tháng
#include "StringUtils.h"     // Xử lý chuỗi
#include "CustomMessageBox.h" // Hộp thoại thông báo tùy chỉnh
#include "Category.h"        // Category DTO

namespace ERP {
    namespace UI {
        namespace Catalog {

            /**
             * @brief CategoryManagementWidget class provides a UI for managing Categories.
             * This widget allows viewing, creating, updating, deleting, and changing category status.
             */
            class CategoryManagementWidget : public QWidget {
                Q_OBJECT

            public:
                /**
                 * @brief Constructor for CategoryManagementWidget.
                 * @param parent Parent widget.
                 * @param categoryService Shared pointer to ICategoryService.
                 * @param securityManager Shared pointer to ISecurityManager.
                 */
                explicit CategoryManagementWidget(
                    QWidget* parent = nullptr,
                    std::shared_ptr<Services::ICategoryService> categoryService = nullptr,
                    std::shared_ptr<Security::ISecurityManager> securityManager = nullptr);

                ~CategoryManagementWidget();

            private slots:
                void loadCategories();
                void onAddCategoryClicked();
                void onEditCategoryClicked();
                void onDeleteCategoryClicked();
                void onUpdateCategoryStatusClicked();
                void onSearchCategoryClicked();
                void onCategoryTableItemClicked(int row, int column);
                void clearForm();

            private:
                std::shared_ptr<Services::ICategoryService> categoryService_;
                std::shared_ptr<Security::ISecurityManager> securityManager_;
                // Current user context
                std::string currentUserId_;
                std::vector<std::string> currentUserRoleIds_;

                QTableWidget* categoryTable_;
                QPushButton* addCategoryButton_;
                QPushButton* editCategoryButton_;
                QPushButton* deleteCategoryButton_;
                QPushButton* updateStatusButton_;
                QPushButton* searchButton_;
                QLineEdit* searchLineEdit_;
                QPushButton* clearFormButton_;

                // Form elements for editing/adding
                QLineEdit* idLineEdit_;
                QLineEdit* nameLineEdit_;
                QLineEdit* descriptionLineEdit_;
                QComboBox* parentCategoryComboBox_; // For selecting parent category
                QComboBox* statusComboBox_; // For status selection (EntityStatus)
                QLineEdit* sortOrderLineEdit_;
                QCheckBox* isActiveCheckBox_; // For isActive

                // Helper functions
                void setupUI();
                void populateParentCategoryComboBox(QComboBox* comboBox);
                void showCategoryInputDialog(ERP::Catalog::DTO::CategoryDTO* category = nullptr);
                void showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon = QMessageBox::Information);
                void updateButtonsState(); // Control button enable/disable based on selection and permissions

                // Permission checking helper
                bool hasPermission(const std::string& permission);
            };

        } // namespace Catalog
    } // namespace UI
} // namespace ERP

#endif // UI_CATALOG_CATEGORYMANAGEMENTWIDGET_H