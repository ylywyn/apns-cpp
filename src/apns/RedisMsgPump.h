#ifndef PUSH_REDISMSGPUMP_H
#define PUSH_REDISMSGPUMP_H

#include "boost/thread.hpp"
#include "boost/asio.hpp"
#include "redisclient/RedisAsyncClient.h"
#include "redisclient/RedisSubClient.h""
#include "ApnsServer.h"
#include "MessagePump.h"
class RedisMsgPump:public MessagePump
{
public:
    RedisMsgPump(ApnsServer* ps):
		MessagePump(ps),
		running_(false),
        init_(false),
		closed_(false),
		pub_(NULL),
		sub_(NULL)
    {}
    virtual ~RedisMsgPump(){Stop();}
    virtual bool Init();
    virtual bool Run();
    virtual void Stop();

    void ReStart();
    bool IsClosed(){return closed_;}
	void SetRunning() { init_ = true; closed_ = false; running_ = true; }
	void InitAndPostMsg(const std::string& jsonMsg);
	void PubToPump(const std::string& channle, const std::string& data);

private:
	void onError();
    void runInner();
	void subConnCallback(bool connected, const std::string &errorMessage);
	void pubConnCallback(bool connected, const std::string &errorMessage);
	void initPub();
	void initSub();

private:
    bool init_;
	bool closed_;
    bool running_;
	RedisAsyncClient* pub_;
	RedisSubClient* sub_;
	boost::asio::io_service ios_service_;
    boost::thread work_thread_;

};

#endif // REDISMSGPUMP_H
