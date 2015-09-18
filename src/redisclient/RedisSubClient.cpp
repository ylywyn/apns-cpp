/*
* Copyright (C) ylywyn@gmail.com
* License: MIT
*/

#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <thread>
#include "RedisSubClient.h"
#include "Log.h"

RedisSubClient::RedisSubClient(boost::asio::io_service &ioService)
	: pimpl_(boost::make_shared<RedisSubImpl>(boost::ref(ioService)))
{
	pimpl_->errorHandler_ = boost::bind(&RedisSubClient::errConnHandler, this, _1);
}

RedisSubClient::~RedisSubClient()
{
	pimpl_->Close();
}

void RedisSubClient::Connect(const boost::asio::ip::address &address,
	unsigned short port,
	const boost::function<void(bool, const std::string &)> &handler)
{
	boost::asio::ip::tcp::endpoint endpoint(address, port);
	Connect(endpoint, handler);
}

void RedisSubClient::Connect(const boost::asio::ip::tcp::endpoint &endpoint,
	const boost::function<void(bool, const std::string &)> &handler)
{
	endpoint_ = endpoint;
	handler_ = handler;
	pimpl_->socket_.async_connect(endpoint, boost::bind(&RedisSubImpl::handle_connect,
		pimpl_, _1, handler));
}


void RedisSubClient::InstallErrorHandler(
	const boost::function<void(const std::string &)> &handler)
{
	pimpl_->errorHandler_ = handler;
}

//
int RedisSubClient::Subscribe(
	const std::string &channel,
	SubReplyCb msgHandler,
	SubCmdReplyCb handler)
{
	assert(pimpl_->state_ == RedisSubImpl::Connected ||
		pimpl_->state_ == RedisSubImpl::Subscribed);

	static const std::string subscribeStr = "SUBSCRIBE";

	if (pimpl_->state_ == RedisSubImpl::Subscribed || pimpl_->state_ == RedisSubImpl::Connected)
	{
		auto c = RedisCommand::NewCommand();
		c->Build(subscribeStr.c_str(), channel);

		pimpl_->Post(boost::bind(&RedisSubImpl::DoPubSubCommand, pimpl_, c, handler));
		pimpl_->subcb_maps_.insert(std::make_pair(channel, msgHandler));
		pimpl_->state_ = RedisSubImpl::Subscribed;
		return ++pimpl_->subscribeSeq_;
	}
	else
	{
		std::stringstream ss;
		ss << "RedisSubClient::Subscribe called with invalid state " << pimpl_->state_;
		LOGFMTW(ss.str().c_str());
		return -1;
	}
}

void RedisSubClient::Unsubscribe(const std::string &channel)
{
#ifdef DEBUG
	static int recursion = 0;
	assert(recursion++ == 0);
#endif

	assert(pimpl_->state_ == RedisSubImpl::Connected ||
		pimpl_->state_ == RedisSubImpl::Subscribed);

	static const std::string unsubscribeStr = "UNSUBSCRIBE";

	if (pimpl_->state_ == RedisSubImpl::Connected ||
		pimpl_->state_ == RedisSubImpl::Subscribed)
	{

		pimpl_->subcb_maps_.erase(channel);

		auto c = RedisCommand::NewCommand();
		c->Build(unsubscribeStr.c_str(), channel);

		pimpl_->Post(boost::bind(&RedisSubImpl::DoPubSubCommand, pimpl_, c, dummyHandler));
	}
	else
	{
		std::stringstream ss;
		ss << "RedisSubClient::Unsubscribe called with invalid state " << pimpl_->state_;
		LOGFMTW(ss.str().c_str());

#ifdef DEBUG
		--recursion;
#endif
		return;
	}

#ifdef DEBUG
	--recursion;
#endif
	}

void RedisSubClient::errConnHandler(const std::string &s)
{
	LOGFMTW(s.c_str());
	pimpl_->Close();
	std::this_thread::sleep_for(std::chrono::seconds(7));
	pimpl_->socket_.async_connect(endpoint_, boost::bind(&RedisSubImpl::handle_connect, pimpl_, _1, handler_));
}