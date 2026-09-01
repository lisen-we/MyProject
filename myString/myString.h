#pragma once

#include <cstddef>
#include <iosfwd>

class String
{
public:
	using size_type = std::size_t;
	using iterator = char*;
	using const_iterator = const char*;

	static constexpr size_type npos = static_cast<size_type>(-1);

	String();
	String(const char* str);
	String(const char* str, size_type count);
	String(const String& other);
	String(String&& other) noexcept;
	~String();

	String& operator=(const String& other);
	String& operator=(String&& other) noexcept;

	char* data() noexcept;
	const char* data() const noexcept;
	const char* c_str() const noexcept;

	char& operator[](size_type index) noexcept;
	const char& operator[](size_type index) const noexcept;
	char& at(size_type index);
	const char& at(size_type index) const;
	char& front();
	const char& front() const;
	char& back();
	const char& back() const;

	size_type size() const noexcept;
	size_type length() const noexcept;
	size_type capacity() const noexcept;
	size_type max_size() const noexcept;
	bool empty() const noexcept;

	void reserve(size_type new_capacity);
	void resize(size_type count, char ch = '\0');
	void shrink_to_fit();
	void clear() noexcept;

	void push_back(char ch);
	void pop_back();
	String& append(const String& other);
	String& append(const char* str);
	String& append(const char* str, size_type count);
	String& operator+=(const String& other);
	String& operator+=(const char* str);
	String& operator+=(char ch);
	String& insert(size_type pos, const String& other);
	String& erase(size_type pos, size_type count = npos);
	String& replace(size_type pos, size_type count, const String& replacement);

	String substr(size_type pos = 0, size_type count = npos) const;
	size_type find(const String& target, size_type pos = 0) const noexcept;
	int compare(const String& other) const noexcept;

	iterator begin() noexcept;
	iterator end() noexcept;
	const_iterator begin() const noexcept;
	const_iterator end() const noexcept;
	const_iterator cbegin() const noexcept;
	const_iterator cend() const noexcept;

	void swap(String& other) noexcept;

private:
	char* data_;
	size_type capacity_;
	size_type size_;

	void ensure_capacity(size_type required);
};

String operator+(const String& lhs, const String& rhs);

bool operator==(const String& lhs, const String& rhs) noexcept;
bool operator!=(const String& lhs, const String& rhs) noexcept;
bool operator<(const String& lhs, const String& rhs) noexcept;
bool operator<=(const String& lhs, const String& rhs) noexcept;
bool operator>(const String& lhs, const String& rhs) noexcept;
bool operator>=(const String& lhs, const String& rhs) noexcept;

std::ostream& operator<<(std::ostream& os, const String& str);
