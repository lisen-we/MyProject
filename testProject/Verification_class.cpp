#include "Verification_class.h"
Vertification_class::Vertification_class()
{

}

Vertification_class::~Vertification_class()
{
}
int Vertification_class::return_empty_class_size()
{
	sample1 A;
	return sizeof(A);
}
void Vertification_class::test_smart_ptr_memory_leak()
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
	std::cout << "离开局部作用域之后：\n";
	std::cout << "A use_count = " << observe_A.use_count() << '\n';
	std::cout << "B use_count = " << observe_B.use_count() << '\n';

	std::cout << std::boolalpha;
	std::cout << "A 是否已销毁：" << observe_A.expired() << '\n';
	std::cout << "B 是否已销毁：" << observe_B.expired() << '\n';
}