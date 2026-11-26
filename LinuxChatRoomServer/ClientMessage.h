#pragma once
#include <unordered_map>
#include <string>
#include <cstddef>
#include <cstdint>
#include <vector>

// 网络整数使用大端序，文本使用 UTF-8 编码。
enum class ClientCommandType : std::uint16_t
{
    LoginRequest = 1,
    GroupChatRequest = 2,
    FileChunk = 3,
    CreateUserRequest = 4
};

constexpr std::uint16_t ClientProtocolVersion = 1;
constexpr std::size_t ClientMessageHeaderSize = 8;
constexpr std::uint32_t MaxClientMessageBodySize = 256u * 1024u;
constexpr std::size_t MaxFileChunkDataSize = 64u * 1024u;

// 消息头布局：正文长度（4 字节）、版本（2 字节）、类型（2 字节）。
// 正文长度不包含 8 字节消息头。
// 结构体只保存解析结果，不能直接发送其内存。
struct ClientMessageHeader
{
    std::uint32_t bodyLength = 0;
    std::uint16_t version = ClientProtocolVersion;
    ClientCommandType type = ClientCommandType::LoginRequest;
};

// 登录正文：账号和密码，分别由 2 字节大端长度和 UTF-8 字节组成。
struct LoginRequestBody
{
    std::string account;
    std::string password;
};

// 注册正文依次为账号、密码、昵称，每项由 2 字节大端长度和 UTF-8 字节组成。
struct CreateUserRequestBody
{
    std::string account;
    std::string password;
    std::string nickname;
};

// 群聊正文：聊天室 ID（4 字节）、文本长度（4 字节）和文本字节。
struct GroupChatRequestBody
{
    std::uint32_t roomId = 0;
    std::string text;
};

// 文件正文：传输 ID（8 字节）、文件偏移（8 字节）和文件字节。
// 文件数据长度等于正文长度减去 16，最多 64 KiB。
// 文件偏移按原始文件字节数计算，从零开始。
// 接收文件分块前，业务层需要先创建对应的文件传输任务。
struct FileChunkBody
{
    std::uint64_t transferId = 0;
    std::uint64_t offset = 0;
    std::vector<std::uint8_t> data;
};

// 根据 header.type 读取对应的正文成员，其他成员保持默认值。
struct ParsedClientMessage
{
    ClientMessageHeader header;
    LoginRequestBody login;
    CreateUserRequestBody createUser;
    GroupChatRequestBody groupChat;
    FileChunkBody fileChunk;
};

struct clientSession
{
    bool loggedIn = false;
    std::string account;
	std::string receiveBuffer;
    // 每次成功解析后覆盖；业务层应在下一次解析前处理或复制该结果。
    ParsedClientMessage parsedMessage;
	clientSession()
	{
		receiveBuffer.reserve(4 * 1024);
	}
};
extern std::unordered_map<int, clientSession> sessions;
bool AppendClientMessage(int client_fd, char* message,int size);
bool ProcessClientMessage(int client_fd);
bool HandleCreateUser(const ParsedClientMessage& parsed);
bool HandleLoginRequest(const ParsedClientMessage& parsed);
bool HandleChatRequest(const ParsedClientMessage& parsed);
bool HandleFileSendRequest(const ParsedClientMessage& parsed);
