#pragma once

#include "../cache/cache_replacement_policy.h"
#include "hash_map_queue.h"
#include "shmem_req.h"

namespace PrL1ShL2MSI
{

class L2CacheReplacementPolicy : public CacheReplacementPolicy
{
public:
   L2CacheReplacementPolicy(UInt32 cache_size, UInt32 associativity, UInt32 cache_line_size,
                            HashMapQueue<IntPtr,ShmemReq*>& L2_cache_req_queue_list);
   ~L2CacheReplacementPolicy();

   UInt32 getReplacementWay(CacheLineInfo** cache_line_info_array, UInt32 set_num);
   void update(CacheLineInfo** cache_line_info_array, UInt32 set_num, UInt32 accessed_way);

private:
   HashMapQueue<IntPtr,ShmemReq*>& _L2_cache_req_queue_list;
   UInt32 _log_cache_line_size;
   
   IntPtr getAddressFromTag(IntPtr tag) const;
};

}
