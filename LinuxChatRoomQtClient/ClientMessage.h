#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpSocket>

constexpr quint16 LoginRequestType = 1;
constexpr quint16 CreateUserRequestType = 4;

// These entry points use the input stored in the application's TCP connection.
void SendCreateUserRequest();
void SendLoginRequest();

class ClientConnection final : public QObject
{
    Q_OBJECT

public:
    static ClientConnection& Instance();
    bool SetServer(const QString& host, quint16 port);
    void SetUserInput(const QString& account, const QString& password,
        const QString& nickname = QString());
    void ConnectToServer();
    void DisconnectFromServer();
    bool IsConnected() const;
    bool IsConnecting() const;
    bool IsBusy() const;

signals:
    void StateChanged();
    void StatusMessage(const QString& message, bool isError);
    void RequestFinished(quint16 requestType, bool succeeded);

private:
    explicit ClientConnection(QObject* parent);
    friend void SendCreateUserRequest();
    friend void SendLoginRequest();
    void SendRequest(quint16 requestType);
    void WritePendingRequest();
    void ReadServerMessages();
    void ResetRequest();
    void FailConnection(const QString& message);

    QTcpSocket socket_;
    QTimer connectTimer_;
    QTimer responseTimer_;
    QString host_ = QStringLiteral("127.0.0.1");
    quint16 port_ = 9527;
    QString account_;
    QString password_;
    QString nickname_;
    QByteArray receiveBuffer_;
    QByteArray pendingFrame_;
    quint16 pendingRequest_ = 0;
    bool requestSent_ = false;
    bool closing_ = false;
};
