#ifndef __PR_L2_CACHE_BLOCK_INFO_H__
#define __PR_L2_CACHE_BLOCK_INFO_H__

#include "cache_state.h"
#include "cache_block_info.h"
#include "mem_component.h"

class PrL2CacheBlockInfo : public CacheBlockInfo
{
   private:
      MemComponent::component_t m_cached_loc;

   public:
      PrL2CacheBlockInfo(IntPtr tag = ~0,
            CacheState::cstate_t cstate = CacheState::INVALID):
         CacheBlockInfo(tag, cstate),
         m_cached_loc(MemComponent::INVALID_MEM_COMPONENT)
      {}

      ~PrL2CacheBlockInfo() {}

      MemComponent::component_t getCachedLoc() { return m_cached_loc; }
      void setCachedLoc(MemComponent::component_t cached_loc) {
         m_cached_loc = cached_loc;
      }

      void invalidate()
      {
         m_cached_loc = MemComponent::INVALID_MEM_COMPONENT;
         CacheBlockInfo::invalidate();
      }

      void clone(CacheBlockInfo* cache_block_info)
      {
         m_cached_loc = ((PrL2CacheBlockInfo*) cache_block_info)->getCachedLoc();
         CacheBlockInfo::clone(cache_block_info);
      }
};
#endif /* __PR_L2_CACHE_BLOCK_INFO_H__ */
