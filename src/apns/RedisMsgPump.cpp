#include "RedisMsgPump.h"
#include <string>
#include <vector>
#include <functional>
#include <boost/bind.hpp>
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "Options.h"
#include "Log.h"


using namespace rapidjson;
static RedisMsgPump* gRedisMsgPump = NULL;

struct IOSMessage {
	bool Dev;
	uint16_t	Appid;
	uint64_t	Messageid;
	uint64_t ExpireTime;
	const char*	Token;
	const char* Payload;
};

void ReplyHandler(const std::vector<RedisReply*>& rs)
{
	for (auto iter = rs.begin(); iter != rs.end(); ++iter)
	{
		//PrintReply(*iter);
	}
}

void ReplyHandler(const RedisReply* rs)
{
	//PrintReply((RedisReply*)rs);
}

void SubscribeCb(const std::string& data)
{
	gRedisMsgPump->InitAndPostMsg(data);
}

void RedisMsgPump::subConnCallback(bool connected, const std::string &errorMessage)
{
	if (!connected)
	{
		LOGFMTE("subscriber conn to redis error:%s", errorMessage.c_str());
		boost::this_thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(15000));
		//initSub();
	}
	else
	{
		gRedisMsgPump->SetRunning();

		auto iter = gOptions.chanels_.begin();
		for (; iter != gOptions.chanels_.end(); ++iter)
		{
			ApnsParams p = *iter;

			char strAppid[20];
			sprintf(strAppid, "%d", p.AppID);
			sub_->Subscribe(strAppid, SubscribeCb, ReplyHandler);
			LOGFMTD("redis pump sub:%s", strAppid);
		}
	}
}

void RedisMsgPump::pubConnCallback(bool connected, const std::string &errorMessage)
{
	if (!connected)
	{
		LOGFMTE("publisher conn to redis error:%s", errorMessage.c_str());
		boost::this_thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(15000));
		//initPub();
	}
}

bool RedisMsgPump::Init()
{
	gRedisMsgPump = this;

	initSub();
	initPub();

	init_ = true;
	return true;
}

void RedisMsgPump::initPub()
{
	if (pub_ != NULL)
	{
		delete pub_;
	}
	pub_ = new RedisAsyncClient(ios_service_);
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(gOptions.RedisServerAddr), gOptions.RedisServerPort);
	LOGFMTD("redis pump pub addr:%s:%d", gOptions.RedisServerAddr.c_str(), gOptions.RedisServerPort);

	pub_->AsyncConnect(endpoint, boost::bind(&RedisMsgPump::pubConnCallback, this, _1, _2));
}

void RedisMsgPump::initSub()
{
	if (sub_ != NULL)
	{
		delete sub_;
	}
	sub_ = new RedisSubClient(ios_service_);

	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(gOptions.RedisServerAddr), gOptions.RedisServerPort);
	LOGFMTD("redis pump sub addr:%s:%d", gOptions.RedisServerAddr.c_str(), gOptions.RedisServerPort);

	sub_->AsyncConnect(endpoint, boost::bind(&RedisMsgPump::subConnCallback, this, _1, _2));
}


bool RedisMsgPump::Run()
{
	running_ = true;
	work_thread_ = boost::thread(boost::bind(&RedisMsgPump::runInner, this));
	return true;
}

void RedisMsgPump::runInner()
{
	LOGFMTD("redis msg pump thread run");
begin:
	if (!init_)
	{
		Init();
	}

	try
	{
		ios_service_.run();
	}
	catch (std::exception& e)
	{
		LOGFMTE("ios_service run error:%s,will re try...", e.what());
	}

	if (!closed_)
	{
		init_ = false;
		boost::this_thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(10000));
		goto begin;
	}
	running_ = false;
	LOGFMTE("ios_service run return(closed:%d)", closed_);
}

void RedisMsgPump::Stop()
{
	closed_ = true;
	init_ = false;
	ios_service_.stop();
	if (sub_ != NULL)
	{
		delete sub_;
		sub_ = NULL;
	}

	if (pub_ != NULL)
	{
		delete pub_;
		pub_ = NULL;
	}
}

void RedisMsgPump::onError()
{
}
//10sec 重连
void RedisMsgPump::ReStart()
{
	if (!closed_)
	{
		init_ = false;
		Stop();
		boost::this_thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(8000));
		Init();
		Run();
	}
}

uint32_t gMsgid = 1;
void RedisMsgPump::InitAndPostMsg(const std::string& data)
{
	size_t len = data.size();
	if (len > 40 && len < 4096)
	{
		rapidjson::Document document;
		document.Parse(data.c_str());
		if (document.HasParseError())
		{
			return;
		}

		IOSMessage iosmsg;
		Value::ConstMemberIterator itr;

		//dev
		itr = document.FindMember("Dev");
		if (itr != document.MemberEnd())
		{
			iosmsg.Dev = itr->value.GetBool();
		}
		else
		{
			iosmsg.Dev = false;
		}

		//appid
		itr = document.FindMember("Appid");
		if (itr != document.MemberEnd())
		{
			iosmsg.Appid = itr->value.GetUint();
		}
		else
		{
			LOGFMTE("can't find  appid from msg");
			return;
		}

		//msgid
		itr = document.FindMember("Messageid");
		if (itr != document.MemberEnd())
		{
			iosmsg.Messageid = itr->value.GetUint64();
			LOGFMTD("msg id: %llu", iosmsg.Messageid);
		}

		//ExpireTime
		itr = document.FindMember("ExpireTime");
		if (itr != document.MemberEnd())
		{
			iosmsg.ExpireTime = itr->value.GetUint64();
		}

		StringBuffer buffer;
		itr = document.FindMember("Payload");
		if (itr != document.MemberEnd())
		{
			const rapidjson::Value &val = document["Payload"];
			Writer<StringBuffer> writer(buffer);
			val.Accept(writer);
			iosmsg.Payload = buffer.GetString();
		}
		else
		{
			LOGFMTE("can't find  payload  from msg");
			return;
		}


		if (document.HasMember("Token"))
		{
			const Value& tk = document["Token"];
			assert(tk.IsArray());

			for (SizeType i = 0; i < tk.Size(); i++) // 使用 SizeType 而不是 size_t
			{
				iosmsg.Token = tk[i].GetString();

				PushNotification* n = gNoticePool.Get();
				if (n == NULL)
				{
					n = new PushNotification();
				}
				n->SetExpire(iosmsg.ExpireTime);

				//msgid溢出,改为自增id
				//n->SetId(iosmsg.Messageid);
				n->SetId(++gMsgid);
				n->SetPayLoad(iosmsg.Payload);
				n->SetToken(iosmsg.Token);
				pusher_->Send(iosmsg.Appid, n, !iosmsg.Dev);
			}
		}

	}
}

//1. 发到后台持久化
//2. 发到集群中做中转
void RedisMsgPump::PubToPump(const std::string& channle, const std::string& data)
{
	pub_->Publish(channle, data);
}
