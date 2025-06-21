// UI/Customer/CustomerManagementWidget.cpp
#include "CustomerManagementWidget.h" // Đã rút gọn include
#include "Customer.h"          // Đã rút gọn include
#include "CustomerService.h"   // Đã rút gọn include
#include "ISecurityManager.h"  // Đã rút gọn include
#include "Logger.h"            // Đã rút gọn include
#include "ErrorHandler.h"      // Đã rút gọn include
#include "Common.h"            // Đã rút gọn include
#include "DateUtils.h"         // Đã rút gọn include
#include "StringUtils.h"       // Đã rút gọn include
#include "CustomMessageBox.h"  // Đã rút gọn include
#include "UserService.h"       // For getting current user roles from SecurityManager
#include "ContactPersonDTO.h"  // Đã rút gọn include
#include "AddressDTO.h"        // Đã rút gọn include

#include <QInputDialog> // For getting input from user
#include <QDoubleValidator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace ERP {
namespace UI {
namespace Customer {

CustomerManagementWidget::CustomerManagementWidget(
    QWidget *parent,
    std::shared_ptr<Services::ICustomerService> customerService,
    std::shared_ptr<Security::ISecurityManager> securityManager)
    : QWidget(parent),
      customerService_(customerService),
      securityManager_(securityManager) {

    if (!customerService_ || !securityManager_) {
        showMessageBox("Lỗi Khởi Tạo", "Dịch vụ khách hàng hoặc dịch vụ bảo mật không khả dụng. Vui lòng liên hệ quản trị viên.", QMessageBox::Critical);
        ERP::Logger::Logger::getInstance().critical("CustomerManagementWidget: Initialized with null dependencies.");
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
            ERP::Logger::Logger::getInstance().warning("CustomerManagementWidget: No active session found. Running with limited privileges.");
        }
    } else {
        currentUserId_ = "system_user";
        currentUserRoleIds_ = {"anonymous"};
        ERP::Logger::Logger::getInstance().warning("CustomerManagementWidget: Authentication Service not available. Running with limited privileges.");
    }

    setupUI();
    loadCustomers();
    updateButtonsState();
}

CustomerManagementWidget::~CustomerManagementWidget() {
    // Layout and widgets are children of this, so they are deleted automatically
}

void CustomerManagementWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit_ = new QLineEdit(this);
    searchLineEdit_->setPlaceholderText("Tìm kiếm theo tên khách hàng...");
    searchButton_ = new QPushButton("Tìm kiếm", this);
    connect(searchButton_, &QPushButton::clicked, this, &CustomerManagementWidget::onSearchCustomerClicked);
    searchLayout->addWidget(searchLineEdit_);
    searchLayout->addWidget(searchButton_);
    mainLayout->addLayout(searchLayout);

    customerTable_ = new QTableWidget(this);
    customerTable_->setColumnCount(7); // ID, Tên, Mã số thuế, Ghi chú, Điều khoản thanh toán, Hạn mức tín dụng, Trạng thái
    customerTable_->setHorizontalHeaderLabels({"ID", "Tên", "Mã số thuế", "Ghi chú", "Điều khoản TT", "Hạn mức TD", "Trạng thái"});
    customerTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    customerTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    customerTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    customerTable_->horizontalHeader()->setStretchLastSection(true);
    connect(customerTable_, &QTableWidget::itemClicked, this, &CustomerManagementWidget::onCustomerTableItemClicked);
    mainLayout->addWidget(customerTable_);

