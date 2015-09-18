#include "ApnsConnection.h"

#include <thread>
#include <chrono>
#include <boost/endian/arithmetic.hpp>

#include "Log.h"
#include "ObjectPool.h"
#include "ErrorrResponse.h"
#include "ApnsBase.h"
#include "ApnsService.h"

void ApnsConnection::Connect()
{
	if (!conn_tsl_)
	{
		conn_tsl_ = std::make_shared<SslConnection>(io_service_, context_, end_point_);
		conn_tsl_->SetConnCb(std::bind(&ApnsConnection::post, this));
		conn_tsl_->SetWriteCb(std::bind(&ApnsConnection::post_cb, this, std::placeholders::_1));
		conn_tsl_->SetReadCb(std::bind(&ApnsConnection::read_cb, this, std::placeholders::_1, std::placeholders::_2));
		conn_tsl_->SetReadSize(ERROR_RESPONSE_BYTES_LENGTH);
	}

	if (!conn_tsl_->IsConnect())
	{
		conn_tsl_->Connect();
	}
}

bool ApnsConnection::SendNotification(PushNotification* notice)
{
	if (!conn_tsl_->IsConnect())
	{
		send_queue_.push_back(notice);
		if (send_queue_.size() > 64)
		{
			//将消息转交到有效连接, 没有有效则丢弃
			ApnsConnection* conn = apns_service_->GetVaildConn();
			while (!send_queue_.empty())
			{
				PushNotification* n = send_queue_.front();
				send_queue_.pop_front();
				if (conn != NULL)
				{
					conn->SendNotification(n);
				}
				else
				{
					gNoticePool.Put(n);
					LOGFMTW("链接长时间断开，丢弃已经缓存的消息！");
				}
			}
		}
		return false;
	}

	//空闲过长，重连
	if (idle_time())
	{
		send_queue_.push_back(notice);
		conn_tsl_->Close();

		LOGFMTW("连接空闲时常过长, 重连~");

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		conn_tsl_->ReConnect();
		return false;
	}

	//放入发送队列
	bool free = send_queue_.empty();
	send_queue_.push_back(notice);

	if (free)
	{
		post();
	}

	//清理 sended_queue_
	size_t num = sended_queue_.size();
	if (num > MAX_SENDED_QUEUE)
	{
		sub_sended_queue();
	}
	return true;
}

void ApnsConnection::close()
{
	conn_tsl_->Close();

	PushNotification* n = NULL;
	while (!sended_queue_.empty())
	{
		n = sended_queue_.front();
		if (n != NULL)
		{
			delete n;
		}
		sended_queue_.pop_front();
	}

	while (!send_queue_.empty())
	{
		n = send_queue_.front();
		if (n != NULL)
		{
			delete n;
		}
		send_queue_.pop_front();
	}
}

void ApnsConnection::sub_sended_queue()
{
	PushNotification* n = NULL;
	int c = int(MAX_SENDED_QUEUE * 0.8);
	while (!sended_queue_.empty() && (--c > 0))
	{
		n = sended_queue_.front();
		sended_queue_.pop_front();
		if (n != NULL)
		{
			n->Clear();
			gNoticePool.Put(n);
		}
	}
}

void ApnsConnection::process_reponse_error(char* reponse)
{
	//parse
	ErrorrResponse err;
	err.cmd = reponse[0];
	if (err.cmd != 8)
	{
		LOGFMTE("error response commond, %d", err.cmd);
		return;
	}
	err.status = reponse[1];
	memcpy(&err.id, reponse + 2, 4);
	err.id = boost::endian::big_to_native(err.id);

	LOGFMTE("read response :%s", ResponseErrorToString((ErrorResponse)err.status));

	//重新发送失败id后的所有消息
	bool find = false;
	size_t c = sended_queue_.size();
	PushNotification* notice = NULL;

	//找到直到遇到失败的那个消息
	for (int i = 0; i < c; ++i)
	{
		notice = sended_queue_[i];
		if (notice == NULL)
		{
			continue;
		}

		if (find)
		{
			send_queue_.push_back(notice);
			sended_queue_[i] = NULL;
		}
		else
		{
			//找到失败的那条消息，
			if (err.id == notice->Id())
			{
				find = true;
				if (err.status == ERROR_CODE_INVALID_TOKEN)
				{
					apns_service_->AddInvalidToken(notice->Token());
				}
				notice->Clear();
				gNoticePool.Put(notice);
			}
		}
	}
}


void ApnsConnection::post()
{
	send_time();

	if (!send_queue_.empty())
	{
		PushNotification* notice_send = send_queue_.front();
		if (notice_send == NULL)
		{
			send_queue_.pop_front();
			return;
		}

		char * data = notice_send->Bytes();
		size_t len = notice_send->BytesLen();
		if (len > 0)
		{
			conn_tsl_->Write(data, len);
		}
		else
		{
			notice_send->Clear();
			gNoticePool.Put(notice_send);
			send_queue_.pop_front();
			LOGFMTE("notice len == 0");
		}
	}
}

void ApnsConnection::post_cb(int bytes_transferred)
{
	PushNotification* notice_send = send_queue_.front();
	if (notice_send != NULL)
	{
		if (notice_send->BytesLen() == bytes_transferred)
		{
			LOGFMTD("successful id:%d", notice_send->Id());
			//发送列表去掉刚才发送的
			send_queue_.pop_front();
			sended_queue_.push_back(notice_send);

			//继续发送
			post();
		}
		else
		{
			//重试发送
			if (notice_send->SubRetryCount() > 0)
			{
				post();
			}
			else
			{
				LOGFMTE("faild id:%d(had retry..)", notice_send->Id());
				//发送列表去掉刚才发送的
				send_queue_.pop_front();
				sended_queue_.push_back(notice_send);
			}
		}
	}
	else
	{
		send_queue_.pop_front();
	}
}

void ApnsConnection::read_cb(char* arg, int size)
{
	if (size == ERROR_RESPONSE_BYTES_LENGTH)
	{
		process_reponse_error(arg);
	}
	else
	{
		LOGFMTD("read unknow error");
	}
}
