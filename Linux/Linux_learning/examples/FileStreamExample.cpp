#include "FileStreamExample.h"

#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <type_traits>
#include <unistd.h>

namespace
{
// fwrite/fread 直接读写对象的内存字节；因此示例类型必须是没有指针、std::string、容器或虚函数的固定布局记录。
struct StudentRecord
{
    int id;
    char name[32];
    int age;
};

static_assert(std::is_trivially_copyable<StudentRecord>::value,
              "fwrite/fread demo requires a trivially copyable type");
} // namespace

void file_stream_fun()
{
    const char* stdio_file_name = "/home/lisen/fopen_demo.txt";
    const char* descriptor_file_name = "/home/lisen/fdopen_demo.txt";
    const char* binary_file_name = "/home/lisen/student_record.bin";

    std::cout << "\n--- fopen / fileno example ---\n";
    // fopen(filename, mode)：filename 是文件路径；"w+" 表示读写打开、文件不存在则创建、已有内容会被清空。
    std::FILE* stream = std::fopen(stdio_file_name, "w+");
    if (stream == nullptr)
    {
        std::perror("fopen fopen_demo.txt failed");
        return;
    }
    printf("%s(%d):%s\n", __FILE__, __LINE__, __FUNCTION__);
    const char* stdio_text = "text written through FILE*\n";
    if (std::fputs(stdio_text, stream) == EOF)
    {
        std::perror("fputs fopen_demo.txt failed");
        std::fclose(stream);
        return;
    }

    // FILE* 使用用户态缓冲；fflush() 先把缓冲内容交给内核，便于后续通过 fd 观察文件状态。
    if (std::fflush(stream) != 0)
    {
        std::perror("fflush fopen_demo.txt failed");
        std::fclose(stream);
        return;
    }

    // ftell(stream)：返回当前 FILE* 的逻辑文件位置，单位是字节；失败时返回 -1L。
    // 刚写入一行文本后，位置通常等于已写入的字节数。
    const long position_after_write = std::ftell(stream);
    if (position_after_write == -1L)
    {
        std::perror("ftell after writing fopen_demo.txt failed");
        std::fclose(stream);
        return;
    }
    std::cout << "ftell after writing: " << position_after_write << " bytes\n";

    // fgetpos(stream, &position)：将当前位置保存到 fpos_t 对象。fpos_t 是不透明类型，不能直接按整数输出。
    // 相比只保存字节偏移量，fpos_t 还能表达某些文本流的多字节转换状态，应通过 fsetpos() 恢复。
    std::fpos_t position_after_write_object = {};
    if (std::fgetpos(stream, &position_after_write_object) != 0)
    {
        std::perror("fgetpos after writing fopen_demo.txt failed");
        std::fclose(stream);
        return;
    }
    std::cout << "fgetpos saved the position after writing\n";

    // fileno(stream) 返回 FILE* 底层使用的文件描述符；返回 -1 表示获取失败。
    const int descriptor_from_stream = ::fileno(stream);
    if (descriptor_from_stream == -1)
    {
        std::perror("fileno failed");
        std::fclose(stream);
        return;
    }
    std::cout << "fopen returned FILE*, fileno(stream) returned fd="
              << descriptor_from_stream << '\n';

    // fseek(stream, 0, SEEK_SET) 将流位置移动到开头，再使用 fgets() 从 FILE* 读取刚写入的文本。
    if (std::fseek(stream, 0, SEEK_SET) != 0)
    {
        std::perror("fseek fopen_demo.txt failed");
        std::fclose(stream);
        return;
    }
    const long position_after_seek = std::ftell(stream);
    if (position_after_seek == -1L)
    {
        std::perror("ftell after fseek fopen_demo.txt failed");
        std::fclose(stream);
        return;
    }
    std::cout << "ftell after fseek(..., 0, SEEK_SET): " << position_after_seek << " bytes\n";

    // fsetpos(stream, &position)：恢复先前由 fgetpos() 保存的位置。
    if (std::fsetpos(stream, &position_after_write_object) != 0)
    {
        std::perror("fsetpos fopen_demo.txt failed");
        std::fclose(stream);
        return;
    }
    const long position_after_restore = std::ftell(stream);
    if (position_after_restore == -1L)
    {
        std::perror("ftell after fsetpos fopen_demo.txt failed");
        std::fclose(stream);
        return;
    }
    std::cout << "fsetpos restored position: " << position_after_restore << " bytes\n";

    // 为继续演示 fgets()，再次将位置移动回文件开头。
    if (std::fseek(stream, 0, SEEK_SET) != 0)
    {
        std::perror("second fseek fopen_demo.txt failed");
        std::fclose(stream);
        return;
    }

    char buffer[128] = {};
    if (std::fgets(buffer, sizeof(buffer), stream) == nullptr)
    {
        if (std::ferror(stream) != 0)
        {
            std::perror("fgets fopen_demo.txt failed");
        }
        else
        {
            std::cout << "fgets reached EOF before reading text\n";
        }
        std::fclose(stream);
        return;
    }
    std::cout << "read through FILE*: " << buffer;

    // fclose() 会关闭 FILE*，同时也会关闭 fileno() 返回的底层文件描述符。
    if (std::fclose(stream) != 0)
    {
        std::perror("fclose fopen_demo.txt failed");
        return;
    }

    std::cout << "\n--- open / fdopen example ---\n";
    // open() 返回 fd；O_RDWR 与 fdopen() 的 "w+" 读写需求兼容，O_TRUNC 用于清空旧测试内容。
    const int descriptor = ::open(descriptor_file_name, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (descriptor == -1)
    {
        std::perror("open fdopen_demo.txt failed");
        return;
    }

    // fdopen(fd, mode)：将已打开的 fd 包装为 FILE*；mode 必须与 fd 的访问方式兼容。
    std::FILE* descriptor_stream = ::fdopen(descriptor, "w+");
    if (descriptor_stream == nullptr)
    {
        std::perror("fdopen failed");
        ::close(descriptor);
        return;
    }

    const char* descriptor_text = "text written after fdopen\n";
    if (std::fputs(descriptor_text, descriptor_stream) == EOF || std::fflush(descriptor_stream) != 0)
    {
        std::perror("write through fdopen stream failed");
        std::fclose(descriptor_stream);
        return;
    }

    if (std::fseek(descriptor_stream, 0, SEEK_SET) != 0)
    {
        std::perror("fseek fdopen_demo.txt failed");
        std::fclose(descriptor_stream);
        return;
    }
    std::memset(buffer, 0, sizeof(buffer));
    if (std::fgets(buffer, sizeof(buffer), descriptor_stream) != nullptr)
    {
        std::cout << "read after fdopen: " << buffer;
    }
    else if (std::ferror(descriptor_stream) != 0)
    {
        std::perror("fgets fdopen_demo.txt failed");
    }
    else
    {
        std::cout << "fgets reached EOF before reading text\n";
    }

    // fdopen() 成功后，FILE* 接管 descriptor 的关闭责任；只能调用 fclose()，不能再额外 close(descriptor)。
    if (std::fclose(descriptor_stream) != 0)
    {
        std::perror("fclose fdopen_demo.txt failed");
    }

    std::cout << "\n--- fwrite / fread structured record example ---\n";
    // "wb"：以二进制写入方式打开；文件不存在时创建，已有内容会被清空。
    std::FILE* binary_stream = std::fopen(binary_file_name, "wb");
    if (binary_stream == nullptr)
    {
        std::perror("fopen student_record.bin for writing failed");
        return;
    }

    const StudentRecord written_record = {1000, "tiger", 28};
    // fwrite(ptr, size, count, stream)：
    // ptr=&written_record 是待写对象的首地址；size=sizeof(StudentRecord) 是一个对象的字节数；
    // count=1 表示写入一个对象；返回值是“成功写入的对象数量”，而不是字节数。
    const std::size_t written_count = std::fwrite(&written_record, sizeof(StudentRecord), 1, binary_stream);
    if (written_count != 1)
    {
        if (std::ferror(binary_stream) != 0)
        {
            std::perror("fwrite student_record.bin failed");
        }
        else
        {
            std::cout << "fwrite wrote an incomplete record\n";
        }
        std::fclose(binary_stream);
        return;
    }
    std::cout << "fwrite wrote " << written_count << " record, "
              << sizeof(StudentRecord) << " bytes per record\n";

    if (std::fclose(binary_stream) != 0)
    {
        std::perror("fclose student_record.bin after writing failed");
        return;
    }

    // "rb"：以二进制只读方式打开；二进制模式避免文本模式可能发生的换行转换。
    binary_stream = std::fopen(binary_file_name, "rb");
    if (binary_stream == nullptr)
    {
        std::perror("fopen student_record.bin for reading failed");
        return;
    }

    StudentRecord read_record = {};
    // fread(ptr, size, count, stream) 的参数含义与 fwrite 对称；返回值同样是成功读取的对象数量。
    const std::size_t read_count = std::fread(&read_record, sizeof(StudentRecord), 1, binary_stream);
    if (read_count != 1)
    {
        if (std::ferror(binary_stream) != 0)
        {
            std::perror("fread student_record.bin failed");
        }
        else if (std::feof(binary_stream) != 0)
        {
            std::cout << "fread reached EOF before reading a complete record\n";
        }
        std::fclose(binary_stream);
        return;
    }

    std::cout << "fread read " << read_count << " record: id=" << read_record.id
              << ", name=" << read_record.name << ", age=" << read_record.age << '\n';

    // 文件中只有一条记录；第二次读取会返回 0，并将 EOF 状态置位。
    StudentRecord record_after_end = {};
    const std::size_t count_after_end = std::fread(&record_after_end, sizeof(StudentRecord), 1, binary_stream);
    if (count_after_end != 0)
    {
        std::cout << "unexpectedly read another record\n";
    }
    else if (std::feof(binary_stream) != 0)
    {
        // feof(stream) 返回非 0 表示此前的读操作已经到达文件末尾；它不会主动尝试读取。
        std::cout << "feof reports EOF after attempting to read past the only record\n";
    }
    else if (std::ferror(binary_stream) != 0)
    {
        std::perror("fread after record failed");
        std::fclose(binary_stream);
        return;
    }

    // clearerr(stream) 清除 EOF 标志和错误标志，但不会改变当前文件位置，也不会修复底层 I/O 错误。
    std::clearerr(binary_stream);
    std::cout << "after clearerr: feof=" << std::feof(binary_stream)
              << ", ferror=" << std::ferror(binary_stream) << '\n';

    // clearerr() 后位置仍在文件末尾，因此还需要 fseek() 回到开头，才能重新读取。
    if (std::fseek(binary_stream, 0, SEEK_SET) != 0)
    {
        std::perror("fseek student_record.bin after clearerr failed");
        std::fclose(binary_stream);
        return;
    }
    StudentRecord reread_record = {};
    if (std::fread(&reread_record, sizeof(StudentRecord), 1, binary_stream) != 1)
    {
        std::perror("fread student_record.bin after clearerr failed");
        std::fclose(binary_stream);
        return;
    }
    std::cout << "read again after clearerr and fseek: id=" << reread_record.id
              << ", name=" << reread_record.name << ", age=" << reread_record.age << '\n';

    if (std::fclose(binary_stream) != 0)
    {
        std::perror("fclose student_record.bin after reading failed");
        return;
    }

    // 注意：这种“内存原样读写”只适合本程序、相同编译器/平台下的平凡可复制类型。
    // std::string、std::vector、指针、含虚函数的类含有地址或运行时状态，不能直接 fwrite/fread；
    // 跨版本、跨平台或长期存档的数据应使用字段序列化格式，并处理字节序、版本和长度。
}
