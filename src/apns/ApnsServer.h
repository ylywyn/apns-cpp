#ifndef APNS_SERVER_H
#define APNS_SERVER_H 
#include <memory>
#include <unordered_map>
#include "Config.h"
#include "IoServicePool.h"

class ApnsService;
class PushNotification;
class MessagePump;
class ApnsServer
{
public:
	ApnsServer();
	~ApnsServer();
	bool Init();
	void Run() { stopped_ = false; ioservicepool_->Run(); }
	bool Send(int appid, PushNotification* notice, bool prod = true);
	void Stop();

private:
	bool init_;
	bool stopped_;
	std::unique_ptr<MessagePump> pump_;
	std::unique_ptr<IoServicePool> ioservicepool_;
	std::unordered_map<int, std::shared_ptr<ApnsService>> apns_serv_dev_;
	std::unordered_map<int, std::shared_ptr<ApnsService>> apns_serv_prod_;
};

#endif