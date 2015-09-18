/*
* Copyright (C) ylywyn@gmail.com
* License: MIT
*/

#include <string.h>
#include "RedisReply.h"
SyncObjectPool<RedisReply, 5000> gReplyPool;

