#include "SslConnection.h"

#include <thread>
#include <chrono>

#include "Log.h"
#include "PushNotification.h"

void SslConnection::Close()
{
	try
	{
		stopped_ = true;
		if (connected_)
		{
			connected_ = false;
			socket_.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
			socket_.lowest_layer().close();
		}
	}
	catch (std::exception& e)
	{
		LOGFMTE("%s", e.what());
	}
}

void SslConnection::Connect()
{
	stopped_ = false;
	socket_.set_verify_mode(boost::asio::ssl::verify_peer);
	socket_.set_verify_callback(boost::bind(&SslConnection::verify_certificate, shared_from_this(), _1, _2));

	boost::asio::async_connect(socket_.lowest_layer(),
		endpoint_iter_,
		boost::bind(&SslConnection::handle_connect, shared_from_this(), boost::asio::placeholders::error));
}

void SslConnection::handle_connect(const boost::system::error_code& error)
{
	if (stopped_)
	{
		LOGFMTD("stopped");
		return;
	}

	if (!error)
	{
		socket_.async_handshake(boost::asio::ssl::stream_base::client,
			boost::bind(&SslConnection::handle_handshake, shared_from_this(),
			boost::asio::placeholders::error));

		LOGFMTD("connection ok, will handshake");
	}
	else if (endpoint_iter_ != boost::asio::ip::tcp::resolver::iterator())
	{
		LOGFMTE("conn error, try another addr");

		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		ReConnect();
	}
	else
	{
		LOGFMTE("%s", error.message().c_str());

		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		ReConnect();
	}
}

bool SslConnection::verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx)
{
	if (stopped_)
	{
		LOGFMTD("isn't connection or stopped");
		return false;
	}

	char subject_name[256];
	X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());

	X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);

	char issuer_name[256] = {};
	X509_NAME_oneline(X509_get_issuer_name(cert), issuer_name, 256);

	char peer_CN[256] = { 0 };
	X509_NAME_get_text_by_NID(X509_get_subject_name(cert), NID_commonName, peer_CN, 255);

	int err = X509_STORE_CTX_get_error(ctx.native_handle());
	if (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY || err == X509_V_ERR_CERT_UNTRUSTED)
	{
		return true;
	}

	return preverified;
}

void SslConnection::handle_handshake(const boost::system::error_code& error)
{
	if (stopped_)
	{
		LOGFMTD("isn't connection or stopped");
		return;
	}

	if (!error)
	{
		connected_ = true;
		LOGFMTD("handshake ok, ready!");

		boost::asio::async_read(socket_,
			boost::asio::buffer(reply_, read_buffsize_),
			boost::bind(&SslConnection::handle_read, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));

		conn_cb_();
	}
	else
	{
		LOGFMTE("Handshake failed:%s", error.message().c_str());
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		ReConnect();
	}
}


void SslConnection::handle_write(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (!connected_ || stopped_)
	{
		LOGFMTD("isn't connection or stopped");
		return;
	}

	if (!error)
	{
		write_cb_(bytes_transferred);
	}
	else
	{
		LOGFMTD("Write failed:%s", error.message().c_str());
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		ReConnect();
	}
}

void SslConnection::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (!connected_ || stopped_)
	{
		LOGFMTD("isn't connection or stopped");
		return;
	}

	if (!error)
	{
		read_cb_(reply_, bytes_transferred);
		ReConnect();
	}
	else
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		if (read_end_reconn_)
		{
			LOGFMTW("read return:%s", error.message().c_str());
			ReConnect();
		}
		else
		{
			LOGFMTD("feed back read return nothing");
			Close();
		}
	}
}

void SslConnection::Write(void* data, size_t len)
{
	if (!connected_ || stopped_)
	{
		LOGFMTD("isn't connection or stopped");
		return;
	}

	boost::asio::async_write(socket_,
		boost::asio::buffer(data, len),
		boost::bind(&SslConnection::handle_write, shared_from_this(),
		boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

}