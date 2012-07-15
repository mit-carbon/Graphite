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
#include "mem_op.h"
#include "shmem_msg.h"
#include "mem_component.h"
#include "l2_cache_hash_fn.h"
#include "l2_cache_replacement_policy.h"

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
   bool processMemOpFromL1Cache(MemComponent::Type mem_component, Core::mem_op_t mem_op_type, Core::lock_signal_t lock_signal, 
                                IntPtr ca_address, Byte* data_buf, UInt32 offset, UInt32 data_length, bool modeled);
   // Write-through Cache. Hence needs to be written by the APP thread
   void writeCacheLine(IntPtr address, Byte* data_buf, UInt32 offset = 0, UInt32 data_length = UINT32_MAX_);
   void invalidateCacheLine(IntPtr address);
   // APIs' to lock and unlock the cache line
   void lockCacheLine(IntPtr address);
   void unlockCacheLine(IntPtr address);
   // Get/set cache line state
   CacheState::Type getCacheLineState(IntPtr address);
   void setCacheLineState(IntPtr address, CacheState::Type cstate);

   // Handle message from L1 Cache
   void handleMsgFromL1Cache(ShmemMsg* shmem_msg);
   // Handle message from Dram Dir
   void handleMsgFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);

private:
   // Data Members
   MemoryManager* _memory_manager;
   Cache* _l2_cache;
   L1CacheCntlr* _l1_cache_cntlr;
   AddressHomeLookup* _dram_directory_home_lookup;
   // Replacement policy and hash function
   L2CacheReplacementPolicy* _l2_cache_replacement_policy_obj;
   L2CacheHashFn* _l2_cache_hash_fn_obj;

   // Outstanding mem op info
   MemOp _outstanding_mem_op;
   UInt64 _outstanding_mem_op_time;

   // L2 cache operations
   void readCacheLine(IntPtr address, Byte* data_buf, UInt32 offset = 0, UInt32 data_length = UINT32_MAX_);
   void insertCacheLine(IntPtr address, CacheState::Type cstate, bool locked, Byte* fill_buf,
                        MemComponent::Type mem_component);

   // L1 cache operations
   void setCacheLineStateInL1(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate);
   void invalidateCacheLineInL1(MemComponent::Type mem_component, IntPtr address);
   void insertCacheLineInL1(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate, Byte* fill_buf);

   // Process upgrade reply from directory
   void processUpgradeReply(ShmemMsg* directory_reply);
   // Assert that L2 cache is ready to handle request from core
   void assertL2CacheReady(Core::mem_op_t mem_op_type, CacheState::Type cstate);

   // Insert cache line in L2/L1 hierarchy
   void insertCacheLineInHierarchy(IntPtr address, CacheState::Type cstate, bool locked, Byte* fill_buf);

   // Process mem op in L2 cache since data is ready
   void processMemOpL2CacheReady(MemComponent::Type mem_component,
                                 Core::mem_op_t mem_op_type,
                                 Core::lock_signal_t lock_signal,
                                 IntPtr address, Byte* data_buf,
                                 UInt32 offset, UInt32 data_length,
                                 HybridL2CacheLineInfo& l2_cache_line_info);

   // Process Request from L1 Cache
   // Check if msg from L1 ends in the L2 cache
   pair<bool,Cache::MissType> getMemOpStatusInL2Cache(MemComponent::Type mem_component, IntPtr address,
                                                      Core::mem_op_t mem_op_type, Core::lock_signal_t lock_signal,
                                                      HybridL2CacheLineInfo& l2_cache_line_info);

   // Process Reply from Dram Directory
   void processReadReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply);
   void processWriteReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply);
   
   void processExReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply);
   void processShReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply);
   void processUpgradeReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply);
   void processReadyReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply);

   // ASYNC replies from Dram Directory
   void processAsyncExReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply);
   void processAsyncShReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply);
   void processAsyncUpgradeReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply);

   // Process Request from Dram Directory
   void processRemoteReadReqFromDramDirectory(tile_id_t sender, ShmemMsg* read_req);
   void processRemoteWriteReqFromDramDirectory(tile_id_t sender, ShmemMsg* read_req);

   void processInvReqFromDramDirectory(tile_id_t sender, ShmemMsg* inv_req);
   void processFlushReqFromDramDirectory(tile_id_t sender, ShmemMsg* flush_req);
   void processWbReqFromDramDirectory(tile_id_t sender, ShmemMsg* wb_req);
   void processInvFlushCombinedReqFromDramDirectory(tile_id_t sender, ShmemMsg* inv_flush_req);

   // Get cache line utilization in cache hierarchy
   UInt32 getLineUtilizationInCacheHierarchy(IntPtr address, HybridL2CacheLineInfo& l2_cache_line_info);

   // Read/write data from/to core buffer
   void readDataFromCoreBuffer(ShmemMsg* directory_reply);
   void writeDataToCoreBuffer(ShmemMsg* directory_reply);
  
   bool acquireLock(ShmemMsg::Type msg_type, IntPtr address);
   void releaseLock(ShmemMsg::Type msg_type, IntPtr address, bool locked);

   // Get Shmem Msg Type
   ShmemMsg::Type getDirectoryReqType(Core::mem_op_t mem_op_type, Core::lock_signal_t lock_signal);

   // Dram Directory Home Lookup
   tile_id_t getDramDirectoryHome(IntPtr address)
   { return _dram_directory_home_lookup->getHome(address); }

   // Utilities
   tile_id_t getTileID();
   UInt32 getCacheLineSize();
   MemoryManager* getMemoryManager()   { return _memory_manager; }
   ShmemPerfModel* getShmemPerfModel();
};

}
