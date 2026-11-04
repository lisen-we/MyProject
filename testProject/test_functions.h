#pragma once

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include <utility>
namespace test_detail
{
inline void begin_test_case(const char* test_name)
{
	std::cout << '\n' << std::string(60, '=') << '\n';
	std::cout << "Test case: " << test_name << '\n';
	std::cout << std::string(60, '-') << '\n';
}

inline void end_test_case()
{
	std::cout << std::string(60, '=') << "\n\n";
}
}

inline void test_empty_class_size()
{
	struct EmptyType {};

	test_detail::begin_test_case("empty class size");
	std::cout << "EmptyType size: " << sizeof(EmptyType) << " byte(s)\n";
	test_detail::end_test_case();
}

inline void test_shared_ptr_circular_reference()
{
	struct TypeB;
	struct TypeA
	{
		std::shared_ptr<TypeB> type_b;
		~TypeA()
		{
			std::cout << "TypeA destroyed\n";
		}
	};
	struct TypeB
	{
		std::shared_ptr<TypeA> type_a;
		~TypeB()
		{
			std::cout << "TypeB destroyed\n";
		}
	};

	test_detail::begin_test_case("shared_ptr circular reference");
	std::weak_ptr<TypeA> observed_a;
	std::weak_ptr<TypeB> observed_b;
	{
		auto type_a = std::make_shared<TypeA>();
		auto type_b = std::make_shared<TypeB>();

		observed_a = type_a;
		observed_b = type_b;
		std::cout << "Before cycle - A use_count: " << type_a.use_count()
			<< ", B use_count: " << type_b.use_count() << '\n';

		type_a->type_b = type_b;
		type_b->type_a = type_a;
		std::cout << "After cycle - A use_count: " << type_a.use_count()
			<< ", B use_count: " << type_b.use_count() << '\n';
	}

	std::cout << "After local scope - A expired: " << std::boolalpha << observed_a.expired()
		<< ", B expired: " << observed_b.expired() << '\n';
	std::cout << "Result: "
		<< (!observed_a.expired() && !observed_b.expired() ? "cycle detected" : "no cycle detected")
		<< '\n';
	test_detail::end_test_case();
}

inline void test_new_and_malloc()
{
	test_detail::begin_test_case("new and malloc");
	int* new_value = new int(5);
	std::cout << "new value: " << *new_value << '\n';
	delete new_value;

	int* malloc_value = static_cast<int*>(std::malloc(sizeof(int)));
	if (malloc_value != nullptr)
	{
		*malloc_value = 10;
		std::cout << "malloc value: " << *malloc_value << '\n';
		std::free(malloc_value);
	}
	else
	{
		std::cerr << "malloc failed\n";
	}
	test_detail::end_test_case();
}

inline void test_function_pointer_polymorphism()
{
	struct Animal;
	using SpeakFunction = void(*)(const Animal*);
	using StatusFunction = void(*)(const Animal*);
	using InformationFunction = void(*)(const Animal*);
	struct AnimalVTable
	{
		SpeakFunction speak;
		StatusFunction status;
		InformationFunction information;
	};
	struct Animal
	{
		std::string name;
		std::size_t age;
		const AnimalVTable* vtable;
	};
	struct Dog
	{
		Animal base;
		std::size_t speak_count;
	};
	struct Cat
	{
		Animal base;
	};

	const AnimalVTable dog_vtable{
		+[](const Animal* animal)
		{
			const auto* dog = reinterpret_cast<const Dog*>(animal);
			for (std::size_t count = 0; count < dog->speak_count; ++count)
			{
				std::cout << "woof\n";
			}
		},
		+[](const Animal*)
		{
			std::cout << "eating a bone\n";
		},
		+[](const Animal* animal)
		{
			const auto* dog = reinterpret_cast<const Dog*>(animal);
			std::cout << "Dog - name: " << dog->base.name << ", age: " << dog->base.age << '\n';
		}
	};
	const AnimalVTable cat_vtable{
		+[](const Animal*)
		{
			std::cout << "meow\n";
		},
		+[](const Animal*)
		{
			std::cout << "sleeping\n";
		},
		+[](const Animal* animal)
		{
			const auto* cat = reinterpret_cast<const Cat*>(animal);
			std::cout << "Cat - name: " << cat->base.name << ", age: " << cat->base.age << '\n';
		}
	};

	test_detail::begin_test_case("function pointer polymorphism");
	Dog dog{ { "Rex", 5, &dog_vtable }, 3 };
	Cat cat{ { "Mittens", 2, &cat_vtable } };
	std::vector<Animal*> animals{ &dog.base, &cat.base };
	for (Animal* animal : animals)
	{
		animal->vtable->information(animal);
		animal->vtable->status(animal);
		animal->vtable->speak(animal);
	}
	test_detail::end_test_case();
}

