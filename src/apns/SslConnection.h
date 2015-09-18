#ifndef SslConnection_H
#define SslConnection_H

const int MAX_LENGTH = 2048;

#include <memory>
#include <functional>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/noncopyable.hpp>

using namespace boost;
using namespace boost::asio;


typedef std::function<void()> ConnectCb;
typedef std::function<void(int arg)> WriteCb;
typedef std::function<void(char* arg, int)> ReadCb;

class SslConnection : public std::enable_shared_from_this<SslConnection>,
	private boost::noncopyable
{
public:
	SslConnection(boost::asio::io_service& io_service,
		ssl::context& context,
		ip::tcp::resolver::iterator ep) :
		stopped_(false),
		connected_(false),
		read_end_reconn_(true),
		endpoint_iter_(ep),
		socket_(io_service, context),
		read_buffsize_(256)
	{
		conn_cb_ = std::bind(&SslConnection::conn_cb, this);
		write_cb_ = std::bind(&SslConnection::write_cb, this, std::placeholders::_1);
		read_cb_ = std::bind(&SslConnection::read_cb, this, std::placeholders::_1, std::placeholders::_2);
	}

	~SslConnection() {}

	void Connect();
	bool IsConnect() { return connected_; }
	void Close();
	void ReConnect()
	{
		Close();
		Connect();
	}
	void Write(void* data, size_t len);


	void SetConnCb(ConnectCb cb) { conn_cb_ = cb; }
	void SetReadCb(ReadCb cb) { read_cb_ = cb; }
	void SetWriteCb(WriteCb cb) { write_cb_ = cb; }

	void SetReadSize(size_t len) { read_buffsize_ = len; }
	void SetReadEndReConn(bool reconn) { read_end_reconn_ = reconn; }

private:
	void handle_connect(const boost::system::error_code& error);
	void handle_handshake(const boost::system::error_code& error);
	bool verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx);
	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
	void handle_write(const boost::system::error_code& error, size_t bytes_transferred);

	void conn_cb() {}
	void read_cb(char* arg, int p) {}
	void write_cb(int arg) {}

private:
	bool stopped_;
	bool connected_;
	bool read_end_reconn_;
	char reply_[512];
	ip::tcp::resolver::iterator endpoint_iter_;
	ssl::stream<boost::asio::ip::tcp::socket> socket_;

	size_t read_buffsize_;
	ReadCb read_cb_;
	WriteCb write_cb_;
	ConnectCb conn_cb_;
};

#endif
