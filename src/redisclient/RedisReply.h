/*
* Copyright (C) ylywyn@gmail.com
* License: MIT
*/

#ifndef REDISCLIENT_REDISREPLY_H
#define REDISCLIENT_REDISREPLY_H
#include <string>
#include <vector>
#include "ObjectPool.h"

enum RedisReplyType
{
	RRT_NIL,
	RRT_ERROR,
	RRT_INT,
	RRT_STRING,
	RRT_BUILK,
	RRT_ARRAY
};

class RedisReply;
extern SyncObjectPool<RedisReply, 5000> gReplyPool;

class RedisReply {
public:
	friend class RedisParser;
	RedisReply() :
		int_data_(0), type_(RRT_NIL)
	{
		data_.resize(1);
	}

	RedisReplyType GetReplyType() { return type_; }
	int GetInt() const { return int_data_; }
	const std::string& GetString()const { return data_[0]; }
	std::string  GetError()
	{
		if (type_ == RRT_ERROR)
		{
			return data_[0];
		}
		return "";
	}
	std::vector<std::string>& GetArray() { return data_; }

	// Return true if value not a error
	bool isError() const { return type_ == RRT_ERROR; }
	bool isOk() const { return type_ != RRT_ERROR; }
	bool isNull() const { return type_ == RRT_NIL; }
	bool isInt() const { return type_ == RRT_INT; }
	bool isArray() const { return type_ == RRT_ARRAY; };
	bool isString() const { return type_ == RRT_STRING; }
	bool isBuilk() const { return type_ == RRT_BUILK; }
	void Clear()
	{
		int l = data_.size();
		for (int i = 0; i < l; ++i)
		{
			data_[i].clear();
		}

		int_data_ = 0;
		type_ = RRT_NIL;
	}

	static RedisReply* NewReply()
	{
		RedisReply* r = gReplyPool.Get();
		if (r == NULL)
		{
			r = new RedisReply();
		}
		return r;
	}

	static inline void DeleteReply(RedisReply* r)
	{
		if (r != NULL)
		{
			r->Clear();
			gReplyPool.Put(r);
		}
	}

	static inline void DeleteReplys(std::vector<RedisReply*>& rs)
	{
		auto iter = rs.begin();
		auto end = rs.end();
		for (; iter != end; ++iter)
		{
			DeleteReply(*iter);
		}
	}
private:
	long int_data_;
	RedisReplyType type_;
	std::vector<std::string> data_;
};

#endif // REDISCLIENT_RedisReply_H
