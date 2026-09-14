#include "Verification_class.h"
Verification_class::Verification_class()
{

}

Verification_class::~Verification_class()
{
}
void Verification_class::return_empty_class_size()
{
	cout << endl << std::string(20, '*') << endl;
	cout << "测试空的类所占用的空间" << endl;
	sample1 A;
	cout << "空的类占据的空间大小是: " << sizeof(A)<<endl;
	cout << std::string(20, '*') << endl << endl;
}
void Verification_class::test_smart_ptr_memory_leak()
{
	std::weak_ptr<sample2_A> observe_A;
	std::weak_ptr<sample2_B> observe_B;
	{
		auto pa = std::make_shared<sample2_A>();
		auto pb = std::make_shared<sample2_B>();

		observe_A = pa;
		observe_B = pb;

		cout << pa.use_count() << "		" << pb.use_count() << endl;
		pa->pb = pb;
		pb->pa = pa;
		cout<< pa.use_count() << "		" << pb.use_count() << endl;
	}
	cout << "离开局部作用域之后：\n";
	cout << "A use_count = " << observe_A.use_count() << '\n';
	cout << "B use_count = " << observe_B.use_count() << '\n';

	cout << std::boolalpha;
	cout << "A 是否已销毁：" << observe_A.expired() << '\n';
	cout << "B 是否已销毁：" << observe_B.expired() << '\n';
}
void Verification_class::test_new_malloc_memory()
{
	//new使用构造函数分配内存
	int* p1 = new int(5);
	cout << "Value allocated with new:" << *p1 << endl;
	delete p1;
	//malloc直接分配内存，再转换为int指针
	int* p2 = (int*)malloc(sizeof(int));
	if (p2 != nullptr)
	{
		*p2 = 10;
		cout << "Value allocated with malloc " << *p2 << endl;
		free(p2);
	}
	else {
		std::cerr << "memory allocation failed " << endl;
	}
}
void Verification_class::test_polymorphism_with_function_pointer()
{
	cout<<endl << std::string(20, '*') << endl;
	cout << "测试使用函数指针来实现多态" << endl;
	int i = sample3_polymorphism_with_function_pointer();
	cout << std::string(20, '*') << endl<<endl;
}

void Verification_class::test_virtual_inheritance()
{
	cout << endl << std::string(20, '*') << endl;
	cout << "测试虚继承" << endl;
	cout << std::boolalpha;

	// B 和 C 虚继承同一个 A，因此 D 中只有一个 A 子对象。
	test1::D first;
	test1::A* a_from_b = static_cast<test1::B*>(&first);
	test1::A* a_from_c = static_cast<test1::C*>(&first);
	cout << "经 B 找到的 A 地址: " << static_cast<const void*>(a_from_b) << endl;
	cout << "经 C 找到的 A 地址: " << static_cast<const void*>(a_from_c) << endl;
	cout << "D 中是否只有一个 A 子对象: " << (a_from_b == a_from_c) << endl;

	// 构造虚基类是最底层派生类 D 的职责。D 未显式初始化 A 时，
	// A 使用默认实参 0；B() 中的 A(1) 和 C() 中的 A(2) 均被忽略。
	cout << "test1::D 中 A::x（预期为 0）: " << first.x << endl;
	a_from_b->x = 10;
	cout << "从 B 修改 A::x 后，经 C 读取（预期为 10）: " << a_from_c->x << endl;

	// A 没有默认构造函数时，D 必须直接初始化虚基类 A。
	test2::D second;
	cout << "test2::D 直接构造 A 后的 x（预期为 3）: " << second.x << endl;

	// 输出顺序应为 A、B、C，且 A 只构造一次。
	cout << "test3::D 的构造顺序（预期 A -> B -> C，A 仅一次）:" << endl;
	test3::D third;

	const bool passed = a_from_b == a_from_c
		&& first.x == 10
		&& second.x == 3
		&& third.x == 3;
	cout << "虚继承要点验证结果: " << (passed ? "通过" : "失败") << endl;
	cout << std::string(20, '*') << endl << endl;
}
