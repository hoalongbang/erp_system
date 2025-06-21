// UI/Common/CustomMessageBox.h
#ifndef UI_COMMON_CUSTOMMESSAGEBOX_H
#define UI_COMMON_CUSTOMMESSAGEBOX_H

#include <QDialog>
#include <QMessageBox> // For QMessageBox::Icon, QMessageBox::StandardButton
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace ERP {
    namespace UI {
        namespace Common {

            /**
             * @brief The CustomMessageBox class provides a custom, non-blocking message box.
             * This is used to replace standard QMessageBox::critical/information/warning
             * calls to avoid blocking the main event loop in certain environments (e.g., if embedded).
             */
            class CustomMessageBox : public QDialog {
                Q_OBJECT

            public:
                /**
                 * @brief Constructor for CustomMessageBox.
                 * @param parent Parent widget.
                 */
                explicit CustomMessageBox(QWidget* parent = nullptr);

                /**
                 * @brief Sets the title of the message box.
                 * @param title The title string.
                 */
                void setWindowTitle(const QString& title);

                /**
                 * @brief Sets the text content of the message box.
                 * @param text The message text.
                 */
                void setText(const QString& text);

                /**
                 * @brief Sets the icon for the message box.
                 * @param icon The QMessageBox::Icon to display.
                 */
                void setIcon(QMessageBox::Icon icon);

                /**
                 * @brief Sets the standard buttons for the message box.
                 * @param buttons A combination of QMessageBox::StandardButton flags.
                 * @return The button that was clicked.
                 */
                QMessageBox::StandardButton setStandardButtons(QMessageBox::StandardButtons buttons);

                /**
                 * @brief Sets the default button for the message box.
                 * @param button The QMessageBox::StandardButton to be the default.
                 */
                void setDefaultButton(QMessageBox::StandardButton button);

            private slots:
                void onButtonClicked();

            private:
                QLabel* iconLabel_;
                QLabel* textLabel_;
                QMessageBox::StandardButton clickedButton_ = QMessageBox::NoButton;
            };

        } // namespace Common
    } // namespace UI
} // namespace ERP

#endif // UI_COMMON_CUSTOMMESSAGEBOX_H