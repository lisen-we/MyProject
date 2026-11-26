#include "ServerMessage.h"
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <cstdio>
bool SendCreateUserResponse(int clientFd, bool registered)
{
    const unsigned char frame[] = {
        0, 0, 0, 1,  // 正文长度
        0, 1,        // 协议版本
        0x80, 0x04,  // 注册响应类型
        static_cast<unsigned char>(registered ? 1 : 0)
    };
    std::size_t sent = 0;
    while (sent < sizeof(frame))
    {
        const ssize_t count = send(clientFd, frame + sent, sizeof(frame) - sent,
            MSG_NOSIGNAL | MSG_DONTWAIT);
        if (count > 0)
            sent += static_cast<std::size_t>(count);
        else if (count < 0 && errno == EINTR)
            continue;
        else
            return false;
    }
    return true;
}
bool SendUserLoginResponse(int clientFd, bool loggedIn)
{
    // 消息头：正文长度 1、协议版本 1、登录响应类型 0x8001。
    const unsigned char frame[] = {0, 0, 0, 1, 0, 1, 0x80, 0x01,
        static_cast<unsigned char>(loggedIn ? 1 : 0)};
    std::size_t sent = 0;
    while (sent < sizeof(frame))
    {
        const ssize_t count = send(clientFd, frame + sent, sizeof(frame) - sent,
            MSG_NOSIGNAL | MSG_DONTWAIT);
        if (count > 0)
            sent += static_cast<std::size_t>(count);
        else if (count < 0 && errno == EINTR)
            continue;
        else
            return false;
    }
    return true;
}