inline void test_virtual_inheritance()
{
	struct Base
	{
		int value;
		explicit Base(int initial_value = 0) : value(initial_value)
		{
			std::cout << "Base constructed\n";
		}
	};
	struct Left : virtual Base
	{
		Left() : Base(1)
		{
			std::cout << "Left constructed\n";
		}
	};
	struct Right : virtual Base
	{
		Right() : Base(2)
		{
			std::cout << "Right constructed\n";
		}
	};
	struct Derived : Left, Right
	{
		Derived() : Base(3) {}
	};

	test_detail::begin_test_case("virtual inheritance");
	Derived instance;
	Base* base_from_left = static_cast<Left*>(&instance);
	Base* base_from_right = static_cast<Right*>(&instance);
	std::cout << "Single Base subobject: " << std::boolalpha
		<< (base_from_left == base_from_right) << '\n';
	std::cout << "Initial Base value: " << instance.value << '\n';
	base_from_left->value = 10;
	std::cout << "Value read through Right: " << base_from_right->value << '\n';
	test_detail::end_test_case();
}

inline void test_function_overload()
{
	struct OverloadedFunction
	{
		void operator()(int) const
		{
			std::cout << "int overload called\n";
		}
		void operator()(double) const
		{
			std::cout << "double overload called\n";
		}
	};

	test_detail::begin_test_case("function overload");
	const OverloadedFunction function;
	function(1);
	function(1.1);
	test_detail::end_test_case();
}

inline void test_value_copy_and_alias()
{
	test_detail::begin_test_case("value copy and alias");
	std::string source = "abcdefg";
	std::string value_copy = source;
	char* alias = &source[0];

	std::cout << "Before reverse - source: " << source
		<< ", value copy: " << value_copy
		<< ", alias: " << alias << '\n';
	std::reverse(source.begin(), source.end());
	std::cout << "After reverse - source: " << source
		<< ", value copy: " << value_copy
		<< ", alias: " << alias << '\n';
	test_detail::end_test_case();
}

// This test intentionally invokes undefined behavior and should be run separately.
inline void test_unsafe_vector_concurrent_insert()
{
	constexpr int insert_count = 100000;
	std::atomic<int> ready{ 0 };
	std::atomic<bool> start{ false };
	std::vector<int> values;

	auto insert = [&](int offset)
		{
			ready.fetch_add(1, std::memory_order_release);
			while (!start.load(std::memory_order_acquire))
			{
				std::this_thread::yield();
			}

			for (int index = 0; index < insert_count; ++index)
			{
				values.push_back(offset + index);
			}
		};

	std::thread first_thread(insert, 0);
	std::thread second_thread(insert, insert_count);
	while (ready.load(std::memory_order_acquire) != 2)
	{
		std::this_thread::yield();
	}
	start.store(true, std::memory_order_release);

	first_thread.join();
	second_thread.join();

	std::cout << "Expected element count: " << insert_count * 2 << '\n';
	std::cout << "Actual element count: " << values.size() << '\n';
}

inline void test_map_and_unorder_map()
{
	std::map<int,std::string> normal_map;
	std::unordered_map<int, std::string> unordered_map;
	std::multimap<int, std::string> multimap;
	normal_map.insert(std::make_pair(1,"abc"));
	normal_map.insert(std::make_pair(2, "e"));
	normal_map.insert(std::make_pair(3, "f"));
	normal_map.insert(std::make_pair(4, "g"));
	normal_map.insert(std::make_pair(5, "h"));
	normal_map.insert(std::make_pair(2, "i"));

	unordered_map.insert(std::make_pair(1, "abc"));
	unordered_map.insert(std::make_pair(2, "e"));
	unordered_map.insert(std::make_pair(3, "f"));
	unordered_map.insert(std::make_pair(4, "g"));
	unordered_map.insert(std::make_pair(5, "h"));
	unordered_map.insert(std::make_pair(2, "i"));

	multimap.insert(std::make_pair(1, "abc"));
	multimap.insert(std::make_pair(2, "e"));
	multimap.insert(std::make_pair(3, "f"));
	multimap.insert(std::make_pair(4, "g"));
	multimap.insert(std::make_pair(5, "h"));
	multimap.insert(std::make_pair(2, "i"));

	for (auto& pair : normal_map)
	{
		std::cout << pair.first << " " << pair.second << std::endl;
	}
	std::cout << "--------------------------"<<std::endl;
	for (auto& pair : unordered_map)
	{
		std::cout << pair.first << " " << pair.second << std::endl;
	}
	std::cout << "--------------------------" << std::endl;
	for (auto& pair : multimap)
	{
		std::cout << pair.first << " " << pair.second << std::endl;
	}
}

