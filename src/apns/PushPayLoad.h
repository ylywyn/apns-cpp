#ifndef PUSHPAYLOAD
#define PUSHPAYLOAD 
#include <string>
#include <vector>
#include <boost/variant.hpp>  

typedef boost::variant<std::string, std::vector<std::string>> Args;
struct AlertType
{
	std::string title;          //ios 8.2
	std::string body;
	std::string title_loc_key;  //ios 8.2
	Args title_loc_args;        //ios 8.2 string or string array
	std::string action_loc_key;
	std::string loc_key;
	Args loc_args;              //string or string array
	std::string launch_image;
};

typedef boost::variant<AlertType, std::string> Alert;

class PushPayLoad
{
public:
	PushPayLoad() :
		badge_(0),
		content_available_(0)
	{}
	~PushPayLoad() {}

	void SetAlert(Alert& alert) { alert_ = alert; }
	void SetBadge(int badge) { badge_ = badge; }
	void SetSound(std::string& s) { sound_ = s; }
	void SetContentAvailable(int ca) { content_available_ = ca; }

	bool ToJson(std::string&  jsonstr);

private:
	int badge_;
	int content_available_;
	Alert alert_;
	std::string sound_;
	static const int MAX_LENGTH = 2048;
};

#endif