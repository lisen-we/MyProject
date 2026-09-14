#include <iostream>	
#include "Verification_class.h"
int main()
{
	using namespace std;
	Verification_class testClass;
	testClass.return_empty_class_size();
	testClass.test_smart_ptr_memory_leak();
	testClass.test_new_malloc_memory();
	testClass.test_polymorphism_with_function_pointer();
	testClass.test_virtual_inheritance();
}
