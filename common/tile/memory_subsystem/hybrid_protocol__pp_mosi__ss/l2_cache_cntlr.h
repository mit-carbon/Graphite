#pragma once

// Forward declarations
namespace HybridProtocol_PPMOSI_SS
{
   class L1CacheCntlr;
   class MemoryManager;
}

#include "cache.h"
#include "cache_line_info.h"
#include "address_home_lookup.h"
#include "shmem_msg.h"
#include "shmem_req.h"
#include "mem_component.h"
#include "lock.h"
#include "l2_cache_hash_fn.h"

namespace HybridProtocol_PPMOSI_SS
{

class L2CacheCntlr
{
public:
   L2CacheCntlr(MemoryManager* memory_manager,
                L1CacheCntlr* l1_cache_cntlr,
                AddressHomeLookup* dram_directory_home_lookup,
                UInt32 cache_line_size,
                UInt32 l2_cache_size,
                UInt32 l2_cache_associativity,
                string l2_cache_replacement_policy,
                UInt32 l2_cache_access_delay,
                bool l2_cache_track_miss_types,
                float frequency);
   ~L2CacheCntlr();

   Cache* getL2Cache() { return _l2_cache; }

   // Handle Request from L1 Cache - This is done for better simulator performance
   void processMemOpFromL1Cache(MemComponent::Type mem_component, Core::lock_signal_t lock_signal, Core::mem_op_t mem_op_type, 
                                IntPtr ca_address, UInt32 offset, Byte* data_buf, UInt32 data_length, bool modeled);
   // Write-through Cache. Hence needs to be written by the APP thread
   void writeCacheLine(IntPtr address, Byte* data_buf, UInt32 offset = 0, UInt32 data_length = UINT32_MAX_);
   // APIs' to lock and unlock the cache line
   void lockCacheLine(IntPtr address);
   void unlockCacheLine(IntPtr address);

   // Handle message from L1 Cache
   void handleMsgFromL1Cache(ShmemMsg* shmem_msg);
   // Handle message from Dram Dir
   void handleMsgFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);
   // Acquiring and Releasing Locks
   void acquireLock();
   void releaseLock();

private:
   // Data Members
   MemoryManager* _memory_manager;
   Cache* _l2_cache;
   L1CacheCntlr* _l1_cache_cntlr;
   AddressHomeLookup* _dram_directory_home_lookup;
   // Replacement policy and hash function
   CacheReplacementPolicy* _l2_cache_replacement_policy_obj;
   L2CacheHashFn* _l2_cache_hash_fn_obj;

   // Outstanding ShmemReq info
   ShmemMsg _outstanding_shmem_msg;
   UInt64 _outstanding_shmem_msg_time;

   // Synchronization between app and sim threads
   Lock _l2_cache_lock;

   // L2 cache operations
   void readCacheLine(IntPtr address, Byte* data_buf, UInt32 offset = 0, UInt32 data_length = UINT32_MAX_);
   void insertCacheLine(IntPtr address, CacheState::Type cstate, Byte* fill_buf, MemComponent::Type mem_component);

   // L1 cache operations
   void setCacheLineStateInL1(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate);
   void invalidateCacheLineInL1(MemComponent::Type mem_component, IntPtr address);
   void insertCacheLineInL1(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate, Byte* fill_buf);
   void lockCacheLineInL1(MemComponent::Type mem_component, IntPtr address);
   void unlockCacheLineInL1(MemComponent::Type mem_component, IntPtr address);

   // Insert cache line in hierarchy
   void insertCacheLineInHierarchy(IntPtr address, CacheState::Type cstate, Byte* fill_buf);

   // Process Request from L1 Cache
   // Check if msg from L1 ends in the L2 cache
   pair<bool,Cache::MissType> getMemOpStatusInL2Cache(MemComponent::Type mem_component, IntPtr address,
                                                      Core::mem_op_t mem_op_type, HybridL2CacheLineInfo& l2_cache_line_info);

   // Process Reply from Dram Directory
   void processReadReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply);
   void processWriteReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply);
   
   void processExReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply);
   void processShReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply);
   void processUpgradeReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply);

   // Process Request from Dram Directory
   void processRemoteReadReqFromDramDirectory(tile_id_t sender, ShmemMsg* read_req);
   void processRemoteWriteReqFromDramDirectory(tile_id_t sender, ShmemMsg* read_req);

   void processInvReqFromDramDirectory(tile_id_t sender, ShmemMsg* inv_req);
   void processFlushReqFromDramDirectory(tile_id_t sender, ShmemMsg* flush_req);
   void processWbReqFromDramDirectory(tile_id_t sender, ShmemMsg* wb_req);
   void processInvFlushCombinedReqFromDramDirectory(tile_id_t sender, ShmemMsg* inv_flush_req);

   // Get Shmem Msg Type
   ShmemMsg::Type getShmemMsgType(Core::mem_op_t mem_op_type);

   // Dram Directory Home Lookup
   tile_id_t getDramDirectoryHome(IntPtr address)
   { return _dram_directory_home_lookup->getHome(address); }

   MemComponent::Type acquireL1CacheLock(ShmemMsg::Type msg_type, IntPtr address);
   
   // Utilities
   tile_id_t getTileId();
   UInt32 getCacheLineSize();
   MemoryManager* getMemoryManager()   { return _memory_manager; }
   ShmemPerfModel* getShmemPerfModel();
};

}
