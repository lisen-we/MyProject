#include "ClientMessage.h"
#include <arpa/inet.h>
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <hiredis/hiredis.h>
#include <sodium.h>

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
    case ClientCommandType::CreateUserRequest:
        if (bodySize < 6)
            throw std::runtime_error("Registration body is too short");
        break;
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
    case ClientCommandType::CreateUserRequest:
    {
        std::size_t offset = 0;
        auto readField = [&](std::string& field)
        {
            if (bodySize - offset < 2)
                throw std::runtime_error("Missing registration field length");
            const std::size_t length = ReadBigEndian<std::uint16_t>(body + offset);
            offset += 2;
            if (length > bodySize - offset)
                throw std::runtime_error("Invalid registration field length");
            field.assign(body + offset, length);
            offset += length;
        };
        readField(parsed.createUser.account);
        readField(parsed.createUser.password);
        readField(parsed.createUser.nickname);
        if (offset != bodySize)
            throw std::runtime_error("Unexpected registration data");
        break;
    }
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
bool HandleCreateUser(const ParsedClientMessage& parsed)
{
    const auto& user = parsed.createUser;
    // 限制输入大小，账号采用简单的 ASCII 字符集。
    if (user.account.empty() || user.account.size() > 64 ||
        user.password.size() < 8 || user.password.size() > 1024 ||
        user.nickname.empty() || user.nickname.size() > 128)
        return false;
    for (unsigned char ch : user.account)
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') || ch == '_' || ch == '-'))
            return false;
    //初始化libsodium密码学库
    if (sodium_init() < 0)
        return false;
    char passwordHash[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str_alg(
        passwordHash,                     // 输出：生成的哈希字符串
        user.password.data(),             // 输入：用户密码的字节
        user.password.size(),             // 输入：密码的字节数
        crypto_pwhash_OPSLIMIT_INTERACTIVE, // 计算量参数
        crypto_pwhash_MEMLIMIT_INTERACTIVE, // 内存使用量参数
        crypto_pwhash_ALG_ARGON2ID13       // 使用 Argon2id 算法
        ) != 0)
        return false;

    timeval timeout{1, 0}; //表示的值为1秒+0微妙
    redisContext* context = redisConnectWithTimeout("127.0.0.1", 6379, timeout);
    if (!context || context->err)
    {
        if (context) redisFree(context);
        return false;
    }
    if (redisSetTimeout(context, timeout) != REDIS_OK)
    {
        redisFree(context);
        return false;
    }
    // Lua 脚本在 Redis 内原子执行，避免并发注册覆盖已有用户。
    const char* script = R"(
        if redis.call('EXISTS', KEYS[1]) == 1 then
            return 0
        end

        redis.call('HSET', KEYS[1],
            'account', ARGV[1],
            'nickname', ARGV[2],
            'password_hash', ARGV[3])

        return 1
    )";
    const std::string key = "user:" + user.account;
    //redisCommand 会把C++中的值按顺序传递给Lua脚本
    /*EVAL% s    1 % b % b % b % s
        脚本  数量  键名     账号     昵称     密码哈希
        KEYS[1]  ARGV[1]  ARGV[2]  ARGV[3]*/
    //%b指的是要求按照指定的字节长度传递数据
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(context,
            "EVAL %s 1 %b %b %b %s",
            script,
            key.data(), key.size(),
            user.account.data(), user.account.size(),
            user.nickname.data(), user.nickname.size(),
            passwordHash)
        );
    const bool success =
        reply &&  //检查是否收到了响应
        reply->type == REDIS_REPLY_INTEGER &&  //检查收到的响应是否为整数
        reply->integer == 1;  //收到的响应是否为1
    if (reply) freeReplyObject(reply);
    redisFree(context);
    return success;
}