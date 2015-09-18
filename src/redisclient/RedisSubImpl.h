/*
* Copyright (C) ylywyn@gmail.com
* License: MIT
*/

#ifndef REDISCLIENT_REDISSUBIMPL_H
#define REDISCLIENT_REDISSUBIMPL_H

#include <string>
#include <queue>
#include <unordered_map>

#include <boost/function.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "RedisParser.h"
#include "RedisCommand.h"

typedef void(*SubReplyCb)(const std::string& msg);
typedef void(*SubCmdReplyCb)(const RedisReply*);


class RedisSubImpl : public boost::enable_shared_from_this<RedisSubImpl> {

	friend class RedisSubClient;

public:
	RedisSubImpl(boost::asio::io_service &ioService);
	~RedisSubImpl();

	//
	void Close();
	void DoPubSubCommand(RedisCommand* c, SubCmdReplyCb handler);
	template<typename Handler>
	inline void Post(const Handler &handler)
	{
		io_service_.post(handler);
	}

	void onRedisError(const RedisReply &);
	void defaulErrorHandler(const std::string &s);
	static void ignoreErrorHandler(const std::string &s);

private:
	void handle_connect(const boost::system::error_code &ec, const boost::function<void(bool, const std::string &)> &handler);
	void process_message();
	void process_reply();
	void write_handle(const boost::system::error_code &ec, const size_t);
	void read_handle(const boost::system::error_code &ec, const size_t);

private:
	enum {
		NotConnected,
		Connected,
		Connecteing,
		Subscribed,
		Closed
	} state_;


	struct QueueItem
	{
		QueueItem(RedisCommand *c, SubCmdReplyCb h) :
			cmd(c), handler(h)
		{}
		RedisCommand * cmd;
		SubCmdReplyCb handler;
	};

	size_t subscribeSeq_;
	std::queue<QueueItem> queue_;
	std::queue<RedisCommand *> cmds_queue_;
	std::unordered_map<std::string, SubReplyCb> subcb_maps_;


	RedisParser parser_;
	boost::array<char, 4096> buff_;
	RedisReply* replys_;

	boost::asio::ip::tcp::socket socket_;
	boost::asio::io_service &io_service_;

	boost::function<void(const std::string &)> errorHandler_;
};

#endif // REDISCLIENT_REDISCLIENTIMPL_H
