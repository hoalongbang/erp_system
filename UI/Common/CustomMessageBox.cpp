// UI/Common/CustomMessageBox.cpp
#include "CustomMessageBox.h"

#include <QPixmap> // For icon display
#include <QIcon>   // For icon loading

namespace ERP {
    namespace UI {
        namespace Common {

            CustomMessageBox::CustomMessageBox(QWidget* parent)
                : QDialog(parent) {

                QVBoxLayout* mainLayout = new QVBoxLayout(this);
                QHBoxLayout* contentLayout = new QHBoxLayout();

                iconLabel_ = new QLabel(this);
                iconLabel_->setFixedSize(32, 32); // Standard icon size
                contentLayout->addWidget(iconLabel_);

                textLabel_ = new QLabel(this);
                textLabel_->setWordWrap(true); // Allow text to wrap
                contentLayout->addWidget(textLabel_);
                contentLayout->addStretch(); // Push content to left

                mainLayout->addLayout(contentLayout);
                mainLayout->addStretch(); // Push buttons to bottom

                // Button box will be added dynamically by setStandardButtons
            }

            void CustomMessageBox::setWindowTitle(const QString& title) {
                QDialog::setWindowTitle(title);
            }

            void CustomMessageBox::setText(const QString& text) {
                textLabel_->setText(text);
            }

            void CustomMessageBox::setIcon(QMessageBox::Icon icon) {
                QPixmap pixmap;
                switch (icon) {
                case QMessageBox::Information:
                    pixmap = QIcon::fromTheme("dialog-information").pixmap(32, 32);
                    if (pixmap.isNull()) pixmap.load(":/icons/info.png"); // Fallback to resource
                    break;
                case QMessageBox::Warning:
                    pixmap = QIcon::fromTheme("dialog-warning").pixmap(32, 32);
                    if (pixmap.isNull()) pixmap.load(":/icons/warning.png"); // Fallback to resource
                    break;
                case QMessageBox::Critical:
                    pixmap = QIcon::fromTheme("dialog-error").pixmap(32, 32);
                    if (pixmap.isNull()) pixmap.load(":/icons/critical.png"); // Fallback to resource
                    break;
                case QMessageBox::Question:
                    pixmap = QIcon::fromTheme("dialog-question").pixmap(32, 32);
                    if (pixmap.isNull()) pixmap.load(":/icons/question.png"); // Fallback to resource
                    break;
                default:
                    pixmap = QPixmap(); // No icon
                    break;
                }
                iconLabel_->setPixmap(pixmap);
                iconLabel_->setVisible(!pixmap.isNull());
            }

