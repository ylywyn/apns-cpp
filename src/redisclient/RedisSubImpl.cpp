/*
* Copyright (C) ylywyn@gmail.com
* License: MIT
*/

#include <algorithm>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <thread>
#include "RedisSubImpl.h"
#include "Log.h"


RedisSubImpl::RedisSubImpl(boost::asio::io_service &ioService)
	: state_(NotConnected), io_service_(ioService),
	socket_(ioService), subscribeSeq_(0)
{
	replys_ = new RedisReply();
}

RedisSubImpl::~RedisSubImpl()
{
	delete replys_;
	Close();
}

void RedisSubImpl::Close()

{
	if (state_ != RedisSubImpl::Closed)
	{
		state_ = RedisSubImpl::Closed;
		boost::system::error_code ignored_ec;

		//errorHandler_ = boost::bind(&RedisSubImpl::ignoreErrorHandler, _1);
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		socket_.close(ignored_ec);
	}
}

void RedisSubImpl::handle_connect(const boost::system::error_code &ec,
	const boost::function<void(bool, const std::string &)> &handler)
{
	if (!ec)
	{
		socket_.set_option(boost::asio::ip::tcp::no_delay(true));
		state_ = RedisSubImpl::Connected;
		handler(true, std::string());
		process_message();
	}
	else
	{
		//handler(false, ec.message());
		errorHandler_(ec.message());
	}
}

void RedisSubImpl::process_message()
{
	using boost::system::error_code;
	socket_.async_read_some(boost::asio::buffer(buff_),
		boost::bind(&RedisSubImpl::read_handle,
			shared_from_this(), _1, _2));
}

void RedisSubImpl::process_reply()
{

	if (replys_->GetReplyType() == RRT_ARRAY)
	{
		auto& arr = replys_->GetArray();
		if (arr.size() == 3)
		{
			const std::string &cmd = arr[0];
			const std::string &channel = arr[1];
			const std::string &value = arr[2];

			if (cmd == "message")
			{
				auto iter = subcb_maps_.find(channel);
				if (iter != subcb_maps_.end())
				{
					iter->second(value);
				}
				return;
			}
		}
	}

	if (!queue_.empty())
	{
		QueueItem& item = queue_.front();
		item.handler(replys_);
		RedisCommand::DeleteCommand(item.cmd);
		queue_.pop();
		return;
	}
	LOGFMTW("queue_.empty");
}

void RedisSubImpl::DoPubSubCommand(RedisCommand* c, SubCmdReplyCb handler)
{
	cmds_queue_.push(c);
	queue_.push(QueueItem(c, handler));

	if (cmds_queue_.size() == 1)
	{
		boost::asio::async_write(socket_,
			boost::asio::buffer(c->Data(), c->Size()),
			boost::bind(&RedisSubImpl::write_handle, shared_from_this(), _1, _2));
	}
}


void RedisSubImpl::write_handle(const boost::system::error_code &ec, const size_t)
{
	if (ec)
	{
		errorHandler_(ec.message());
		return;
	}
	else
	{
		cmds_queue_.pop();
	}

	if (!cmds_queue_.empty())
	{
		RedisCommand* c = cmds_queue_.front();
		boost::asio::async_write(socket_,
			boost::asio::buffer(c->Data(), c->Size()),
			boost::bind(&RedisSubImpl::write_handle, shared_from_this(), _1, _2));
	}
}

void RedisSubImpl::read_handle(const boost::system::error_code &ec, const size_t size)
{
	if (ec || size == 0)
	{
		errorHandler_(ec.message());
		return;
	}

	for (size_t pos = 0; pos < size;)
	{
		auto result = parser_.Parse(buff_.data() + pos, size - pos, replys_);
		if (result.second == RedisParser::CompletedOne)
		{
			process_reply();
			replys_->Clear();
		}
		else if (result.second == RedisParser::Incompleted)
		{
			//processMessage();
			//continue;
		}
		else
		{
			errorHandler_("[async parse] Parser error");
			return;
		}
		pos += result.first;
	}
	process_message();
}

void RedisSubImpl::onRedisError(const RedisReply &v)
{
	//std::string message = v.toString();
	//errorHandler(message);
}

void RedisSubImpl::defaulErrorHandler(const std::string &s)
{
	LOGFMTW(s.c_str());
	//Close();
}

void RedisSubImpl::ignoreErrorHandler(const std::string &)
{
	// empty
}


