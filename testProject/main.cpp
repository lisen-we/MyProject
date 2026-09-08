#include <iostream>	
#include "Verification_class.h"
int main()
{
	using namespace std;
	Vertification_class testClass;
	cout << testClass.return_empty_class_size() << endl;
	testClass.test_smart_ptr_memory_leak();
}