#include "PushPayLoad.h"
#include "rapidjson/document.h"  
#include "rapidjson/prettywriter.h"  
#include "rapidjson/stringbuffer.h"  
#include <iostream>
using namespace boost;
using namespace std;
// alert field
// ...

const char* APS = "aps";
const char* ALERT = "alert";
const char* BADGE = "badge";
const char* SOUND = "sound";
const char* CONTENTAVAILABLE = "content-available";



void AddStringValue(const string& name, const string& value, rapidjson::Value& father, rapidjson::Document::AllocatorType& allocator)
{
	rapidjson::Value strName(rapidjson::kStringType);
	strName.SetString(name.c_str(), name.size(), allocator);

	rapidjson::Value strValue(rapidjson::kStringType);
	strValue.SetString(value.c_str(), value.size(), allocator);

	father.AddMember(strName, strValue, allocator);
}

void PushbackStringValue(const string& value, rapidjson::Value& father, rapidjson::Document::AllocatorType& allocator)
{
	rapidjson::Value strValue(rapidjson::kStringType);
	strValue.SetString(value.c_str(), value.size(), allocator);

	father.PushBack(strValue, allocator);
}

bool MarshalAlert(Alert& alert_, rapidjson::Value& root, rapidjson::Document::AllocatorType& allocator)
{
	if (alert_.type() == typeid(AlertType))
	{
		AlertType alert = get<AlertType>(alert_);
		rapidjson::Value array(rapidjson::kArrayType);
		rapidjson::Value object(rapidjson::kObjectType);

		//title
		if (!alert.title.empty())
		{
			AddStringValue("title", alert.title, object, allocator);
		}
		//body
		if (!alert.body.empty())
		{
			AddStringValue("body", alert.body, object, allocator);
		}
		//title_loc_key
		if (!alert.title_loc_key.empty())
		{
			AddStringValue("title_loc_key", alert.title_loc_key, object, allocator);
		}
		//title_loc_args
		if (alert.title_loc_args.type() == typeid(string))
		{
			string args = get<string>(alert.title_loc_args);
			if (!args.empty())
			{
				AddStringValue("title_loc_args", args, object, allocator);
			}
		}
		else if (alert.title_loc_args.type() == typeid(vector<string>))
		{
			vector<string> args = get<vector<string>>(alert.title_loc_args);
			if (!args.empty())
			{
				rapidjson::Value arr(rapidjson::kArrayType);
                //for each (string str in args)
                 for(vector<string>::iterator it=args.begin();it!=args.end();it++)
				{
                    PushbackStringValue(*it, arr, allocator);
				}
                object.AddMember("title_loc_args", arr, allocator);
			}
		}

		//action_loc_key
		if (!alert.action_loc_key.empty())
		{
			AddStringValue("action_loc_key", alert.action_loc_key, object, allocator);
		}
		//loc_key
		if (!alert.loc_key.empty())
		{
			AddStringValue("loc_key", alert.loc_key, object, allocator);
		}

		//loc_args
		if (alert.loc_args.type() == typeid(string))
		{
			string args = get<string>(alert.loc_args);
			if (!args.empty())
			{
				AddStringValue("loc_args", args, object, allocator);
			}
		}
		else if (alert.loc_args.type() == typeid(vector<string>))
		{
			vector<string> args = get<vector<string>>(alert.loc_args);
			if (!args.empty())
			{
				rapidjson::Value arr(rapidjson::kArrayType);
//				for each (string str in args)
                 for(vector<string>::iterator it=args.begin();it!=args.end();it++)
				{
                    PushbackStringValue(*it, arr, allocator);
				}
				object.AddMember("loc_args", arr, allocator);
			}
		}

		//launch_image
		if (!alert.launch_image.empty())
		{
			AddStringValue("launch_image", alert.launch_image, object, allocator);
		}

		root.AddMember("alert", object, allocator);
	}
	else if (alert_.type() == typeid(string))
	{
		string alert = get<string>(alert_);
		AddStringValue("alert", alert, root, allocator);
	}

	return false;
}


bool PushPayLoad::ToJson(string&  jsonstr)
{
	rapidjson::Document document;
	document.SetObject();
	rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

	rapidjson::Value root(rapidjson::kObjectType);
	//alert
	MarshalAlert(alert_, root, allocator);

	//badge
	if (badge_ > 0)
	{
		root.AddMember("badge", badge_, allocator);
	}

	//sound_
	if (!sound_.empty())
	{
		AddStringValue("sound", sound_, root, allocator);
	}

	//content_available_
	if (content_available_ > 0)
	{
		root.AddMember("content-available", content_available_, allocator);
	}

	document.AddMember("aps", root, allocator);

	rapidjson::StringBuffer  buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	document.Accept(writer);
	jsonstr = buffer.GetString();

	if (jsonstr.size() > MAX_LENGTH)
	{
		return false;
	}

	return true;
}
