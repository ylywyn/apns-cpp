#include "PushNotification.h"
#include <boost/endian/arithmetic.hpp>
#include "ApnsBase.h"

SyncObjectPool<PushNotification, 4096> gNoticePool;


PushNotification::PushNotification() :
	bytes_len_(0),
	retries_(3),
	priority_(PRIORITY_SENT_IMMEDIATELY)
{
}


PushNotification::~PushNotification()
{
}

char* PushNotification::Bytes()
{
	if (bytes_len_ == 0)
	{
		if (!to_bytes())
		{
			bytes_len_ = 0;
		}
	}
	return buff_;
}

bool PushNotification::to_bytes()
{
	bytes_len_ = 0;

	int len = 0;
	memcpy(buff_ + len, (const void*)&COMMAND, COMMAND_LENGTH);
	len += COMMAND_LENGTH + FRAME_LENGTH; //跳过frame_len,最后赋值

	//Device token
	memcpy(buff_ + len, (const void*)&DEVICETOKEN_ITEMID, ITEMID_LENGTH);
	len += ITEMID_LENGTH;

	uint16_t device_len = boost::endian::native_to_big((uint16_t)DEVICETOKEN_LENGTH);
	memcpy(buff_ + len, (const void*)&device_len, ITEMDATA_LENGTH);
	len += ITEMDATA_LENGTH;

	char tokenBytes[32];
	Token2Bytes(token_.c_str(), tokenBytes);
	memcpy(buff_ + len, tokenBytes, DEVICETOKEN_LENGTH);
	len += DEVICETOKEN_LENGTH;

	//Payload
	memcpy(buff_ + len, (const void*)&PAYLOAD_ITEMID, ITEMID_LENGTH);
	len += ITEMID_LENGTH;

	uint16_t payload_len = boost::endian::native_to_big((uint16_t)(payload_.size()));
	memcpy(buff_ + len, (const void*)&payload_len, ITEMDATA_LENGTH);
	len += ITEMDATA_LENGTH;

	memcpy(buff_ + len, payload_.c_str(), payload_.size());
	len += payload_.size();

	//Notification identifier
	memcpy(buff_ + len, (const void*)&NOTIFICATION_IDENTIFIER_ITEMID, ITEMID_LENGTH);
	len += ITEMID_LENGTH;

	uint16_t id_len = boost::endian::native_to_big((uint16_t)NOTIFICATION_IDENTIFIER_LENGTH);
	memcpy(buff_ + len, (const void*)&id_len, ITEMDATA_LENGTH);
	len += ITEMDATA_LENGTH;

	uint32_t id = boost::endian::native_to_big(id_);
	memcpy(buff_ + len, (const void*)&id, NOTIFICATION_IDENTIFIER_LENGTH);
	len += NOTIFICATION_IDENTIFIER_LENGTH;

	//Expiration date
	memcpy(buff_ + len, (const void*)&EXPIRATIONDATE_ITEMID, ITEMID_LENGTH);
	len += ITEMID_LENGTH;

	uint16_t ex_len = boost::endian::native_to_big((uint16_t)EXPIRATIONDATE_LENGTH);
	memcpy(buff_ + len, (const void*)&ex_len, ITEMDATA_LENGTH);
	len += ITEMDATA_LENGTH;

	uint32_t ex = boost::endian::native_to_big(expire_);
	memcpy(buff_ + len, (const void*)&ex, EXPIRATIONDATE_LENGTH);
	len += EXPIRATIONDATE_LENGTH;

	//Priority
	memcpy(buff_ + len, (const void*)&PRIORITY_ITEMID, ITEMID_LENGTH);
	len += ITEMID_LENGTH;

	uint16_t priority_len = boost::endian::native_to_big((uint16_t)PRIORITY_LENGTH);
	memcpy(buff_ + len, (const void*)&priority_len, ITEMDATA_LENGTH);
	len += ITEMDATA_LENGTH;

	memcpy(buff_ + len, (const void*)&priority_, PRIORITY_LENGTH);
	len += PRIORITY_LENGTH;

	//frame len
	int frame_len = len - (COMMAND_LENGTH + FRAME_LENGTH);
	frame_len = boost::endian::native_to_big(frame_len);
	memcpy(buff_ + COMMAND_LENGTH, (const void*)&frame_len, FRAME_LENGTH);

	bytes_len_ = len;

	return true;
}