            QMessageBox::StandardButton CustomMessageBox::setStandardButtons(QMessageBox::StandardButtons buttons) {
                // Remove existing buttons first
                QLayoutItem* item;
                while ((item = layout()->takeAt(layout()->count() - 1)) != nullptr) {
                    if (QHBoxLayout* hbox = qobject_cast<QHBoxLayout*>(item->layout())) {
                        QLayoutItem* btnItem;
                        while ((btnItem = hbox->takeAt(0)) != nullptr) {
                            delete btnItem->widget();
                            delete btnItem;
                        }
                        delete hbox;
                    }
                    delete item;
                }

                QHBoxLayout* buttonLayout = new QHBoxLayout();
                buttonLayout->addStretch(); // Push buttons to the right

                if (buttons & QMessageBox::Ok) {
                    QPushButton* okButton = new QPushButton("OK", this);
                    connect(okButton, &QPushButton::clicked, this, &CustomMessageBox::onButtonClicked);
                    okButton->setProperty("buttonRole", static_cast<int>(QMessageBox::AcceptRole));
                    buttonLayout->addWidget(okButton);
                }
                if (buttons & QMessageBox::Save) {
                    QPushButton* saveButton = new QPushButton("Lưu", this);
                    connect(saveButton, &QPushButton::clicked, this, &CustomMessageBox::onButtonClicked);
                    saveButton->setProperty("buttonRole", static_cast<int>(QMessageBox::AcceptRole));
                    buttonLayout->addWidget(saveButton);
                }
                if (buttons & QMessageBox::SaveAll) {
                    QPushButton* saveAllButton = new QPushButton("Lưu tất cả", this);
                    connect(saveAllButton, &QPushButton::clicked, this, &CustomMessageBox::onButtonClicked);
                    saveAllButton->setProperty("buttonRole", static_cast<int>(QMessageBox::AcceptRole));
                    buttonLayout->addWidget(saveAllButton);
                }
                if (buttons & QMessageBox::Open) {
                    QPushButton* openButton = new QPushButton("Mở", this);
                    connect(openButton, &QPushButton::clicked, this, &CustomMessageBox::onButtonClicked);
                    openButton->setProperty("buttonRole", static_cast<int>(QMessageBox::AcceptRole));
                    buttonLayout->addWidget(openButton);
                }
                if (buttons & QMessageBox::Yes) {
                    QPushButton* yesButton = new QPushButton("Có", this);
                    connect(yesButton, &QPushButton::clicked, this, &CustomMessageBox::onButtonClicked);
                    yesButton->setProperty("buttonRole", static_cast<int>(QMessageBox::YesRole));
                    buttonLayout->addWidget(yesButton);
                }
                if (buttons & QMessageBox::YesAll) {
                    QPushButton* yesAllButton = new QPushButton("Có tất cả", this);
                    connect(yesAllButton, &QPushButton::clicked, this, &CustomMessageBox::onButtonClicked);
                    yesAllButton->setProperty("buttonRole", static_cast<int>(QMessageBox::YesRole));
                    buttonLayout->addWidget(yesAllButton);
                }
                if (buttons & QMessageBox::No) {
                    QPushButton* noButton = new QPushButton("Không", this);
                    connect(noButton, &QPushButton::clicked, this, &CustomMessageBox::onButtonClicked);
                    noButton->setProperty("buttonRole", static_cast<int>(QMessageBox::NoRole));
                    buttonLayout->addWidget(noButton);
                }
                if (buttons & QMessageBox::NoAll) {
                    QPushButton* noAllButton = new QPushButton("Không tất cả", this);
                    connect(noAllButton, &QPushButton::clicked, this, &CustomMessageBox::onButtonClicked);
                    noAllButton->setProperty("buttonRole", static_cast<int>(QMessageBox::NoRole));
                    buttonLayout->addWidget(noAllButton);
                }
                if (buttons & QMessageBox::Abort) {
                    QPushButton* abortButton = new QPushButton("Hủy bỏ", this);
                    connect(abortButton, &QPushButton::clicked, this, &CustomMessageBox::onButtonClicked);
                    abortButton->setProperty("buttonRole", static_cast<int>(QMessageBox::AbortRole));
                    buttonLayout->addWidget(abortButton);
                }
                if (buttons & QMessageBox::Retry) {
                    QPushButton* retryButton = new QPushButton("Thử lại", this);
                    connect(retryButton, &QPushButton::clicked, this, &CustomMessageBox::onButtonClicked);
                    retryButton->setProperty("buttonRole", static_cast<int>(QMessageBox::RetryRole));
                    buttonLayout->addWidget(retryButton);
                }
                if (buttons & QMessageBox::Ignore) {
                    QPushButton* ignoreButton = new QPushButton("Bỏ qua", this);
                    connect(ignoreButton, &QPushButton::clicked, this, &CustomMessageBox::onButtonClicked);
                    ignoreButton->setProperty("buttonRole", static_cast<int>(QMessageBox::IgnoreRole));
                    buttonLayout->addWidget(ignoreButton);
                }
                if (buttons & QMessageBox::Close) {
                    QPushButton* closeButton = new QPushButton("Đóng", this);
                    connect(closeButton, &QPushButton::clicked, this, &CustomMessageBox::onButtonClicked);
                    closeButton->setProperty("buttonRole", static_cast<int>(QMessageBox::RejectRole));
                    buttonLayout->addWidget(closeButton);
                }
                if (buttons & QMessageBox::Cancel) {
                    QPushButton* cancelButton = new QPushButton("Hủy", this);
                    connect(cancelButton, &QPushButton::clicked, this, &CustomMessageBox::onButtonClicked);
                    cancelButton->setProperty("buttonRole", static_cast<int>(QMessageBox::RejectRole));
                    buttonLayout->addWidget(cancelButton);
                }
                if (buttons & QMessageBox::Discard) {
                    QPushButton* discardButton = new QPushButton("Bỏ", this);
                    connect(discardButton, &QPushButton::clicked, this, &CustomMessageBox::onButtonClicked);
                    discardButton->setProperty("buttonRole", static_cast<int>(QMessageBox::DestructiveRole));
                    buttonLayout->addWidget(discardButton);
                }
                if (buttons & QMessageBox::Help) {
                    QPushButton* helpButton = new QPushButton("Trợ giúp", this);
                    connect(helpButton, &QPushButton::clicked, this, &CustomMessageBox::onButtonClicked);
                    helpButton->setProperty("buttonRole", static_cast<int>(QMessageBox::HelpRole));
                    buttonLayout->addWidget(helpButton);
                }

                layout()->addItem(buttonLayout);
                return clickedButton_; // This will be QMessageBox::NoButton initially, actual value set in exec() after interaction.
            }

