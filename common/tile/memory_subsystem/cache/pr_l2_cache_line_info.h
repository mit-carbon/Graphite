#pragma once

#include "cache_state.h"
#include "cache_line_info.h"
#include "mem_component.h"
#include "aggregate_cache_line_utilization.h"

class PrL2CacheLineInfo : public CacheLineInfo
{
public:
   PrL2CacheLineInfo(IntPtr tag = ~0, CacheState::CState cstate = CacheState::INVALID)
      : CacheLineInfo(tag, cstate)
      , _cached_loc_bitvec(0)
   {}

   ~PrL2CacheLineInfo() {}

   MemComponent::component_t getCachedLoc();
   MemComponent::component_t getSingleCachedLoc();
   void setCachedLoc(MemComponent::component_t cached_loc);
   void clearCachedLoc(MemComponent::component_t cached_loc);
   void clearAllCachedLoc() { _cached_loc_bitvec = 0; }
   UInt32 getCachedLocBitVec() { return _cached_loc_bitvec; }

   void invalidate();
   void assign(CacheLineInfo* cache_line_info);

   CacheLineUtilization getUtilization(MemComponent::component_t mem_component);
   void incrUtilization(MemComponent::component_t, CacheLineUtilization& utilization);
   AggregateCacheLineUtilization getAggregateUtilization();

private:
   UInt32 _cached_loc_bitvec;
   CacheLineUtilization _L1_I_utilization;
   CacheLineUtilization _L1_D_utilization;
};
