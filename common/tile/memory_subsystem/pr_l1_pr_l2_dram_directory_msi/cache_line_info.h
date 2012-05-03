#pragma once

#include "../cache/cache_line_info.h"

namespace PrL1PrL2DramDirectoryMSI
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

}

#include "cache_state.h"
#include "cache_line_info.h"
#include "mem_component.h"

namespace PrL1PrL2DramDirectoryMSI
{

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

private:
   MemComponent::Type _cached_loc;
};

}
