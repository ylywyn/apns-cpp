#include "Options.h"

#include <stdio.h>
#include <string>
#include <boost/algorithm/string.hpp> 

#include "Config.h"
#include "Log.h"

using namespace std;
Options gOptions;

Options::Options() :
	RedisServerPort(6379),
	RedisServerAddr("127.0.0.1"),
	LogLevel(1),
	LogType(0)
{
}

bool Options::Init()
{
	Config config;
	if (!config.load("ApnsPusher.conf"))
	{
		printf("can't find config file: ApnsPusher.conf");
		return false;
	}

	//redis
	string redisport = config.getValue("common", "redis_port", "");
	if (!redisport.empty())
	{
		RedisServerPort = atoi(redisport.c_str());
	}

	string redisaddr = config.getValue("common", "redis_addr", "");
	if (!redisaddr.empty())
	{
		RedisServerAddr = redisaddr;
	}

	//log
	string log_level = config.getValue("common", "log_level", "");
	if (!log_level.empty())
	{
		LogLevel = atoi(log_level.c_str());
	}

	string log_type = config.getValue("common", "log_type", "");
	if (!log_type.empty())
	{
		LogType = atoi(log_type.c_str());
	}

	vector<string> sectionlist;
	config.sectionList(sectionlist);
   // for_each (string section in sectionlist)
    for(vector<string>::iterator it=sectionlist.begin();it!=sectionlist.end();it++)
	{
        string section = *it;
		string strAppid = config.getValue(section, "appid", "");
		string strName = config.getValue(section, "name", "");
		string strCertfile = config.getValue(section, "certfile", "");
		string strKeyfile = config.getValue(section, "keyfile", "");
		string strDev = config.getValue(section, "develop", "");
		string strApnsconnCount = config.getValue(section, "apnsconncount", "");
		string strIdleTime = config.getValue(section, "idletime", "");
		string strFeedackTime = config.getValue(section, "feedbacktime", "");
		if (strAppid.empty() ||
			strName.empty() ||
			strDev.empty() ||
			strApnsconnCount.empty() ||
			strIdleTime.empty() ||
			strCertfile.empty() ||
			strKeyfile.empty() ||
			strFeedackTime.empty())
		{
			continue;
		}
		int appid = atoi(strAppid.c_str());
		int connCount = atoi(strApnsconnCount.c_str());
		int idletime = atoi(strIdleTime.c_str());
		int feedackTime = atoi(strFeedackTime.c_str());
		bool bdev = false;
		if (strDev == "true")
		{
			bdev = true;
		}

		ApnsParams param;
		param.AppID = appid;
		param.Name = strName;
		param.IdleTime = idletime;
		param.CertFile = strCertfile;
		param.KeyFile = strKeyfile;
		param.ApnsConnCount = connCount;
		param.FeedBackTime = feedackTime;

		chanels_.push_back(param);
	}
	return true;
}