    // Form elements for editing/adding
    QGridLayout *formLayout = new QGridLayout();
    idLineEdit_ = new QLineEdit(this); idLineEdit_->setReadOnly(true);
    nameLineEdit_ = new QLineEdit(this);
    taxIdLineEdit_ = new QLineEdit(this);
    notesLineEdit_ = new QLineEdit(this);
    defaultPaymentTermsLineEdit_ = new QLineEdit(this);
    creditLimitLineEdit_ = new QLineEdit(this); creditLimitLineEdit_->setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));
    statusComboBox_ = new QComboBox(this);
    statusComboBox_->addItem("Active", static_cast<int>(ERP::Common::EntityStatus::ACTIVE));
    statusComboBox_->addItem("Inactive", static_cast<int>(ERP::Common::EntityStatus::INACTIVE));
    statusComboBox_->addItem("Pending", static_cast<int>(ERP::Common::EntityStatus::PENDING));
    statusComboBox_->addItem("Deleted", static_cast<int>(ERP::Common::EntityStatus::DELETED));

    formLayout->addWidget(new QLabel("ID:", this), 0, 0); formLayout->addWidget(idLineEdit_, 0, 1);
    formLayout->addWidget(new QLabel("Tên:*", this), 1, 0); formLayout->addWidget(nameLineEdit_, 1, 1);
    formLayout->addWidget(new QLabel("Mã số thuế:", this), 2, 0); formLayout->addWidget(taxIdLineEdit_, 2, 1);
    formLayout->addWidget(new QLabel("Ghi chú:", this), 3, 0); formLayout->addWidget(notesLineEdit_, 3, 1);
    formLayout->addWidget(new QLabel("Điều khoản TT mặc định:", this), 4, 0); formLayout->addWidget(defaultPaymentTermsLineEdit_, 4, 1);
    formLayout->addWidget(new QLabel("Hạn mức TD:", this), 5, 0); formLayout->addWidget(creditLimitLineEdit_, 5, 1);
    formLayout->addWidget(new QLabel("Trạng thái:", this), 6, 0); formLayout->addWidget(statusComboBox_, 6, 1);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addCustomerButton_ = new QPushButton("Thêm mới", this); connect(addCustomerButton_, &QPushButton::clicked, this, &CustomerManagementWidget::onAddCustomerClicked);
    editCustomerButton_ = new QPushButton("Sửa", this); connect(editCustomerButton_, &QPushButton::clicked, this, &CustomerManagementWidget::onEditCustomerClicked);
    deleteCustomerButton_ = new QPushButton("Xóa", this); connect(deleteCustomerButton_, &QPushButton::clicked, this, &CustomerManagementWidget::onDeleteCustomerClicked);
    updateStatusButton_ = new QPushButton("Cập nhật trạng thái", this); connect(updateStatusButton_, &QPushButton::clicked, this, &CustomerManagementWidget::onUpdateCustomerStatusClicked);
    clearFormButton_ = new QPushButton("Xóa Form", this); connect(clearFormButton_, &QPushButton::clicked, this, &CustomerManagementWidget::clearForm);
    buttonLayout->addWidget(addCustomerButton_);
    buttonLayout->addWidget(editCustomerButton_);
    buttonLayout->addWidget(deleteCustomerButton_);
    buttonLayout->addWidget(updateStatusButton_);
    buttonLayout->addWidget(clearFormButton_);
    mainLayout->addLayout(buttonLayout);
}

void CustomerManagementWidget::loadCustomers() {
    ERP::Logger::Logger::getInstance().info("CustomerManagementWidget: Loading customers...");
    customerTable_->setRowCount(0);

    std::vector<ERP::Customer::DTO::CustomerDTO> customers = customerService_->getAllCustomers({}, currentUserId_, currentUserRoleIds_);

    customerTable_->setRowCount(customers.size());
    for (int i = 0; i < customers.size(); ++i) {
        const auto& customer = customers[i];
        customerTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(customer.id)));
        customerTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(customer.name)));
        customerTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(customer.taxId.value_or(""))));
        customerTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(customer.notes.value_or(""))));
        customerTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(customer.defaultPaymentTerms.value_or(""))));
        customerTable_->setItem(i, 5, new QTableWidgetItem(QString::number(customer.creditLimit.value_or(0.0), 'f', 2)));
        customerTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(customer.status))));
    }
    customerTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("CustomerManagementWidget: Customers loaded successfully.");
}

void CustomerManagementWidget::onAddCustomerClicked() {
    if (!hasPermission("Customer.CreateCustomer")) {
        showMessageBox("Lỗi", "Bạn không có quyền thêm khách hàng.", QMessageBox::Warning);
        return;
    }
    clearForm();
    showCustomerInputDialog();
}

