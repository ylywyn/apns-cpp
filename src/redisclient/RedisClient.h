/*
* Copyright (C) ylywyn@gmail.com
* License: MIT
*/

#ifndef REDISCLIENT_REDISCLIENT_H
#define REDISCLIENT_REDISCLIENT_H

#include <string>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include "RedisReply.h"
#include "RedisCommand.h"
#include "RedisClientImpl.h"


class RedisClientImpl;

class RedisClient : boost::noncopyable {
public:
	RedisClient(boost::asio::io_service &ioService);
	~RedisClient();

	// Connect to redis server
	bool Connect(
		const boost::asio::ip::tcp::endpoint &endpoint,
		std::string &errmsg);

	// Connect to redis server
	bool Connect(
		const boost::asio::ip::address &address,
		unsigned short port,
		std::string &errmsg);

	// Set custom error handler. 
	void InstallErrorHandler(
		const boost::function<void(const std::string &)> &handler);


	bool Execute(const RedisCommand &c);

	RedisReply* GetReply() {
		if (!pileLine_ && !pimpl_->replys_.empty())
		{
			return pimpl_->replys_[0];
		}
		return NULL;
	}

	//PipeLineReply
	std::vector<RedisReply*>* GetPipeLineReplys()
	{
		if (pileLine_)
		{ 
			return &pimpl_->replys_;
		} 
		return NULL;
	}
protected:
	bool stateValid() const
	{
		if (pimpl_->state_ == RedisClientImpl::Connected)
		{
			return true;
		}
		return false;
	}

private:
	bool pileLine_;
	boost::shared_ptr<RedisClientImpl> pimpl_;
};

#endif // REDISCLIENT_REDISCLIENT_H
