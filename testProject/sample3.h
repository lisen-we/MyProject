#pragma once
#include <string>
#include <iostream>
#include <vector>
//不用虚函数而使用函数指针来实现多态
struct Animal;
using speak = void(*)(const Animal* self);
using status = void(*)(const Animal* self);
using information = void(*)(const Animal* self);
struct animal_vtable
{
	//模拟的虚函数表
	speak speak_fun;
	status status_fun;
	information information_fun;
};
struct Animal
{
	std::string name;
	std::size_t age;
	const animal_vtable* vptr;
};
struct Date {
	std::size_t year;
	std::size_t month;
	std::size_t day;
};
struct Dog
{
	Animal base;
	Date date;
	std::size_t speak_count;

};
struct Cat
{
	Animal base;
	Date date;
};
static void dog_speak(const Animal* self)
{
	//使用C风格的转换而不是使用static_cast或者dynamic_cast,因为前者只是对于内存进行重新解释，而后者要求有实际的继承关系
	const Dog* dog = (Dog*)(self); 
	for (std::size_t i = 0; i < dog->speak_count; i++)
	{
		std::cout << "汪汪！" << std::endl;
	}
}
static void cat_speak(const Animal* self)
{
	std::cout << "喵喵！" << std::endl;
}
static void dog_status(const Animal* self)
{
	std::cout << "在吃骨头" << std::endl;
}
static void cat_status(const Animal* self)
{
	std::cout << "在睡觉" << std::endl;
}
static void dog_information(const Animal* self)
{
	Dog* dog = (Dog*)self;
	std::cout << std::string(20, '=') << std::endl;
	std::cout << "当前动物的类别是狗" << std::endl;
	std::cout << "age: " << dog->base.age << std::endl;
	std::cout << "name: " << dog->base.name << std::endl;
}
static void cat_information(const Animal* self)
{
	Cat* cat = (Cat*)self;
	std::cout << std::string(20, '=')<<std::endl;
	std::cout << "当前动物的类别是猫" << std::endl;
	std::cout << "age: " << cat->base.age << std::endl;
	std::cout << "name: " << cat->base.name << std::endl;
}
static const animal_vtable dog_vtable = { dog_speak,dog_status,dog_information};
static const animal_vtable cat_vtable = { cat_speak,cat_status,cat_information };
inline void dog_init(Dog* dog, Date date, std::size_t speak_count,std::string name,std::size_t age)
{
	dog->date = date;
	dog->speak_count = speak_count;
	dog->base.name = name;
	dog->base.age = age;
	dog->base.vptr = &dog_vtable;
}
inline void cat_init(Cat* cat, Date date, std::string name, std::size_t age)
{
	cat->date = date;
	cat->base.name = name;
	cat->base.age = age;
	cat->base.vptr = &cat_vtable;
}
inline void animal_information(Animal* a)
{
	a->vptr->information_fun(a);
}
inline void animal_speak(Animal* a)
{
	a->vptr->speak_fun(a);
}
inline void animal_status(Animal* a)
{
	a->vptr->status_fun(a);
}
inline int sample3_polymorphism_with_function_pointer()
{
	//测试使用函数指针来实现多态
	std::vector<Animal*> vec;
	Dog dog;
	Cat cat;
	Date dog_brith = { 2021,2,14 };
	Date cat_brith = { 2024,3,11 };
	dog_init(&dog, dog_brith, 3, "大黄", 5);
	cat_init(&cat, cat_brith, "小咪", 2);
	vec.push_back((Animal*)(&dog));
	vec.push_back((Animal*)(&cat));
	while (!vec.empty())
	{
		Animal* animal = vec.back();
		vec.pop_back();
		animal_information(animal);
		animal_status(animal);
		animal_speak(animal);
	}
	return 0;
}