void CustomerManagementWidget::onEditCustomerClicked() {
    if (!hasPermission("Customer.UpdateCustomer")) {
        showMessageBox("Lỗi", "Bạn không có quyền sửa khách hàng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = customerTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Sửa Khách Hàng", "Vui lòng chọn một khách hàng để sửa.", QMessageBox::Information);
        return;
    }

    QString customerId = customerTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Customer::DTO::CustomerDTO> customerOpt = customerService_->getCustomerById(customerId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (customerOpt) {
        showCustomerInputDialog(&(*customerOpt));
    } else {
        showMessageBox("Sửa Khách Hàng", "Không tìm thấy khách hàng để sửa.", QMessageBox::Critical);
    }
}

void CustomerManagementWidget::onDeleteCustomerClicked() {
    if (!hasPermission("Customer.DeleteCustomer")) {
        showMessageBox("Lỗi", "Bạn không có quyền xóa khách hàng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = customerTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Xóa Khách Hàng", "Vui lòng chọn một khách hàng để xóa.", QMessageBox::Information);
        return;
    }

    QString customerId = customerTable_->item(selectedRow, 0)->text();
    QString customerName = customerTable_->item(selectedRow, 1)->text();

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Xóa Khách Hàng");
    confirmBox.setText("Bạn có chắc chắn muốn xóa khách hàng '" + customerName + "' (ID: " + customerId + ")?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmBox.setDefaultButton(QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (customerService_->deleteCustomer(customerId.toStdString(), currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Xóa Khách Hàng", "Khách hàng đã được xóa thành công.", QMessageBox::Information);
            loadCustomers();
            clearForm();
        } else {
            showMessageBox("Lỗi Xóa", "Không thể xóa khách hàng. Vui lòng kiểm tra log để biết thêm chi tiết.", QMessageBox::Critical);
        }
    }
}

void CustomerManagementWidget::onUpdateCustomerStatusClicked() {
    if (!hasPermission("Customer.ChangeCustomerStatus")) {
        showMessageBox("Lỗi", "Bạn không có quyền cập nhật trạng thái khách hàng.", QMessageBox::Warning);
        return;
    }

    int selectedRow = customerTable_->currentRow();
    if (selectedRow < 0) {
        showMessageBox("Cập nhật trạng thái", "Vui lòng chọn một khách hàng để cập nhật trạng thái.", QMessageBox::Information);
        return;
    }

    QString customerId = customerTable_->item(selectedRow, 0)->text();
    std::optional<ERP::Customer::DTO::CustomerDTO> customerOpt = customerService_->getCustomerById(customerId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (!customerOpt) {
        showMessageBox("Cập nhật trạng thái", "Không tìm thấy khách hàng để cập nhật trạng thái.", QMessageBox::Critical);
        return;
    }

    ERP::Customer::DTO::CustomerDTO currentCustomer = *customerOpt;
    ERP::Common::EntityStatus newStatus = (currentCustomer.status == ERP::Common::EntityStatus::ACTIVE) ?
                                          ERP::Common::EntityStatus::INACTIVE :
                                          ERP::Common::EntityStatus::ACTIVE;

    Common::CustomMessageBox confirmBox(this);
    confirmBox.setWindowTitle("Cập nhật trạng thái khách hàng");
    confirmBox.setText("Bạn có chắc chắn muốn thay đổi trạng thái khách hàng '" + QString::fromStdString(currentCustomer.name) + "' thành " + QString::fromStdString(ERP::Common::entityStatusToString(newStatus)) + "?");
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (confirmBox.exec() == QMessageBox::Yes) {
        if (customerService_->updateCustomerStatus(customerId.toStdString(), newStatus, currentUserId_, currentUserRoleIds_)) {
            showMessageBox("Cập nhật trạng thái", "Trạng thái khách hàng đã được cập nhật thành công.", QMessageBox::Information);
            loadCustomers();
            clearForm();
        } else {
            showMessageBox("Lỗi", "Không thể cập nhật trạng thái khách hàng. Vui lòng kiểm tra log.", QMessageBox::Critical);
        }
    }
}


void CustomerManagementWidget::onSearchCustomerClicked() {
    QString searchText = searchLineEdit_->text();
    std::map<std::string, std::any> filter;
    if (!searchText.isEmpty()) {
        filter["name_contains"] = searchText.toStdString(); // Assuming service supports "contains" filter
    }
    customerTable_->setRowCount(0);
    std::vector<ERP::Customer::DTO::CustomerDTO> customers = customerService_->getAllCustomers(filter, currentUserId_, currentUserRoleIds_);

    customerTable_->setRowCount(customers.size());
    for (int i = 0; i < customers.size(); ++i) {
        const auto& customer = customers[i];
        customerTable_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(customer.id)));
        customerTable_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(customer.name)));
        customerTable_->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(customer.taxId.value_or(""))));
        customerTable_->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(customer.notes.value_or(""))));
        customerTable_->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(customer.defaultPaymentTerms.value_or(""))));
        customerTable_->setItem(i, 5, new QTableWidgetItem(QString::number(customer.creditLimit.value_or(0.0), 'f', 2)));
        customerTable_->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(ERP::Common::entityStatusToString(customer.status))));
    }
    customerTable_->resizeColumnsToContents();
    ERP::Logger::Logger::getInstance().info("CustomerManagementWidget: Search completed.");
}

