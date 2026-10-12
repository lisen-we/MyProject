#include "StringToNumberExample.h"

#include <cerrno>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

void str_to_num_fun()
{
    long long_value = 0;
    int int_value = 0;

    // 验证当前 Linux 目标平台的数据模型，不要假设各类型的字节数。
    std::cout << "sizeof(long): " << sizeof(long_value)
              << ", sizeof(int): " << sizeof(int_value)
              << ", sizeof(long long): " << sizeof(long long)
              << ", sizeof(long double): " << sizeof(long double) << '\n';

    // atoi、atol、atoll 和 atof 仅演示基础转换；它们无法区分错误输入和合法的 0。
    std::cout << "atoi(\"123456\"): " << std::atoi("123456") << '\n';
    std::cout << "atol(\"234567\"): " << std::atol("234567") << '\n';
    std::cout << "atoll(\"1234567890123\"): " << std::atoll("1234567890123") << '\n';
    std::cout << "atof(\"1.234567\"): " << std::atof("1.234567") << '\n';

    // strtol 通过 end_ptr 返回未转换的后缀，并通过 errno 报告数值范围错误。
    const char* long_text = "-987654321";
    char* end_ptr = nullptr;
    errno = 0;
    const long long_result = std::strtol(long_text, &end_ptr, 10);
    std::cout << "strtol(\"" << long_text << "\", base 10): " << long_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno
              << (errno == ERANGE ? " (range error)" : "") << '\n';

    // abc 后缀未被转换，验证 strtoll 会在遇到第一个非数字字符时停止。
    const char* long_long_text = "-987654321000abc123456";
    errno = 0;
    const long long long_long_result = std::strtoll(long_long_text, &end_ptr, 10);
    std::cout << "strtoll(\"" << long_long_text << "\", base 10): " << long_long_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno
              << (errno == ERANGE ? " (range error)" : "") << '\n';

    // base 为 0 时会根据前缀自动识别进制：0x 表示十六进制，前导 0 表示八进制。
    const char* hex_text = "0x7f";
    errno = 0;
    const long hex_result = std::strtol(hex_text, &end_ptr, 0);
    std::cout << "strtol(\"" << hex_text << "\", base 0): " << hex_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno << '\n';

    const char* octal_text = "0755";
    errno = 0;
    const long octal_result = std::strtol(octal_text, &end_ptr, 0);
    std::cout << "strtol(\"" << octal_text << "\", base 0): " << octal_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno << '\n';

    // 指定 base 为 2 时，字符串按二进制解释；strtol 会自动忽略开头的空白和正负号。
    const char* binary_text = "  +101101";
    errno = 0;
    const long binary_result = std::strtol(binary_text, &end_ptr, 2);
    std::cout << "strtol(\"" << binary_text << "\", base 2): " << binary_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno << '\n';

    // end_ptr 与输入首地址相同，表示输入中没有任何可转换的数字。
    const char* invalid_integer_text = "abc";
    errno = 0;
    const long invalid_integer_result = std::strtol(invalid_integer_text, &end_ptr, 10);
    std::cout << "strtol(\"" << invalid_integer_text << "\", base 10): " << invalid_integer_result
              << ", no digits: " << (end_ptr == invalid_integer_text)
              << ", errno: " << errno << '\n';

    // 超出 long long 范围时，返回边界值并设置 errno 为 ERANGE。
    const char* overflow_text = "9223372036854775808";
    errno = 0;
    const long long overflow_result = std::strtoll(overflow_text, &end_ptr, 10);
    std::cout << "strtoll(\"" << overflow_text << "\", base 10): " << overflow_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno
              << (errno == ERANGE ? " (range error)" : "") << '\n';

    // 使用同一文本分别验证转换为 float、double 和 long double 的精度差异。
    const char* floating_text = "1.234567890123";
    errno = 0;
    const float float_result = std::strtof(floating_text, &end_ptr);
    std::cout << std::setprecision(16)
              << "strtof(\"" << floating_text << "\"): " << float_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno << '\n';

    errno = 0;
    const double double_result = std::strtod(floating_text, &end_ptr);
    std::cout << "strtod(\"" << floating_text << "\"): " << double_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno << '\n';

    errno = 0;
    const long double long_double_result = std::strtold(floating_text, &end_ptr);
    std::cout << "strtold(\"" << floating_text << "\"): " << long_double_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno << '\n';

    // strtod 支持科学计数法；ms 后缀留在 remaining 中，便于继续解析单位。
    const char* scientific_text = "6.02e23ms";
    errno = 0;
    const double scientific_result = std::strtod(scientific_text, &end_ptr);
    std::cout << std::scientific << "strtod(\"" << scientific_text << "\"): " << scientific_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno << std::defaultfloat << '\n';

    // C++ 标准库的 stoi/stod 以异常报告错误，position 返回成功转换的字符数量。
    try
    {
        std::size_t position = 0;
        const int cpp_integer_result = std::stoi("0x2aXYZ", &position, 0);
        std::cout << "stoi(\"0x2aXYZ\", base 0): " << cpp_integer_result
                  << ", parsed length: " << position << ", remaining: \"XYZ\"\n";

        position = 0;
        const double cpp_double_result = std::stod("3.14159rad", &position);
        std::cout << "stod(\"3.14159rad\"): " << cpp_double_result
                  << ", parsed length: " << position << ", remaining: \"rad\"\n";

        static_cast<void>(std::stoll("999999999999999999999999"));
    }
    catch (const std::exception& error)
    {
        std::cout << "C++ conversion exception: " << error.what() << '\n';
    }
}
