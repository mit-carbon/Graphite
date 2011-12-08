#pragma once

#include "cache_state.h"
#include "cache_line_info.h"

class PrL1CacheLineInfo : public CacheLineInfo
{
public:
   PrL1CacheLineInfo(IntPtr tag = ~0, CacheState::CState cstate = CacheState::INVALID)
      : CacheLineInfo(tag, cstate) {}
   ~PrL1CacheLineInfo() {}
};
