#include "LoginPage.h"

#include "ClientMessage.h"
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>

namespace
{
    QLineEdit* CreateInput(const char* name, const QString& placeholder,
        int maxLength, bool password = false)
    {
        auto* input = new QLineEdit;
        input->setObjectName(QString::fromLatin1(name));
        input->setPlaceholderText(placeholder);
        input->setMaxLength(maxLength);
        input->setMinimumHeight(38);
        if (password)
            input->setEchoMode(QLineEdit::Password);
        return input;
    }

    QFormLayout* CreateForm(QWidget* page)
    {
        auto* form = new QFormLayout(page);
        form->setContentsMargins(22, 26, 22, 24);
        form->setVerticalSpacing(16);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        return form;
    }
}

LoginPage::LoginPage(QWidget* parent)
    : QWidget(parent), connection_(ClientConnection::Instance())
{
    setObjectName(QStringLiteral("loginWindow"));
    setWindowTitle(QStringLiteral("聊天室 · 登录与注册"));
    setMinimumWidth(560);
    resize(600, 690);
    setStyleSheet(QStringLiteral(
        "QWidget { font-family: 'Microsoft YaHei UI'; font-size: 14px; color: #26344b; }"
        "QWidget#loginWindow { background: #f3f6fb; }"
        "QFrame#serverCard { background: white; border: 1px solid #dce3ed; border-radius: 10px; }"
        "QLineEdit, QSpinBox { background: white; border: 1px solid #cbd5e1; border-radius: 5px; padding: 6px; }"
        "QLineEdit:focus, QSpinBox:focus { border: 1px solid #2563eb; }"
        "QPushButton { min-height: 34px; border: 1px solid #cbd5e1; border-radius: 5px; padding: 4px 15px; background: white; }"
        "QPushButton:hover { background: #edf3ff; }"
        "QPushButton[primary=\"true\"] { background: #2563eb; color: white; border-color: #2563eb; }"
        "QPushButton[primary=\"true\"]:hover { background: #1d4ed8; }"
        "QPushButton:disabled { background: #e2e8f0; color: #7b879a; border-color: #dce3ed; }"
        "QTabWidget::pane { background: white; border: 1px solid #dce3ed; border-radius: 7px; }"
        "QTabBar::tab { padding: 12px 32px; color: #64748b; }"
        "QTabBar::tab:selected { color: #2563eb; font-weight: bold; border-bottom: 3px solid #2563eb; }"));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(30, 26, 30, 26);
    layout->setSpacing(18);
    auto* title = new QLabel(QStringLiteral("连接你的聊天室"));
    title->setStyleSheet(QStringLiteral("font-size: 26px; font-weight: bold;"));
    layout->addWidget(title);
    auto* subtitle = new QLabel(QStringLiteral("填写服务器地址，登录账号或创建新账号。"));
    subtitle->setStyleSheet(QStringLiteral("color: #64748b;"));
    layout->addWidget(subtitle);

    auto* serverCard = new QFrame;
    serverCard->setObjectName(QStringLiteral("serverCard"));
    auto* serverLayout = new QVBoxLayout(serverCard);
    serverLayout->setContentsMargins(18, 16, 18, 16);
    serverLayout->addWidget(new QLabel(QStringLiteral("服务器连接")));
    auto* addressRow = new QHBoxLayout;
    hostEdit_ = CreateInput("serverHost", QStringLiteral("服务器 IP 或主机名"), 253);
    hostEdit_->setText(QStringLiteral("127.0.0.1"));
    hostEdit_->setAccessibleName(QStringLiteral("服务器地址"));
    portEdit_ = new QSpinBox;
    portEdit_->setObjectName(QStringLiteral("serverPort"));
    portEdit_->setRange(1, 65535);
    portEdit_->setValue(9527);
    portEdit_->setMinimumHeight(38);
    portEdit_->setAccessibleName(QStringLiteral("服务器端口"));
    connectButton_ = new QPushButton(QStringLiteral("连接"));
    connectButton_->setObjectName(QStringLiteral("connectButton"));
    addressRow->addWidget(hostEdit_, 1);
    addressRow->addWidget(portEdit_);
    addressRow->addWidget(connectButton_);
    serverLayout->addLayout(addressRow);
    connectionLabel_ = new QLabel;
    connectionLabel_->setObjectName(QStringLiteral("connectionStatus"));
    serverLayout->addWidget(connectionLabel_);
    layout->addWidget(serverCard);

    pages_ = new QStackedWidget;
    tabs_ = new QTabWidget;
    tabs_->setObjectName(QStringLiteral("authTabs"));
    tabs_->addTab(CreateLoginForm(), QStringLiteral("登录"));
    tabs_->addTab(CreateRegistrationForm(), QStringLiteral("注册账号"));
    pages_->addWidget(tabs_);
    pages_->addWidget(CreateConnectedPage());
    layout->addWidget(pages_);
    statusLabel_ = new QLabel(QStringLiteral("点击登录或注册时，会自动连接服务器。"));
    statusLabel_->setObjectName(QStringLiteral("requestStatus"));
    statusLabel_->setWordWrap(true);
    statusLabel_->setMinimumHeight(48);
    layout->addWidget(statusLabel_);
    layout->addStretch();

    connect(connectButton_, &QPushButton::clicked, this, [this]()
    {
        if (connection_.IsConnected() || connection_.IsConnecting())
            connection_.DisconnectFromServer();
        else if (connection_.SetServer(hostEdit_->text(), static_cast<quint16>(portEdit_->value())))
            connection_.ConnectToServer();
    });
    connect(&connection_, &ClientConnection::StateChanged, this, &LoginPage::RefreshConnectionState);
    connect(&connection_, &ClientConnection::StatusMessage, this, &LoginPage::ShowStatus);
    connect(&connection_, &ClientConnection::RequestFinished, this,
        [this](quint16 type, bool succeeded)
    {
        loginPassword_->clear();
        registerPassword_->clear();
        confirmPassword_->clear();
        if (!succeeded)
            return;
        if (type == CreateUserRequestType)
        {
            loginAccount_->setText(pendingAccount_);
            tabs_->setCurrentIndex(0);
            loginPassword_->setFocus();
        }
        else if (type == LoginRequestType)
        {
            currentAccount_->setText(QStringLiteral("当前账号：%1").arg(pendingAccount_));
            pages_->setCurrentIndex(1);
        }
    });
    RefreshConnectionState();
}

