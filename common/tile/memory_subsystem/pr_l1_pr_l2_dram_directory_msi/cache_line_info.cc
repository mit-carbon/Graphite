#include "cache_line_info.h"
#include "cache_utils.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMSI
{

CacheLineInfo::CacheLineInfo(IntPtr tag, CacheState::Type cstate, UInt64 curr_time)
   : ::CacheLineInfo(tag, cstate, curr_time)
{}

CacheLineInfo::~CacheLineInfo()
{}

CacheLineInfo*
CacheLineInfo::create(SInt32 cache_level)
{
   switch (cache_level)
   {
   case L1:
      return new PrL1CacheLineInfo();
   case L2:
      return new PrL2CacheLineInfo();
   default:
      LOG_PRINT_ERROR("Unrecognized Cache Level(%u)", cache_level);
      return (CacheLineInfo*) NULL;
   }
}

//// PrL2 CacheLineInfo

PrL2CacheLineInfo::PrL2CacheLineInfo(IntPtr tag, CacheState::Type cstate, MemComponent::Type cached_loc, UInt64 curr_time)
   : CacheLineInfo(tag, cstate, curr_time)
   , _cached_loc(cached_loc)
{}

PrL2CacheLineInfo::~PrL2CacheLineInfo()
{}

MemComponent::Type 
PrL2CacheLineInfo::getCachedLoc()
{
   return _cached_loc;
}

void 
PrL2CacheLineInfo::setCachedLoc(MemComponent::Type cached_loc)
{
   assert(cached_loc != MemComponent::INVALID_MEM_COMPONENT);
   assert(_cached_loc == MemComponent::INVALID_MEM_COMPONENT);
   _cached_loc = cached_loc;
}

void
PrL2CacheLineInfo::clearCachedLoc(MemComponent::Type cached_loc)
{
   assert(cached_loc != MemComponent::INVALID_MEM_COMPONENT);
   assert(_cached_loc == cached_loc);
   _cached_loc = MemComponent::INVALID_MEM_COMPONENT;
}

void 
PrL2CacheLineInfo::invalidate()
{
   CacheLineInfo::invalidate();
   _cached_loc = MemComponent::INVALID_MEM_COMPONENT;
}

void 
PrL2CacheLineInfo::assign(CacheLineInfo* cache_line_info)
{
   CacheLineInfo::assign(cache_line_info);
   _cached_loc = ((PrL2CacheLineInfo*) cache_line_info)->getCachedLoc();
}

}
