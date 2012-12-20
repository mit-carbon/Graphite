#include "l2_cache_replacement_policy.h"
#include "utils.h"
#include "config.h"
#include "cache_line_info.h"
#include "log.h"

namespace PrL1ShL2MSI
{

L2CacheReplacementPolicy::L2CacheReplacementPolicy(UInt32 cache_size, UInt32 associativity, UInt32 cache_line_size,
                                                   HashMapList<IntPtr,ShmemReq*>& L2_cache_req_list)
   : CacheReplacementPolicy(cache_size, associativity, cache_line_size)
   , _L2_cache_req_list(L2_cache_req_list)
{
   _log_cache_line_size = floorLog2(cache_line_size);
}

L2CacheReplacementPolicy::~L2CacheReplacementPolicy()
{}

UInt32
L2CacheReplacementPolicy::getReplacementWay(CacheLineInfo** cache_line_info_array, UInt32 set_num)
{
   UInt32 way = UINT32_MAX_;
   SInt32 min_num_sharers = (SInt32) Config::getSingleton()->getTotalTiles() + 1;

   for (UInt32 i = 0; i < _associativity; i++)
   {
      ShL2CacheLineInfo* L2_cache_line_info = dynamic_cast<ShL2CacheLineInfo*>(cache_line_info_array[i]);

      if (L2_cache_line_info->getCState() == CacheState::INVALID)
      {
         return i;
      }
      else
      {
         IntPtr address = getAddressFromTag(L2_cache_line_info->getTag());

         DirectoryEntry* directory_entry = L2_cache_line_info->getDirectoryEntry();
         if (directory_entry->getNumSharers() < min_num_sharers && 
             _L2_cache_req_list.empty(address))
         {
            min_num_sharers = directory_entry->getNumSharers();
            way = i;
         }
      }
   }

   if (way == UINT32_MAX_)
   {
      for (UInt32 i = 0; i < _associativity; i++)
      {
         ShL2CacheLineInfo* L2_cache_line_info = dynamic_cast<ShL2CacheLineInfo*>(cache_line_info_array[i]);
         assert(L2_cache_line_info->getCState() != CacheState::INVALID);
         IntPtr address = getAddressFromTag(L2_cache_line_info->getTag());
         DirectoryEntry* directory_entry = L2_cache_line_info->getDirectoryEntry();
         assert(_L2_cache_req_list.count(address) > 0);
         fprintf(stderr, "i(%u), Address(%#lx), CState(%u), DState(%u), Num Waiters(%u)\n",
                 i, address, L2_cache_line_info->getCState(),
                 directory_entry->getDirectoryBlockInfo()->getDState(),
                 (UInt32) _L2_cache_req_list.count(address));
      }
   }
   LOG_ASSERT_ERROR(way != UINT32_MAX_, "Could not find a replacement candidate");
   return way;
}

void
L2CacheReplacementPolicy::update(CacheLineInfo** cache_line_info_array, UInt32 set_num, UInt32 accessed_way)
{}

IntPtr
L2CacheReplacementPolicy::getAddressFromTag(IntPtr tag) const
{
   return tag << _log_cache_line_size;
}

}
