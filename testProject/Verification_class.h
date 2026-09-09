#pragma once
#include "sample1.h"
#include "sample2.h"
#include <memory>
#include <iostream>
class Verification_class
{
public:
	Verification_class();
	~Verification_class();
	int return_empty_class_size();
	void test_smart_ptr_memory_leak();
	void test_new_malloc_memory();
private:
	
};