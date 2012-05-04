#pragma once

#include "../cache/cache_line_info.h"
#include "cache_level.h"

namespace ShL1ShL2
{

CacheLineInfo* createCacheLineInfo(SInt32 cache_level);

typedef CacheLineInfo ShL1CacheLineInfo;
typedef CacheLineInfo ShL2CacheLineInfo;

}
