#pragma once

#include "../cache/cache_line_info.h"

namespace PrL1PrL2DramDirectoryMOSI
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

typedef CacheLineInfo PrL1CacheLineInfo;

#include "cache_state.h"
#include "cache_line_info.h"
#include "mem_component.h"
#include "common_defines.h"

#ifdef TRACK_DETAILED_CACHE_COUNTERS
#include "aggregate_cache_line_utilization.h"
#include "aggregate_cache_line_lifetime.h"
#endif

class PrL2CacheLineInfo : public CacheLineInfo
{
public:
   PrL2CacheLineInfo(IntPtr tag = ~0, CacheState::Type cstate = CacheState::INVALID,
                     MemComponent::Type cached_loc = MemComponent::INVALID_MEM_COMPONENT, UInt64 curr_time = 0);
   ~PrL2CacheLineInfo();

   MemComponent::Type getCachedLoc();
   void setCachedLoc(MemComponent::Type cached_loc);
   void clearCachedLoc(MemComponent::Type cached_loc);

   void invalidate();
   void assign(CacheLineInfo* cache_line_info);

#ifdef TRACK_DETAILED_CACHE_COUNTERS
   void incrUtilization(MemComponent::Type mem_component, CacheLineUtilization& utilization);
   void incrLifetime(MemComponent::Type mem_component, UInt64 lifetime);
   AggregateCacheLineUtilization getAggregateUtilization();
   AggregateCacheLineLifetime getAggregateLifetime(UInt64 curr_time);
#endif

private:
   MemComponent::Type _cached_loc;

#ifdef TRACK_DETAILED_CACHE_COUNTERS
   CacheLineUtilization _L1_I_utilization;
   CacheLineUtilization _L1_D_utilization;
   UInt64 _L1_I_lifetime;
   UInt64 _L1_D_lifetime;
#endif
};

}