QWidget* LoginPage::CreateLoginForm()
{
    auto* page = new QWidget;
    auto* form = CreateForm(page);
    loginAccount_ = CreateInput("loginAccount", QStringLiteral("请输入账号"), 64);
    loginPassword_ = CreateInput("loginPassword", QStringLiteral("请输入密码"), 1024, true);
    form->addRow(QStringLiteral("账号"), loginAccount_);
    form->addRow(QStringLiteral("密码"), loginPassword_);
    auto* button = new QPushButton(QStringLiteral("登录"));
    button->setObjectName(QStringLiteral("loginButton"));
    button->setProperty("primary", true);
    form->addRow(button);
    connect(button, &QPushButton::clicked, this, &LoginPage::HandleSignIn);
    connect(loginPassword_, &QLineEdit::returnPressed, this, &LoginPage::HandleSignIn);
    connect(loginAccount_, &QLineEdit::returnPressed, this, [this]() { loginPassword_->setFocus(); });
    return page;
}

QWidget* LoginPage::CreateRegistrationForm()
{
    auto* page = new QWidget;
    auto* form = CreateForm(page);
    registerAccount_ = CreateInput("registerAccount", QStringLiteral("1–64 位字母、数字、_ 或 -"), 64);
    registerNickname_ = CreateInput("registerNickname", QStringLiteral("昵称，最多 128 个 UTF-8 字节"), 128);
    registerPassword_ = CreateInput("registerPassword", QStringLiteral("密码，8–1024 个 UTF-8 字节"), 1024, true);
    confirmPassword_ = CreateInput("confirmPassword", QStringLiteral("再次输入密码"), 1024, true);
    form->addRow(QStringLiteral("账号"), registerAccount_);
    form->addRow(QStringLiteral("昵称"), registerNickname_);
    form->addRow(QStringLiteral("密码"), registerPassword_);
    form->addRow(QStringLiteral("确认密码"), confirmPassword_);
    auto* button = new QPushButton(QStringLiteral("创建账号"));
    button->setObjectName(QStringLiteral("registerButton"));
    button->setProperty("primary", true);
    form->addRow(button);
    connect(button, &QPushButton::clicked, this, &LoginPage::HandleSignUp);
    connect(confirmPassword_, &QLineEdit::returnPressed, this, &LoginPage::HandleSignUp);
    return page;
}

