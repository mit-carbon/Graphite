#include "pr_l2_cache_line_info.h"
#include "cache_utils.h"
#include "log.h"
   
PrL2CacheLineInfo::PrL2CacheLineInfo(IntPtr tag, CacheState::Type cstate, MemComponent::Type cached_loc, UInt64 curr_time)
   : CacheLineInfo(tag, cstate, curr_time)
   , _cached_loc_bitvec(0)
   , _L1_I_lifetime(0)
   , _L1_D_lifetime(0)
{
   if (cached_loc != MemComponent::INVALID_MEM_COMPONENT)
      setCachedLoc(cached_loc);
}

PrL2CacheLineInfo::~PrL2CacheLineInfo()
{}

MemComponent::Type 
PrL2CacheLineInfo::getCachedLoc()
{
   LOG_ASSERT_ERROR(_cached_loc_bitvec != ((1 << MemComponent::L1_ICACHE) | (1 << MemComponent::L1_DCACHE)),
                    "_cached_loc_bitvec(%u)", _cached_loc_bitvec);

   switch (_cached_loc_bitvec)
   {
   case ((UInt32) 1) << MemComponent::L1_ICACHE:
      return MemComponent::L1_ICACHE;

   case ((UInt32) 1) << MemComponent::L1_DCACHE:
      return MemComponent::L1_DCACHE;

   case ((UInt32) 0):
      return MemComponent::INVALID_MEM_COMPONENT;

   default:
      LOG_PRINT_ERROR("Error: _cached_loc_bitvec(%u)", _cached_loc_bitvec);
      return MemComponent::INVALID_MEM_COMPONENT;
   }
}

void 
PrL2CacheLineInfo::setCachedLoc(MemComponent::Type cached_loc)
{
   LOG_ASSERT_ERROR(cached_loc != MemComponent::INVALID_MEM_COMPONENT,
         "_cached_loc_bitvec(%u), cached_loc(%u)",
         _cached_loc_bitvec, cached_loc);

   LOG_ASSERT_ERROR(!(_cached_loc_bitvec & (1 << cached_loc)),
         "_cached_loc_bitvec(%u), cached_loc(%u)",
         _cached_loc_bitvec, cached_loc);

   _cached_loc_bitvec |= (((UInt32) 1) << cached_loc);
}

void
PrL2CacheLineInfo::clearCachedLoc(MemComponent::Type cached_loc)
{
   LOG_ASSERT_ERROR(cached_loc != MemComponent::INVALID_MEM_COMPONENT,
         "_cached_loc_bitvec(%u), cached_loc(%u)",
         _cached_loc_bitvec, cached_loc);

   LOG_ASSERT_ERROR(_cached_loc_bitvec & (1 << cached_loc),
         "_cached_loc_bitvec(%u), cached_loc(%u)", 
         _cached_loc_bitvec, cached_loc);

   _cached_loc_bitvec &= (~(((UInt32) 1) << cached_loc));
}

void 
PrL2CacheLineInfo::invalidate(CacheLineUtilization& utilization, UInt64 time)
{
   _cached_loc_bitvec = 0;
   _L1_I_utilization = CacheLineUtilization();
   _L1_D_utilization = CacheLineUtilization();
   _L1_I_lifetime = 0;
   _L1_D_lifetime = 0;
   CacheLineInfo::invalidate(utilization, time);
}

void 
PrL2CacheLineInfo::assign(CacheLineInfo* cache_line_info)
{
   _cached_loc_bitvec = ((PrL2CacheLineInfo*) cache_line_info)->getCachedLocBitVec();
   AggregateCacheLineUtilization aggregate_utilization = ((PrL2CacheLineInfo*) cache_line_info)->getAggregateUtilization();
   _L1_I_utilization = aggregate_utilization.L1_I;
   _L1_D_utilization = aggregate_utilization.L1_D;
   AggregateCacheLineLifetime aggregate_lifetime = ((PrL2CacheLineInfo*) cache_line_info)->getAggregateLifetime(0);
   _L1_I_lifetime = aggregate_lifetime.L1_I;
   _L1_D_lifetime = aggregate_lifetime.L1_D;
   CacheLineInfo::assign(cache_line_info);
}

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
