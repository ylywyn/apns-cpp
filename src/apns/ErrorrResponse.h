#ifndef ERROR_RESPONCE_H
#define ERROR_RESPONCE_H
#include <stdint.h>
struct ErrorrResponse
{
	unsigned char cmd;
	unsigned char status;
	uint32_t id;
};

#endif