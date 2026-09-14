#pragma once
#include <iostream>
//验证虚继承解决菱形继承中出现的问题
namespace test1
{
	struct A {
		int x;
		A(int v = 0) :x(v) {}
	};
	struct B :virtual A
	{
		B() :A(1) {}
	};
	struct C :virtual A
	{
		C() :A(2) {}
	};
	struct D :B, C
	{
		D() {}
	};
}
namespace test2
{
	struct A {
		int x;
		A(int v) :x(v) {}
	};
	struct B :virtual A
	{
		B() :A(1) {}
	};
	struct C :virtual A
	{
		C() :A(2) {}
	};
	struct D :B, C
	{
		//A没有默认的构造函数，编译错误
		//D() {}
		D() :A(3) {}
	};
}
namespace test3
{
	struct A {
		int x;
		A(int v) :x(v) 
		{
			std::cout << "执行了A的构造函数" << std::endl;
		}
	};
	struct B :virtual A
	{
		B() :A(1) 
		{
			std::cout << "执行了B的构造函数" << std::endl;
		}
	};
	struct C :virtual A
	{
		C() :A(2)
		{
			std::cout << "执行了C的构造函数" << std::endl;
		}
	};
	struct D :B, C
	{
		//A没有默认的构造函数，编译错误
		//D() {}
		D() :A(3) {}
	};
}