#pragma once
// 基于 epoll 的 TCP/UDP 聊天室。
// mode："s" 启动服务端，"c" 启动客户端；protocol："TCP" 或 "UDP"。
void MyEchoServer(const char* mode, const char* protocol);
