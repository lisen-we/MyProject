#include "myString.h"
#include <memory>
hstring::size_t c_str_length(const char* str)
{
	if (str == nullptr) return 0;
	//返回输入的str的长度
	hstring::size_t length = 0;
	while (str[length] != '\0')
	{
		if (length == hstring::max_size)
			break;
		++length;
	}
	return length;
}
hstring::hstring()
{
	data_ = new char[1];
	data_[0] = '\0';
	size_ = 0;
	capacity_ = 0;
}
hstring::hstring(const char* str):hstring(str,c_str_length(str))
{
}
hstring::hstring(const char* str, size_t size):hstring()
{
	size_t copy_length = size;
	size_t str_length = c_str_length(str);
	if (size == 0)
		return;
	if (str == nullptr && size != 0)
		throw "The input string is nullptr,but the input argument size isn't zero";
	if (size > str_length)
		copy_length = str_length;
	//以copy_length为准
	if (copy_length > max_size)
		throw "The input string is too long";
	reserve(copy_length);
	size_ = copy_length;
	std::memcpy(data_, str, copy_length);
	data_[size_] = '\0';
}
hstring::hstring(const hstring& other):hstring(other.c_str(),other.size_)
{
	
}
hstring::hstring(hstring&& other) noexcept:data_(other.data_),capacity_(other.capacity_),size_(other.size_)
{
	//使用初始化列表进行构造，使用hstring()委托构造会导致char_[0] = '\0'的内存泄露
	other.data_ = nullptr;
	other.capacity_ = 0;
	other.size_ = 0;
}
hstring::hstring(int value):hstring()
{
	//用于将整型数字转换成hstring对象
	char reverse[21];
	size_t count = 0;
	//输入的为负值需要首先转换成对应的正值，为了防止int溢出需要使用无符号数运算
	size_t abs_value = value<0?0U - static_cast<size_t>(value):value;
	do {
		// 不能直接转换数字，否则得到的是编码 num 对应的字符
		reverse[count++] = static_cast<char>('0'+abs_value % 10);
		abs_value /= 10;
	} while (abs_value != 0);
	if (value < 0) reverse[count++] = '-';
	//将reverse颠倒写入到data_中
	reserve(count);
	for (int i = 0; i < count; i++)
	{
		data_[i] = reverse[count - 1 - i];
	}
	size_ = count;
	data_[size_] = '\0';
}
hstring& hstring::operator=(const hstring& other)
{
	if (this != &other)
	{
		hstring temp(other);
		swap(temp);
	}
	return *this;
}
hstring& hstring::operator=(hstring&& other) noexcept
{
	if (this != &other)
	{
		delete[] data_;
		data_ = other.data_;
		size_ = other.size_;
		capacity_ = other.capacity_;
		other.data_ = nullptr;
		other.size_ = 0;
		other.capacity_ = 0;
	}
	return *this;
}
hstring::~hstring()
{
	delete[] data_;
}
size_t hstring::size() const noexcept
{
	return size_;
}
size_t hstring::capacity() const noexcept
{
	return capacity_;
}
char* hstring::data()noexcept
{
	return data_;
}
const char* hstring::data() const noexcept
{
	return data_;
}
const char* hstring::c_str() const noexcept
{
	return data_ == nullptr ? "" : data_;
}
bool hstring::empty()const noexcept
{
	return size_ == 0;
}
void hstring::clear() noexcept
{
	if (data_ == nullptr) return;
	size_ = 0;
	data_[0] = '\0';
}
void hstring::swap(hstring& other) noexcept
{
	if (this != &other)
	{
		char* temp_data_ = other.data_;
		other.data_ = data_;
		data_ = temp_data_;

		size_t temp_size_ = other.size_;
		other.size_ = size_;
		size_ = temp_size_;

		size_t temp_capacity_ = other.capacity_;
		other.capacity_ = capacity_;
		capacity_ = temp_capacity_;
	}
}
void hstring::reserve(size_t required)
{
	//预留 required 长度的capacity
	if (capacity_ >= required) return;
	if (required > max_size)
		throw "The length is greater than the maximum length of the string";
	char* new_data_ = new char[required + 1];
	if(size_!=0)
		std::memcpy(new_data_, data_, size_);
	capacity_ = required;
	new_data_[size_] = '\0';
	delete[] data_;
	data_ = new_data_;
}
void hstring::ensure_capacity(size_t required)
{
	//判断capacity是否能够满足需要的长度，如果不满足则每次申请双倍空间
	size_t space = capacity_ == 0 ? 1 : capacity_;
	while (space < required)
	{
		if (space >= max_size / 2)
		{
			space = required;
			break;
		}
		space *= 2;
	}
	reserve(space);
	capacity_ = space;
}
size_t hstring::find(size_t pos, const hstring& target)const noexcept
{
	if (pos > size_)
		return nopos;
	if (target.size_ == 0)
		return pos;
	//避免 size_ - target.size_ 无符号下溢
	if (target.size_ > size_ - pos)
		return nopos;
	for (size_t i = pos; i <= size_ - target.size_; i++)
	{
		bool match = true;
		for (size_t j = 0; j < target.size_; j++)
		{
			if (target.data_[j] != data_[j+i])
			{
				match = false;
				break;
			}
		}
		if (match == true)
			return static_cast<size_t>(i);
	}
	return nopos;
}
hstring& hstring::replace(size_t pos, const hstring& target, const hstring& replacement)
{
	size_t target_pos = find(pos,target);
	if (target_pos == nopos) return *this;
	//target_pos 是查找到的target的起始下标
	erase(target_pos, target);
	insert(target_pos, replacement);
	return *this;
}
hstring& hstring::erase(size_t pos, const hstring& target)
{
	//从pos位置查找第一个target子串并删除
	size_t target_pos = find(pos, target);
	if (target_pos == nopos) return *this;
	size_t temp = target_pos;
	for (size_t i = target_pos + target.size_; i < size_; i++)
	{
		//将target后的所有字符移动到target_pos处
		data_[temp++] = data_[i];
	}
	size_ = temp;
	data_[temp] = '\0';
	return *this;
}
hstring& hstring::insert(size_t pos, const hstring& target)
{
	if (pos > size_) return *this;
	hstring temp(target);
	//首先确保capacity的长度是足够的
	ensure_capacity(size_ + temp.size_);
	// 使用倒序循环的时候需要防止产生环。
	// TODO: size_t 是无符号类型，i >= 0 恒成立；pos 为 0 时 i-- 会回绕为最大值，导致死循环和越界写入。
	for (size_t i = size_; i >= pos&&i>=0; i--)
	{
		//从后向前移动
		data_[i + temp.size_] = data_[i];
	}
	for (size_t j = 0; j < temp.size_; j++)
	{
		//将target中的字符串移动过来
		data_[j + pos] = temp.data_[j];
	}
	size_ += temp.size();
	return *this;
}

// TODO: 头文件已声明 operator+=、operator+、operator-=、operator-、operator[]、operator==、operator!=，
//       但本 cpp 尚未提供定义；只要调用它们，链接阶段就会报“无法解析的外部符号”。
