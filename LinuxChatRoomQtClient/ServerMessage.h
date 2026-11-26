#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

constexpr quint16 CreateUserResponseType = 0x8004;
constexpr quint16 LoginResponseType = 0x8001;

struct ServerResponse
{
    quint16 type = 0;
    bool succeeded = false;
};

enum class ResponseReadResult
{
    Incomplete,
    Complete,
    Invalid
};

// Consume one complete frame and leave any following frames in the buffer.
ResponseReadResult TakeServerResponse(QByteArray& buffer,
    ServerResponse& response, QString& error);
QString HandleCreateUserResponse(bool succeeded);
QString HandleLoginResponse(bool succeeded);
