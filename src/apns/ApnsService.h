#ifndef APNSSERVICE_H
#define APNSSERVICE_H
#include <string>
#include <vector>
#include <memory>
#include <unordered_set>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/noncopyable.hpp>

#include "Timer.h"
#include "ApnsConnection.h"
#include "PushNotification.h"
#include "ApnsFeedBackConnection.h"

struct ApnsParams
{
	//服务名
	std::string Name;
	int AppID;

	//认证
	std::string CertFile;
	std::string KeyFile;

	//开发模式
	bool Developer;

	//一个服务的连接数
	short ApnsConnCount;

	//空闲时长
	int IdleTime;
	int FeedBackTime;
};

class ApnsService : private boost::noncopyable, public Timer
{
public:
	ApnsService(const ApnsParams& param, boost::asio::io_service* io_service);
	~ApnsService();

	std::string Name() { return apns_params_.Name; }
	bool Start();
	void Stop();
	void PostNotification(PushNotification* notice) { io_service_->post(boost::bind(&ApnsService::send, this, notice)); }
	void AddInvalidToken(const std::string& token);
	ApnsConnection* GetVaildConn();
	bool CheckToken(const std::string& token);

private:
	bool init_ssl_ctx();
	void send(PushNotification* send);
	boost::asio::ip::tcp::resolver::iterator resolver_addr(bool feedback);

	virtual bool OnTimer(unsigned char id, void* user_data);
	void run_feedback();

private:
	bool stopped_;
	int next_apnsconn_;
	ApnsParams apns_params_;
	
	boost::asio::io_service* io_service_;
	std::unique_ptr<boost::asio::ssl::context> ssl_ctx_;
	std::vector<std::shared_ptr<ApnsConnection>> apns_conns_;

	std::unique_ptr<ApnsFeedBackConnection> fd_conn_;
	//Invalid token
	std::unordered_set<std::string> invalid_token_;
};

#endif