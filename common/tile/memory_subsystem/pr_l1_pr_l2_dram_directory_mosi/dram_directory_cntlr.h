#pragma once

#include <string>
using namespace std;

// Forward Decls
namespace PrL1PrL2DramDirectoryMOSI
{
   class MemoryManager;
}

#include "directory_cache.h"
#include "hash_map_queue.h"
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
      DirectoryType _directory_type;

      HashMapQueue<IntPtr,ShmemReq*>* _dram_directory_req_queue_list;
      DataList* _cached_data_list;

      bool _enabled;

      // Event Counters
      UInt64 _total_exreq;
      UInt64 _total_exreq_in_modified_state;
      UInt64 _total_exreq_in_shared_state;
      UInt64 _total_exreq_with_upgrade_replies;
      UInt64 _total_exreq_in_uncached_state;
#ifdef TRACK_DETAILED_CACHE_COUNTERS
      UInt64 _total_exreq_in_modified_state_with_flushrep[MAX_TRACKED_UTILIZATION+1];
      UInt64 _total_exreq_in_shared_state_with_invrep[MAX_TRACKED_UTILIZATION+1];
      UInt64 _total_exreq_in_shared_state_with_flushrep[MAX_TRACKED_UTILIZATION+1];
#endif
      UInt64 _total_exreq_serialization_time;
      UInt64 _total_exreq_processing_time;

      UInt64 _total_shreq;
      UInt64 _total_shreq_in_modified_state;
      UInt64 _total_shreq_in_shared_state;
      UInt64 _total_shreq_in_uncached_state;
#ifdef TRACK_DETAILED_CACHE_COUNTERS
      UInt64 _total_shreq_in_modified_state_with_wbrep[MAX_TRACKED_UTILIZATION+1];
      UInt64 _total_shreq_in_shared_state_with_wbrep[MAX_TRACKED_UTILIZATION+1];
#endif
      UInt64 _total_shreq_serialization_time;
      UInt64 _total_shreq_processing_time;

      UInt64 _total_nullifyreq;
      UInt64 _total_nullifyreq_in_modified_state;
      UInt64 _total_nullifyreq_in_shared_state;
      UInt64 _total_nullifyreq_in_uncached_state;
#ifdef TRACK_DETAILED_CACHE_COUNTERS
      UInt64 _total_nullifyreq_in_modified_state_with_flushrep[MAX_TRACKED_UTILIZATION+1];
      UInt64 _total_nullifyreq_in_shared_state_with_invrep[MAX_TRACKED_UTILIZATION+1];
      UInt64 _total_nullifyreq_in_shared_state_with_flushrep[MAX_TRACKED_UTILIZATION+1];
#endif
      UInt64 _total_nullifyreq_serialization_time;
      UInt64 _total_nullifyreq_processing_time;

      UInt64 _total_invalidations_unicast_mode;
      UInt64 _total_sharers_invalidated_unicast_mode;
      UInt64 _total_invalidation_processing_time_unicast_mode;

      UInt64 _total_invalidations_broadcast_mode;
      UInt64 _total_sharers_invalidated_broadcast_mode;
      UInt64 _total_invalidation_processing_time_broadcast_mode;

#ifdef TRACK_DETAILED_CACHE_COUNTERS
      // Computing sharer count vs private copy threshold
      UInt32 _max_sharers_by_PCT[MAX_PRIVATE_COPY_THRESHOLD+1];
      UInt64 _total_sharers_invalidated_by_utilization[MAX_TRACKED_UTILIZATION+1];
      UInt64 _total_invalidations;
#endif

      UInt32 getCacheLineSize();
      MemoryManager* getMemoryManager() { return _memory_manager; }
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
      void updateInvalidationLatencyCounters(bool initial_broadcast_mode, UInt64 shmem_req_latency);

      // Add/Remove Sharer
      bool addSharer(DirectoryEntry* directory_entry, tile_id_t sharer_id);
      void removeSharer(DirectoryEntry* directory_entry, tile_id_t sharer_id, bool reply_expected);

#ifdef TRACK_DETAILED_CACHE_COUNTERS
      // Cache Line Utilization
      void updateCacheLineUtilizationCounters(const ShmemReq* dir_request, DirectoryEntry* directory_entry,
                                              tile_id_t sender, const ShmemMsg* shmem_msg);

      // Output summary
      void initializeSharerCounters();
      void updateSharerCounters(const ShmemReq* dir_request, DirectoryEntry* directory_entry,
                                tile_id_t sender, UInt64 cache_line_utilization);
      void outputSharerCountSummary(ostream& out);
      static void dummyOutputSharerCountSummary(ostream& out);
#endif
   };
}
