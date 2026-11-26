#include "ServerMessage.h"

#include <QtCore/QtEndian>

ResponseReadResult TakeServerResponse(QByteArray& buffer,
    ServerResponse& response, QString& error)
{
    error.clear();
    constexpr int headerSize = 8;
    if (buffer.size() < headerSize)
        return ResponseReadResult::Incomplete;
    const auto* bytes = reinterpret_cast<const uchar*>(buffer.constData());
    const quint32 bodySize = qFromBigEndian<quint32>(bytes);
    const quint16 version = qFromBigEndian<quint16>(bytes + 4);
    const quint16 type = qFromBigEndian<quint16>(bytes + 6);
    if (version != 1 || bodySize != 1 ||
        (type != CreateUserResponseType && type != LoginResponseType))
    {
        error = QStringLiteral("服务器响应的版本、类型或长度不符合当前协议。");
        return ResponseReadResult::Invalid;
    }
    if (buffer.size() < headerSize + 1)
        return ResponseReadResult::Incomplete;
    const unsigned char result = static_cast<unsigned char>(buffer.at(headerSize));
    if (result > 1)
    {
        error = QStringLiteral("服务器响应包含无效的结果状态。");
        return ResponseReadResult::Invalid;
    }
    response.type = type;
    response.succeeded = result == 1;
    buffer.remove(0, headerSize + 1);
    return ResponseReadResult::Complete;
}

QString HandleCreateUserResponse(bool succeeded)
{
    return succeeded
        ? QStringLiteral("注册成功，请使用新账号登录。")
        : QStringLiteral("注册失败：账号可能已存在，或服务器暂时无法处理。");
}

QString HandleLoginResponse(bool succeeded)
{
    return succeeded
        ? QStringLiteral("登录成功，已连接聊天室服务器。")
        : QStringLiteral("登录失败：账号或密码不正确，或服务器暂时无法处理。");
}
