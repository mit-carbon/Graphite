#include "cache_line_info.h"
#include "cache_utils.h"
#include "common_defines.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

CacheLineInfo*
createCacheLineInfo(SInt32 cache_level)
{
   switch (cache_level)
   {
   case L1:
      return new HybridL1CacheLineInfo();
   case L2:
      return new HybridL2CacheLineInfo();
   default:
      LOG_PRINT_ERROR("Unrecognized Cache Level(%u)", cache_level);
      return (CacheLineInfo*) NULL;
   }
}

//// Hybrid L2 CacheLineInfo

HybridL2CacheLineInfo::HybridL2CacheLineInfo(IntPtr tag, CacheState::Type cstate, MemComponent::Type cached_loc, UInt64 curr_time)
   : CacheLineInfo(tag, cstate, curr_time)
   , _cached_loc(cached_loc)
{}

HybridL2CacheLineInfo::~HybridL2CacheLineInfo()
{}

MemComponent::Type 
HybridL2CacheLineInfo::getCachedLoc()
{
   return _cached_loc;
}

void 
HybridL2CacheLineInfo::setCachedLoc(MemComponent::Type cached_loc)
{
   assert(cached_loc != MemComponent::INVALID);
   assert(_cached_loc == MemComponent::INVALID);
   _cached_loc = cached_loc;
}

void
HybridL2CacheLineInfo::clearCachedLoc(MemComponent::Type cached_loc)
{
   assert(cached_loc != MemComponent::INVALID);
   assert(_cached_loc == cached_loc);
   _cached_loc = MemComponent::INVALID;
}

void 
HybridL2CacheLineInfo::invalidate()
{
   CacheLineInfo::invalidate();
   _cached_loc = MemComponent::INVALID;
}

void 
HybridL2CacheLineInfo::assign(CacheLineInfo* cache_line_info)
{
   CacheLineInfo::assign(cache_line_info);
   HybridL2CacheLineInfo* L2_cache_line_info = dynamic_cast<HybridL2CacheLineInfo*>(cache_line_info);
   _cached_loc = L2_cache_line_info->getCachedLoc();
}

}
