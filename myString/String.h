#pragma once

#include <cstddef>

// 一个教学用途的基础 CString：逻辑长度最多为 256 个字符。
class String
{
public:
	using size_type = std::size_t;
	static constexpr size_type max_length = 256;
	static constexpr size_type npos = static_cast<size_type>(-1);

	String();
	String(const char* str);
	String(const char* str, size_type length);
	String(char ch);
	String(int value);
	String(unsigned int value);
	String(long value);
	String(unsigned long value);
	String(long long value);
	String(unsigned long long value);
	String(const String& other);
	String(String&& other) noexcept;
	~String();

	String& operator=(const String& other);
	String& operator=(String&& other) noexcept;

	char* data() noexcept;
	const char* data() const noexcept;
	const char* c_str() const noexcept;

	size_type size() const noexcept;
	size_type capacity() const noexcept;
	bool empty() const noexcept;

	char& operator[](size_type index) noexcept;
	const char& operator[](size_type index) const noexcept;
	char& at(size_type index);
	const char& at(size_type index) const;

	void reserve(size_type new_capacity);
	void resize(size_type count, char ch = '\0');
	void clear() noexcept;
	void push_back(char ch);
	void pop_back();

	String& append(const String& other);
	String& append(const char* str);
	String& operator+=(const String& other);
	String& operator+=(const char* str);
	String& operator+=(char ch);

	String& insert(size_type pos, const String& other);
	String& erase(size_type pos, size_type count = npos);
	String& replace(size_type pos, size_type count, const String& other);

	String substr(size_type pos = 0, size_type count = npos) const;
	size_type find(const String& target, size_type pos = 0) const noexcept;
	int compare(const String& other) const noexcept;

	void swap(String& other) noexcept;

private:
	char* data_;
	size_type size_;
	size_type capacity_;

	static size_type c_string_length(const char* str);
	void ensure_capacity(size_type required);
	void assign_unsigned(unsigned long long value, bool negative);
};

String operator+(const String& lhs, const String& rhs);
String operator+(const String& lhs, const char* rhs);
String operator+(const char* lhs, const String& rhs);
String operator+(const String& lhs, char rhs);
String operator+(char lhs, const String& rhs);

// 自定义语义：a - b 表示从 a 中删除第一次出现的 b。
String operator-(String lhs, const String& rhs);
String operator-(String lhs, const char* rhs);
String operator-(const char* lhs, const String& rhs);

bool operator==(const String& lhs, const String& rhs) noexcept;
bool operator!=(const String& lhs, const String& rhs) noexcept;
