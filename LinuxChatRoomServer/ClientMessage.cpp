#include "ClientMessage.h"
#include <arpa/inet.h>
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <utility>

std::unordered_map<int, clientSession> sessions;

constexpr int clientMessageHeaderLength = 8;
bool AppendClientMessage(int client_fd,char* message,int size)
{
	if (message == nullptr || size <= 0)
		return false;
	clientSession &buffer = sessions.at(client_fd);
	buffer.receiveBuffer.append(message,size);
	if (buffer.receiveBuffer.length() < clientMessageHeaderLength)
	{
		return false;
	}
	std::uint32_t bufferLeft8;
	std::memcpy(&bufferLeft8, buffer.receiveBuffer.data(), 4);
	int dataLength = ntohl(bufferLeft8);
	return buffer.receiveBuffer.length()- clientMessageHeaderLength >= dataLength;
}
namespace
{
    // 按字节读取大端整数，避免未对齐访问，也适用于 64 位字段。
    template <typename Integer>
    Integer ReadBigEndian(const char* bytes)
    {
        Integer value = 0;
        for (std::size_t i = 0; i < sizeof(Integer); ++i)
            value = static_cast<Integer>((value << 8) |
                static_cast<unsigned char>(bytes[i]));
        return value;
    }
}

bool ProcessClientMessage(int client_fd)
{
    clientSession& session = sessions.at(client_fd);
    std::string& buffer = session.receiveBuffer;

    // 消息头未收齐，保留所有字节，等待后续接收。
    if (buffer.size() < ClientMessageHeaderSize)
        return false;

    ParsedClientMessage parsed;
    parsed.header.bodyLength = ReadBigEndian<std::uint32_t>(buffer.data());
    parsed.header.version = ReadBigEndian<std::uint16_t>(buffer.data() + 4);
    parsed.header.type = static_cast<ClientCommandType>(
        ReadBigEndian<std::uint16_t>(buffer.data() + 6));

    if (parsed.header.version != ClientProtocolVersion)
        throw std::runtime_error("Unsupported protocol version");
    if (parsed.header.bodyLength > MaxClientMessageBodySize)
        throw std::runtime_error("Message body is too large");

    // 根据类型先检查正文最小长度，防止随后读取字段越界。
    const std::size_t bodySize = parsed.header.bodyLength;
    switch (parsed.header.type)
    {
    case ClientCommandType::LoginRequest:
        if (bodySize < 2)
            throw std::runtime_error("Login body is too short");
        break;
    case ClientCommandType::GroupChatRequest:
        if (bodySize < 8)
            throw std::runtime_error("Chat body is too short");
        break;
    case ClientCommandType::FileChunk:
        if (bodySize < 16 || bodySize - 16 > MaxFileChunkDataSize)
            throw std::runtime_error("Invalid file chunk size");
        break;
    default:
        throw std::runtime_error("Unknown command type");
    }

    const std::size_t frameSize = ClientMessageHeaderSize + bodySize;
    if (buffer.size() < frameSize)
        return false;

    // 正文只覆盖当前帧，不包含后续帧。所有字段使用明确的字节长度。
    const char* body = buffer.data() + ClientMessageHeaderSize;
    switch (parsed.header.type)
    {
    case ClientCommandType::LoginRequest:
    {
        const std::uint16_t length = ReadBigEndian<std::uint16_t>(body);
        if (length != bodySize - 2)
            throw std::runtime_error("Invalid nickname length");
        parsed.login.nickname.assign(body + 2, length);
        break;
    }
    case ClientCommandType::GroupChatRequest:
    {
        parsed.groupChat.roomId = ReadBigEndian<std::uint32_t>(body);
        const std::uint32_t length = ReadBigEndian<std::uint32_t>(body + 4);
        if (length != bodySize - 8)
            throw std::runtime_error("Invalid chat text length");
        parsed.groupChat.text.assign(body + 8, length);
        break;
    }
    case ClientCommandType::FileChunk:
        parsed.fileChunk.transferId = ReadBigEndian<std::uint64_t>(body);
        parsed.fileChunk.offset = ReadBigEndian<std::uint64_t>(body + 8);
        parsed.fileChunk.data.assign(body + 16, body + bodySize);
        break;
    default:
        throw std::runtime_error("Unknown command type");
    }

    // 全部解析成功后才提交结果并移除当前帧，后续消息留在缓冲区。
    // 登录权限、UTF-8 有效性及文件任务状态等校验交给业务层处理。
    session.parsedMessage = std::move(parsed);
    buffer.erase(0, frameSize);
    return true;
}
bool HandleLoginRequest(const ParsedClientMessage& parsed)
{

}
bool HandleChatRequest(const ParsedClientMessage& parsed)
{

}
bool HandleFileSendRequest(const ParsedClientMessage& parsed)
{

}