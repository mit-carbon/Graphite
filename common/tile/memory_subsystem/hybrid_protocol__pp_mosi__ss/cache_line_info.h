#pragma once

#include "../cache/cache_line_info.h"
#include "cache_state.h"
#include "cache_line_info.h"
#include "cache_level.h"
#include "mem_component.h"

namespace HybridProtocol_PPMOSI_SS
{

CacheLineInfo* createCacheLineInfo(SInt32 cache_level);

class HybridL1CacheLineInfo : public CacheLineInfo
{
public:
   HybridL1CacheLineInfo(IntPtr tag = ~0, CacheState::Type cstate = CacheState::INVALID);
   ~HybridL1CacheLineInfo();

   UInt32 getUtilization() const             { return _utilization;     }
   void incrUtilization(UInt32 count = 1)    { _utilization += count;   }

   void invalidate();
   void assign(CacheLineInfo* cache_line_info);
   
private:
   UInt32 _utilization;
};

class HybridL2CacheLineInfo : public HybridL1CacheLineInfo
{
public:
   HybridL2CacheLineInfo(IntPtr tag = ~0, CacheState::Type cstate = CacheState::INVALID,
                         bool locked = false, MemComponent::Type cached_loc = MemComponent::INVALID);
   ~HybridL2CacheLineInfo();

   void lock()             { _locked = true;  }
   void unlock()           { _locked = false; }
   bool isLocked() const   { return _locked;  }
   
   MemComponent::Type getCachedLoc();
   void setCachedLoc(MemComponent::Type cached_loc);
   void clearCachedLoc(MemComponent::Type cached_loc);

   void invalidate();
   void assign(CacheLineInfo* cache_line_info);

private:
   bool _locked;
   MemComponent::Type _cached_loc;
};

#define SPELL_LOCKED(x)    (x ? "TRUE" : "FALSE")

}
