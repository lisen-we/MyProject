#include "myString.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <utility>

String::String()
	: data_(new char[1]{'\0'}), capacity_(0), size_(0)
{
	// 类不变量：正常对象始终在 data_[size_] 保存 C 字符串结束符。
}

String::String(const char* str)
	: String(str, str == nullptr ? 0 : std::strlen(str))
{
}

String::String(const char* str, size_type count)
	: String()
{
	if (str == nullptr) {
		if (count != 0) {
			throw std::invalid_argument("String: null pointer with non-zero length");
		}
		return;
	}

	if (count == 0) {
		return;
	}

	reserve(count);
	// 按显式长度复制，因此字符串中间可以合法地包含 '\0'。
	std::memcpy(data_, str, count);
	size_ = count;
	data_[size_] = '\0';
}

String::String(const String& other)
	: String(other.c_str(), other.size_)
{
}

String::String(String&& other) noexcept
	: data_(other.data_), capacity_(other.capacity_), size_(other.size_)
{
	// 移动只转移缓冲区所有权；被移动对象保留为可析构、可重新赋值的空状态。
	other.data_ = nullptr;
	other.capacity_ = 0;
	other.size_ = 0;
}

String::~String()
{
	delete[] data_;
}

String& String::operator=(const String& other)
{
	if (this != &other) {
		// 先完整构造副本再交换，分配失败时当前对象保持不变。
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
		capacity_ = other.capacity_;
		size_ = other.size_;

		other.data_ = nullptr;
		other.capacity_ = 0;
		other.size_ = 0;
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
		throw std::out_of_range("String::at: index out of range");
	}
	return data_[index];
}

const char& String::at(size_type index) const
{
	if (index >= size_) {
		throw std::out_of_range("String::at: index out of range");
	}
	return data_[index];
}

char& String::front()
{
	if (empty()) {
		throw std::out_of_range("String::front: empty string");
	}
	return data_[0];
}

const char& String::front() const
{
	if (empty()) {
		throw std::out_of_range("String::front: empty string");
	}
	return data_[0];
}

char& String::back()
{
	if (empty()) {
		throw std::out_of_range("String::back: empty string");
	}
	return data_[size_ - 1];
}

const char& String::back() const
{
	if (empty()) {
		throw std::out_of_range("String::back: empty string");
	}
	return data_[size_ - 1];
}

String::size_type String::size() const noexcept
{
	return size_;
}

String::size_type String::length() const noexcept
{
	return size_;
}

String::size_type String::capacity() const noexcept
{
	return capacity_;
}

String::size_type String::max_size() const noexcept
{
	// 预留一个字节给 data_[size_] 的结束符，避免 capacity + 1 溢出。
	return std::numeric_limits<size_type>::max() - 1;
}

bool String::empty() const noexcept
{
	return size_ == 0;
}

