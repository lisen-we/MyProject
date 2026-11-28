#pragma once
bool SendCreateUserResponse(int clientFd, int epollFd, bool registered);
bool SendUserLoginResponse(int clientFd, int epollFd, bool loggedIn);
#include <cstdint>
#include <string>
bool FlushClientSendBuffer(int clientFd, int epollFd);
bool SendGroupChatMessage(int clientFd, int epollFd, std::uint32_t roomId,
    const std::string& account, const std::string& text);