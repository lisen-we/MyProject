#pragma once
#include "sample1.h"
class Vertification_class
{
public:
	Vertification_class();
	~Vertification_class();
	int return_empty_class_size();
private:
	
};

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