void String::reserve(size_type new_capacity)
{
	if (new_capacity <= capacity_) {
		return;
	}
	if (new_capacity > max_size()) {
		throw std::length_error("String::reserve: requested capacity is too large");
	}

	// capacity_ 不包含结束符，所以实际申请量必须额外加一。
	char* new_data = new char[new_capacity + 1];
	if (size_ != 0) {
		std::memcpy(new_data, data_, size_);
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
	if (required > max_size()) {
		throw std::length_error("String: resulting string is too large");
	}

	// 几何扩容使连续 push_back/append 具有均摊 O(1) 的增长成本。
	size_type new_capacity = capacity_ == 0 ? 1 : capacity_;
	while (new_capacity < required) {
		if (new_capacity > max_size() / 2) {
			new_capacity = required;
			break;
		}
		new_capacity *= 2;
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
	std::fill(data_ + size_, data_ + count, ch);
	size_ = count;
	data_[size_] = '\0';
}

void String::shrink_to_fit()
{
	if (capacity_ == size_ && data_ != nullptr) {
		return;
	}

	char* new_data = new char[size_ + 1];
	if (size_ != 0) {
		std::memcpy(new_data, data_, size_);
	}
	new_data[size_] = '\0';

	delete[] data_;
	data_ = new_data;
	capacity_ = size_;
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
	if (size_ == max_size()) {
		throw std::length_error("String::push_back: resulting string is too large");
	}
	ensure_capacity(size_ + 1);
	data_[size_++] = ch;
	data_[size_] = '\0';
}

void String::pop_back()
{
	if (empty()) {
		throw std::out_of_range("String::pop_back: empty string");
	}
	--size_;
	data_[size_] = '\0';
}

String& String::append(const String& other)
{
	if (other.size_ == 0) {
		return *this;
	}
	if (other.size_ > max_size() - size_) {
		throw std::length_error("String::append: resulting string is too large");
	}

	const size_type old_size = size_;
	ensure_capacity(size_ + other.size_);

	if (this == &other) {
		// 自追加扩容后源数据仍是当前缓冲区；memmove 明确允许可能的重叠。
		std::memmove(data_ + old_size, data_, old_size);
		size_ = old_size * 2;
	} else {
		std::memcpy(data_ + old_size, other.data_, other.size_);
		size_ = old_size + other.size_;
	}
	data_[size_] = '\0';
	return *this;
}

String& String::append(const char* str)
{
	if (str == nullptr) {
		return *this;
	}
	return append(str, std::strlen(str));
}

String& String::append(const char* str, size_type count)
{
	if (count == 0) {
		return *this;
	}
	if (str == nullptr) {
		throw std::invalid_argument("String::append: null pointer with non-zero length");
	}

	// 先复制到临时对象，避免 str 指向自身缓冲区时因扩容而失效。
	String temp(str, count);
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
		throw std::out_of_range("String::insert: position out of range");
	}
	if (other.empty()) {
		return *this;
	}
	if (other.size_ > max_size() - size_) {
		throw std::length_error("String::insert: resulting string is too large");
	}
	if (this == &other) {
		String temp(other);
		return insert(pos, temp);
	}

	ensure_capacity(size_ + other.size_);
	// 连同原结束符一起右移，源区间和目标区间重叠，必须使用 memmove。
	std::memmove(data_ + pos + other.size_, data_ + pos, size_ - pos + 1);
	std::memcpy(data_ + pos, other.data_, other.size_);
	size_ += other.size_;
	return *this;
}

String& String::erase(size_type pos, size_type count)
{
	if (pos > size_) {
		throw std::out_of_range("String::erase: position out of range");
	}

	const size_type available = size_ - pos;
	const size_type erased = count > available ? available : count;
	if (erased == 0) {
		return *this;
	}

	std::memmove(data_ + pos, data_ + pos + erased, size_ - pos - erased + 1);
	size_ -= erased;
	return *this;
}

String& String::replace(size_type pos, size_type count, const String& replacement)
{
	if (pos > size_) {
		throw std::out_of_range("String::replace: position out of range");
	}

	const size_type available = size_ - pos;
	const size_type removed = count > available ? available : count;
	const size_type kept = size_ - removed;
	if (replacement.size_ > max_size() - kept) {
		throw std::length_error("String::replace: resulting string is too large");
	}

	// 在独立对象中组成新内容，既支持用自身替换，也提供强异常保证。
	String result;
	result.reserve(kept + replacement.size_);
	result.append(c_str(), pos);
	result.append(replacement);
	result.append(c_str() + pos + removed, size_ - pos - removed);
	swap(result);
	return *this;
}

String String::substr(size_type pos, size_type count) const
{
	if (pos > size_) {
		throw std::out_of_range("String::substr: position out of range");
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
	if (target.empty()) {
		return pos;
	}
	if (target.size_ > size_ - pos) {
		return npos;
	}

	for (size_type i = pos; i <= size_ - target.size_; ++i) {
		if (std::memcmp(c_str() + i, target.c_str(), target.size_) == 0) {
			return i;
		}
	}
	return npos;
}

int String::compare(const String& other) const noexcept
{
	const size_type common = std::min(size_, other.size_);
	if (common != 0) {
		const int result = std::memcmp(c_str(), other.c_str(), common);
		if (result != 0) {
			return result;
		}
	}
	if (size_ < other.size_) {
		return -1;
	}
	if (size_ > other.size_) {
		return 1;
	}
	return 0;
}

String::iterator String::begin() noexcept
{
	return data_;
}

String::iterator String::end() noexcept
{
	return data_ == nullptr ? nullptr : data_ + size_;
}

String::const_iterator String::begin() const noexcept
{
	return data_;
}

String::const_iterator String::end() const noexcept
{
	return data_ == nullptr ? nullptr : data_ + size_;
}

String::const_iterator String::cbegin() const noexcept
{
	return begin();
}

String::const_iterator String::cend() const noexcept
{
	return end();
}

void String::swap(String& other) noexcept
{
	using std::swap;
	swap(data_, other.data_);
	swap(capacity_, other.capacity_);
	swap(size_, other.size_);
}

String operator+(const String& lhs, const String& rhs)
{
	String result(lhs);
	if (rhs.size() > result.max_size() - result.size()) {
		throw std::length_error("operator+: resulting string is too large");
	}
	result.reserve(result.size() + rhs.size());
	result += rhs;
	return result;
}

bool operator==(const String& lhs, const String& rhs) noexcept
{
	return lhs.compare(rhs) == 0;
}

bool operator!=(const String& lhs, const String& rhs) noexcept
{
	return !(lhs == rhs);
}

bool operator<(const String& lhs, const String& rhs) noexcept
{
	return lhs.compare(rhs) < 0;
}

bool operator<=(const String& lhs, const String& rhs) noexcept
{
	return lhs.compare(rhs) <= 0;
}

bool operator>(const String& lhs, const String& rhs) noexcept
{
	return lhs.compare(rhs) > 0;
}

bool operator>=(const String& lhs, const String& rhs) noexcept
{
	return lhs.compare(rhs) >= 0;
}

std::ostream& operator<<(std::ostream& os, const String& str)
{
	// write 按长度输出，不会在字符串内部的 '\0' 处提前停止。
	return os.write(str.c_str(), static_cast<std::streamsize>(str.size()));
}
