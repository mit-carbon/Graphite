#pragma once

#include "cache_state.h"
#include "cache_line_info.h"
#include "mem_component.h"

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
   void clearAllCachedLoc();

   UInt32 getCachedLocBitVec() { return _cached_loc_bitvec; }

   void invalidate();
   void assign(CacheLineInfo* cache_line_info);

private:
   UInt32 _cached_loc_bitvec;
};
