#include "ApnsService.h"
#include <memory>
#ifdef WIN32
#include <io.h>
#endif

#include <assert.h>

#include "Log.h"
#include "ApnsBase.h"


ApnsService::ApnsService(const ApnsParams& param, boost::asio::io_service* io_service) :
	apns_params_(param),
	io_service_(io_service),
	stopped_(true),
	Timer(*io_service),
	next_apnsconn_(0)
{
}


ApnsService::~ApnsService()
{
	Stop();
}

bool ApnsService::Start()
{
	auto iter = resolver_addr(false);
	if (iter == boost::asio::ip::tcp::resolver::iterator())
	{
		LOGFMTE("resolver apns host faild");
		return false;
	}

	if (!init_ssl_ctx())
	{
		return false;
	}

	for (size_t i = 0; i < apns_params_.ApnsConnCount; i++)
	{
		auto conn = std::make_shared<ApnsConnection>(*io_service_, *ssl_ctx_, iter, this);
		conn->SetIdleTime(apns_params_.IdleTime);
		conn->Connect();
		apns_conns_.push_back(conn);
	}


	//feed back service
	auto iter_feedback = resolver_addr(true);
	if (iter_feedback == boost::asio::ip::tcp::resolver::iterator())
	{
		LOGFMTE("resolver apns feedback host faild");
		return false;
	}

	fd_conn_.reset(new  ApnsFeedBackConnection(*io_service_, *ssl_ctx_, iter_feedback, this));
	//get feedback time interval
	int t = 24 * 3600;
	if (apns_params_.FeedBackTime > 0 && apns_params_.FeedBackTime < (30 * t))
	{
		t = apns_params_.FeedBackTime;
	}

	SetTimer(1, 1000 * t, NULL);

	LOGFMTD("ApnsService %s start!", apns_params_.Name.c_str());
	stopped_ = false;
	return true;
}

void ApnsService::Stop()
{
	if (!stopped_)
	{
		stopped_ = true;

		StopAllTimer();

		int num_apnsconn = apns_conns_.size();
		for (int i = 0; i < num_apnsconn; ++i)
		{
			apns_conns_[i]->Close();
		}
		io_service_->stop();
	}
}

void ApnsService::AddInvalidToken(const std::string& token)
{
	invalid_token_.insert(token);
}

bool ApnsService::CheckToken(const std::string& token)
{
	if (token.size() != 64)
	{
		LOGFMTD("invalid token length ： %s.", token.c_str());
		return false;
	}

	//先去掉过滤吧, 发送2000条消息后正常的token也返回错误!
	//auto iter = invalid_token_.find(token);
	//if (iter != invalid_token_.end())
	//{
	//	LOGFMTD("invalid token ： %s.", token.c_str());
	//	return false;
	//}

	return true;
}

void ApnsService::send(PushNotification* notice)
{
	if (stopped_)
	{
		gNoticePool.Put(notice);
		return;
	}

	// 检测token
	if (!CheckToken(notice->Token()))
	{
		notice->Clear();
		gNoticePool.Put(notice);
		return;
	}

	//寻找一个有效的连接,发送
	ApnsConnection* conn = GetVaildConn();
	if (conn != NULL)
	{
		conn->SendNotification(notice);
	}
	else
	{
		gNoticePool.Put(notice);
		LOGFMTE("%s :no valid apnsconn error", apns_params_.Name.c_str());
	}
}

boost::asio::ip::tcp::resolver::iterator ApnsService::resolver_addr(bool feedback)
{
	std::string addr;
	std::string	port;
	if (!feedback)
	{
		addr.assign(HOST_PRODUCTION_ENV);
		port.assign(PORT_PRODUCTION_ENV);
		if (apns_params_.Developer)
		{
			addr.assign(HOST_DEVELOPMENT_ENV);
			port.assign(PORT_DEVELOPMENT_ENV);
		}
	}
	else
	{
		addr.assign(FEEDBACK_HOST_PRODUCTION_ENV);
		port.assign(FEEDBACK_PORT_PRODUCTION_ENV);
		if (apns_params_.Developer)
		{
			addr.assign(FEEDBACK_HOST_DEVELOPMENT_ENV);
			port.assign(FEEDBACK_PORT_DEVELOPMENT_ENV);
		}
	}

	boost::asio::ip::tcp::resolver resolver(*io_service_);
	boost::asio::ip::tcp::resolver::query query(addr, port);
	boost::asio::ip::tcp::resolver::iterator iterator = boost::asio::ip::tcp::resolver::iterator();
	try
	{
		iterator = resolver.resolve(query);
	}
	catch (const boost::system::system_error& err)
	{
		LOGFMTE("resolver error:%s", err.what());
	}
	return iterator;
}

bool ApnsService::init_ssl_ctx()
{
	if (access(apns_params_.CertFile.c_str(), 0) == -1)
	{
		LOGFMTE("can't find cert file");
		return false;
	}

	if (access(apns_params_.KeyFile.c_str(), 0) == -1)
	{
		LOGFMTE("can't find key file");
		return false;
	}

	//ssl_ctx_ = std::move(std::make_unique<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client));
	ssl_ctx_.reset(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv12_client));
	ssl_ctx_->use_certificate_file(apns_params_.CertFile.c_str(), boost::asio::ssl::context_base::pem);
	ssl_ctx_->use_rsa_private_key_file(apns_params_.KeyFile.c_str(), boost::asio::ssl::context_base::pem);

	return true;
}

bool ApnsService::OnTimer(unsigned char id, void* user_data)
{
	run_feedback();
	return true;
}

void ApnsService::run_feedback()
{
	fd_conn_->Close();

	//async conn
	fd_conn_->Connect();
}

ApnsConnection* ApnsService::GetVaildConn()
{
	int num_apnsconn = apns_conns_.size();
	int i = 0;
	while (i < num_apnsconn)
	{
		auto& conn = apns_conns_[next_apnsconn_];
		++next_apnsconn_;
		if (next_apnsconn_ >= num_apnsconn)
		{
			next_apnsconn_ = 0;
		}
		if (conn->Connectd())
		{
			return conn.get();
		}
		++i;
	}

	return NULL;
}

