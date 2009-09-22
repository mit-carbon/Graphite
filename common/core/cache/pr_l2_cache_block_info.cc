#include "pr_l2_cache_block_info.h"

MemComponent::component_t 
PrL2CacheBlockInfo::getCachedLoc() 
{ 
   return m_cached_loc; 
}

void 
PrL2CacheBlockInfo::setCachedLoc(MemComponent::component_t cached_loc) 
{
   assert(m_cached_loc == MemComponent::INVALID_MEM_COMPONENT);
   assert(cached_loc != MemComponent::INVALID_MEM_COMPONENT);
   m_cached_loc = cached_loc;
}

void
PrL2CacheBlockInfo::clearCachedLoc(MemComponent::component_t cached_loc)
{
   assert(m_cached_loc == cached_loc);
   assert(cached_loc != MemComponent::INVALID_MEM_COMPONENT);
   m_cached_loc = MemComponent::INVALID_MEM_COMPONENT;
}

void 
PrL2CacheBlockInfo::invalidate()
{
   m_cached_loc = MemComponent::INVALID_MEM_COMPONENT;
   CacheBlockInfo::invalidate();
}

void 
PrL2CacheBlockInfo::clone(CacheBlockInfo* cache_block_info)
{
   m_cached_loc = ((PrL2CacheBlockInfo*) cache_block_info)->getCachedLoc();
   CacheBlockInfo::clone(cache_block_info);
}
