#include <cerrno>
#include <climits>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

void type_conversion_fun()
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

        // std::stoll 遇到无法表示的大整数会抛出 std::out_of_range。
        static_cast<void>(std::stoll("999999999999999999999999"));
    }
    catch (const std::exception& error)
    {
        std::cout << "C++ conversion exception: " << error.what() << '\n';
    }
}

void str_to_num_fun()
{
    // 函数名沿用学习示例；ecvt、fcvt、gcvt 的实际功能是将浮点数转换为字符串。
    int decimal_point = 0;
    int sign = 0;

    // ecvt 的第二个参数是有效数字位数；decimal_point 表示小数点应插入的位置。
    std::cout << "ecvt(0.123456789, 10): "
              << ::ecvt(0.123456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 小于 1 的正数会得到负的 decimal_point，表示小数点在首个数字的左侧。
    std::cout << "ecvt(0.0123456789, 10): "
              << ::ecvt(0.0123456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 负数的 sign 为 1；返回字符串本身不包含负号。
    std::cout << "ecvt(-123.456789, 10): "
              << ::ecvt(-123.456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 整数的 decimal_point 等于整数部分的位数。
    std::cout << "ecvt(100.0, 4): "
              << ::ecvt(100.0, 4, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 0 的 decimal_point 为 1；这有助于将结果字符串还原成 0.0000 等形式。
    std::cout << "ecvt(0.0, 5): "
              << ::ecvt(0.0, 5, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 有效数字不足以完整表示时，ecvt 会按当前 C 运行库的规则进行舍入。
    std::cout << "ecvt(9.9999, 3): "
              << ::ecvt(9.9999, 3, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    std::cout << "----------------------------------------\n";

    // fcvt 的第二个参数是小数点后的位数，返回结果不包含小数点。
    std::cout << "fcvt(0.123456789, 10): "
              << ::fcvt(0.123456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 验证 fcvt 会保留指定的小数位，即使数值非常接近 0。
    std::cout << "fcvt(0.0123456789, 10): "
              << ::fcvt(0.0123456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 验证 fcvt 对负数同样通过 sign 返回符号信息。
    std::cout << "fcvt(-123.456789, 10): "
              << ::fcvt(-123.456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 整数使用 fcvt 时仍会按指定的小数位补零。
    std::cout << "fcvt(100.0, 3): "
              << ::fcvt(100.0, 3, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 指定两位小数会触发舍入，12.34567 的转换结果可用来观察这一行为。
    std::cout << "fcvt(12.34567, 2): "
              << ::fcvt(12.34567, 2, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 负零仍可能通过 sign 保留符号信息，便于处理科学计算中的符号位。
    std::cout << "fcvt(-0.0, 3): "
              << ::fcvt(-0.0, 3, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    std::cout << "----------------------------------------\n";

    char buffer[128] = {};
    // gcvt 将包含小数点和负号的可读字符串写入调用者提供的缓冲区。
    std::cout << "gcvt(0.123456789, 10): " << ::gcvt(0.123456789, 10, buffer) << '\n';
    std::cout << "gcvt(0.0123456789, 10): " << ::gcvt(0.0123456789, 10, buffer) << '\n';
    std::cout << "gcvt(-123.456789, 10): " << ::gcvt(-123.456789, 10, buffer) << '\n';
    std::cout << "gcvt(100.0, 10): " << ::gcvt(100.0, 10, buffer) << '\n';
    // 大数和极小数通常会使用科学计数法，以避免生成过长的字符串。
    std::cout << "gcvt(123456789.0, 4): " << ::gcvt(123456789.0, 4, buffer) << '\n';
    std::cout << "gcvt(0.000000012345, 6): " << ::gcvt(0.000000012345, 6, buffer) << '\n';

    // ecvt/fcvt 返回由 C 库维护的静态缓冲区，下一次调用会覆盖上一次结果；应立即使用或复制。
    // gcvt 使用调用者提供的 buffer，buffer 必须足够大，且下一次 gcvt 调用会覆盖该 buffer 的内容。
    std::cout << "----------------------------------------\n";
    std::cout << "std::to_string(-123.456789): " << std::to_string(-123.456789) << '\n';

    // ostringstream 是 C++ 中控制小数位数的常用方式，适合替代旧式 fcvt/gcvt。
    std::ostringstream formatted_number;
    formatted_number << std::fixed << std::setprecision(3) << 12.34567;
    std::cout << "ostringstream fixed(3) for 12.34567: " << formatted_number.str() << '\n';
}

int main()
{
    type_conversion_fun();
    str_to_num_fun();
    return 0;
}
