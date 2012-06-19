#pragma once

#include "../cache/cache_line_info.h"
#include "cache_state.h"
#include "cache_line_info.h"
#include "cache_level.h"
#include "mem_component.h"
#include "common_defines.h"

namespace HybridProtocol_PPMOSI_SS
{

CacheLineInfo* createCacheLineInfo(SInt32 cache_level);

typedef CacheLineInfo HybridL1CacheLineInfo;

class HybridL2CacheLineInfo : public CacheLineInfo
{
public:
   HybridL2CacheLineInfo(IntPtr tag = ~0, CacheState::Type cstate = CacheState::INVALID,
                         MemComponent::Type cached_loc = MemComponent::INVALID, UInt64 curr_time = 0);
   ~HybridL2CacheLineInfo();

   MemComponent::Type getCachedLoc();
   void setCachedLoc(MemComponent::Type cached_loc);
   void clearCachedLoc(MemComponent::Type cached_loc);

   void invalidate();
   void assign(CacheLineInfo* cache_line_info);

private:
   MemComponent::Type _cached_loc;
};

}
