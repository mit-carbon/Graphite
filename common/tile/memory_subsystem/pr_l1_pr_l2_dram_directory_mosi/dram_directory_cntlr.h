#pragma once

#include <string>
using namespace std;

// Forward Decls
namespace PrL1PrL2DramDirectoryMOSI
{
   class MemoryManager;
}

#include "directory_cache.h"
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
   public:
      DramDirectoryCntlr(MemoryManager* memory_manager,
            DramCntlr* dram_cntlr,
            UInt32 dram_directory_total_entries,
            UInt32 dram_directory_associativity,
            UInt32 cache_block_size,
            UInt32 dram_directory_max_num_sharers,
            UInt32 dram_directory_max_hw_sharers,
            std::string dram_directory_type_str,
            UInt32 num_dram_cntlrs,
            UInt64 dram_directory_cache_access_delay_in_ns);
      ~DramDirectoryCntlr();

      void handleMsgFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);

      DirectoryCache* getDramDirectoryCache() { return _dram_directory_cache; }
     
      void enable() { _enabled = true; }
      void disable() { _enabled = false; }
      void reset() { initializePerformanceCounters(); }

      void outputSummary(ostream& out);
      static void dummyOutputSummary(ostream& out);
   
   private:
      class DataList
      {
      private:
         UInt32 _block_size;
         std::map<IntPtr, Byte*> _data_list;

      public:
         DataList(UInt32 block_size);
         ~DataList();
         
         void insert(IntPtr address, Byte* data);
         Byte* lookup(IntPtr address);
         void erase(IntPtr address);
      };

      // Functional Models
      MemoryManager* _memory_manager;
      DirectoryCache* _dram_directory_cache;
      DramCntlr* _dram_cntlr;

      // Type of directory - (full_map, limited_broadcast, limited_no_broadcast, ackwise, limitless)
      Directory::DirectoryType _directory_type;

      ReqQueueList* _dram_directory_req_queue_list;
      DataList* _cached_data_list;

      bool _enabled;

      // Performance Counters
      UInt64 _num_exreq;
      UInt64 _num_shreq;
      UInt64 _num_nullifyreq;

      UInt64 _num_exreq_with_upgrade_rep;
      UInt64 _num_exreq_encountering_exclusive_owners;
      UInt64 _num_exreq_with_data_onchip;
      UInt64 _num_shreq_with_data_onchip;

      UInt64 _num_exreq_generating_invreq;
      UInt64 _num_exreq_generating_broadcast_invreq;
      UInt64 _num_nullifyreq_generating_invreq;
      UInt64 _num_nullifyreq_generating_broadcast_invreq;
      UInt64 _num_nullifyreq_with_uncached_directory_entry;

      UInt32 getCacheLineSize();
      MemoryManager* getMemoryManager() { return _memory_manager; }
      ShmemPerfModel* getShmemPerfModel();

      // Private Functions
      DirectoryEntry* processDirectoryEntryAllocationReq(ShmemReq* shmem_req);
      void processNullifyReq(ShmemReq* shmem_req, bool first_call = false);

      void processNextReqFromL2Cache(IntPtr address);
      void processExReqFromL2Cache(ShmemReq* shmem_req, bool first_call = false);
      void processShReqFromL2Cache(ShmemReq* shmem_req, bool first_call = false);
      void retrieveDataAndSendToL2Cache(ShmemMsg::msg_t reply_msg_type, tile_id_t receiver, IntPtr address, bool msg_modeled);

      void processInvRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);
      void processFlushRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);
      void processWbRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);
      void sendDataToDram(IntPtr address, Byte* data_buf, bool msg_modeled);
   
      void sendShmemMsg(ShmemMsg::msg_t requester_msg_type, ShmemMsg::msg_t send_msg_type, IntPtr address, tile_id_t requester, tile_id_t single_receiver, bool all_tiles_sharers, vector<tile_id_t>& sharers_list, bool msg_modeled);
      void restartShmemReq(tile_id_t sender, ShmemReq* shmem_req, DirectoryState::dstate_t curr_dstate);

      // Update Performance Counters
      void initializePerformanceCounters();
      void updateShmemReqPerfCounters(ShmemMsg::msg_t shmem_msg_type, DirectoryState::dstate_t dstate, tile_id_t requester,
            tile_id_t sharer, UInt32 num_sharers);
      void updateBroadcastPerfCounters(ShmemMsg::msg_t shmem_msg_type, bool inv_req_sent, bool broadcast_inv_req_sent);

      // Add/Remove Sharer
      bool addSharer(DirectoryEntry* directory_entry, tile_id_t sharer_id);
      void removeSharer(DirectoryEntry* directory_entry, tile_id_t sharer_id, bool reply_expected);
   };
}