void CustomerManagementWidget::onCustomerTableItemClicked(int row, int column) {
    if (row < 0) return;
    QString customerId = customerTable_->item(row, 0)->text();
    std::optional<ERP::Customer::DTO::CustomerDTO> customerOpt = customerService_->getCustomerById(customerId.toStdString(), currentUserId_, currentUserRoleIds_);

    if (customerOpt) {
        idLineEdit_->setText(QString::fromStdString(customerOpt->id));
        nameLineEdit_->setText(QString::fromStdString(customerOpt->name));
        taxIdLineEdit_->setText(QString::fromStdString(customerOpt->taxId.value_or("")));
        notesLineEdit_->setText(QString::fromStdString(customerOpt->notes.value_or("")));
        defaultPaymentTermsLineEdit_->setText(QString::fromStdString(customerOpt->defaultPaymentTerms.value_or("")));
        creditLimitLineEdit_->setText(QString::number(customerOpt->creditLimit.value_or(0.0), 'f', 2));

        int statusIndex = statusComboBox_->findData(static_cast<int>(customerOpt->status));
        if (statusIndex != -1) {
            statusComboBox_->setCurrentIndex(statusIndex);
        }
        // No direct QCheckBox for isActive here, use statusComboBox

    } else {
        showMessageBox("Thông tin Khách Hàng", "Không thể tải chi tiết khách hàng đã chọn.", QMessageBox::Warning);
        clearForm();
    }
    updateButtonsState();
}

void CustomerManagementWidget::clearForm() {
    idLineEdit_->clear();
    nameLineEdit_->clear();
    taxIdLineEdit_->clear();
    notesLineEdit_->clear();
    defaultPaymentTermsLineEdit_->clear();
    creditLimitLineEdit_->clear();
    statusComboBox_->setCurrentIndex(0); // Default to Active
    customerTable_->clearSelection();
    updateButtonsState();
}


