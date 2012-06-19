#pragma once

#include <iostream>

#include "../memory_manager.h"
#include "cache.h"
#include "l1_cache_cntlr.h"
#include "l2_cache_cntlr.h"
#include "dram_directory_cntlr.h"
#include "dram_cntlr.h"
#include "address_home_lookup.h"
#include "shmem_msg.h"
#include "mem_component.h"
#include "semaphore.h"
#include "fixed_types.h"
#include "shmem_perf_model.h"
#include "network.h"

namespace HybridProtocol_PPMOSI_SS
{

class MemoryManager : public ::MemoryManager
{
public:
   MemoryManager(Tile* tile, Network* network, ShmemPerfModel* shmem_perf_model);
   ~MemoryManager();

   UInt32 getCacheLineSize() { return _cache_line_size; }

   Cache* getL1ICache() { return _l1_cache_cntlr->getL1ICache(); }
   Cache* getL1DCache() { return _l1_cache_cntlr->getL1DCache(); }
   Cache* getL2Cache() { return _l2_cache_cntlr->getL2Cache(); }
   DirectoryCache* getDramDirectoryCache() { return _dram_directory_cntlr->getDramDirectory(); }
   DramCntlr* getDramCntlr() { return _dram_cntlr; }
   bool isDramCntlrPresent() { return _dram_cntlr_present; }
   
   bool coreInitiateMemoryAccess(MemComponent::Type mem_component,
                                 Core::lock_signal_t lock_signal, Core::mem_op_t mem_op_type,
                                 IntPtr address, UInt32 offset,
                                 Byte* data_buf, UInt32 data_length,
                                 bool modeled);

   void handleMsgFromNetwork(NetPacket& packet);

   void sendMsg(tile_id_t receiver, ShmemMsg& shmem_msg);
   void broadcastMsg(ShmemMsg& shmem_msg);
 
   void enableModels();
   void disableModels();

   void outputSummary(std::ostream &os);
   
   UInt32 getModeledLength(const void* pkt_data)
   { return ((ShmemMsg*) pkt_data)->getModeledLength(); }
   bool isModeled(const void* pkt_data)
   { return  ((ShmemMsg*) pkt_data)->isModeled(); }
   tile_id_t getShmemRequester(const void* pkt_data)
   { return ((ShmemMsg*) pkt_data)->getRequester(); }

   // Performance Models
   void incrCycleCount(MemComponent::Type mem_component, CachePerfModel::CacheAccess_t access_type);
   
   // Synchronize between app and sim threads
   void wakeUpAppThread();
   void waitForAppThread();
   void wakeUpSimThread();
   void waitForSimThread();

private:
   L1CacheCntlr* _l1_cache_cntlr;
   L2CacheCntlr* _l2_cache_cntlr;
   DramDirectoryCntlr* _dram_directory_cntlr;
   DramCntlr* _dram_cntlr;
   bool _dram_cntlr_present;
   
   AddressHomeLookup* _dram_directory_home_lookup;
   AddressHomeLookup* _dram_home_lookup;

   UInt32 _cache_line_size;
   bool _enabled;

   // Synchronize between app and sim threads
   Semaphore _app_thread_sem;
   Semaphore _sim_thread_sem;

   // Performance Models
   CachePerfModel* _l1_icache_perf_model;
   CachePerfModel* _l1_dcache_perf_model;
   CachePerfModel* _l2_cache_perf_model;
};

}
