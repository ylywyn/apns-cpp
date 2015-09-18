// ApnsPusher.cpp : Defines the entry point for the console application.
//
#include <thread>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "Log.h"
#include "Options.h"
#include "PushPayLoad.h"
#include "ApnsService.h"
#include "PushNotification.h"
#include "ApnsServer.h"

#ifndef WIN32
#include "Daemon.h"
#include "Single_proc.h"
#endif

ApnsServer* server = NULL;

void InitLog()
{
	if (gOptions.LogType == 0)
	{
		logger::ILogManager::get()->setLoggerOutConsole(0, true);
		logger::ILogManager::get()->setLoggerOutFile(0, false);
	}
	else
	{
		logger::ILogManager::get()->setLoggerLimitsize(0, 100);
		logger::ILogManager::get()->setLoggerOutConsole(0, false);
		logger::ILogManager::get()->setLoggerOutFile(0, true);
	}

	logger::ILogManager::get()->setLoggerLevel(0, gOptions.LogLevel);
	logger::ILogManager::get()->setLoggerSync(true);

	logger::ILogManager::get()->start();
}


PushNotification* make_notice(const std::string& msg)
{
	PushNotification* n = gNoticePool.Get();
	if (n == NULL)
	{
		n = new PushNotification();
	}
	n->SetExpire(100);
	n->SetId(1);
	//n->SetToken("8a112a3da31ffcc569a9bf4ffbff1709a951ab99436e4c1c0778566aa4cf50f1");
	n->SetToken("4a4901d3cbbfc6309e8bda13b8f57291dfe88165557ef47126e3cec6ba79e482");

	PushPayLoad *payload = new PushPayLoad();
	//Alert alert = "hello";
	//AlertType  a;
	//a.title = "yangl";
	//a.body = "yang body";

	Alert alert = msg;
	payload->SetAlert(alert);
	payload->SetBadge(1);
	std::string str = "default";
	payload->SetSound(str);
	payload->ToJson(str);
	//n->SetPayLoad("{\"aps\":{\"alert\":\"hello,heh\",\"badge\":1,\"sound\":\"default\"}}");
	n->SetPayLoad(str);
	return n;
}


int main(int argc, char* argv[])
{
#ifndef WIN32
	//启动Linux守护进程
	if (!is_single_proc("Pusher"))
	{
		printf("Pusher is runing~");
		return 0;
	}
	daemonize();
#endif  

	gOptions.LogLevel = 1;
	gOptions.LogType = 0;
	if (!gOptions.Init())
	{
		printf("配置文件初始化失败\n");
		return false;
	}

	InitLog();

	server = new ApnsServer();
	if (server->Init())
	{
		server->Run();
		while (true)
		{
			boost::this_thread::sleep(boost::get_system_time() + boost::posix_time::seconds(3600));
		}
	}
	else
	{
		std::cout << "can't find config file!" << std::endl;
	}

	if (server != NULL)
	{
		delete server;
	}
	std::cout << "Bye ~ ~" << std::endl;
	return 0;
}

