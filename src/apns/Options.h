#ifndef OPTIONS_H
#define OPTIONS_H
#include <string>
#include <vector>
#include "ApnsService.h"
class Options
{
public:
    Options();

    //redis server info
    int RedisServerPort;
    std::string RedisServerAddr;

    //log lever
    int LogLevel;
    int LogType;

	//Params
	std::vector<ApnsParams> chanels_;

    //read from config file
    bool Init();
};

extern Options gOptions;
#endif // OPTIONS_H
