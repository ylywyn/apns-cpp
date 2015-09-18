#ifndef APNS_BASE_H
#define APNS_BASE_H

#include "ApnsBase.h"
#include <stdio.h>

void Bytes2Token(const char *bytes, char *token)
{
	int i = 0;
	int k = 0;
	int len = 32;
	while (i < len)
	{
		int bi = bytes[i];
		int ii = bi > 0 ? bi : 256 + bi;
		char temp = ii / 16;
		token[k] = char(temp < 10 ? temp + 48 : 87 + temp);
		temp = ii % 16;
		token[++k] = char(temp < 10 ? temp + 48 : 87 + temp);
		++i;
		++k;
	}
}


void Token2Bytes(const char *token, char *bytes)
{
	int val;
	while (*token)
	{
		sscanf(token, "%2x", &val);
		*(bytes++) = (char)val;

		token += 2;
		while (*token == ' ')
		{ // skip space
			++token;
		}
	}
}


const char* ResponseErrorToString(ErrorResponse code)
{
	char* desc = 0;
	switch (code)
	{
	case ERROR_CODE_NO_ERRORS:
		desc = "No errors encountered";
		break;
	case ERROR_CODE_PROCESSING_ERROR:
		desc = "Processing error";
		break;
	case ERROR_CODE_MISSING_TOKEN:
		desc = "Missing device token";
		break;
	case ERROR_CODE_MISSING_TOPIC:
		desc = "Missing topic";
		break;
	case ERROR_CODE_MISSING_PAYLOAD:
		desc = "Missing payload";
		break;
	case ERROR_CODE_INVALID_TOKEN_SIZE:
		desc = "Invalid token size";
		break;
	case ERROR_CODE_INVALID_TOPIC_SIZE:
		desc = "Invalid topic size";
		break;
	case ERROR_CODE_INVALID_PAYLOAD_SIZE:
		desc = "Invalid payload size";
		break;
	case ERROR_CODE_INVALID_TOKEN:
		desc = "Invalid token";
		break;
	case ERROR_CODE_SHUTDOWN:
		desc = "Shutdown";
		break;
	default:
		desc = "Unkown";
	}
	return desc;
}

//dev
const char* HOST_DEVELOPMENT_ENV = "gateway.sandbox.push.apple.com";
const char* PORT_DEVELOPMENT_ENV = "2195";
const char* FEEDBACK_HOST_DEVELOPMENT_ENV = "feedback.sandbox.push.apple.com";
const char* FEEDBACK_PORT_DEVELOPMENT_ENV = "2196";

//prod
const char* HOST_PRODUCTION_ENV = "gateway.push.apple.com";
const char* PORT_PRODUCTION_ENV = "2195";
const char* FEEDBACK_HOST_PRODUCTION_ENV = "feedback.push.apple.com";
const char* FEEDBACK_PORT_PRODUCTION_ENV = "2196";

const int ERROR_RESPONSE_BYTES_LENGTH = 6;
const int FEEDBACK_BYTES_LENGTH = 38;
const int PAY_LOAD_MAX_LENGTH = 256;
const char* CHARSET_ENCODING = "UTF-8";
#endif