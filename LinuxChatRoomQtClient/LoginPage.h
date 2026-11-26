#pragma once

#include <QtWidgets/QWidget>

class ClientConnection;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTabWidget;

class LoginPage final : public QWidget
{
public:
    explicit LoginPage(QWidget* parent = nullptr);

private:
    QWidget* CreateLoginForm();
    QWidget* CreateRegistrationForm();
    QWidget* CreateConnectedPage();
    void HandleSignIn();
    void HandleSignUp();
    void RefreshConnectionState();
    void ShowStatus(const QString& message, bool isError);

    ClientConnection& connection_;
    QLineEdit* hostEdit_ = nullptr;
    QSpinBox* portEdit_ = nullptr;
    QPushButton* connectButton_ = nullptr;
    QLabel* connectionLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QStackedWidget* pages_ = nullptr;
    QTabWidget* tabs_ = nullptr;
    QLineEdit* loginAccount_ = nullptr;
    QLineEdit* loginPassword_ = nullptr;
    QLineEdit* registerAccount_ = nullptr;
    QLineEdit* registerNickname_ = nullptr;
    QLineEdit* registerPassword_ = nullptr;
    QLineEdit* confirmPassword_ = nullptr;
    QLabel* currentAccount_ = nullptr;
    QString pendingAccount_;
};