QWidget* LoginPage::CreateConnectedPage()
{
    auto* page = new QWidget;
    page->setObjectName(QStringLiteral("loggedInPage"));
    auto* layout = new QVBoxLayout(page);
    auto* title = new QLabel(QStringLiteral("登录成功"));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("font-size: 24px; color: #15803d; font-weight: bold;"));
    currentAccount_ = new QLabel;
    currentAccount_->setObjectName(QStringLiteral("currentAccount"));
    currentAccount_->setAlignment(Qt::AlignCenter);
    auto* description = new QLabel(QStringLiteral("欢迎回来，已与聊天室服务器建立连接。"));
    description->setAlignment(Qt::AlignCenter);
    description->setWordWrap(true);
    auto* disconnectButton = new QPushButton(QStringLiteral("断开连接，返回登录"));
    disconnectButton->setObjectName(QStringLiteral("returnToLoginButton"));
    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(currentAccount_);
    layout->addWidget(description);
    layout->addSpacing(20);
    layout->addWidget(disconnectButton);
    layout->addStretch();
    connect(disconnectButton, &QPushButton::clicked, &connection_, &ClientConnection::DisconnectFromServer);
    return page;
}

void LoginPage::HandleSignIn()
{
    if (connection_.IsBusy())
        return;
    if (!connection_.SetServer(hostEdit_->text(), static_cast<quint16>(portEdit_->value())))
        return;
    pendingAccount_ = loginAccount_->text();
    connection_.SetUserInput(pendingAccount_, loginPassword_->text());
    SendLoginRequest();
}

void LoginPage::HandleSignUp()
{
    if (connection_.IsBusy())
        return;
    if (registerPassword_->text() != confirmPassword_->text())
    {
        ShowStatus(QStringLiteral("两次输入的密码不一致。"), true);
        confirmPassword_->setFocus();
        return;
    }
    if (!connection_.SetServer(hostEdit_->text(), static_cast<quint16>(portEdit_->value())))
        return;
    pendingAccount_ = registerAccount_->text();
    connection_.SetUserInput(pendingAccount_, registerPassword_->text(), registerNickname_->text());
    SendCreateUserRequest();
}

void LoginPage::RefreshConnectionState()
{
    const bool connected = connection_.IsConnected();
    const bool connecting = connection_.IsConnecting();
    const bool busy = connection_.IsBusy();
    hostEdit_->setEnabled(!connected && !connecting && !busy);
    portEdit_->setEnabled(!connected && !connecting && !busy);
    connectButton_->setEnabled(!busy);
    connectButton_->setText(connected ? QStringLiteral("断开")
        : connecting ? QStringLiteral("取消连接") : QStringLiteral("连接"));
    connectionLabel_->setText(connected ? QStringLiteral("● 已连接")
        : connecting ? QStringLiteral("● 正在连接…") : QStringLiteral("● 未连接"));
    connectionLabel_->setStyleSheet(connected
        ? QStringLiteral("color: #15803d;") : QStringLiteral("color: #64748b;"));
    tabs_->setEnabled(!busy);
    if (!connected)
        pages_->setCurrentIndex(0);
}

void LoginPage::ShowStatus(const QString& message, bool isError)
{
    statusLabel_->setText(message);
    statusLabel_->setStyleSheet(isError
        ? QStringLiteral("color: #b91c1c;") : QStringLiteral("color: #375579;"));
}
