#include "ApnsFeedBackConnection.h"

#include <thread>
#include <chrono>
#include <boost/endian/arithmetic.hpp>

#include "Log.h"
#include "ApnsBase.h"
#include "ApnsService.h"
#include "PushNotification.h"

void ApnsFeedBackConnection::close()
{
	if (conn_tsl_)
	{
		conn_tsl_->Close();
	}
}

void ApnsFeedBackConnection::Connect()
{
	if (!conn_tsl_)
	{
		conn_tsl_ = std::make_shared<SslConnection>(io_service_, context_, end_point_);
		conn_tsl_->SetReadCb(std::bind(&ApnsFeedBackConnection::read_cb, this, std::placeholders::_1, std::placeholders::_2));
		conn_tsl_->SetReadSize(FEEDBACK_BYTES_LENGTH);

		//read end of file: no feed back, close the conn
		conn_tsl_->SetReadEndReConn(false);
	}

	if (!conn_tsl_->IsConnect())
	{
		conn_tsl_->Connect();
	}
}

//
// |**4bytes**|**2bytes**|**32bytes**|
//    time-t   token len    token
void ApnsFeedBackConnection::read_cb(char* arg, int size)
{
	if (size == FEEDBACK_BYTES_LENGTH)
	{
		long t = (long)arg;
		long l = (long)(arg + 4);
		char* token = arg + 4 + 2;

		FeedBack fb;
		fb.time = boost::endian::native_to_big((uint32_t)t);
		fb.len = boost::endian::native_to_big((uint16_t)l);

		if (fb.len == 32)
		{
			Bytes2Token(token, fb.token);
			process_feedback(fb);
			return;
		}
		else
		{
			LOGFMTE("feed back read bad token length");
		}
	}
	else
	{
		LOGFMTE("feed back read unknow error");
	}
}

void  ApnsFeedBackConnection::process_feedback(const FeedBack& fb)
{
	// time ? 
	apns_service_->AddInvalidToken(fb.token);
}
