#include "myString.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace
{
void expect(bool condition, const char* message)
{
	assert(condition);
	if (!condition) {
		std::cerr << "FAILED: " << message << '\n';
		std::exit(EXIT_FAILURE);
	}
}

void expect_bytes(const String& value, const char* expected, String::size_type count,
	const char* message)
{
	expect(value.size() == count, message);
	expect(count == 0 || std::memcmp(value.data(), expected, count) == 0, message);
	expect(value.c_str()[value.size()] == '\0', "String must keep a trailing null character");
}

template <typename Exception, typename Function>
void expect_throw(Function function, const char* message)
{
	bool thrown = false;
	try {
		function();
	} catch (const Exception&) {
		thrown = true;
	}
	expect(thrown, message);
}

void test_construction()
{
	String empty;
	expect(empty.empty(), "default construction");
	expect(empty.size() == 0 && empty.capacity() == 0, "empty size and capacity");
	expect(std::strcmp(empty.c_str(), "") == 0, "empty c_str");

	String null_string(nullptr);
	String null_with_zero(nullptr, 0);
	expect(null_string.empty() && null_with_zero.empty(), "null pointer creates empty string");
	expect_throw<std::invalid_argument>([] { String invalid(nullptr, 1); },
		"null pointer with non-zero count must throw");

	String hello("hello");
	expect_bytes(hello, "hello", 5, "C string construction");

	const char binary[] = {'a', '\0', 'b'};
	String embedded_null(binary, 3);
	expect_bytes(embedded_null, binary, 3, "embedded null construction");
}

void test_copy_and_move()
{
	const char binary[] = {'x', '\0', 'y'};
	String source(binary, 3);
	String copy(source);
	expect_bytes(copy, binary, 3, "copy construction");
	copy[0] = 'X';
	expect(source[0] == 'x', "copy must be deep");

	String assigned("old");
	assigned = source;
	expect_bytes(assigned, binary, 3, "copy assignment preserves embedded null");
	assigned = assigned;
	expect_bytes(assigned, binary, 3, "self copy assignment");

	String moved(std::move(source));
	expect_bytes(moved, binary, 3, "move construction");
	expect(source.empty() && std::strcmp(source.c_str(), "") == 0, "moved-from state");
	source += "reused";
	expect(source == String("reused"), "reuse moved-from object");

	String move_assigned("discarded");
	move_assigned = std::move(moved);
	expect_bytes(move_assigned, binary, 3, "move assignment");
	expect(moved.empty(), "move-assigned source state");
	move_assigned = std::move(move_assigned);
	expect_bytes(move_assigned, binary, 3, "self move assignment");

	String from_literal;
	from_literal = "literal";
	expect(from_literal == String("literal"), "assignment through temporary String");
}

void test_access_and_capacity()
{
	String value("abc");
	expect(value.front() == 'a' && value.back() == 'c', "front and back");
	value.at(1) = 'B';
	expect(value == String("aBc"), "checked element access");
	expect_throw<std::out_of_range>([&] { (void)value.at(3); }, "at must check bounds");

	for (char& ch : value) {
		if (ch >= 'A' && ch <= 'Z') {
			ch = static_cast<char>(ch - 'A' + 'a');
		}
	}
	expect(value == String("abc"), "mutable iterators");

	const char* old_content = value.c_str();
	(void)old_content;
	value.reserve(20);
	expect(value.capacity() >= 20 && value == String("abc"), "reserve preserves content");
	value.resize(5, 'x');
	expect(value == String("abcxx"), "resize grows with fill character");
	value.resize(2);
	expect(value == String("ab"), "resize shrinks");
	value.shrink_to_fit();
	expect(value.capacity() == value.size(), "shrink_to_fit");
	value.clear();
	expect(value.empty() && std::strcmp(value.c_str(), "") == 0, "clear");

	for (int i = 0; i < 1000; ++i) {
		value.push_back(static_cast<char>('a' + i % 26));
	}
	expect(value.size() == 1000 && value.capacity() >= value.size(), "geometric growth");
	value.pop_back();
	expect(value.size() == 999 && value.c_str()[999] == '\0', "pop_back");
}

void test_modifiers()
{
	String value("hello");
	value += ' ';
	value.append("world");
	expect(value == String("hello world"), "append and operator+=");

	String self("abc");
	self += self;
	expect(self == String("abcabc"), "self append");

	String alias("abcd");
	alias.append(alias.c_str() + 1, 3);
	expect(alias == String("abcdbcd"), "append from own buffer");

	String inserted("ace");
	inserted.insert(1, String("b"));
	inserted.insert(3, String("d"));
	inserted.insert(inserted.size(), String("f"));
	expect(inserted == String("abcdef"), "insert at beginning, middle and end");

	String self_insert("ab");
	self_insert.insert(1, self_insert);
	expect(self_insert == String("aabb"), "self insert");

	inserted.erase(1, 3);
	expect(inserted == String("aef"), "erase middle range");
	inserted.erase(1);
	expect(inserted == String("a"), "erase to end");

	String replaced("hello world");
	replaced.replace(6, 5, String("String"));
	expect(replaced == String("hello String"), "replace range");
	String self_replace("abc");
	self_replace.replace(1, 1, self_replace);
	expect(self_replace == String("aabcc"), "replace with self");
}

void test_algorithms_and_errors()
{
	String text("one two one");
	expect(text.find(String("one")) == 0, "find first match");
	expect(text.find(String("one"), 1) == 8, "find with start position");
	expect(text.find(String("missing")) == String::npos, "find missing value");
	expect(text.find(String(""), text.size()) == text.size(), "find empty value");
	expect(text.substr(4, 3) == String("two"), "substr");
	expect(text.substr(8) == String("one"), "substr to end");

	const char binary_text[] = {'a', '\0', 'b', 'a', '\0', 'b'};
	const char binary_target[] = {'a', '\0', 'b'};
	String binary(binary_text, 6);
	String target(binary_target, 3);
	expect(binary.find(target, 1) == 3, "find supports embedded null");

	expect(String("abc") < String("abd"), "lexicographical less-than");
	expect(String("abc") <= String("abc"), "less-than or equal");
	expect(String("abd") > String("abc"), "greater-than");
	expect(String("abc") >= String("abc"), "greater-than or equal");
	expect(String("abc") != String("ab"), "not equal");
	expect(String("abc") + String("def") == String("abcdef"), "operator+");

	std::ostringstream output;
	output << binary;
	expect(output.str().size() == 6, "stream output preserves embedded null length");
	expect(std::memcmp(output.str().data(), binary_text, 6) == 0,
		"stream output preserves embedded null content");

	String empty;
	expect_throw<std::out_of_range>([&] { (void)empty.front(); }, "front on empty string");
	expect_throw<std::out_of_range>([&] { (void)empty.back(); }, "back on empty string");
	expect_throw<std::out_of_range>([&] { empty.pop_back(); }, "pop_back on empty string");
	expect_throw<std::out_of_range>([&] { text.insert(text.size() + 1, String("x")); },
		"insert position validation");
	expect_throw<std::out_of_range>([&] { text.erase(text.size() + 1); },
		"erase position validation");
	expect_throw<std::out_of_range>([&] { (void)text.substr(text.size() + 1); },
		"substr position validation");
	expect_throw<std::invalid_argument>([&] { text.append(nullptr, 1); },
		"append null pointer validation");
}
} // namespace

int main()
{
	test_construction();
	test_copy_and_move();
	test_access_and_capacity();
	test_modifiers();
	test_algorithms_and_errors();

	std::cout << "All custom String tests passed.\n";
	return EXIT_SUCCESS;
}
