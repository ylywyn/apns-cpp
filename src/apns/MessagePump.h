#ifndef PUSH_MESSAGEPUMP_H
#define PUSH_MESSAGEPUMP_H
#include <string>

class ApnsServer;
class MessagePump
{
public:
    MessagePump(ApnsServer* ps):pusher_(ps){}
    virtual ~MessagePump(){}

	virtual bool Init() = 0;
	virtual bool Run() = 0;
	virtual void Stop() = 0;
	virtual void PubToPump(const std::string& channle, const std::string& data) = 0;
 protected:
	 ApnsServer* pusher_;
};

#endif // MESSAGEPUMP_H
