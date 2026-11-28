#include "ServerMessage.h"
#include "ClientMessage.h"
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <cstdio>
#include <vector>
#include <cstdint>
namespace
{
    // 限制每个慢客户端积压的数据量，防止无限增长。
    constexpr std::size_t MaxPendingSendBytes = 4u * 1024u * 1024u;

    bool QueueClientFrame(int clientFd, int epollFd, const char* data, std::size_t size)
    {
        auto it = sessions.find(clientFd);
        if (it == sessions.end()) return false;
        clientSession& session = it->second;
        //pending 表示的是缓冲区中剩余的需要发送的字节数量
        const std::size_t pending = session.sendBuffer.size() - session.sendOffset;
        //当前的函数执行逻辑是将size字节的数据追加到发送缓冲区中，所以当追加的数据大于
        //最大的剩余空间的时候返回错误
        if (size > MaxPendingSendBytes - pending) return false;
        if (session.sendOffset != 0)
        {
            session.sendBuffer.erase(0, session.sendOffset);
            session.sendOffset = 0;
        }
        //将data追加到sendBuffer后
        session.sendBuffer.append(data, size);
        return FlushClientSendBuffer(clientFd, epollFd);
    }
}

bool FlushClientSendBuffer(int clientFd, int epollFd)
{
    auto it = sessions.find(clientFd);
    if (it == sessions.end()) return false;
    clientSession& session = it->second;
    while (session.sendOffset < session.sendBuffer.size())
    {
        //从offset往后开始发送数据
        const ssize_t count = send(clientFd, session.sendBuffer.data() + session.sendOffset,
            session.sendBuffer.size() - session.sendOffset, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (count > 0)
            session.sendOffset += static_cast<std::size_t>(count);
        else if (count < 0 && errno == EINTR)
            continue;
        //如果暂时无法发送数据也退出循环
        else if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            break;
        else
            return false;
    }
    // 发送缓冲区中是否还有未发送数据
    const bool pending = session.sendOffset < session.sendBuffer.size();
    if (!pending)
    {
        session.sendBuffer.clear();
        session.sendOffset = 0;
    }
    epoll_event event{};
    event.data.fd = clientFd;
    //这里的作用是更新epfd监听客户端套接字的事件
    //如果存在未发送完成的数据就更新监听事件为EPOLLOUT
    //如果发送完成了就继续监听socket的EPOLLIN
    event.events = EPOLLIN | (pending ? EPOLLOUT : 0);
    return epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &event) == 0;
}

// 群聊通知正文：房间 ID、账号长度、账号、文本长度、文本。
bool SendGroupChatMessage(int clientFd, int epollFd, std::uint32_t roomId,
    const std::string& account, const std::string& text)
{
    std::vector<unsigned char> frame;
    auto put16 = [&](std::uint16_t n) { frame.push_back(n >> 8); frame.push_back(n & 0xff); };
    auto put32 = [&](std::uint32_t n) {
        frame.push_back(n >> 24); frame.push_back((n >> 16) & 0xff);
        frame.push_back((n >> 8) & 0xff); frame.push_back(n & 0xff);
    };
    put32(static_cast<std::uint32_t>(4 + 2 + account.size() + 4 + text.size()));
    put16(1);
    put16(0x8002);
    put32(roomId);
    put16(static_cast<std::uint16_t>(account.size()));
    frame.insert(frame.end(), account.begin(), account.end());
    put32(static_cast<std::uint32_t>(text.size()));
    frame.insert(frame.end(), text.begin(), text.end());
    return QueueClientFrame(clientFd, epollFd,
        reinterpret_cast<const char*>(frame.data()), frame.size());
}

bool SendCreateUserResponse(int clientFd, int epollFd, bool registered)
{
    const unsigned char frame[] = {
        0, 0, 0, 1,  // 正文长度
        0, 1,        // 协议版本
        0x80, 0x04,  // 注册响应类型
        static_cast<unsigned char>(registered ? 1 : 0)
    };
    return QueueClientFrame(clientFd, epollFd,
        reinterpret_cast<const char*>(frame), sizeof(frame));
}
bool SendUserLoginResponse(int clientFd, int epollFd, bool loggedIn)
{
    // 消息头：正文长度 1、协议版本 1、登录响应类型 0x8001。
    const unsigned char frame[] = {0, 0, 0, 1, 0, 1, 0x80, 0x01,
        static_cast<unsigned char>(loggedIn ? 1 : 0)};
    return QueueClientFrame(clientFd, epollFd,
        reinterpret_cast<const char*>(frame), sizeof(frame));
}