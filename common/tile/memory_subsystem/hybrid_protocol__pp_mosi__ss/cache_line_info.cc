#include "cache_line_info.h"
#include "cache_utils.h"
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

//// Hybrid L1 CacheLineInfo
HybridL1CacheLineInfo::HybridL1CacheLineInfo(IntPtr tag, CacheState::Type cstate)
   : CacheLineInfo(tag, cstate)
   , _utilization(0)
{}

HybridL1CacheLineInfo::~HybridL1CacheLineInfo()
{}

void
HybridL1CacheLineInfo::invalidate()
{
   CacheLineInfo::invalidate();
   _utilization = 0;
}

void
HybridL1CacheLineInfo::assign(CacheLineInfo* cache_line_info)
{
   CacheLineInfo::assign(cache_line_info);
   HybridL1CacheLineInfo* l1_cache_line_info = dynamic_cast<HybridL1CacheLineInfo*>(cache_line_info);
   _utilization = l1_cache_line_info->getUtilization();
}

//// Hybrid L2 CacheLineInfo

HybridL2CacheLineInfo::HybridL2CacheLineInfo(IntPtr tag, CacheState::Type cstate,
                                             bool locked, MemComponent::Type cached_loc)
   : HybridL1CacheLineInfo(tag, cstate)
   , _locked(locked)
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
   HybridL1CacheLineInfo::invalidate();
   LOG_ASSERT_ERROR(!_locked, "Cannot invalidate cache line with tag(%#lx) when locked", _tag);
   _cached_loc = MemComponent::INVALID;
}

void 
HybridL2CacheLineInfo::assign(CacheLineInfo* cache_line_info)
{
   HybridL1CacheLineInfo::assign(cache_line_info);
   HybridL2CacheLineInfo* l2_cache_line_info = dynamic_cast<HybridL2CacheLineInfo*>(cache_line_info);
   _locked = l2_cache_line_info->isLocked();
   _cached_loc = l2_cache_line_info->getCachedLoc();
}

}
