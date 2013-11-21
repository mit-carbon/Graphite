#pragma once

// Forward declarations
namespace PrL1ShL2MSI
{
   class MemoryManager;
}

#include <map>
using std::map;

#include "core.h"
#include "cache.h"
#include "cache_line_info.h"
#include "address_home_lookup.h"
#include "shmem_msg.h"
#include "shmem_req.h"
#include "mem_component.h"
#include "fixed_types.h"
#include "hash_map_list.h"
#include "shmem_perf_model.h"
#include "cache_replacement_policy.h"
#include "cache_hash_fn.h"

namespace PrL1ShL2MSI
{
   class L2CacheCntlr
   {
   public:
      L2CacheCntlr(MemoryManager* memory_manager,
                   AddressHomeLookup* dram_home_lookup,
                   UInt32 cache_line_size,
                   UInt32 L2_cache_size,
                   UInt32 L2_cache_associativity,
                   UInt32 L2_cache_num_banks,
                   string L2_cache_replacement_policy,
                   UInt32 L2_cache_data_access_cycles,
                   UInt32 L2_cache_tags_access_cycles,
                   string L2_cache_perf_model_type,
                   bool L2_cache_track_miss_types);
      ~L2CacheCntlr();

      Cache* getL2Cache() { return _L2_cache; }

      // Handle message from L1 Cache
      void handleMsgFromL1Cache(tile_id_t sender, ShmemMsg* shmem_msg);
      // Handle message from Dram 
      void handleMsgFromDram(tile_id_t sender, ShmemMsg* shmem_msg);
      // Output summary
      void outputSummary(ostream& out);

      void enable() { _enabled = true; }
      void disable() { _enabled = false; }

   private:
      // Data Members
      MemoryManager* _memory_manager;
      Cache* _L2_cache;
      CacheReplacementPolicy* _L2_cache_replacement_policy_obj;
      CacheHashFn* _L2_cache_hash_fn_obj;
      AddressHomeLookup* _dram_home_lookup;

      // Is enabled?
      bool _enabled;

      // Req list into the L2 cache
      HashMapList<IntPtr,ShmemReq*> _L2_cache_req_queue;
      // Evicted cache line map
      map<IntPtr,ShL2CacheLineInfo> _evicted_cache_line_map;

      // L2 cache operations
      void getCacheLineInfo(IntPtr address, ShL2CacheLineInfo* L2_cache_line_info,
                            ShmemMsg::Type shmem_msg_type, bool update_miss_counters = false);
      void setCacheLineInfo(IntPtr address, ShL2CacheLineInfo* L2_cache_line_info);
      void readCacheLine(IntPtr address, Byte* data_buf);
      void writeCacheLine(IntPtr address, Byte* data_buf);
      void allocateCacheLine(IntPtr address, ShL2CacheLineInfo* L2_cache_line_info);

      // Process Request to invalidate the sharers of a cache line
      void processNullifyReq(ShmemReq* nullify_req, Byte* data_buf);
      // Process Request from L1-I/L1-D caches
      void processExReqFromL1Cache(ShmemReq* shmem_req, Byte* data_buf, bool first_call = false);
      void processShReqFromL1Cache(ShmemReq* shmem_req, Byte* data_buf, bool first_call = false);
      void processInvRepFromL1Cache(tile_id_t sender, const ShmemMsg* shmem_msg, ShL2CacheLineInfo* L2_cache_line_info);
      void processFlushRepFromL1Cache(tile_id_t sender, const ShmemMsg* shmem_msg, ShL2CacheLineInfo* L2_cache_line_info);
      void processWbRepFromL1Cache(tile_id_t sender, const ShmemMsg* shmem_msg, ShL2CacheLineInfo* L2_cache_line_info);

      // Restart the shmem request
      void restartShmemReq(ShmemReq* shmem_req, ShL2CacheLineInfo* L2_cache_line_info, Byte* data_buf);
      // Process the next request to a cache line
      void processNextReqFromL1Cache(IntPtr address);
      // Process shmem request
      void processShmemReq(ShmemReq* shmem_req);
      // Send invalidation msg to multiple tiles
      void sendInvalidationMsg(ShmemMsg::Type requester_msg_type,
                               IntPtr address, MemComponent::Type receiver_mem_component,
                               bool all_tiles_sharers, vector<tile_id_t>& sharers_list,
                               tile_id_t requester, bool msg_modeled);
      // Read data from L2 cache and send to L1-I/L1-D cache
      void readCacheLineAndSendToL1Cache(ShmemMsg::Type reply_msg_type,
                                         IntPtr address, MemComponent::Type requester_mem_component,
                                         Byte* data_buf,
                                         tile_id_t requester, bool msg_modeled);
      // Fetch/store data from/in DRAM
      void fetchDataFromDram(IntPtr address, tile_id_t requester, bool msg_modeled);
      void storeDataInDram(IntPtr address, Byte* data_buf, tile_id_t requester, bool msg_modeled);

      // Utilities
      tile_id_t getTileId();
      UInt32 getCacheLineSize();
      ShmemPerfModel* getShmemPerfModel();
      Core::mem_op_t getMemOpTypeFromShmemMsgType(ShmemMsg::Type shmem_msg_type);

      // Dram Home Lookup
      tile_id_t getDramHome(IntPtr address) { return _dram_home_lookup->getHome(address); }
   };

}
