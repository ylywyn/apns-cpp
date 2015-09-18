/*
* Copyright (C) ylywyn@gmail.com
* License: MIT
*/

#ifndef REDISCLIENT_REDISPARSER_H
#define REDISCLIENT_REDISPARSER_H

#include <stack>
#include <vector>

#include "RedisReply.h"

class RedisParser
{
public:
	RedisParser();

	enum ParseResult {
		CompletedAll,
		CompletedOne,
		Incompleted,
		Error,
	};

	std::pair<size_t, ParseResult> Parse(const char *ptr, size_t size, std::vector<RedisReply*>& rs, int& index);

	//对应sub客户端的 parse, sub端,不能使用管线
	std::pair<size_t, ParseResult> Parse(const char *ptr, size_t size, RedisReply* rs);

protected:
	std::pair<size_t, ParseResult> parse_chunk(const char *ptr, size_t size, std::vector<RedisReply*>& rs, int& index);
	std::pair<size_t, ParseResult> parse_array(const char *ptr, size_t size, std::vector<RedisReply*>& rs, int& index);

	std::pair<size_t, ParseResult> parse_chunk(const char *ptr, size_t size, RedisReply* rs);
	std::pair<size_t, ParseResult> parse_array(const char *ptr, size_t size, RedisReply* rs);

	static inline bool isChar(int c)
	{
		return c >= 0 && c <= 127;
	}

	static inline bool isControl(int c)
	{
		return (c >= 0 && c <= 31) || (c == 127);
	}

	static inline bool isCharNotControl(int c)
	{
		return c > 31 && c < 127;
	}
private:
	enum State {
		Start = 0,

		String = 1,
		StringLF = 2,

		ErrorString = 3,
		ErrorLF = 4,

		Integer = 5,
		IntegerLF = 6,

		BulkSize = 7,
		BulkSizeLF = 8,
		Bulk = 9,
		BulkCR = 10,
		BulkLF = 11,

		ArraySize = 12,
		ArraySizeLF = 13,

	} state;

	long bulk_size_;
	long array_size_;
	std::stack<long int> arrayStack;
	RedisReply* reply_;

	static const char stringReply = '+';
	static const char errorReply = '-';
	static const char integerReply = ':';
	static const char bulkReply = '$';
	static const char arrayReply = '*';
};


#endif // REDISCLIENT_REDISPARSER_H
