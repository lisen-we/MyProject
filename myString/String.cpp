#include "String.h"

#include <memory.h>

String::size_type String::c_string_length(const char* str)
{
	if (str == nullptr) {
		return 0;
	}

	size_type length = 0;
	while (str[length] != '\0') {
		if (length == max_length) {
			throw "String: maximum length is 256";
		}
		++length;
	}
	return length;
}

String::String()
	: data_(new char[1]), size_(0), capacity_(0)
{
	data_[0] = '\0';
}

String::String(const char* str)
	: String(str, c_string_length(str))
{
}

String::String(const char* str, size_type length)
	: String()
{
	if (str == nullptr && length != 0) {
		throw "String: null pointer with non-zero length";
	}
	if (length > max_length) {
		throw "String: maximum length is 256";
	}
	if (length == 0) {
		return;
	}

	reserve(length);
	::memcpy(data_, str, length);
	size_ = length;
	data_[size_] = '\0';
}

String::String(char ch)
	: String()
{
	push_back(ch);
}

String::String(int value)
	: String()
{
	const bool negative = value < 0;
	const unsigned int magnitude = negative
		? 0U - static_cast<unsigned int>(value)
		: static_cast<unsigned int>(value);
	assign_unsigned(magnitude, negative);
}

String::String(unsigned int value)
	: String()
{
	assign_unsigned(value, false);
}

String::String(long value)
	: String()
{
	const bool negative = value < 0;
	const unsigned long magnitude = negative
		? 0UL - static_cast<unsigned long>(value)
		: static_cast<unsigned long>(value);
	assign_unsigned(magnitude, negative);
}

String::String(unsigned long value)
	: String()
{
	assign_unsigned(value, false);
}

String::String(long long value)
	: String()
{
	const bool negative = value < 0;
	const unsigned long long magnitude = negative
		? 0ULL - static_cast<unsigned long long>(value)
		: static_cast<unsigned long long>(value);
	assign_unsigned(magnitude, negative);
}

String::String(unsigned long long value)
	: String()
{
	assign_unsigned(value, false);
}

void String::assign_unsigned(unsigned long long value, bool negative)
{
	char reversed[20];
	size_type digit_count = 0;

	do {
		reversed[digit_count++] = static_cast<char>('0' + value % 10);
		value /= 10;
	} while (value != 0);

	ensure_capacity(digit_count + (negative ? 1 : 0));
	if (negative) {
		data_[size_++] = '-';
	}
	while (digit_count != 0) {
		data_[size_++] = reversed[--digit_count];
	}
	data_[size_] = '\0';
}

String::String(const String& other)
	: String(other.c_str(), other.size_)
{
}

String::String(String&& other) noexcept
	: data_(other.data_), size_(other.size_), capacity_(other.capacity_)
{
	// 新对象接管缓冲区；other 不再拥有该内存。
	other.data_ = nullptr;
	other.size_ = 0;
	other.capacity_ = 0;
}

String::~String()
{
	delete[] data_;
}

String& String::operator=(const String& other)
{
	if (this != &other) {
		String temp(other);
		swap(temp);
	}
	return *this;
}

