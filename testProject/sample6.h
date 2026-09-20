#pragma once
#include <iostream>
//用于测试浅拷贝和深拷贝
class copy_sample
{
public:
	copy_sample()
	{
		ptr_ = nullptr;
		size_ = 0;
	}
	~copy_sample()
	{
		if (!ptr_)
			delete []ptr_;
		size_ = 0;
	}
	copy_sample(char* str, int len)
	{
		ptr_ = new char[len + 1];
		memcpy(ptr_, str, len);
		size_ = len;
		ptr_[size_] = '\0';
	}
	//使用函数重载来实现深拷贝和浅拷贝
	copy_sample(copy_sample& other,bool deepcopy = true):copy_sample()
	{
		if (deepcopy)
		{
			//深拷贝
			ptr_ = new char[other.size()+1];
			memcpy(ptr_, other.data(), other.size());
			size_ = other.size();
			ptr_[size_] = '\0';
		}
		else {
			//浅拷贝
			ptr_ = other.data();
			size_ = other.size();
		}
	}
	void print()
	{
		std::cout << ptr_ << std::endl;
	}
	void swap()
	{
		for (int i = 0; i < size_/2; i++)
		{
			char temp = ptr_[i];
			ptr_[i] = ptr_[size_ - 1 - i];
			ptr_[size_ - 1 - i] = temp;
		}
	}
	char* data() { return ptr_; }
	int size() { return size_; }
private:
	char* ptr_;
	int size_;
};