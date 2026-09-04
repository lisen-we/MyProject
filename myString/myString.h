#include <cstddef>
class hstring
{
public:
	using size_t = std::size_t;
	static constexpr size_t max_size = 256;
	static constexpr size_t nopos = static_cast<size_t>(-1);
	hstring();
	hstring(const char* str);
	hstring(const char* str, size_t size);
	hstring(const hstring& other);
	hstring(hstring&& other) noexcept;
	hstring(int value);
	hstring& operator=(const hstring& other);
	hstring& operator=(hstring&& other) noexcept;
	~hstring();

	size_t size()const noexcept;
	size_t capacity()const noexcept;
	char* data() noexcept;
	const char* data()const noexcept;
	const char* c_str()const noexcept;

	bool empty()const noexcept;
	void clear() noexcept;
	void swap(hstring& other) noexcept;
	void reserve(size_t required);
	size_t find(size_t pos, const hstring& target)const noexcept;
	hstring& replace(size_t pos, const hstring& target, const hstring& replacement);
	hstring& erase(size_t pos,const hstring& target);
	hstring& insert(size_t pos, const hstring& target);

	// TODO: 下方 operator+=、operator+、operator-=、operator-、operator[]、operator==、operator!= 目前只有声明；cpp 中没有定义，调用会链接失败。
	// TODO: operator[] 还缺少 const 重载；operator== 和 operator!= 应声明为 const。
	hstring& operator+=(const hstring& other);
	hstring operator+(const hstring& other) const;
	hstring& operator-=(const hstring& other);
	hstring operator-(const hstring& other) const;
	char& operator[](size_t index);
	bool operator==(const hstring& other) const;
	bool operator!=(const hstring& other) const;


private:
	size_t size_;
	size_t capacity_;
	char* data_;
	void ensure_capacity(size_t required);
};
