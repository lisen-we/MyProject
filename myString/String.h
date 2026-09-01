#pragma once

#include <cstddef>

class String
{
public:
	// 类型与特殊值
	using sizet = std::size_t;                                    // 字符串长度、容量和下标类型
	static constexpr sizet npos = static_cast<sizet>(-1);        // 表示未找到或一直操作到末尾

	// 生命周期与资源管理（Rule of Five）
	String();                                                     // 构造空字符串
	String(const char* str);                                      // 使用以 '\0' 结尾的 C 字符串构造
	String(const char* str, sizet len);                           // 使用指定长度的字符序列构造
	String(const String& other);                                  // 深拷贝构造
	String(String&& other) noexcept;                              // 移动构造并接管 other 的资源
	~String();                                                    // 释放字符串持有的动态内存

	String& operator=(const String& other);                       // 深拷贝赋值
	String& operator=(String&& other) noexcept;                   // 移动赋值并接管 other 的资源
	void swap(String& other) noexcept;                            // 交换两个字符串的内部状态

	// 底层数据访问
	char* data() noexcept;                                        // 返回可修改的底层字符缓冲区
	const char* data() const noexcept;                            // 返回只读的底层字符缓冲区
	const char* c_str() const noexcept;                           // 返回以 '\0' 结尾的只读 C 字符串

	// 长度与容量
	sizet size() const noexcept;                                  // 返回当前有效字符数量
	sizet capacity() const noexcept;                              // 返回不重新分配时可容纳的字符数量
	bool empty() const noexcept;                                  // 判断字符串是否为空
	void reserve(sizet new_capacity);                             // 预留容量，不改变字符串长度
	void resize(sizet count, char ch = '\0');                    // 修改长度，扩展部分使用 ch 填充

	// 元素访问
	char& operator[](sizet index) noexcept;                       // 按下标访问字符，不检查越界
	const char& operator[](sizet index) const noexcept;           // 只读下标访问，不检查越界
	char& at(sizet index);                                        // 按下标访问字符，越界时抛出异常
	const char& at(sizet index) const;                            // 只读下标访问，越界时抛出异常

	// 内容修改
	void clear() noexcept;                                        // 清空内容但保留已分配容量
	void push_back(char ch);                                      // 在末尾添加一个字符
	void pop_back();                                              // 删除最后一个字符

	String& append(const String& other);                          // 在末尾追加另一个 String
	String& append(const char* str);                              // 在末尾追加 C 字符串
	String& operator+=(const String& other);                      // 追加另一个 String
	String& operator+=(const char* str);                          // 追加 C 字符串
	String& operator+=(char ch);                                  // 追加一个字符

	String& insert(sizet pos, const String& other);               // 在 pos 位置插入另一个 String
	String& erase(sizet pos, sizet count = npos);                 // 从 pos 开始删除 count 个字符
	String& replace(sizet pos, sizet count, const String& other); // 将指定范围替换为另一个 String

	// 字符串操作
	String substr(sizet pos = 0, sizet count = npos) const;       // 返回从 pos 开始的子字符串
	sizet find(const String& target, sizet pos = 0) const noexcept; // 从 pos 开始查找目标字符串
	int compare(const String& other) const noexcept;              // 按字典序比较两个字符串

private:
	char* mdata;                                                  // 连续字符缓冲区，末尾保留 '\0'
	sizet msize;                                                  // 当前有效字符数量，不包含末尾 '\0'
	sizet mcapacity;                                              // 当前容量，不包含末尾 '\0'
};
