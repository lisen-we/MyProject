#include "ScanfExample.h"

#include "VariadicExample.h"

#include <cstdio>

void scanf_fun()
{
    char c;
    int d;
    double f;
    // 输入是空格可有可无（整数和小数之间必须有分隔）；若格式串使用其他分隔符，输入也必须匹配。
    std::scanf("%c%d%f", &c, &d, &f);
    stdarg_test("inputchar = %c inputint = %d inputdouble = %lf", c, d, f);
}
