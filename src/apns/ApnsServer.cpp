#include "ApnsServer.h"
#include <boost/asio.hpp>

#include "Log.h"
#include "Options.h"
#include "ApnsService.h"
#include "PushNotification.h"
#include "RedisMsgPump.h"


ApnsServer::ApnsServer() :
	init_(false),
	stopped_(true)
{

}

ApnsServer::~ApnsServer()
{
	Stop();
}


bool ApnsServer::Init()
{
	if (init_)
	{
		return true;
	}

	if (gOptions.chanels_.size() <= 0)
	{
		return false;
	}
    pump_.reset(new RedisMsgPump(this));
	if (pump_->Init())
	{
		pump_->Run();
	}
	else
	{
		return false;
	}


	// threads num same with cpu cores
	int cpus = boost::thread::hardware_concurrency();
	if (cpus <= 1)
	{
		cpus = 2;
	}
	// 
    ioservicepool_.reset(new IoServicePool(cpus - 1));

	//start services
    //for each (const ApnsParams& param in gOptions.chanels_)
    for(std::vector<ApnsParams>::iterator it=gOptions.chanels_.begin();it!=gOptions.chanels_.end(); ++it)
	{

		boost::asio::io_service* ios = ioservicepool_->GetIoService();
        std::shared_ptr<ApnsService> s(new ApnsService(*it, ios));
		s->Start();
        if ((*it).Developer)
		{
            apns_serv_dev_.insert(std::make_pair((*it).AppID, s));
		}
		else
		{
            apns_serv_prod_.insert(std::make_pair((*it).AppID, s));
		}
	}
	return true;
}

bool ApnsServer::Send(int appid, PushNotification* notice, bool prod)
{
	if (stopped_)
	{
		return false;
	}

	if (prod)
	{
		auto it = apns_serv_prod_.find(appid);
		if (it != apns_serv_prod_.end())
		{
			it->second->PostNotification(notice);
			return true;
		}
		else
		{
			gNoticePool.Put(notice);
			LOGFMTW("can't find the appid: %d", appid);
			return false;
		}
	}
	else
	{
		auto it = apns_serv_dev_.find(appid);
		if (it != apns_serv_dev_.end())
		{
			it->second->PostNotification(notice);
			return true;
		}
		else
		{
			gNoticePool.Put(notice);
			LOGFMTW("can't find the appid: %d", appid);
			return false;
		}
	}
}

void ApnsServer::Stop()
{
	if (stopped_)
	{
		return;
	}
	stopped_ = true;

	pump_->Stop();

	for (auto i = apns_serv_dev_.begin(); i != apns_serv_dev_.end(); ++i)
	{
		i->second->Stop();
	}
	apns_serv_dev_.clear();

	for (auto i = apns_serv_prod_.begin(); i != apns_serv_prod_.end(); ++i)
	{
		i->second->Stop();
	}
	apns_serv_prod_.clear();

	ioservicepool_->Stop();
}
