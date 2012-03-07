#include "pr_l2_cache_line_info.h"
#include "log.h"

MemComponent::component_t 
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
PrL2CacheLineInfo::setCachedLoc(MemComponent::component_t cached_loc)
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
PrL2CacheLineInfo::clearCachedLoc(MemComponent::component_t cached_loc)
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
PrL2CacheLineInfo::invalidate()
{
   _cached_loc_bitvec = 0;
   _L1_I_utilization = CacheLineUtilization();
   _L1_D_utilization = CacheLineUtilization();
   CacheLineInfo::invalidate();
}

void 
PrL2CacheLineInfo::assign(CacheLineInfo* cache_line_info)
{
   _cached_loc_bitvec = ((PrL2CacheLineInfo*) cache_line_info)->getCachedLocBitVec();
   _L1_I_utilization = ((PrL2CacheLineInfo*) cache_line_info)->getUtilization(MemComponent::L1_ICACHE);
   _L1_D_utilization = ((PrL2CacheLineInfo*) cache_line_info)->getUtilization(MemComponent::L1_DCACHE);
   CacheLineInfo::assign(cache_line_info);
}

CacheLineUtilization
PrL2CacheLineInfo::getUtilization(MemComponent::component_t mem_component)
{
   switch (mem_component)
   {
   case MemComponent::L1_ICACHE:
      return _L1_I_utilization;
   case MemComponent::L1_DCACHE:
      return _L1_D_utilization;
   default:
      LOG_PRINT_ERROR("Unrecognized mem component(%u)", mem_component);
      return CacheLineUtilization();
   }
}

void
PrL2CacheLineInfo::incrUtilization(MemComponent::component_t mem_component, CacheLineUtilization& utilization)
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
