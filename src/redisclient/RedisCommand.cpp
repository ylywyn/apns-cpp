#include "RedisCommand.h"
#include <string.h>
#include <string>

SyncObjectPool<RedisCommand, 5000> gRedisCommand;

static const char crlf[] = { '\r', '\n' };
static const char crlf1[] = "*1\r\n$";
static const char crlf2[] = "*2\r\n$";
static const char crlf3[] = "*3\r\n$";
static const char crlf4[] = "*4\r\n$";
static const char crlf5[] = "*5\r\n$";


#define APPEND_CMD1(cmd, t)  \
    append(data_, crlf1, 5); \
    sprintf(t, "%d", strlen(cmd));\
	append(data_, t);\
	append<>(data_, crlf);\
	append(data_, cmd); \
	append<>(data_, crlf);

#define APPEND_CMD2(cmd, t)  \
    append(data_, crlf2, 5); \
    sprintf(t, "%d", strlen(cmd));\
	append(data_, t);\
	append<>(data_, crlf);\
	append(data_, cmd); \
	append<>(data_, crlf);

#define APPEND_CMD3(cmd, t)  \
    append(data_, crlf3, 5); \
    sprintf(t, "%d", strlen(cmd));\
	append(data_, t);\
	append<>(data_, crlf);\
	append(data_, cmd); \
	append<>(data_, crlf);

#define APPEND_CMD4(cmd, t)  \
    append(data_, crlf4, 5); \
    sprintf(t, "%d", strlen(cmd));\
	append(data_, t);\
	append<>(data_, crlf);\
	append(data_, cmd); \
	append<>(data_, crlf);

#define APPEND_CMD5(cmd, t)  \
    append(data_, crlf5, 5); \
    sprintf(t, "%d", strlen(cmd));\
	append(data_, t);\
	append<>(data_, crlf);\
	append(data_, cmd); \
	append<>(data_, crlf);

#define  APPEND_ARG(arg, t)\
	append(data_, '$'); \
    sprintf(t, "%d", arg.size());\
	append(data_, t);\
	append<>(data_, crlf);\
	append(data_, arg.data());\
	append<>(data_, crlf);

void RedisCommand::Build(const char* cmd)
{
	count_ = 1;
	data_.clear();
	char temp[16];

	APPEND_CMD1(cmd, temp);
}

void RedisCommand::Build(const char* cmd, const RedisBuffer &arg1)
{
	count_ = 1;
	data_.clear();
	char temp[16];

	APPEND_CMD2(cmd, temp);
	APPEND_ARG(arg1, temp);
}


void RedisCommand::Build(const char* cmd, const RedisBuffer &arg1, const RedisBuffer &arg2)
{
	count_ = 1;
	data_.clear();
	char temp[16];

	APPEND_CMD3(cmd, temp);
	APPEND_ARG(arg1, temp);
	APPEND_ARG(arg2, temp);
}


void RedisCommand::Build(const char* cmd, const RedisBuffer &arg1,
	const RedisBuffer &arg2, const RedisBuffer &arg3)
{
	count_ = 1;
	data_.clear();
	char temp[16];

	APPEND_CMD4(cmd, temp);
	APPEND_ARG(arg1, temp);
	APPEND_ARG(arg2, temp);
	APPEND_ARG(arg3, temp);
}

void RedisCommand::Build(const char* cmd, const RedisBuffer &arg1,
	const RedisBuffer &arg2, const RedisBuffer &arg3, const RedisBuffer &arg4)
{
	count_ = 1;
	data_.clear();
	char temp[16];

	APPEND_CMD5(cmd, temp);
	APPEND_ARG(arg1, temp);
	APPEND_ARG(arg2, temp);
	APPEND_ARG(arg3, temp);
	APPEND_ARG(arg4, temp);
}

void RedisCommand::Build(const char* cmd, const std::vector<std::string> &args)
{
	count_ = 1;
	data_.clear();
	char temp[16];

	append(data_, '*');
	sprintf(temp, "%d", args.size() + 1);
	append(data_, temp);
	append<>(data_, crlf);

	append(data_, '$');
	sprintf(temp, "%d", strlen(cmd));
	append(data_, temp);
	append<>(data_, crlf);
	append(data_, cmd);
	append<>(data_, crlf);

	std::vector<std::string>::const_iterator it = args.begin(), end = args.end();
	for (; it != end; ++it)
	{
		append(data_, '$');
		sprintf(temp, "%d", it->size());
		append(data_, temp);
		append<>(data_, crlf);
		append(data_, *it);
		append<>(data_, crlf);
	}
}

void RedisCommand::AppendBuild(const char* cmd)
{
	++count_;
	char temp[16];

	APPEND_CMD1(cmd, temp);
}

void RedisCommand::AppendBuild(const char* cmd, const RedisBuffer &arg1)
{
	++count_;
	char temp[16];

	APPEND_CMD2(cmd, temp);
	APPEND_ARG(arg1, temp);
}

void RedisCommand::AppendBuild(const char* cmd, const RedisBuffer &arg1, const RedisBuffer &arg2)
{
	++count_;
	char temp[16];

	APPEND_CMD3(cmd, temp);
	APPEND_ARG(arg1, temp);
	APPEND_ARG(arg2, temp);
}

void RedisCommand::AppendBuild(const char* cmd, const RedisBuffer &arg1,
	const RedisBuffer &arg2, const RedisBuffer &arg3)
{
	++count_;
	char temp[16];

	APPEND_CMD4(cmd, temp);
	APPEND_ARG(arg1, temp);
	APPEND_ARG(arg2, temp);
	APPEND_ARG(arg3, temp);
}

void RedisCommand::AppendBuild(const char* cmd, const RedisBuffer &arg1,
	const RedisBuffer &arg2, const RedisBuffer &arg3, const RedisBuffer &arg4)
{
	++count_;
	char temp[16];

	APPEND_CMD5(cmd, temp);
	APPEND_ARG(arg1, temp);
	APPEND_ARG(arg2, temp);
	APPEND_ARG(arg3, temp);
	APPEND_ARG(arg4, temp);
}

void RedisCommand::AppendBuild(const char* cmd, const std::vector<std::string> &args)
{
	++count_;
	char temp[16];

	append(data_, '*');
	sprintf(temp, "%d", args.size() + 1);
	append(data_, temp);
	append<>(data_, crlf);

	append(data_, '$');
	sprintf(temp, "%d", strlen(cmd));
	append(data_, temp);
	append<>(data_, crlf);
	append(data_, cmd);
	append<>(data_, crlf);

	std::vector<std::string>::const_iterator it = args.begin(), end = args.end();
	for (; it != end; ++it)
	{
		append(data_, '$');
		sprintf(temp, "%d", it->size());
		append(data_, temp);
		append<>(data_, crlf);
		append(data_, *it);
		append<>(data_, crlf);
	}
}