            void CustomMessageBox::setDefaultButton(QMessageBox::StandardButton button) {
                QList<QPushButton*> buttons = findChildren<QPushButton*>();
                for (QPushButton* btn : buttons) {
                    if (btn->property("buttonRole").toInt() == static_cast<int>(button)) {
                        btn->setDefault(true);
                        btn->setFocus();
                        break;
                    }
                }
            }

            void CustomMessageBox::onButtonClicked() {
                QPushButton* button = qobject_cast<QPushButton*>(sender());
                if (button) {
                    // Map button roles to StandardButton values
                    int role = button->property("buttonRole").toInt();
                    if (role == static_cast<int>(QMessageBox::AcceptRole)) {
                        if (button->text() == "OK") clickedButton_ = QMessageBox::Ok;
                        else if (button->text() == "Lưu") clickedButton_ = QMessageBox::Save;
                        else if (button->text() == "Lưu tất cả") clickedButton_ = QMessageBox::SaveAll;
                        else if (button->text() == "Mở") clickedButton_ = QMessageBox::Open;
                        else clickedButton_ = QMessageBox::Ok; // Fallback
                    }
                    else if (role == static_cast<int>(QMessageBox::YesRole)) {
                        if (button->text() == "Có") clickedButton_ = QMessageBox::Yes;
                        else if (button->text() == "Có tất cả") clickedButton_ = QMessageBox::YesAll;
                        else clickedButton_ = QMessageBox::Yes; // Fallback
                    }
                    else if (role == static_cast<int>(QMessageBox::NoRole)) {
                        if (button->text() == "Không") clickedButton_ = QMessageBox::No;
                        else if (button->text() == "Không tất cả") clickedButton_ = QMessageBox::NoAll;
                        else clickedButton_ = QMessageBox::No; // Fallback
                    }
                    else if (role == static_cast<int>(QMessageBox::AbortRole)) {
                        clickedButton_ = QMessageBox::Abort;
                    }
                    else if (role == static_cast<int>(QMessageBox::RetryRole)) {
                        clickedButton_ = QMessageBox::Retry;
                    }
                    else if (role == static_cast<int>(QMessageBox::IgnoreRole)) {
                        clickedButton_ = QMessageBox::Ignore;
                    }
                    else if (role == static_cast<int>(QMessageBox::RejectRole)) {
                        if (button->text() == "Đóng") clickedButton_ = QMessageBox::Close;
                        else if (button->text() == "Hủy") clickedButton_ = QMessageBox::Cancel;
                        else clickedButton_ = QMessageBox::Close; // Fallback
                    }
                    else if (role == static_cast<int>(QMessageBox::DestructiveRole)) {
                        clickedButton_ = QMessageBox::Discard;
                    }
                    else if (role == static_cast<int>(QMessageBox::HelpRole)) {
                        clickedButton_ = QMessageBox::Help;
                    }

                    done(static_cast<int>(clickedButton_)); // Close the dialog with the button's result
                }
            }

        } // namespace Common
    } // namespace UI
} // namespace ERP