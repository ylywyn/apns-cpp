#include "RedisCommand.h"
#include <string>

SyncObjectPool<RedisCommand, 5000> gRedisCommand;

static char temp[16];
static const char crlf[] = { '\r', '\n' };
static const char crlf1[] = "*1\r\n$";
static const char crlf2[] = "*2\r\n$";
static const char crlf3[] = "*3\r\n$";
static const char crlf4[] = "*4\r\n$";
static const char crlf5[] = "*5\r\n$";

#define APPEND_CMD1(cmd)  \
    append(data_, crlf1, 5); \
	append(data_, itoa(strlen(cmd), temp, 10));\
	append<>(data_, crlf);\
	append(data_, cmd); \
	append<>(data_, crlf);

#define APPEND_CMD2(cmd)  \
    append(data_, crlf2, 5); \
	append(data_, itoa(strlen(cmd), temp, 10));\
	append<>(data_, crlf);\
	append(data_, cmd); \
	append<>(data_, crlf);

#define APPEND_CMD3(cmd)  \
    append(data_, crlf3, 5); \
	append(data_, itoa(strlen(cmd), temp, 10));\
	append<>(data_, crlf);\
	append(data_, cmd); \
	append<>(data_, crlf);

#define APPEND_CMD4(cmd)  \
    append(data_, crlf4, 5); \
	append(data_, itoa(strlen(cmd), temp, 10));\
	append<>(data_, crlf);\
	append(data_, cmd); \
	append<>(data_, crlf);

#define APPEND_CMD5(cmd)  \
    append(data_, crlf5, 5); \
	append(data_, itoa(strlen(cmd), temp, 10));\
	append<>(data_, crlf);\
	append(data_, cmd); \
	append<>(data_, crlf);

#define  APPEND_ARG(arg)\
	append(data_, '$'); \
	append(data_, itoa(arg.size(), temp, 10));\
	append<>(data_, crlf);\
	append(data_, arg.data());\
	append<>(data_, crlf);

void RedisCommand::Build(const char* cmd)
{
	count_ = 1;
	data_.clear();
	APPEND_CMD1(cmd);
}

void RedisCommand::Build(const char* cmd, const RedisBuffer &arg1)
{
	count_ = 1;
	data_.clear();
	APPEND_CMD2(cmd);
	APPEND_ARG(arg1);
}


void RedisCommand::Build(const char* cmd, const RedisBuffer &arg1, const RedisBuffer &arg2)
{
	count_ = 1;
	data_.clear();
	APPEND_CMD3(cmd);
	APPEND_ARG(arg1);
	APPEND_ARG(arg2);
}


void RedisCommand::Build(const char* cmd, const RedisBuffer &arg1,
	const RedisBuffer &arg2, const RedisBuffer &arg3)
{
	count_ = 1;
	data_.clear();
	APPEND_CMD4(cmd);
	APPEND_ARG(arg1);
	APPEND_ARG(arg2);
	APPEND_ARG(arg3);
}

void RedisCommand::Build(const char* cmd, const RedisBuffer &arg1,
	const RedisBuffer &arg2, const RedisBuffer &arg3, const RedisBuffer &arg4)
{
	count_ = 1;
	data_.clear();
	APPEND_CMD5(cmd);
	APPEND_ARG(arg1);
	APPEND_ARG(arg2);
	APPEND_ARG(arg3);
	APPEND_ARG(arg4);
}

void RedisCommand::Build(const char* cmd, const std::vector<std::string> &args)
{
	count_ = 1;
	data_.clear();
	append(data_, '*');
	append(data_, itoa(args.size() + 1, temp, 10));
	append<>(data_, crlf);

	append(data_, '$');
	append(data_, itoa(strlen(cmd), temp, 10));
	append<>(data_, crlf);
	append(data_, cmd);
	append<>(data_, crlf);

	std::vector<std::string>::const_iterator it = args.begin(), end = args.end();
	for (; it != end; ++it)
	{
		append(data_, '$');
		append(data_, itoa(it->size(), temp, 10));
		append<>(data_, crlf);
		append(data_, *it);
		append<>(data_, crlf);
	}
}

void RedisCommand::AppendBuild(const char* cmd)
{
	++count_;
	APPEND_CMD1(cmd);
}

void RedisCommand::AppendBuild(const char* cmd, const RedisBuffer &arg1)
{
	++count_;
	APPEND_CMD2(cmd);
	APPEND_ARG(arg1);
}

void RedisCommand::AppendBuild(const char* cmd, const RedisBuffer &arg1, const RedisBuffer &arg2)
{
	++count_;
	APPEND_CMD3(cmd);
	APPEND_ARG(arg1);
	APPEND_ARG(arg2);
}

void RedisCommand::AppendBuild(const char* cmd, const RedisBuffer &arg1,
	const RedisBuffer &arg2, const RedisBuffer &arg3)
{
	++count_;
	APPEND_CMD4(cmd);
	APPEND_ARG(arg1);
	APPEND_ARG(arg2);
	APPEND_ARG(arg3);
}

void RedisCommand::AppendBuild(const char* cmd, const RedisBuffer &arg1,
	const RedisBuffer &arg2, const RedisBuffer &arg3, const RedisBuffer &arg4)
{
	++count_;
	APPEND_CMD5(cmd);
	APPEND_ARG(arg1);
	APPEND_ARG(arg2);
	APPEND_ARG(arg3);
	APPEND_ARG(arg4);
}

void RedisCommand::AppendBuild(const char* cmd, const std::vector<std::string> &args)
{
	++count_;
	append(data_, '*');
	append(data_, itoa(args.size() + 1, temp, 10));
	append<>(data_, crlf);

	append(data_, '$');
	append(data_, itoa(strlen(cmd), temp, 10));
	append<>(data_, crlf);
	append(data_, cmd);
	append<>(data_, crlf);

	std::vector<std::string>::const_iterator it = args.begin(), end = args.end();
	for (; it != end; ++it)
	{
		append(data_, '$');
		append(data_, itoa(it->size(), temp, 10));
		append<>(data_, crlf);
		append(data_, *it);
		append<>(data_, crlf);
	}
}