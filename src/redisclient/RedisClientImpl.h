/*
* Copyright (C) ylywyn@gmail.com
* License: MIT
*/

#ifndef REDISCLIENT_REDISCLIENTIMPL_H
#define REDISCLIENT_REDISCLIENTIMPL_H

#include <string>
#include <queue>

#include <boost/function.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "RedisParser.h"
#include "RedisCommand.h"


typedef void(*CmdReplyCb)(const std::vector<RedisReply*>&);


class RedisClientImpl : public boost::enable_shared_from_this<RedisClientImpl> {
	friend class RedisClient;
	friend class RedisAsyncClient;

public:
	RedisClientImpl(boost::asio::io_service &ioService);
	~RedisClientImpl();

	//处理同步命令
	bool DoSyncCommand(const RedisCommand &c);

	//处理异步命令
	void DoAsyncCommand(RedisCommand* c, CmdReplyCb handler);

	//
	void Close();
	void onRedisError(const RedisReply &);
	void defaulErrorHandler(const std::string &s);
	static void ignoreErrorHandler(const std::string &s);

	template<typename Handler>
	inline void post(const Handler &handler)
	{
		io_service_.post(handler);
	}

private:
	void handleAsyncConnect(const boost::system::error_code &ec,
		const boost::function<void(bool, const std::string &)> &handler);
	void process_message();
	void process_reply();
	void write_handle(const boost::system::error_code &ec, const size_t);
	void read_handle(const boost::system::error_code &ec, const size_t);
	void reset_replys(int n);

private:
	enum {
		NotConnected,
		Connected,
		Connecteing,
		Closed
	} state_;


	struct QueueItem
	{
		QueueItem(RedisCommand *c, CmdReplyCb h) :
			cmd(c), handler(h)
		{}
		RedisCommand * cmd;
		CmdReplyCb handler;
	};

	RedisCommand* last_cmd_;
	std::queue<QueueItem> queue_;
	std::queue<RedisCommand *> cmds_queue_;


	RedisParser parser_;
	boost::array<char, 32> read_buf_;
	std::vector<RedisReply*> replys_;

	boost::asio::ip::tcp::socket socket_;
	boost::asio::io_service &io_service_;

	boost::function<void(const std::string &)> errorHandler_;
};

#endif // REDISCLIENT_REDISCLIENTIMPL_H
