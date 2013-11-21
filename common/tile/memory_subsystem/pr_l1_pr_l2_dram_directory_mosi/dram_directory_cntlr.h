#pragma once

#include <map>
#include <string>
using std::map;
using std::string;

// Forward Decls
namespace PrL1PrL2DramDirectoryMOSI
{
   class MemoryManager;
}

#include "directory_cache.h"
#include "hash_map_list.h"
#include "dram_cntlr.h"
#include "address_home_lookup.h"
#include "shmem_req.h"
#include "shmem_msg.h"
#include "mem_component.h"
#include "time_types.h"

namespace PrL1PrL2DramDirectoryMOSI
{
   class DramDirectoryCntlr
   {
   public:
      DramDirectoryCntlr(MemoryManager* memory_manager,
                         DramCntlr* dram_cntlr,
                         string dram_directory_total_entries_str,
                         UInt32 dram_directory_associativity,
                         UInt32 cache_block_size,
                         UInt32 dram_directory_max_num_sharers,
                         UInt32 dram_directory_max_hw_sharers,
                         string dram_directory_type_str,
                         UInt32 num_dram_cntlrs,
                         string dram_directory_access_cycles_str);

      ~DramDirectoryCntlr();

      void handleMsgFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);

      DirectoryCache* getDramDirectoryCache() { return _dram_directory_cache; }
     
      void outputSummary(ostream& out);
      static void dummyOutputSummary(ostream& out);
   
      void enable() { _enabled = true; }
      void disable() { _enabled = false; }

   private:
      class DataList
      {
      public:
         DataList();
         ~DataList();
         
         void insert(IntPtr address, Byte* data, UInt32 size);
         Byte* lookup(IntPtr address);
         void erase(IntPtr address);
      
      private:
         map<IntPtr, Byte*> _data_list;
      };

      // Functional Models
      MemoryManager* _memory_manager;
      DirectoryCache* _dram_directory_cache;
      DramCntlr* _dram_cntlr;

      // Type of directory - (full_map, limited_broadcast, limited_no_broadcast, ackwise, limitless)
      DirectoryType _directory_type;

      HashMapList<IntPtr,ShmemReq*> _dram_directory_req_queue;
      DataList _cached_data_list;

      bool _enabled;

      // Event Counters
      UInt64 _total_exreq;
      UInt64 _total_exreq_in_modified_state;
      UInt64 _total_exreq_in_shared_state;
      UInt64 _total_exreq_with_upgrade_replies;
      UInt64 _total_exreq_in_uncached_state;
      Time _total_exreq_serialization_time;
      Time _total_exreq_processing_time;

      UInt64 _total_shreq;
      UInt64 _total_shreq_in_modified_state;
      UInt64 _total_shreq_in_shared_state;
      UInt64 _total_shreq_in_uncached_state;
      Time _total_shreq_serialization_time;
      Time _total_shreq_processing_time;

      UInt64 _total_nullifyreq;
      UInt64 _total_nullifyreq_in_modified_state;
      UInt64 _total_nullifyreq_in_shared_state;
      UInt64 _total_nullifyreq_in_uncached_state;
      Time _total_nullifyreq_serialization_time;
      Time _total_nullifyreq_processing_time;

      UInt64 _total_invalidations_unicast_mode;
      UInt64 _total_sharers_invalidated_unicast_mode;
      Time _total_invalidation_processing_time_unicast_mode;

      UInt64 _total_invalidations_broadcast_mode;
      UInt64 _total_sharers_invalidated_broadcast_mode;
      Time _total_invalidation_processing_time_broadcast_mode;

      UInt32 getCacheLineSize();
      ShmemPerfModel* getShmemPerfModel();

      // Private Functions
      DirectoryEntry* processDirectoryEntryAllocationReq(ShmemReq* shmem_req);
      void processNullifyReq(ShmemReq* shmem_req, DirectoryEntry* directory_entry, bool first_call = false);

      void processNextReqFromL2Cache(IntPtr address);
      void processExReqFromL2Cache(ShmemReq* shmem_req, DirectoryEntry* directory_entry, bool first_call = false);
      void processShReqFromL2Cache(ShmemReq* shmem_req, DirectoryEntry* directory_entry, bool first_call = false);
      void retrieveDataAndSendToL2Cache(ShmemMsg::Type reply_msg_type, tile_id_t receiver, IntPtr address, bool msg_modeled);

      void processInvRepFromL2Cache(tile_id_t sender, const ShmemMsg* shmem_msg);
      void processFlushRepFromL2Cache(tile_id_t sender, const ShmemMsg* shmem_msg);
      void processWbRepFromL2Cache(tile_id_t sender, const ShmemMsg* shmem_msg);
      void sendDataToDram(IntPtr address, Byte* data_buf, bool msg_modeled);
   
      void sendShmemMsg(ShmemMsg::Type requester_msg_type, ShmemMsg::Type send_msg_type, IntPtr address,
                        tile_id_t requester, tile_id_t single_receiver, bool all_tiles_sharers, vector<tile_id_t>& sharers_list,
                        bool msg_modeled);
      void restartShmemReq(tile_id_t sender, ShmemReq* shmem_req, DirectoryEntry* directory_entry);

      // Update Performance Counters
      void initializeEventCounters();
      void updateShmemReqEventCounters(ShmemReq* shmem_req, DirectoryEntry* directory_entry);
      void updateInvalidationEventCounters(bool in_broadcast_mode, SInt32 num_sharers);
      void updateShmemReqLatencyCounters(const ShmemReq* shmem_req);
      void updateInvalidationLatencyCounters(bool initial_broadcast_mode, Time shmem_req_latency);

      // Add/Remove Sharer
      bool addSharer(DirectoryEntry* directory_entry, tile_id_t sharer_id);
      void removeSharer(DirectoryEntry* directory_entry, tile_id_t sharer_id, bool reply_expected);
   };
}
