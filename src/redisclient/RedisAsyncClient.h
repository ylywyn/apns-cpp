/*
* Copyright (C) ylywyn@gmail.com
* License: MIT
*/

#ifndef REDISASYNCCLIENT_REDISCLIENT_H
#define REDISASYNCCLIENT_REDISCLIENT_H

#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <string>
#include <list>

#include "RedisClientImpl.h"
#include "RedisReply.h"
#include "RedisBuffer.h"

class RedisClientImpl;

class RedisAsyncClient : boost::noncopyable {
public:
	struct Handle {
		size_t id;
		std::string channel;
	};

	RedisAsyncClient(boost::asio::io_service &ioService);
	~RedisAsyncClient();

	// Connect to redis server
	void Connect(
		const boost::asio::ip::address &address,
		unsigned short port,
		const boost::function<void(bool, const std::string &)> &handler);

	// Connect to redis server
	void Connect(
		const boost::asio::ip::tcp::endpoint &endpoint,
		const boost::function<void(bool, const std::string &)> &handler);

	// backward compatibility
	inline void AsyncConnect(
		const boost::asio::ip::address &address,
		unsigned short port,
		const boost::function<void(bool, const std::string &)> &handler)
	{
		Connect(address, port, handler);
	}

	// backward compatibility
	inline void AsyncConnect(
		const boost::asio::ip::tcp::endpoint &endpoint,
		const boost::function<void(bool, const std::string &)> &handler)
	{
		Connect(endpoint, handler);
	}


	// Set custom error handler. 
	void InstallErrorHandler(
		const boost::function<void(const std::string &)> &handler);

	bool Execute(RedisCommand* c, CmdReplyCb handler = &dummyHandler);

	// ·¢²¼
	void Publish(const std::string &channel, const RedisBuffer &msg, CmdReplyCb handler = &dummyHandler);

protected:
	void errConnHandler(const std::string &s);

	static void dummyHandler(const std::vector<RedisReply*>&) {}

	bool stateValid() const
	{
		if (pimpl_->state_ == RedisClientImpl::Connected)
		{
			return true;
		}
		return false;
	}

private:
	boost::function<void(bool, const std::string &)> handler_;
	boost::asio::ip::tcp::endpoint endpoint_;
	boost::shared_ptr<RedisClientImpl> pimpl_;
};

#endif // REDISASYNCCLIENT_REDISCLIENT_H
