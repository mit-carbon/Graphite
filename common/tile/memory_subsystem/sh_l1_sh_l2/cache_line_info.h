#pragma once

#include "../cache/cache_line_info.h"

namespace ShL1ShL2
{

enum CacheLevel
{
   L1,
   L2
};

class CacheLineInfo : public ::CacheLineInfo
{
public:
   CacheLineInfo(IntPtr tag = ~0, CacheState::Type cstate = CacheState::INVALID, UInt64 curr_time = 0);
   ~CacheLineInfo();
   
   static CacheLineInfo* create(SInt32 cache_level);
};

typedef CacheLineInfo ShL1CacheLineInfo;
typedef CacheLineInfo ShL2CacheLineInfo;

}
