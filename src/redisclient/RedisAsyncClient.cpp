/*
* Copyright (C) ylywyn@gmail.com
* License: MIT
*/

#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <thread>
#include "RedisAsyncClient.h"
#include "Log.h"

RedisAsyncClient::RedisAsyncClient(boost::asio::io_service &ioService)
	: pimpl_(boost::make_shared<RedisClientImpl>(boost::ref(ioService)))
{
	pimpl_->errorHandler_ = boost::bind(&RedisAsyncClient::errConnHandler, this, _1);
}

RedisAsyncClient::~RedisAsyncClient()
{
	pimpl_->Close();
}

void RedisAsyncClient::Connect(const boost::asio::ip::address &address,
	unsigned short port,
	const boost::function<void(bool, const std::string &)> &handler)
{
	boost::asio::ip::tcp::endpoint endpoint(address, port);
	Connect(endpoint, handler);
}

void RedisAsyncClient::Connect(const boost::asio::ip::tcp::endpoint &endpoint,
	const boost::function<void(bool, const std::string &)> &handler)
{
	endpoint_ = endpoint;
	handler_ = handler;
	pimpl_->socket_.async_connect(endpoint, boost::bind(&RedisClientImpl::handleAsyncConnect,
		pimpl_, _1, handler));
}


void RedisAsyncClient::InstallErrorHandler(
	const boost::function<void(const std::string &)> &handler)
{
	pimpl_->errorHandler_ = handler;
}

bool RedisAsyncClient::Execute(RedisCommand* c, CmdReplyCb handler)
{
	if (stateValid())
	{
		pimpl_->post(boost::bind(&RedisClientImpl::DoAsyncCommand, pimpl_, c, handler));
		return true;
	}
	return false;
}

void RedisAsyncClient::Publish(const std::string &channel, const RedisBuffer &msg, CmdReplyCb handler)
{
	static const std::string publishStr = "PUBLISH";

	if (pimpl_->state_ == RedisClientImpl::Connected)
	{
		auto c = RedisCommand::NewCommand();
		c->Build(publishStr.c_str(), channel, msg);
		pimpl_->post(boost::bind(&RedisClientImpl::DoAsyncCommand, pimpl_, c, handler));
	}
	else
	{
		std::stringstream ss;
		ss << "RedisSubClient::command called with invalid state " << pimpl_->state_;
		LOGFMTW(ss.str().c_str());
	}
}


void RedisAsyncClient::errConnHandler(const std::string &s)
{
	LOGFMTW(s.c_str());
	pimpl_->Close();

	std::this_thread::sleep_for(std::chrono::seconds(6));
	pimpl_->socket_.async_connect(endpoint_, boost::bind(&RedisClientImpl::handleAsyncConnect, pimpl_, _1, handler_));
}