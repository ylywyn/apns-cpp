
#ifndef MUDUO_BASE_SINGLETON_H
#define MUDUO_BASE_SINGLETON_H

#include <boost/noncopyable.hpp>
#include <assert.h>
#include <mutex>
#include <memory>

template<typename T>
class Singleton : boost::noncopyable
{
public:
	static T& Instance()
	{
		std::call_once(ponce_, &Singleton::init);
		assert(value_);
		return *value_;
	}

private:
	Singleton();
	~Singleton();

	static void init()
	{
		value_ = std::make_shared<T>();
	}


private:
	static std::once_flag  ponce_;
	static std::shared_ptr<T> value_;
};

template<typename T>
std::once_flag Singleton<T>::ponce_;

template<typename T>
std::shared_ptr<T> Singleton<T>::value_;
#endif