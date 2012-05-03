#include "cache_line_info.h"
#include "cache_utils.h"
#include "common_defines.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMOSI
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
#ifdef TRACK_DETAILED_CACHE_COUNTERS
   , _L1_I_lifetime(0)
   , _L1_D_lifetime(0)
#endif
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
#ifdef TRACK_DETAILED_CACHE_COUNTERS
   _L1_I_utilization = CacheLineUtilization();
   _L1_D_utilization = CacheLineUtilization();
   _L1_I_lifetime = 0;
   _L1_D_lifetime = 0;
#endif
}

void 
PrL2CacheLineInfo::assign(CacheLineInfo* cache_line_info)
{
   CacheLineInfo::assign(cache_line_info);
   _cached_loc = ((PrL2CacheLineInfo*) cache_line_info)->getCachedLoc();
#ifdef TRACK_DETAILED_CACHE_COUNTERS
   AggregateCacheLineUtilization aggregate_utilization = ((PrL2CacheLineInfo*) cache_line_info)->getAggregateUtilization();
   _L1_I_utilization = aggregate_utilization.L1_I;
   _L1_D_utilization = aggregate_utilization.L1_D;
   AggregateCacheLineLifetime aggregate_lifetime = ((PrL2CacheLineInfo*) cache_line_info)->getAggregateLifetime(0);
   _L1_I_lifetime = aggregate_lifetime.L1_I;
   _L1_D_lifetime = aggregate_lifetime.L1_D;
#endif
}

#ifdef TRACK_DETAILED_CACHE_COUNTERS
void
PrL2CacheLineInfo::incrUtilization(MemComponent::Type mem_component, CacheLineUtilization& utilization)
{
   switch (mem_component)
   {
   case MemComponent::L1_ICACHE:
      _L1_I_utilization += utilization;
      break;
   case MemComponent::L1_DCACHE:
      _L1_D_utilization += utilization;
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized mem component(%u)", mem_component);
      break;
   }
}

void
PrL2CacheLineInfo::incrLifetime(MemComponent::Type mem_component, UInt64 lifetime)
{
   switch (mem_component)
   {
   case MemComponent::L1_ICACHE:
      _L1_I_lifetime += lifetime;
      break;
   case MemComponent::L1_DCACHE:
      _L1_D_lifetime += lifetime;
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized mem component(%u)", mem_component);
      break;
   }
}

AggregateCacheLineUtilization
PrL2CacheLineInfo::getAggregateUtilization()
{
   AggregateCacheLineUtilization aggregate_utilization;
   aggregate_utilization.L1_I = _L1_I_utilization;
   aggregate_utilization.L1_D = _L1_D_utilization;
   aggregate_utilization.L2 = _utilization;
   return aggregate_utilization;
}

AggregateCacheLineLifetime
PrL2CacheLineInfo::getAggregateLifetime(UInt64 curr_time)
{
   AggregateCacheLineLifetime aggregate_lifetime;
   aggregate_lifetime.L1_I = _L1_I_lifetime;
   aggregate_lifetime.L1_D = _L1_D_lifetime;
   aggregate_lifetime.L2 = computeLifetime(_birth_time, curr_time);
   return aggregate_lifetime;
}
#endif

}
