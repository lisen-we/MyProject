#include "Verification_class.h"
#include <cstdlib>
#include <string>

void Verification_class::begin_test_case(const char* test_name) const
{
	std::cout << '\n' << std::string(60, '=') << '\n';
	std::cout << "测试用例：" << test_name << '\n';
	std::cout << std::string(60, '-') << '\n';
}

void Verification_class::end_test_case() const
{
	std::cout << std::string(60, '=') << "\n\n";
}

Verification_class::Verification_class()
{
}

Verification_class::~Verification_class()
{
}

void Verification_class::return_empty_class_size()
{
	begin_test_case("空类大小");
	sample1 empty_sample;
	std::cout << "sample1 的大小：" << sizeof(empty_sample) << " 字节\n";
	end_test_case();
}

void Verification_class::test_smart_ptr_memory_leak()
{
	begin_test_case("shared_ptr 循环引用");
	std::weak_ptr<sample2_A> observe_A;
	std::weak_ptr<sample2_B> observe_B;
	{
		auto pa = std::make_shared<sample2_A>();
		auto pb = std::make_shared<sample2_B>();

		observe_A = pa;
		observe_B = pb;
		std::cout << "建立循环前 - A use_count: " << pa.use_count()
			<< ", B use_count: " << pb.use_count() << '\n';

		pa->pb = pb;
		pb->pa = pa;
		std::cout << "建立循环后 - A use_count: " << pa.use_count()
			<< ", B use_count: " << pb.use_count() << '\n';
	}

	std::cout << "离开局部作用域后：\n";
	std::cout << "A use_count: " << observe_A.use_count() << '\n';
	std::cout << "B use_count: " << observe_B.use_count() << '\n';
	std::cout << std::boolalpha;
	std::cout << "A 是否已销毁：" << observe_A.expired() << '\n';
	std::cout << "B 是否已销毁：" << observe_B.expired() << '\n';
	std::cout << "验证结果："
		<< (!observe_A.expired() && !observe_B.expired() ? "检测到循环引用" : "未检测到循环引用") << '\n';
	end_test_case();
}

void Verification_class::test_new_malloc_memory()
{
	begin_test_case("new 与 malloc 的内存分配");
	int* new_value = new int(5);
	std::cout << "new 分配的值：" << *new_value << '\n';
	delete new_value;

	int* malloc_value = static_cast<int*>(std::malloc(sizeof(int)));
	if (malloc_value != nullptr)
	{
		*malloc_value = 10;
		std::cout << "malloc 分配的值：" << *malloc_value << '\n';
		std::free(malloc_value);
	}
	else
	{
		std::cerr << "malloc 内存分配失败\n";
	}
	end_test_case();
}

void Verification_class::test_polymorphism_with_function_pointer()
{
	begin_test_case("使用函数指针实现多态");
	const int result = sample3_polymorphism_with_function_pointer();
	std::cout << "测试函数返回值：" << result << '\n';
	end_test_case();
}

void Verification_class::test_virtual_inheritance()
{
	begin_test_case("虚继承");
	std::cout << std::boolalpha;

	// B 和 C 虚继承同一个 A，因此 D 中只有一个 A 子对象。
	test1::D first;
	test1::A* a_from_b = static_cast<test1::B*>(&first);
	test1::A* a_from_c = static_cast<test1::C*>(&first);
	std::cout << "经 B 找到的 A 地址: " << static_cast<const void*>(a_from_b) << '\n';
	std::cout << "经 C 找到的 A 地址: " << static_cast<const void*>(a_from_c) << '\n';
	std::cout << "D 中是否只有一个 A 子对象: " << (a_from_b == a_from_c) << '\n';

	// 构造虚基类是最底层派生类 D 的职责。D 未显式初始化 A 时，
	// A 使用默认实参 0；B() 中的 A(1) 和 C() 中的 A(2) 均被忽略。
	std::cout << "test1::D 中 A::x（预期为 0）: " << first.x << '\n';
	a_from_b->x = 10;
	std::cout << "从 B 修改 A::x 后，经 C 读取（预期为 10）: " << a_from_c->x << '\n';

	// A 没有默认构造函数时，D 必须直接初始化虚基类 A。
	test2::D second;
	std::cout << "test2::D 直接构造 A 后的 x（预期为 3）: " << second.x << '\n';

	// 输出顺序应为 A、B、C，且 A 只构造一次。
	std::cout << "test3::D 的构造顺序（预期 A -> B -> C，A 仅一次）：\n";
	test3::D third;

	const bool passed = a_from_b == a_from_c
		&& first.x == 10
		&& second.x == 3
		&& third.x == 3;
	std::cout << "虚继承要点验证结果: " << (passed ? "通过" : "失败") << '\n';
	end_test_case();
}

void Verification_class::test_diferent_param_function()
{
	begin_test_case("函数重载");
	std::cout << "调用 foo(int)：\n";
	foo(1);
	std::cout << "调用 foo(double)：\n";
	foo(1.1);
	std::cout << "两个重载均已成功调用。\n";
	end_test_case();
}

void Verification_class::test_deep_copy()
{
	begin_test_case("深拷贝与浅拷贝");
	char text[] = "abcdefg";
	copy_sample source(text, static_cast<int>(sizeof(text) - 1));
	copy_sample deep_copy(source, true);
	copy_sample shallow_copy(source, false);

	std::cout << "修改源对象前：\n";
	std::cout << "源对象（地址 " << static_cast<const void*>(source.data()) << "）：";
	source.print();
	std::cout << "深拷贝（地址 " << static_cast<const void*>(deep_copy.data()) << "）：";
	deep_copy.print();
	std::cout << "浅拷贝（地址 " << static_cast<const void*>(shallow_copy.data()) << "）：";
	shallow_copy.print();

	source.swap();
	std::cout << "\n反转源对象后：\n";
	std::cout << "源对象：";
	source.print();
	std::cout << "深拷贝（应保持原值）：";
	deep_copy.print();
	std::cout << "浅拷贝（应随源对象变化）：";
	shallow_copy.print();
	end_test_case();
}