inline void test_vector_shrink_to_fit()
{
	std::vector<int> vec;
	for (int i = 0; i < 10; i++)
	{
		vec.push_back(i);
	}
	std::cout << "size:" << vec.size() << " capacity: " << vec.capacity() << std::endl;
	vec.clear();
	std::cout << "size:" << vec.size() << " capacity: " << vec.capacity() << std::endl;
	vec.shrink_to_fit();
	std::cout << "size:" << vec.size() << " capacity: " << vec.capacity() << std::endl;
}

inline void test_emplace()
{
	std::vector<std::pair<int, std::string>> vec;
	vec.emplace(vec.end(),1,"abc");
	vec.emplace_back(2, "abc");
	for (auto& pair : vec)
	{
		std::cout << pair.first << " " << pair.second << std::endl;
	}
}

inline void test_iterator()
{
	std::vector<int> vec;
	vec.emplace_back(1);
	vec.emplace_back(2);
	vec.emplace_back(3);
	std::cout << "capacity" << vec.capacity() << std::endl;
	auto it = vec.begin();
	std::cout << "begin: " << *it << std::endl;
	vec.pop_back();
	//迭代器没有失效
	std::cout << "begin: " << *it << std::endl;
	std::cout << "capacity" << vec.capacity() << std::endl;
	vec.emplace_back(4);
	std::cout << "capacity" << vec.capacity() << std::endl;
	vec.emplace_back(5);
	std::cout << "capacity" << vec.capacity() << std::endl;
	vec.emplace_back(6);
	std::cout << "capacity" << vec.capacity() << std::endl;
	//迭代器失效
	std::cout << "begin: " << *it << std::endl;
}
inline void test_vector_reserve()
{
	std::vector<int> vec;
	vec.reserve(100);
	//预留100的空间，这样迭代器就不失效
	for (int i = 0; i < 50; i++)
	{
		vec.push_back(i);
	}
	std::cout<< "capacity" << vec.capacity() << std::endl;
	auto it = vec.begin();
	for (int i = 0; i < 10; i++)
	{
		vec.pop_back();
		std::cout << "capacity:" << vec.capacity() << " size: " << vec.size() << std::endl;
		std::cout << *it << std::endl;
	}
}

inline void test_priority_queue()
{
	std::vector<int> vec{2, 3, 5, -10, 100, 9, -3, 77, 64};
	for (auto& i : vec) std::cout << i << ' ';
	std::make_heap(vec.begin(), vec.end());
	std::cout << std::endl;
	std::cout << "vec:";
	for (auto& i : vec) std::cout << i << ' ';
	std::cout << std::endl << "vec(after sort):";
	while (!vec.empty())
	{
		std::pop_heap(vec.begin(), vec.end());
		std::cout << vec.back() << " ";
		vec.pop_back();
	}
}

inline void test_priority_queue_use_decltype()
{
	struct student
	{
		std::string name;
		int score;
	};
	//使用匿名函数直接创建优先队列
	auto compare = [](student left,student right)
		{
			if (left.score != right.score)
				return left.score < right.score;
			else
				return left.name < right.name;
		};
	std::priority_queue<student, std::vector<student>, decltype(compare)> queue(compare);
	queue.emplace(student{ "xiaoming", 60 });
	queue.emplace(student{ "xiaohong", 70 });
	queue.emplace(student{ "xiaohuang", 80 });
	queue.emplace(student{ "xiaogou", 30 });
	queue.emplace(student{ "xiaopi", 39 });
	while (!queue.empty())
	{
		std::cout << "name: " << queue.top().name << " score: " << queue.top().score << std::endl;
		queue.pop();
	}
	//直接将vector转换成堆
	std::vector<student> vec;
	vec.emplace_back(student{ "xiaoming", 60 });
	vec.emplace_back(student{ "xiaohong", 70 });
	vec.emplace_back(student{ "xiaohuang", 80 });
	vec.emplace_back(student{ "xiaogou", 30 });
	vec.emplace_back(student{ "xiaopi", 39 });
	for (auto& value : vec)
	{
		std::cout << "name: " << value.name << " score: " << value.score << std::endl;
	}
	struct compare_student
	{
		bool operator()(const student& left, const student& right)
		{
			if (left.score != right.score)
				return left.score < right.score;
			else
				return left.name < right.name;
		}
	};
	std::priority_queue<student, std::vector<student>, compare_student> queue1(compare_student{}, std::move(vec));
	std::cout <<std::endl;
	while (!queue1.empty())
	{
		std::cout << "name: " << queue1.top().name << " score: " << queue1.top().score << std::endl;
		queue1.pop();
	}
}
