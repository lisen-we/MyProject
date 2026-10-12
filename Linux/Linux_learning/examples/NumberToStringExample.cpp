#include "NumberToStringExample.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

void num_to_str_fun()
{
    // 函数名沿用学习示例；ecvt、fcvt、gcvt 的实际功能是将浮点数转换为字符串。
    int decimal_point = 0;
    int sign = 0;

    // ecvt 的第二个参数是有效数字位数；decimal_point 表示小数点应插入的位置。
    std::cout << "ecvt(0.123456789, 10): "
              << ::ecvt(0.123456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';
    std::cout << "ecvt(0.0123456789, 10): "
              << ::ecvt(0.0123456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';
    std::cout << "ecvt(-123.456789, 10): "
              << ::ecvt(-123.456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';
    std::cout << "ecvt(100.0, 4): "
              << ::ecvt(100.0, 4, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';
    std::cout << "ecvt(0.0, 5): "
              << ::ecvt(0.0, 5, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';
    std::cout << "ecvt(9.9999, 3): "
              << ::ecvt(9.9999, 3, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    std::cout << "----------------------------------------\n";

    // fcvt 的第二个参数是小数点后的位数，返回结果不包含小数点。
    std::cout << "fcvt(0.123456789, 10): " << ::fcvt(0.123456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';
    std::cout << "fcvt(0.0123456789, 10): " << ::fcvt(0.0123456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';
    std::cout << "fcvt(-123.456789, 10): " << ::fcvt(-123.456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';
    std::cout << "fcvt(100.0, 3): " << ::fcvt(100.0, 3, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';
    std::cout << "fcvt(12.34567, 2): " << ::fcvt(12.34567, 2, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';
    std::cout << "fcvt(-0.0, 3): " << ::fcvt(-0.0, 3, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    std::cout << "----------------------------------------\n";
    char buffer[128] = {};
    // gcvt 将包含小数点和负号的可读字符串写入调用者提供的缓冲区。
    std::cout << "gcvt(0.123456789, 10): " << ::gcvt(0.123456789, 10, buffer) << '\n';
    std::cout << "gcvt(0.0123456789, 10): " << ::gcvt(0.0123456789, 10, buffer) << '\n';
    std::cout << "gcvt(-123.456789, 10): " << ::gcvt(-123.456789, 10, buffer) << '\n';
    std::cout << "gcvt(100.0, 10): " << ::gcvt(100.0, 10, buffer) << '\n';
    std::cout << "gcvt(123456789.0, 4): " << ::gcvt(123456789.0, 4, buffer) << '\n';
    std::cout << "gcvt(0.000000012345, 6): " << ::gcvt(0.000000012345, 6, buffer) << '\n';

    // ecvt/fcvt 返回静态缓冲区；gcvt 使用调用者提供的 buffer，结果都应及时复制。
    std::cout << "----------------------------------------\n";
    std::cout << "std::to_string(-123.456789): " << std::to_string(-123.456789) << '\n';
    std::ostringstream formatted_number;
    formatted_number << std::fixed << std::setprecision(3) << 12.34567;
    std::cout << "ostringstream fixed(3) for 12.34567: " << formatted_number.str() << '\n';
}
