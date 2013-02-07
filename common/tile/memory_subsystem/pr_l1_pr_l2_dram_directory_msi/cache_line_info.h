#pragma once

#include "../cache/cache_line_info.h"
#include "cache_state.h"
#include "cache_level.h"
#include "mem_component.h"

namespace PrL1PrL2DramDirectoryMSI
{

CacheLineInfo* createCacheLineInfo(SInt32 cache_level);

typedef CacheLineInfo PrL1CacheLineInfo;

class PrL2CacheLineInfo : public CacheLineInfo
{
public:
   PrL2CacheLineInfo(IntPtr tag = ~0, CacheState::Type cstate = CacheState::INVALID,
                     MemComponent::Type cached_loc = MemComponent::INVALID);
   ~PrL2CacheLineInfo();

   MemComponent::Type getCachedLoc();
   void setCachedLoc(MemComponent::Type cached_loc);
   void setForcedCachedLoc(MemComponent::Type cached_loc);
   void clearCachedLoc(MemComponent::Type cached_loc);

   void invalidate();
   void assign(CacheLineInfo* cache_line_info);

private:
   MemComponent::Type _cached_loc;
};

}
