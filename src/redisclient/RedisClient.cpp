/*
* Copyright (C) ylywyn@gmail.com
* License: MIT
*/


#include "RedisClient.h"

#include <boost/make_shared.hpp>
#include <boost/bind.hpp>


RedisClient::RedisClient(boost::asio::io_service &ioService)
	: pimpl_(boost::make_shared<RedisClientImpl>(boost::ref(ioService)))
{
	pimpl_->errorHandler_ = boost::bind(&RedisClientImpl::defaulErrorHandler, pimpl_, _1);
}

RedisClient::~RedisClient()
{
	pimpl_->Close();
}

bool RedisClient::Connect(const boost::asio::ip::tcp::endpoint &endpoint,
	std::string &errmsg)
{
	boost::system::error_code ec;
	pimpl_->socket_.open(endpoint.protocol(), ec);

	if (!ec)
	{
		pimpl_->socket_.set_option(boost::asio::ip::tcp::no_delay(true), ec);
		if (!ec)
		{
			pimpl_->socket_.connect(endpoint, ec);
		}
	}

	if (!ec)
	{
		pimpl_->state_ = RedisClientImpl::Connected;
		return true;
	}
	else
	{
		errmsg = ec.message();
		return false;
	}
}

bool RedisClient::Connect(const boost::asio::ip::address &address,
	unsigned short port,
	std::string &errmsg)
{
	boost::asio::ip::tcp::endpoint endpoint(address, port);
	return Connect(endpoint, errmsg);
}

void RedisClient::InstallErrorHandler(
	const boost::function<void(const std::string &)> &handler)
{
	pimpl_->errorHandler_ = handler;
}

bool RedisClient::Execute(const RedisCommand &c)
{
	pileLine_ = c.IsPipeLineCmd();
	if (stateValid())
	{
		return pimpl_->DoSyncCommand(c);
	}
	return false;
}

