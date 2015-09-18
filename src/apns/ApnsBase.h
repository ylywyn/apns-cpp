#ifndef APNSSTRINGS_H
#define APNSSTRINGS_H

void Bytes2Token(const char *bytes, char *token);
void Token2Bytes(const char *token, char *bytes);

enum  ErrorResponse 
{
	ERROR_CODE_NO_ERRORS = 0,
	ERROR_CODE_PROCESSING_ERROR = 1,
	ERROR_CODE_MISSING_TOKEN = 2,
	ERROR_CODE_MISSING_TOPIC = 3,
	ERROR_CODE_MISSING_PAYLOAD = 4,
	ERROR_CODE_INVALID_TOKEN_SIZE = 5,
	ERROR_CODE_INVALID_TOPIC_SIZE = 6,
	ERROR_CODE_INVALID_PAYLOAD_SIZE = 7,
	ERROR_CODE_INVALID_TOKEN = 8,
	ERROR_CODE_SHUTDOWN = 10,
	ERROR_CODE_NONE = 255,
};

const char* ResponseErrorToString(ErrorResponse code);


//dev
extern const char* HOST_DEVELOPMENT_ENV;
extern const char* PORT_DEVELOPMENT_ENV;
extern const char* FEEDBACK_HOST_DEVELOPMENT_ENV;
extern const char* FEEDBACK_PORT_DEVELOPMENT_ENV;

//prod 
extern const char* HOST_PRODUCTION_ENV;
extern const char* PORT_PRODUCTION_ENV;
extern const char* FEEDBACK_HOST_PRODUCTION_ENV;
extern const char* FEEDBACK_PORT_PRODUCTION_ENV;


extern const int ERROR_RESPONSE_BYTES_LENGTH;
extern const int FEEDBACK_BYTES_LENGTH;
extern const int PAY_LOAD_MAX_LENGTH;
extern const char* CHARSET_ENCODING;

//payload fieldnames
#endif
