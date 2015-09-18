#ifndef NOTIFICATION_POOL_H
#define NOTIFICATION_POOL_H
#include <queue>
#include <boost/lockfree/queue.hpp>
#include <boost/noncopyable.hpp>

template < class T, size_t c>
class SyncObjectPool : private boost::noncopyable
{
public:
	SyncObjectPool() :pool_(c)
	{}
	~SyncObjectPool() { clear(); }

	T* Get()
	{
		T* obj = NULL;
		pool_.pop(obj);
		return obj;
	}

	void Put(T* obj)
	{
		if (!pool_.push(obj))
		{
			delete obj;
			clear((size_t)(c*0.6));
		}
	}

private:
	void clear(size_t n = 0)
	{
		T* obj = NULL;
		if (n == 0)
		{
			while (true)
			{
				pool_.pop(obj);
				if (obj != NULL)
				{
					delete obj;
					obj = NULL;
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			while (--n > 0)
			{
				pool_.pop(obj);
				if (obj != NULL)
				{
					delete obj;
					obj = NULL;
				}
				else
				{
					break;
				}
			}
		}
	}

private:
	boost::lockfree::queue<T*, boost::lockfree::fixed_sized<true>> pool_;
};

template < class T, size_t c>
class ObjectPool : private boost::noncopyable
{
public:
	ObjectPool()
	{}
	~ObjectPool() { clear(); }

	T* Get()
	{
		T* obj = NULL;
		if (!pool_.empty())
		{
			obj = pool_.front();
			pool_.pop();
		}
		return obj;
	}

	void Put(T* obj)
	{
		if (pool_.size() < c)
		{
			pool_.push(obj);
		}
		else
		{
			delete obj;
			clear((size_t)(c*0.4));
		}
	}

private:
	void clear(size_t n = 0)
	{
		T* obj = NULL;
		if (n == 0)
		{
			while (!pool_.empty())
			{
				obj = pool_.front();
				pool_.pop();
				if (obj != NULL)
				{
					delete obj;
				}
			}
		}
		else
		{
			while (!pool_.empty() && (--n > 0))
			{
				obj = pool_.front();
				pool_.pop();
				if (obj != NULL)
				{
					delete obj;
				}
			}
		}
	}

private:
	std::queue<T*> pool_;
};


#endif