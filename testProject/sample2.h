#pragma once
#include <iostream>
using std::cout;
using std::endl;
struct sample2_B;
//前向声明，因为shared_ptr主要保存的是指针，所以对于A来说
//只需要知道sample2_B是struct就行了，如果A中使用的是B p
//就必须要知道sample2_B的完整定义了
struct sample2_A
{
	std::shared_ptr<sample2_B> pb;
	~sample2_A()
	{
		cout << "A已经被销毁！" << endl;
	}
};
struct sample2_B
{
	std::shared_ptr<sample2_A> pa;
	~sample2_B()
	{
		cout << "B已经被销毁" << endl;
	}
};