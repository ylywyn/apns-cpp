/*
* Copyright (C) ylywyn@gmail.com
* License: MIT
*/

#ifndef REDISSUBCLIENT_REDISCLIENT_H
#define REDISSUBCLIENT_REDISCLIENT_H

#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <string>
#include <list>

#include "RedisSubImpl.h"
#include "RedisReply.h"
#include "RedisBuffer.h"

class RedisSubClient : boost::noncopyable {
public:
	struct Handle {
		size_t id;
		std::string channel;
	};

	RedisSubClient(boost::asio::io_service &ioService);
	~RedisSubClient();

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

	//¶©ÔÄ
	int Subscribe(
		const std::string &channelName,
		SubReplyCb msgHandler,
		SubCmdReplyCb handler = &dummyHandler);

	//È¡Ïû¶©ÔÄ
	void Unsubscribe(const std::string &channel);


protected:
	void errConnHandler(const std::string &s);
	static void dummyHandler(const RedisReply*) {}

	bool stateValid() const
	{
		if (pimpl_->state_ == RedisSubImpl::Connected || pimpl_->state_ == RedisSubImpl::Subscribed)
		{
			return true;
		}
		return false;
	}

private:
	boost::function<void(bool, const std::string &)> handler_;
	boost::asio::ip::tcp::endpoint endpoint_;
	boost::shared_ptr<RedisSubImpl> pimpl_;
};

#endif // REDISASYNCCLIENT_REDISCLIENT_H
