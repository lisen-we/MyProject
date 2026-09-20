#pragma once
#include "sample1.h"
#include "sample2.h"
#include "sample3.h"
#include "sample4.h"
#include "sample5.h"
#include "sample6.h"
#include <memory>
#include <iostream>

class Verification_class
{
public:
	Verification_class();
	~Verification_class();
	void return_empty_class_size();
	void test_smart_ptr_memory_leak();
	void test_new_malloc_memory();
	void test_polymorphism_with_function_pointer();
	void test_virtual_inheritance();
	void test_diferent_param_function();
	void test_deep_copy();
private:
	void begin_test_case(const char* test_name) const;
	void end_test_case() const;
};
