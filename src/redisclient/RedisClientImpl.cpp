/*
* Copyright (C) ylywyn@gmail.com
* License: MIT
*/

#include <algorithm>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <thread>
#include "RedisClientImpl.h"

#include <iostream>
using namespace std;


RedisClientImpl::RedisClientImpl(boost::asio::io_service &ioService)
	: state_(NotConnected), io_service_(ioService),
	socket_(ioService), last_cmd_(NULL)
{
	replys_.resize(1);
}

RedisClientImpl::~RedisClientImpl()
{
	Close();
}

void RedisClientImpl::Close()
{
	if (state_ != RedisClientImpl::Closed)
	{
		boost::system::error_code ignored_ec;

		//errorHandler_ = boost::bind(&RedisClientImpl::ignoreErrorHandler, _1);
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		socket_.close(ignored_ec);
		state_ = RedisClientImpl::Closed;
	}
}

void RedisClientImpl::handleAsyncConnect(const boost::system::error_code &ec,
	const boost::function<void(bool, const std::string &)> &handler)
{
	if (!ec)
	{
		socket_.set_option(boost::asio::ip::tcp::no_delay(true));
		state_ = RedisClientImpl::Connected;
		handler(true, std::string());
		process_message();
	}
	else
	{
		//handler(false, ec.message());
		errorHandler_(ec.message());
	}
}

void RedisClientImpl::process_message()
{
	using boost::system::error_code;
	socket_.async_read_some(boost::asio::buffer(read_buf_),
		boost::bind(&RedisClientImpl::read_handle,
			shared_from_this(), _1, _2));
}

void RedisClientImpl::process_reply()
{
	assert(!queue_.empty());

	QueueItem& item = queue_.front();

	//处理
	/*if (state_ == RedisClientImpl::Subscribed)
	{
		auto& arr = replys_[0]->GetArray();
		if (arr.size() == 3)
		{
			const string &cmd = arr[0];
			const string &channel = arr[1];
			const string &value = arr[2];

			if (cmd == "message")
			{
				auto iter = subcb_maps_.find(channel);
				if (iter != subcb_maps_.end())
				{
					iter->second(value);
				}
			}
			else if (cmd == "subscribe")
			{
				item.handler(replys_);
			}
			else if (cmd == "unsubscribe")
			{
				item.handler(replys_);
			}
			else
			{
				std::stringstream ss;
				ss << "[RedisClient] invalid command: " << cmd;
				errorHandler_(ss.str());
				return;
			}
		}
		else
		{
			errorHandler_("[RedisClient] Protocol error");
			return;
		}
	}
	else*/
	{
		item.handler(replys_);
	}
}

void RedisClientImpl::DoAsyncCommand(RedisCommand* c, CmdReplyCb handler)
{
	cmds_queue_.push(c);
	queue_.push(QueueItem(c, handler));

	if (cmds_queue_.size() == 1)
	{
		boost::asio::async_write(socket_,
			boost::asio::buffer(c->Data(), c->Size()),
			boost::bind(&RedisClientImpl::write_handle, shared_from_this(), _1, _2));
	}
}


void RedisClientImpl::write_handle(const boost::system::error_code &ec, const size_t)
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
			boost::bind(&RedisClientImpl::write_handle, shared_from_this(), _1, _2));
	}
}

void RedisClientImpl::read_handle(const boost::system::error_code &ec, const size_t size)
{
	if (ec || size == 0)
	{
		errorHandler_(ec.message());
		return;
	}

	RedisCommand* c = queue_.front().cmd;
	if (c != last_cmd_)
	{
		reset_replys(c->GetCmdCount());
		last_cmd_ = c;
	}

	//index 是 replys_中vector的索引
	int index = 0;
	for (size_t pos = 0; pos < size;)
	{
		auto result = parser_.Parse(read_buf_.data() + pos, size - pos, replys_, index);
		if (result.second == RedisParser::CompletedAll)
		{
			assert(c->GetCmdCount() == replys_.size());
			process_reply();

			//一次reply后，清理
			index = 0;
			queue_.pop();
			RedisCommand::DeleteCommand(c);

			//重置replys
			if (!queue_.empty())
			{
				c = queue_.front().cmd;
				last_cmd_ = c;
				reset_replys(c->GetCmdCount());
			}
		}
		else if (result.second == RedisParser::Incompleted)
		{
			//processMessage();
			//continue;
		}
		else if (result.second == RedisParser::CompletedOne)
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

void RedisClientImpl::onRedisError(const RedisReply &v)
{
	//std::string message = v.toString();
	//errorHandler(message);
}

void RedisClientImpl::defaulErrorHandler(const std::string &s)
{
	throw std::runtime_error(s);
}

void RedisClientImpl::ignoreErrorHandler(const std::string &)
{
	// empty
}

void RedisClientImpl::reset_replys(int n)
{
	assert(n != 0);
	if (replys_.size() == 1 && (n == 1))
	{
		RedisReply::DeleteReply(replys_[0]);
	}
	else
	{
		RedisReply::DeleteReplys(replys_);
		replys_.resize(n);
	}
}

bool RedisClientImpl::DoSyncCommand(const RedisCommand &c)
{
	boost::system::error_code ec;
	boost::asio::write(socket_, boost::asio::buffer(c.Data(), c.Size()), boost::asio::transfer_all(), ec);

	if (ec)
	{
		errorHandler_(ec.message());
		return false;
	}

	int index = 0;
	reset_replys(c.GetCmdCount());
	for (;;)
	{
		size_t size = socket_.read_some(boost::asio::buffer(read_buf_));
		if (size == 0)
		{
			errorHandler_("[RedisClient] read error");
			return false;
		}

		for (size_t pos = 0; pos < size;)
		{
			auto result = parser_.Parse(read_buf_.data() + pos, size - pos, replys_, index);
			if (result.second == RedisParser::CompletedAll)
			{
				return true;
			}
			else if (result.second == RedisParser::Incompleted)
			{
				pos += result.first;
				continue;
			}
			else if (result.second == RedisParser::CompletedOne)
			{
				pos += result.first;
				continue;
			}
			else
			{
				errorHandler_("[RedisClient] Parser error");
				return false;
			}
		}
	}
}
