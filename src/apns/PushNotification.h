#ifndef PUSH_NOTIFICATION
#define PUSH_NOTIFICATION

#include <string>
#include "ObjectPool.h"
#include "PushPayLoad.h"

class PushNotification
{
public:
	PushNotification();
	~PushNotification();

	char* Bytes();
	size_t BytesLen() { return bytes_len_; };

	void SetId(uint32_t id) { id_ = id; }
	uint32_t Id() { return id_; }

	void SetExpire(int ex) { expire_ = ex; }
	int  Expire() { return expire_; }

	void SetToken(const char* token) { token_.assign(token); }
	void SetToken(const std::string& token) { token_ = token; }
	const std::string& Token() { return token_; }

	void SetPayLoad(const char* payload) { payload_.assign(payload); }
	void SetPayLoad(const std::string& payload) { payload_ = payload; }
	std::string PayLoad() { return payload_; }

	unsigned short SubRetryCount() { return --retries_; }
	void Clear()
	{
		id_ = 0;
		bytes_len_ = 0;
		token_ = "";
		payload_ = "";
	}
private:
	bool to_bytes();

private:
	const unsigned char COMMAND = 2;

	const int COMMAND_LENGTH = 1;
	const int FRAME_LENGTH = 4;

	const int ITEMID_LENGTH = 1;
	const int ITEMDATA_LENGTH = 2;

	const unsigned char DEVICETOKEN_ITEMID = 1;
	const unsigned char PAYLOAD_ITEMID = 2;
	const unsigned char NOTIFICATION_IDENTIFIER_ITEMID = 3;
	const unsigned char EXPIRATIONDATE_ITEMID = 4;
	const unsigned char PRIORITY_ITEMID = 5;

	const int DEVICETOKEN_LENGTH = 32;
	const int NOTIFICATION_IDENTIFIER_LENGTH = 4;
	const int EXPIRATIONDATE_LENGTH = 4;
	const int PRIORITY_LENGTH = 1;

	const unsigned char PRIORITY_SENT_A_TIME = 5;
	const unsigned char PRIORITY_SENT_IMMEDIATELY = 10;
	
private:
	//prop
	uint32_t id_;
	uint32_t expire_;
	unsigned char priority_;
	std::string token_;
	std::string payload_;

	unsigned short retries_;
	size_t bytes_len_;
	char buff_[3096];
};

extern SyncObjectPool<PushNotification, 4096> gNoticePool;

#endif