#pragma once

#include "../cache/cache_line_info.h"
#include "cache_state.h"
#include "cache_line_info.h"
#include "cache_level.h"
#include "mem_component.h"
#include "utilization_defines.h"

namespace PrL1PrL2DramDirectoryMOSI
{

CacheLineInfo* createCacheLineInfo(SInt32 cache_level);

class PrL1CacheLineInfo : public CacheLineInfo
{
public:
   PrL1CacheLineInfo(IntPtr tag = ~0, CacheState::Type cstate = CacheState::INVALID);
   ~PrL1CacheLineInfo();

#ifdef TRACK_DETAILED_CACHE_COUNTERS
   UInt32 getUtilization() const             { return _utilization;     }
   void incrUtilization(UInt32 count = 1)    { _utilization += count;   }
#endif

   void invalidate();
   void assign(CacheLineInfo* cache_line_info);
   
private:
#ifdef TRACK_DETAILED_CACHE_COUNTERS
   UInt32 _utilization;
#endif
};

class PrL2CacheLineInfo : public PrL1CacheLineInfo
{
public:
   PrL2CacheLineInfo(IntPtr tag = ~0, CacheState::Type cstate = CacheState::INVALID,
                     MemComponent::Type cached_loc = MemComponent::INVALID);
   ~PrL2CacheLineInfo();

   MemComponent::Type getCachedLoc();
   void setCachedLoc(MemComponent::Type cached_loc);
   void clearCachedLoc(MemComponent::Type cached_loc);

   void invalidate();
   void assign(CacheLineInfo* cache_line_info);

private:
   MemComponent::Type _cached_loc;
};

}
