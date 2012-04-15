#pragma once

#include "cache_state.h"
#include "cache_line_info.h"
#include "mem_component.h"
#include "aggregate_cache_line_utilization.h"
#include "aggregate_cache_line_lifetime.h"

class PrL2CacheLineInfo : public CacheLineInfo
{
public:
   PrL2CacheLineInfo(IntPtr tag = ~0, CacheState::Type cstate = CacheState::INVALID,
                     MemComponent::Type cached_loc = MemComponent::INVALID_MEM_COMPONENT, UInt64 curr_time = 0);
   ~PrL2CacheLineInfo();

   MemComponent::Type getCachedLoc();
   MemComponent::Type getSingleCachedLoc();
   void setCachedLoc(MemComponent::Type cached_loc);
   void clearCachedLoc(MemComponent::Type cached_loc);
   void clearAllCachedLoc() { _cached_loc_bitvec = 0; }
   UInt32 getCachedLocBitVec() { return _cached_loc_bitvec; }

   void invalidate(CacheLineUtilization& utilization, UInt64 time);
   void assign(CacheLineInfo* cache_line_info);

   void incrUtilization(MemComponent::Type mem_component, CacheLineUtilization& utilization);
   void incrLifetime(MemComponent::Type mem_component, UInt64 lifetime);
   
   AggregateCacheLineUtilization getAggregateUtilization();
   AggregateCacheLineLifetime getAggregateLifetime(UInt64 curr_time);

private:
   UInt32 _cached_loc_bitvec;
   CacheLineUtilization _L1_I_utilization;
   CacheLineUtilization _L1_D_utilization;
   UInt64 _L1_I_lifetime;
   UInt64 _L1_D_lifetime;
};
