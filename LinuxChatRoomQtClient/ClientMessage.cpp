#include "ClientMessage.h"

#include "ServerMessage.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QDataStream>

namespace
{
    QString ValidateInput(const QString& account, const QString& password,
        const QString& nickname, bool registering)
    {
        const QByteArray accountBytes = account.toUtf8();
        if (accountBytes.isEmpty() || accountBytes.size() > 64)
            return QStringLiteral("账号长度必须为 1–64 个字符。");
        for (char ch : accountBytes)
        {
            if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9') || ch == '_' || ch == '-'))
                return QStringLiteral("账号只能包含英文字母、数字、下划线和连字符。");
        }
        const int passwordSize = password.toUtf8().size();
        if (passwordSize < 8 || passwordSize > 1024)
            return QStringLiteral("密码的 UTF-8 长度必须为 8–1024 字节。");
        const int nicknameSize = nickname.toUtf8().size();
        if (registering && (nicknameSize == 0 || nicknameSize > 128))
            return QStringLiteral("昵称不能为空，且 UTF-8 长度不能超过 128 字节。");
        return QString();
    }

    void AppendField(QDataStream& stream, const QString& value)
    {
        const QByteArray bytes = value.toUtf8();
        stream << static_cast<quint16>(bytes.size());
        stream.writeRawData(bytes.constData(), bytes.size());
    }

    QByteArray BuildRequest(quint16 type, const QString& account,
        const QString& password, const QString& nickname)
    {
        QByteArray body;
        QDataStream bodyStream(&body, QIODevice::WriteOnly);
        bodyStream.setByteOrder(QDataStream::BigEndian);
        AppendField(bodyStream, account);
        AppendField(bodyStream, password);
        if (type == CreateUserRequestType)
            AppendField(bodyStream, nickname);
        QByteArray frame;
        QDataStream headerStream(&frame, QIODevice::WriteOnly);
        headerStream.setByteOrder(QDataStream::BigEndian);
        headerStream << static_cast<quint32>(body.size()) << quint16(1) << type;
        frame.append(body);
        return frame;
    }
}

ClientConnection& ClientConnection::Instance()
{
    static ClientConnection* connection = new ClientConnection(QCoreApplication::instance());
    return *connection;
}

ClientConnection::ClientConnection(QObject* parent)
    : QObject(parent), socket_(this), connectTimer_(this), responseTimer_(this)
{
    connectTimer_.setSingleShot(true);
    responseTimer_.setSingleShot(true);
    connect(&connectTimer_, &QTimer::timeout, this, [this]()
    {
        FailConnection(QStringLiteral("连接超时，请检查服务器地址、端口及网络。"));
    });
    connect(&responseTimer_, &QTimer::timeout, this, [this]()
    {
        // Close the connection so a late response cannot match a later request.
        FailConnection(QStringLiteral("等待服务器响应超时，连接已关闭，请重试。"));
    });
    connect(&socket_, &QTcpSocket::connected, this, [this]()
    {
        connectTimer_.stop();
        emit StateChanged();
        if (IsBusy())
            WritePendingRequest();
        else
            emit StatusMessage(QStringLiteral("已连接，可以登录或注册。"), false);
    });
    connect(&socket_, &QTcpSocket::disconnected, this, [this]()
    {
        if (closing_)
            return;
        const bool interrupted = IsBusy();
        connectTimer_.stop();
        ResetRequest();
        receiveBuffer_.clear();
        emit StateChanged();
        emit StatusMessage(interrupted
            ? QStringLiteral("连接已断开，当前请求未完成，请重新连接后重试。")
            : QStringLiteral("与服务器的连接已断开。"), interrupted);
    });
    connect(&socket_, &QTcpSocket::stateChanged, this,
        [this](QAbstractSocket::SocketState) { emit StateChanged(); });
    connect(&socket_, &QTcpSocket::readyRead, this, &ClientConnection::ReadServerMessages);
    connect(&socket_, &QTcpSocket::bytesWritten, this,
        [this](qint64) { WritePendingRequest(); });
    connect(&socket_,
        static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
        this, [this](QAbstractSocket::SocketError error)
    {
        // disconnected() reports an orderly remote close, including pending work.
        if (error != QAbstractSocket::RemoteHostClosedError)
            FailConnection(QStringLiteral("TCP 连接失败：%1").arg(socket_.errorString()));
    });
}

bool ClientConnection::SetServer(const QString& host, quint16 port)
{
    const QString trimmedHost = host.trimmed();
    if (trimmedHost.isEmpty() || port == 0)
    {
        emit StatusMessage(QStringLiteral("请输入服务器地址和有效端口。"), true);
        return false;
    }
    if (host_ == trimmedHost && port_ == port)
        return true;
    if (IsBusy() || IsConnecting() || IsConnected())
    {
        emit StatusMessage(QStringLiteral("请先断开当前连接，再修改服务器地址。"), true);
        return false;
    }
    host_ = trimmedHost;
    port_ = port;
    return true;
}

