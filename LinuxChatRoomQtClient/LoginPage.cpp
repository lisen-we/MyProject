#include "LoginPage.h"

#include "ClientMessage.h"
#include "ui_LoginPage.h"

LoginPage::LoginPage(QWidget* parent)
    : QWidget(parent), ui_(new Ui::LoginPage), connection_(ClientConnection::Instance())
{
    ui_->setupUi(this);
    connect(ui_->loginButton, &QPushButton::clicked, this, &LoginPage::HandleSignIn);
    connect(ui_->loginPassword, &QLineEdit::returnPressed, this, &LoginPage::HandleSignIn);
    connect(ui_->loginAccount, &QLineEdit::returnPressed, this,
        [this]() { ui_->loginPassword->setFocus(); });
    connect(ui_->registerButton, &QPushButton::clicked, this, &LoginPage::HandleSignUp);
    connect(ui_->confirmPassword, &QLineEdit::returnPressed, this, &LoginPage::HandleSignUp);
    connect(ui_->returnToLoginButton, &QPushButton::clicked,
        &connection_, &ClientConnection::DisconnectFromServer);

    connect(ui_->connectButton, &QPushButton::clicked, this, [this]()
    {
        if (connection_.IsConnected() || connection_.IsConnecting())
            connection_.DisconnectFromServer();
        else if (connection_.SetServer(ui_->serverHost->text(), static_cast<quint16>(ui_->serverPort->value())))
            connection_.ConnectToServer();
    });
    connect(&connection_, &ClientConnection::StateChanged, this, &LoginPage::RefreshConnectionState);
    connect(&connection_, &ClientConnection::StatusMessage, this, &LoginPage::ShowStatus);
    connect(&connection_, &ClientConnection::RequestFinished, this,
        [this](quint16 type, bool succeeded)
    {
        ui_->loginPassword->clear();
        ui_->registerPassword->clear();
        ui_->confirmPassword->clear();
        if (!succeeded)
            return;
        if (type == CreateUserRequestType)
        {
            ui_->loginAccount->setText(pendingAccount_);
            ui_->authTabs->setCurrentIndex(0);
            ui_->loginPassword->setFocus();
        }
        else if (type == LoginRequestType)
        {
            ui_->currentAccount->setText(QStringLiteral("当前账号：%1").arg(pendingAccount_));
            ui_->pages->setCurrentIndex(1);
        }
    });
    RefreshConnectionState();
}

LoginPage::~LoginPage() = default;

void LoginPage::HandleSignIn()
{
    if (connection_.IsBusy())
        return;
    if (!connection_.SetServer(ui_->serverHost->text(), static_cast<quint16>(ui_->serverPort->value())))
        return;
    pendingAccount_ = ui_->loginAccount->text();
    connection_.SetUserInput(pendingAccount_, ui_->loginPassword->text());
    SendLoginRequest();
}

void LoginPage::HandleSignUp()
{
    if (connection_.IsBusy())
        return;
    if (ui_->registerPassword->text() != ui_->confirmPassword->text())
    {
        ShowStatus(QStringLiteral("两次输入的密码不一致。"), true);
        ui_->confirmPassword->setFocus();
        return;
    }
    if (!connection_.SetServer(ui_->serverHost->text(), static_cast<quint16>(ui_->serverPort->value())))
        return;
    pendingAccount_ = ui_->registerAccount->text();
    connection_.SetUserInput(pendingAccount_, ui_->registerPassword->text(), ui_->registerNickname->text());
    SendCreateUserRequest();
}

void LoginPage::RefreshConnectionState()
{
    const bool connected = connection_.IsConnected();
    const bool connecting = connection_.IsConnecting();
    const bool busy = connection_.IsBusy();
    ui_->serverHost->setEnabled(!connected && !connecting && !busy);
    ui_->serverPort->setEnabled(!connected && !connecting && !busy);
    ui_->connectButton->setEnabled(!busy);
    ui_->connectButton->setText(connected ? QStringLiteral("断开")
        : connecting ? QStringLiteral("取消连接") : QStringLiteral("连接"));
    ui_->connectionStatus->setText(connected ? QStringLiteral("● 已连接")
        : connecting ? QStringLiteral("● 正在连接…") : QStringLiteral("● 未连接"));
    ui_->connectionStatus->setStyleSheet(connected
        ? QStringLiteral("color: #15803d;") : QStringLiteral("color: #64748b;"));
    ui_->authTabs->setEnabled(!busy);
    if (!connected)
        ui_->pages->setCurrentIndex(0);
}

void LoginPage::ShowStatus(const QString& message, bool isError)
{
    ui_->requestStatus->setText(message);
    ui_->requestStatus->setStyleSheet(isError
        ? QStringLiteral("color: #b91c1c;") : QStringLiteral("color: #375579;"));
}
