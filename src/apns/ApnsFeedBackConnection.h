#ifndef APNS_FEEDBACK_CONN_H
#define APNS_FEEDBACK_CONN_H

#include <memory>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/noncopyable.hpp>
#include "SslConnection.h"

using namespace boost;
using namespace boost::asio;

struct FeedBack
{
	uint32_t time;
	uint16_t len;
	char token[64];
};

class ApnsService;
class ApnsFeedBackConnection : public std::enable_shared_from_this<ApnsFeedBackConnection>,
	private boost::noncopyable
{
public:
	ApnsFeedBackConnection(boost::asio::io_service& io_service,
		ssl::context& context,
		ip::tcp::resolver::iterator ep,
		ApnsService* apns_serv) :
		end_point_(ep),
		io_service_(io_service),
		context_(context),
		apns_service_(apns_serv)
	{
	}

	~ApnsFeedBackConnection() { close(); }

	void Connect();
	void Close() { close(); }

private:
	void close();
	void read_cb(char* arg, int size);
	void process_feedback(const FeedBack& db);

private:
	ApnsService* apns_service_;
	boost::asio::io_service& io_service_;
	boost::asio::ssl::context& context_;
	ip::tcp::resolver::iterator end_point_;
	std::shared_ptr<SslConnection> conn_tsl_;
};
#endif
