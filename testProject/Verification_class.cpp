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