void ClientConnection::SetUserInput(const QString& account,
    const QString& password, const QString& nickname)
{
    if (IsBusy())
        return;
    account_ = account;
    password_ = password;
    nickname_ = nickname;
}

bool ClientConnection::IsConnected() const
{
    return socket_.state() == QAbstractSocket::ConnectedState;
}

bool ClientConnection::IsConnecting() const
{
    return socket_.state() == QAbstractSocket::HostLookupState ||
        socket_.state() == QAbstractSocket::ConnectingState;
}

bool ClientConnection::IsBusy() const
{
    return pendingRequest_ != 0;
}

void ClientConnection::ConnectToServer()
{
    if (IsConnected() || IsConnecting())
        return;
    receiveBuffer_.clear();
    emit StatusMessage(QStringLiteral("正在连接 %1:%2…").arg(host_).arg(port_), false);
    connectTimer_.start(5000);
    socket_.connectToHost(host_, port_);
    emit StateChanged();
}

void ClientConnection::ResetRequest()
{
    responseTimer_.stop();
    pendingRequest_ = 0;
    pendingFrame_.clear();
    requestSent_ = false;
    password_.clear();
}

void ClientConnection::DisconnectFromServer()
{
    closing_ = true;
    connectTimer_.stop();
    ResetRequest();
    socket_.abort();
    receiveBuffer_.clear();
    closing_ = false;
    emit StateChanged();
    emit StatusMessage(QStringLiteral("连接已断开。"), false);
}

void ClientConnection::FailConnection(const QString& message)
{
    closing_ = true;
    connectTimer_.stop();
    ResetRequest();
    socket_.abort();
    receiveBuffer_.clear();
    closing_ = false;
    emit StateChanged();
    emit StatusMessage(message, true);
}

void ClientConnection::SendRequest(quint16 requestType)
{
    if (IsBusy())
    {
        emit StatusMessage(QStringLiteral("正在等待服务器响应，请勿重复提交。"), true);
        return;
    }
    const QString error = ValidateInput(account_, password_, nickname_,
        requestType == CreateUserRequestType);
    if (!error.isEmpty())
    {
        password_.clear();
        emit StatusMessage(error, true);
        return;
    }
    pendingFrame_ = BuildRequest(requestType, account_, password_, nickname_);
    password_.clear();
    pendingRequest_ = requestType;
    requestSent_ = false;
    emit StateChanged();
    if (IsConnected())
        WritePendingRequest();
    else
        ConnectToServer();
}

void ClientConnection::WritePendingRequest()
{
    if (!IsConnected() || !IsBusy() || requestSent_)
        return;
    if (!responseTimer_.isActive())
        responseTimer_.start(10000);
    const qint64 accepted = socket_.write(pendingFrame_);
    if (accepted < 0)
    {
        FailConnection(QStringLiteral("请求发送失败：%1").arg(socket_.errorString()));
        return;
    }
    pendingFrame_.remove(0, static_cast<int>(accepted));
    if (pendingFrame_.isEmpty())
    {
        requestSent_ = true;
        emit StatusMessage(pendingRequest_ == CreateUserRequestType
            ? QStringLiteral("注册请求已发送，正在等待服务器响应…")
            : QStringLiteral("登录请求已发送，正在等待服务器响应…"), false);
    }
}

void ClientConnection::ReadServerMessages()
{
    receiveBuffer_.append(socket_.readAll());
    while (!receiveBuffer_.isEmpty())
    {
        ServerResponse response;
        QString error;
        const ResponseReadResult result = TakeServerResponse(receiveBuffer_, response, error);
        if (result == ResponseReadResult::Incomplete)
            return;
        if (result == ResponseReadResult::Invalid)
        {
            FailConnection(error);
            return;
        }
        if (!requestSent_ || response.type != (pendingRequest_ | 0x8000))
        {
            FailConnection(QStringLiteral("收到了与当前请求不匹配的服务器响应。"));
            return;
        }
        const quint16 completedRequest = pendingRequest_;
        ResetRequest();
        emit StateChanged();
        emit StatusMessage(completedRequest == CreateUserRequestType
            ? HandleCreateUserResponse(response.succeeded)
            : HandleLoginResponse(response.succeeded), !response.succeeded);
        emit RequestFinished(completedRequest, response.succeeded);
    }
}

void SendCreateUserRequest()
{
    ClientConnection::Instance().SendRequest(CreateUserRequestType);
}

void SendLoginRequest()
{
    ClientConnection::Instance().SendRequest(LoginRequestType);
}