String& String::operator=(String&& other) noexcept
{
	if (this != &other) {
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

char* String::data() noexcept
{
	return data_;
}

const char* String::data() const noexcept
{
	return data_;
}

const char* String::c_str() const noexcept
{
	return data_ == nullptr ? "" : data_;
}

String::size_type String::size() const noexcept
{
	return size_;
}

String::size_type String::capacity() const noexcept
{
	return capacity_;
}

bool String::empty() const noexcept
{
	return size_ == 0;
}

char& String::operator[](size_type index) noexcept
{
	return data_[index];
}

const char& String::operator[](size_type index) const noexcept
{
	return data_[index];
}

char& String::at(size_type index)
{
	if (index >= size_) {
		throw "String::at: index out of range";
	}
	return data_[index];
}

const char& String::at(size_type index) const
{
	if (index >= size_) {
		throw "String::at: index out of range";
	}
	return data_[index];
}

void String::reserve(size_type new_capacity)
{
	if (new_capacity <= capacity_) {
		return;
	}
	if (new_capacity > max_length) {
		throw "String: maximum length is 256";
	}

	// capacity_ 不计 '\0'，因此总是额外申请一个结束符位置。
	char* new_data = new char[new_capacity + 1];
	if (size_ != 0) {
		::memcpy(new_data, data_, size_);
	}
	new_data[size_] = '\0';

	delete[] data_;
	data_ = new_data;
	capacity_ = new_capacity;
}

void String::ensure_capacity(size_type required)
{
	if (required <= capacity_) {
		return;
	}
	if (required > max_length) {
		throw "String: maximum length is 256";
	}

	size_type new_capacity = capacity_ == 0 ? 1 : capacity_;
	while (new_capacity < required) {
		new_capacity = new_capacity > max_length / 2
			? max_length
			: new_capacity * 2;
	}
	reserve(new_capacity);
}

void String::resize(size_type count, char ch)
{
	if (count <= size_) {
		size_ = count;
		if (data_ != nullptr) {
			data_[size_] = '\0';
		}
		return;
	}

	ensure_capacity(count);
	::memset(data_ + size_, static_cast<unsigned char>(ch), count - size_);
	size_ = count;
	data_[size_] = '\0';
}

void String::clear() noexcept
{
	size_ = 0;
	if (data_ != nullptr) {
		data_[0] = '\0';
	}
}

void String::push_back(char ch)
{
	ensure_capacity(size_ + 1);
	data_[size_++] = ch;
	data_[size_] = '\0';
}

void String::pop_back()
{
	if (empty()) {
		throw "String::pop_back: empty string";
	}
	data_[--size_] = '\0';
}

String& String::append(const String& other)
{
	if (other.size_ == 0) {
		return *this;
	}
	if (other.size_ > max_length - size_) {
		throw "String: maximum length is 256";
	}

	const size_type old_size = size_;
	ensure_capacity(old_size + other.size_);
	for (size_type i = 0; i < other.size_; ++i) {
		data_[old_size + i] = other.data_[i];
	}
	size_ = old_size + other.size_;
	data_[size_] = '\0';
	return *this;
}

String& String::append(const char* str)
{
	if (str == nullptr) {
		return *this;
	}

	// 先制作副本，保证 str 指向当前缓冲区内部时扩容也不会使其失效。
	String temp(str, c_string_length(str));
	return append(temp);
}

String& String::operator+=(const String& other)
{
	return append(other);
}

String& String::operator+=(const char* str)
{
	return append(str);
}

String& String::operator+=(char ch)
{
	push_back(ch);
	return *this;
}

String& String::insert(size_type pos, const String& other)
{
	if (pos > size_) {
		throw "String::insert: position out of range";
	}
	if (other.empty()) {
		return *this;
	}
	if (other.size_ > max_length - size_) {
		throw "String: maximum length is 256";
	}
	if (this == &other) {
		String temp(other);
		return insert(pos, temp);
	}

	ensure_capacity(size_ + other.size_);
	// 从后向前搬运，避免源区和目标区重叠时覆盖尚未复制的字符。
	for (size_type i = size_ + 1; i > pos; --i) {
		data_[i + other.size_ - 1] = data_[i - 1];
	}
	for (size_type i = 0; i < other.size_; ++i) {
		data_[pos + i] = other.data_[i];
	}
	size_ += other.size_;
	return *this;
}

String& String::erase(size_type pos, size_type count)
{
	if (pos > size_) {
		throw "String::erase: position out of range";
	}

	const size_type available = size_ - pos;
	const size_type removed = count > available ? available : count;
	for (size_type i = pos; i + removed <= size_; ++i) {
		data_[i] = data_[i + removed];
	}
	size_ -= removed;
	return *this;
}

String& String::replace(size_type pos, size_type count, const String& other)
{
	if (pos > size_) {
		throw "String::replace: position out of range";
	}

	const size_type available = size_ - pos;
	const size_type removed = count > available ? available : count;
	const size_type kept = size_ - removed;
	if (other.size_ > max_length - kept) {
		throw "String: maximum length is 256";
	}

	// 先在临时对象中组成前缀 + other + 后缀，成功后再交换。
	String result;
	result.reserve(kept + other.size_);
	for (size_type i = 0; i < pos; ++i) {
		result.push_back(data_[i]);
	}
	for (size_type i = 0; i < other.size_; ++i) {
		result.push_back(other.data_[i]);
	}
	for (size_type i = pos + removed; i < size_; ++i) {
		result.push_back(data_[i]);
	}
	swap(result);
	return *this;
}

String String::substr(size_type pos, size_type count) const
{
	if (pos > size_) {
		throw "String::substr: position out of range";
	}
	const size_type available = size_ - pos;
	const size_type copied = count > available ? available : count;
	return String(c_str() + pos, copied);
}

String::size_type String::find(const String& target, size_type pos) const noexcept
{
	if (pos > size_) {
		return npos;
	}
	if (target.size_ == 0) {
		return pos;
	}
	if (target.size_ > size_ - pos) {
		return npos;
	}

	for (size_type i = pos; i <= size_ - target.size_; ++i) {
		bool matched = true;
		for (size_type j = 0; j < target.size_; ++j) {
			if (data_[i + j] != target.data_[j]) {
				matched = false;
				break;
			}
		}
		if (matched) {
			return i;
		}
	}
	return npos;
}

int String::compare(const String& other) const noexcept
{
	const size_type shared = size_ < other.size_ ? size_ : other.size_;
	for (size_type i = 0; i < shared; ++i) {
		if (data_[i] < other.data_[i]) {
			return -1;
		}
		if (data_[i] > other.data_[i]) {
			return 1;
		}
	}
	return size_ == other.size_ ? 0 : (size_ < other.size_ ? -1 : 1);
}

void String::swap(String& other) noexcept
{
	char* data = data_;
	data_ = other.data_;
	other.data_ = data;

	size_type size = size_;
	size_ = other.size_;
	other.size_ = size;

	size_type capacity = capacity_;
	capacity_ = other.capacity_;
	other.capacity_ = capacity;
}

String operator+(const String& lhs, const String& rhs)
{
	String result(lhs);
	result.append(rhs);
	return result;
}

String operator+(const String& lhs, const char* rhs)
{
	return lhs + String(rhs);
}

String operator+(const char* lhs, const String& rhs)
{
	return String(lhs) + rhs;
}

String operator+(const String& lhs, char rhs)
{
	return lhs + String(rhs);
}

String operator+(char lhs, const String& rhs)
{
	return String(lhs) + rhs;
}

String operator-(String lhs, const String& rhs)
{
	if (!rhs.empty()) {
		const String::size_type pos = lhs.find(rhs);
		if (pos != String::npos) {
			lhs.erase(pos, rhs.size());
		}
	}
	return lhs;
}

String operator-(String lhs, const char* rhs)
{
	return lhs - String(rhs);
}

String operator-(const char* lhs, const String& rhs)
{
	return String(lhs) - rhs;
}

bool operator==(const String& lhs, const String& rhs) noexcept
{
	return lhs.compare(rhs) == 0;
}

bool operator!=(const String& lhs, const String& rhs) noexcept
{
	return !(lhs == rhs);
}
