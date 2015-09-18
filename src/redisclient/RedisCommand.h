#ifndef REDISCLIENT_COMMAND_H
#define REDISCLIENT_COMMAND_H
#include <vector>
#include "ObjectPool.h"
#include "RedisBuffer.h"

class RedisCommand;
extern SyncObjectPool<RedisCommand, 5000> gRedisCommand;
class RedisCommand
{
public:
	RedisCommand() :count_(0)
	{
		data_.reserve(256);
	}
	~RedisCommand()
	{
	}

	void Build(const char* cmd);
	void Build(const char* cmd, const RedisBuffer &arg1);
	void Build(const char* cmd, const RedisBuffer &arg1, const RedisBuffer &arg2);
	void Build(const char* cmd, const RedisBuffer &arg1,
		const RedisBuffer &arg2, const RedisBuffer &arg3);
	void Build(const char* cmd, const RedisBuffer &arg1,
		const RedisBuffer &arg2, const RedisBuffer &arg3, const RedisBuffer &arg4);
	void Build(const char* cmd, const std::vector<std::string> &args);

	//PipeLine Command
	void AppendBuild(const char* cmd);
	void AppendBuild(const char* cmd, const RedisBuffer &arg1);
	void AppendBuild(const char* cmd, const RedisBuffer &arg1, const RedisBuffer &arg2);
	void AppendBuild(const char* cmd, const RedisBuffer &arg1,
		const RedisBuffer &arg2, const RedisBuffer &arg3);
	void AppendBuild(const char* cmd, const RedisBuffer &arg1,
		const RedisBuffer &arg2, const RedisBuffer &arg3, const RedisBuffer &arg4);
	void AppendBuild(const char* cmd, const std::vector<std::string> &args);

	void Clear() { count_ = 0; data_.clear(); }
	int Size() const { return data_.size(); }
	const char* Data() const { return data_.data(); }

	//debug
	std::string ToString()const { std::string ret(data_.begin(), data_.end()); return ret; }

	bool IsPipeLineCmd()const { return count_ > 1; }
	int GetCmdCount() const { return count_; }

	static RedisCommand* NewCommand()
	{
		RedisCommand* c = gRedisCommand.Get();
		if (c == NULL)
		{
			c = new RedisCommand();
		}
		return c;
	}

	static inline void DeleteCommand(RedisCommand* c)
	{
		if (c != NULL)
		{
			c->Clear();
			gRedisCommand.Put(c);
		}
	}

private:
	static void append(std::vector<char> &vec, const RedisBuffer &buf)
	{
		vec.insert(vec.end(), buf.data(), buf.data() + buf.size());
	}

	static void append(std::vector<char> &vec, const std::string &s)
	{
		vec.insert(vec.end(), s.begin(), s.end());
	}

	static void append(std::vector<char> &vec, const char *s)
	{
		vec.insert(vec.end(), s, s + strlen(s));
	}

	static void append(std::vector<char> &vec, const char *s, size_t len)
	{
		vec.insert(vec.end(), s, s + len);
	}
	
	static void append(std::vector<char> &vec, char c)
	{
		vec.push_back(c);
	}

	template<size_t size>
	static inline void append(std::vector<char> &vec, const char(&s)[size])
	{
		vec.insert(vec.end(), s, s + size);
	}
private:
	uint8_t count_;
	std::vector<char> data_;
};

#endif