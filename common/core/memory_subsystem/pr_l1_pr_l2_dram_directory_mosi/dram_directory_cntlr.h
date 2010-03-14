#pragma once

#include <string>
using namespace std;

// Forward Decls
namespace PrL1PrL2DramDirectoryMOSI
{
   class MemoryManager;
}

#include "dram_directory_cache.h"
#include "req_queue_list.h"
#include "dram_cntlr.h"
#include "address_home_lookup.h"
#include "shmem_req.h"
#include "shmem_msg.h"
#include "mem_component.h"

namespace PrL1PrL2DramDirectoryMOSI
{
   class DramDirectoryCntlr
   {
      private:
         class DataList
         {
            private:
               UInt32 m_block_size;
               std::map<IntPtr, Byte*> m_data_list;

            public:
               DataList(UInt32 block_size);
               ~DataList();
               
               void insert(IntPtr address, Byte* data);
               Byte* lookup(IntPtr address);
               void erase(IntPtr address);
         };

         // Functional Models
         MemoryManager* m_memory_manager;
         
         DramDirectoryCache* m_dram_directory_cache;
         ReqQueueList* m_dram_directory_req_queue_list;
         DataList* m_cached_data_list;

         DramCntlr* m_dram_cntlr;

         core_id_t m_core_id;
         UInt32 m_cache_block_size;

         ShmemPerfModel* m_shmem_perf_model;

         // Performance Counters
         UInt64 m_num_exreq;
         UInt64 m_num_shreq;
         UInt64 m_num_nullifyreq;

         UInt64 m_num_exreq_with_upgrade_rep;
         UInt64 m_num_exreq_with_data_onchip;
         UInt64 m_num_shreq_with_data_onchip;

         UInt64 m_num_exreq_generating_invreq;
         UInt64 m_num_exreq_generating_broadcast_invreq;
         UInt64 m_num_nullifyreq_generating_invreq;
         UInt64 m_num_nullifyreq_generating_broadcast_invreq;

         UInt32 getCacheBlockSize() { return m_cache_block_size; }
         MemoryManager* getMemoryManager() { return m_memory_manager; }
         ShmemPerfModel* getShmemPerfModel() { return m_shmem_perf_model; }

         // Private Functions
         DirectoryEntry* processDirectoryEntryAllocationReq(ShmemReq* shmem_req);
         void processNullifyReq(ShmemReq* shmem_req, bool first_call = false);

         void processNextReqFromL2Cache(IntPtr address);
         void processExReqFromL2Cache(ShmemReq* shmem_req, bool first_call = false);
         void processShReqFromL2Cache(ShmemReq* shmem_req, bool first_call = false);
         void retrieveDataAndSendToL2Cache(ShmemMsg::msg_t reply_msg_type, core_id_t receiver, IntPtr address);

         void processInvRepFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg);
         void processFlushRepFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg);
         void processWbRepFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg);
         void sendDataToDram(IntPtr address, core_id_t requester, Byte* data_buf);
      
         void sendShmemMsg(ShmemMsg::msg_t requester_msg_type, ShmemMsg::msg_t send_msg_type, IntPtr address, core_id_t requester, core_id_t single_receiver, pair<bool, vector<core_id_t> >& sharers_list_pair);
         void restartShmemReq(core_id_t sender, ShmemReq* shmem_req, DirectoryState::dstate_t curr_dstate);

         // Update Performance Counters
         void initializePerfCounters(void);
         void updateShmemReqPerfCounters(ShmemMsg::msg_t shmem_msg_type, DirectoryState::dstate_t dstate, core_id_t requester, core_id_t sharer);
         void updateBroadcastPerfCounters(ShmemMsg::msg_t shmem_msg_type, bool inv_req_sent, bool broadcast_inv_req_sent);

      public:
         DramDirectoryCntlr(core_id_t core_id,
               MemoryManager* memory_manager,
               DramCntlr* dram_cntlr,
               UInt32 dram_directory_total_entries,
               UInt32 dram_directory_associativity,
               UInt32 cache_block_size,
               UInt32 dram_directory_max_num_sharers,
               UInt32 dram_directory_max_hw_sharers,
               std::string dram_directory_type_str,
               UInt32 num_dram_cntlrs,
               UInt32 dram_directory_cache_access_time,
               ShmemPerfModel* shmem_perf_model);
         ~DramDirectoryCntlr();

         void handleMsgFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg);

         DramDirectoryCache* getDramDirectoryCache() { return m_dram_directory_cache; }
         
         void outputSummary(ostream& out);
         static void dummyOutputSummary(ostream& out);
   };

}
