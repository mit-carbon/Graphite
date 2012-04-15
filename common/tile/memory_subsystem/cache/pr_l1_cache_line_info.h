#pragma once

#include "cache_state.h"
#include "cache_line_info.h"

class PrL1CacheLineInfo : public CacheLineInfo
{
public:
   PrL1CacheLineInfo(IntPtr tag = ~0, CacheState::Type cstate = CacheState::INVALID, UInt64 curr_time = 0)
      : CacheLineInfo(tag, cstate, curr_time) {}
   ~PrL1CacheLineInfo() {}
};
