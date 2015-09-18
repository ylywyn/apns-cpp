#ifndef APNSCONNECTION
#define APNSCONNECTION

#include <time.h>
#include <memory>
#include <vector>
#include <deque>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/noncopyable.hpp>

#include "PushNotification.h"
#include "SslConnection.h"

using namespace boost;
using namespace boost::asio;

class ApnsService;
class ApnsConnection : private boost::noncopyable
{
public:
	ApnsConnection(boost::asio::io_service& io_service,
		ssl::context& context,
		ip::tcp::resolver::iterator ep,
		ApnsService* s) :
		io_service_(io_service),
		context_(context),
		end_point_(ep),
		apns_service_(s),
		idle_time_(DEFAULT_IDLE_TIME),
		last_sendtime_((int64_t)time(NULL))
	{
	}
	~ApnsConnection() { close(); }

	void Connect();
	bool Connectd() { return conn_tsl_->IsConnect(); }
	void Close() { close(); }
	bool SendNotification(PushNotification*);
	void SetIdleTime(uint32_t t) { idle_time_ = t; }

private:
	void close();
	bool idle_time() { return (time(NULL) - last_sendtime_) > idle_time_; }
	void send_time() { last_sendtime_ = (long)time(NULL); }

	// 减少sended中的数量
	void sub_sended_queue();

	//处理发送失败
	void process_reponse_error(char* reponse);

	//
	void post();
	void post_cb(int arg);
	void read_cb(char* arg, int size);

private:
	uint32_t idle_time_;
	uint64_t last_sendtime_;
	static const int DEFAULT_IDLE_TIME = 60 * 60;
	static const int MAX_SENDED_QUEUE = 256;

	std::deque<PushNotification*> send_queue_;
	std::deque<PushNotification*> sended_queue_;

	ApnsService* apns_service_;
	boost::asio::io_service& io_service_;
	boost::asio::ssl::context& context_;
	ip::tcp::resolver::iterator end_point_;
	std::shared_ptr<SslConnection> conn_tsl_;
};

#endif
