#pragma once

#include <string>
using std::string;

// Forward declaration
namespace HybridProtocol_PPMOSI_SS
{
   class L2CacheCntlr;
   class MemoryManager;
}

#include "tile.h"
#include "cache.h"
#include "cache_line_info.h"
#include "shmem_msg.h"
#include "mem_component.h"
#include "fixed_types.h"
#include "shmem_perf_model.h"
#include "cache_replacement_policy.h"
#include "cache_hash_fn.h"

namespace HybridProtocol_PPMOSI_SS
{

class L1CacheCntlr
{
public:
   L1CacheCntlr(MemoryManager* memory_manager,
                UInt32 cache_line_size,
                UInt32 l1_icache_size,
                UInt32 l1_icache_associativity,
                string l1_icache_replacement_policy,
                UInt32 l1_icache_access_delay,
                bool l1_icache_track_miss_types,
                UInt32 l1_dcache_size,
                UInt32 l1_dcache_associativity,
                string l1_dcache_replacement_policy,
                UInt32 l1_dcache_access_delay,
                bool l1_dcache_track_miss_types,
                float frequency);
   ~L1CacheCntlr();

   Cache* getL1ICache() { return _l1_icache; }
   Cache* getL1DCache() { return _l1_dcache; }

   void setL2CacheCntlr(L2CacheCntlr* l2_cache_cntlr);

   bool processMemOpFromCore(MemComponent::Type mem_component,
                             Core::mem_op_t mem_op_type, 
                             Core::lock_signal_t lock_signal,
                             IntPtr address, Byte* data_buf,
                             UInt32 offset, UInt32 data_length,
                             bool modeled);

   CacheState::Type getCacheLineState(MemComponent::Type mem_component, IntPtr address);
   void setCacheLineState(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate);
   void insertCacheLine(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate, Byte* fill_buf,
                        bool& eviction, IntPtr& evicted_address, UInt32& evicted_line_utilization);
   void invalidateCacheLine(MemComponent::Type mem_component, IntPtr address);

   // From L2 cache cntlr
   void processRemoteReadReq(MemComponent::Type mem_component, ShmemMsg* remote_req);
   void processRemoteWriteReq(MemComponent::Type mem_component, ShmemMsg* remote_req);
   void processMemOpL1CacheReady(MemComponent::Type mem_component,
                                 Core::mem_op_t mem_op_type, Core::lock_signal_t lock_signal,
                                 IntPtr address, Byte* data_buf, UInt32 offset, UInt32 data_length);
   // Get cache line
   UInt32 getCacheLineUtilization(MemComponent::Type mem_component, IntPtr address);

private:
   MemoryManager* _memory_manager;
   Cache* _l1_icache;
   Cache* _l1_dcache;
   CacheReplacementPolicy* _l1_icache_replacement_policy_obj;
   CacheReplacementPolicy* _l1_dcache_replacement_policy_obj;
   CacheHashFn* _l1_icache_hash_fn_obj;
   CacheHashFn* _l1_dcache_hash_fn_obj;
   L2CacheCntlr* _l2_cache_cntlr;

   void accessCache(MemComponent::Type mem_component,
                    Core::lock_signal_t lock_signal, Core::mem_op_t mem_op_type, 
                    IntPtr address, UInt32 offset,
                    Byte* data_buf, UInt32 data_length,
                    HybridL1CacheLineInfo& l1_cache_line_info);
   pair<bool,Cache::MissType> getMemOpStatusInL1Cache(MemComponent::Type mem_component, IntPtr address,
                                                      Core::mem_op_t mem_op_type, Core::lock_signal_t lock_signal,
                                                      HybridL1CacheLineInfo& l1_cache_line_info);

   // Assert that L1 cache is ready to handle request from core
   void assertL1CacheReady(Core::mem_op_t mem_op_type, CacheState::Type cstate);
   
   Cache* getL1Cache(MemComponent::Type mem_component);
   void writeCacheLine(Cache* l1_cache, IntPtr address, Byte* data_buf, UInt32 offset, UInt32 data_length);

   // Utilities
   tile_id_t getTileID();
   UInt32 getCacheLineSize();
   MemoryManager* getMemoryManager()   { return _memory_manager; }
   ShmemPerfModel* getShmemPerfModel();
};

}