void CustomerManagementWidget::showCustomerInputDialog(ERP::Customer::DTO::CustomerDTO* customer) {
    QDialog dialog(this);
    dialog.setWindowTitle(customer ? "Sửa Khách Hàng" : "Thêm Khách Hàng Mới");
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit nameEdit(this);
    QLineEdit taxIdEdit(this);
    QLineEdit notesEdit(this);
    QLineEdit defaultPaymentTermsEdit(this);
    QLineEdit creditLimitEdit(this);
    creditLimitEdit.setValidator(new QDoubleValidator(0.0, 999999999.0, 2, this));

    if (customer) {
        nameEdit.setText(QString::fromStdString(customer->name));
        taxIdEdit.setText(QString::fromStdString(customer->taxId.value_or("")));
        notesEdit.setText(QString::fromStdString(customer->notes.value_or("")));
        defaultPaymentTermsEdit.setText(QString::fromStdString(customer->defaultPaymentTerms.value_or("")));
        creditLimitEdit.setText(QString::number(customer->creditLimit.value_or(0.0), 'f', 2));
    } else {
        creditLimitEdit.setText("0.00");
    }

    formLayout->addRow("Tên:*", &nameEdit);
    formLayout->addRow("Mã số thuế:", &taxIdEdit);
    formLayout->addRow("Ghi chú:", &notesEdit);
    formLayout->addRow("Điều khoản TT mặc định:", &defaultPaymentTermsEdit);
    formLayout->addRow("Hạn mức TD:", &creditLimitEdit);
    // Contact persons and addresses will need separate sub-widgets or dialogs if editable here
    // For simplicity, we are not adding complex nested DTO editing in this basic form

    dialogLayout->addLayout(formLayout);

    QPushButton *okButton = new QPushButton(customer ? "Lưu" : "Thêm", &dialog);
    QPushButton *cancelButton = new QPushButton("Hủy", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    dialogLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ERP::Customer::DTO::CustomerDTO newCustomerData;
        if (customer) {
            newCustomerData = *customer;
        }

        newCustomerData.name = nameEdit.text().toStdString();
        newCustomerData.taxId = taxIdEdit.text().isEmpty() ? std::nullopt : std::make_optional(taxIdEdit.text().toStdString());
        newCustomerData.notes = notesEdit.text().isEmpty() ? std::nullopt : std::make_optional(notesEdit.text().toStdString());
        newCustomerData.defaultPaymentTerms = defaultPaymentTermsEdit.text().isEmpty() ? std::nullopt : std::make_optional(defaultPaymentTermsEdit.text().toStdString());
        newCustomerData.creditLimit = creditLimitEdit.text().toDouble();
        newCustomerData.status = ERP::Common::EntityStatus::ACTIVE; // Always set to active on creation/update via form

        bool success = false;
        if (customer) {
            success = customerService_->updateCustomer(newCustomerData, currentUserId_, currentUserRoleIds_);
            if (success) showMessageBox("Sửa Khách Hàng", "Khách hàng đã được cập nhật thành công.", QMessageBox::Information);
            else showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể cập nhật khách hàng. Vui lòng kiểm tra log.")), QMessageBox::Critical);
        } else {
            std::optional<ERP::Customer::DTO::CustomerDTO> createdCustomer = customerService_->createCustomer(newCustomerData, currentUserId_, currentUserRoleIds_);
            if (createdCustomer) {
                showMessageBox("Thêm Khách Hàng", "Khách hàng mới đã được thêm thành công.", QMessageBox::Information);
                success = true;
            } else {
                showMessageBox("Lỗi", QString::fromStdString(ERP::ErrorHandling::ErrorHandler::getLastUserMessage().value_or("Không thể thêm khách hàng mới. Vui lòng kiểm tra log.")), QMessageBox::Critical);
            }
        }
        if (success) {
            loadCustomers();
            clearForm();
        }
    }
}


void CustomerManagementWidget::showMessageBox(const QString& title, const QString& message, QMessageBox::Icon icon) {
    Common::CustomMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.exec();
}

bool CustomerManagementWidget::hasPermission(const std::string& permission) {
    if (!securityManager_) return false;
    return securityManager_->hasPermission(currentUserId_, currentUserRoleIds_, permission);
}

void CustomerManagementWidget::updateButtonsState() {
    bool canCreate = hasPermission("Customer.CreateCustomer");
    bool canUpdate = hasPermission("Customer.UpdateCustomer");
    bool canDelete = hasPermission("Customer.DeleteCustomer");
    bool canChangeStatus = hasPermission("Customer.ChangeCustomerStatus");

    addCustomerButton_->setEnabled(canCreate);
    searchButton_->setEnabled(hasPermission("Customer.ViewCustomers"));

    bool isRowSelected = customerTable_->currentRow() >= 0;
    editCustomerButton_->setEnabled(isRowSelected && canUpdate);
    deleteCustomerButton_->setEnabled(isRowSelected && canDelete);
    updateStatusButton_->setEnabled(isRowSelected && canChangeStatus);

    bool enableForm = isRowSelected && canUpdate;
    nameLineEdit_->setEnabled(enableForm);
    taxIdLineEdit_->setEnabled(enableForm);
    notesLineEdit_->setEnabled(enableForm);
    defaultPaymentTermsLineEdit_->setEnabled(enableForm);
    creditLimitLineEdit_->setEnabled(enableForm);
    statusComboBox_->setEnabled(enableForm);

    if (!isRowSelected) {
        idLineEdit_->clear();
        nameLineEdit_->clear();
        taxIdLineEdit_->clear();
        notesLineEdit_->clear();
        defaultPaymentTermsLineEdit_->clear();
        creditLimitLineEdit_->clear();
        statusComboBox_->setCurrentIndex(0);
    }
}


} // namespace Customer
} // namespace UI
} // namespace ERP