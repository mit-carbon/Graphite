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
PrL2CacheLineInfo::clearAllCachedLoc()
{
   _cached_loc_bitvec = 0;
}


void 
PrL2CacheLineInfo::invalidate()
{
   _cached_loc_bitvec = 0;
   CacheLineInfo::invalidate();
}

void 
PrL2CacheLineInfo::assign(CacheLineInfo* cache_line_info)
{
   _cached_loc_bitvec = ((PrL2CacheLineInfo*) cache_line_info)->getCachedLocBitVec();
   CacheLineInfo::assign(cache_line_info);
}
