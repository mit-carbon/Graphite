#pragma once

#include <string>
using namespace std;

// Forward Decls
namespace HybridProtocol_PPMOSI_SS
{
   class MemoryManager;
}

#include "directory_cache.h"
#include "dram_cntlr.h"
#include "address_home_lookup.h"
#include "buffered_req.h"
#include "shmem_msg.h"
#include "mem_component.h"
#include "hash_map_queue.h"
#include "adaptive_directory_entry.h"

namespace HybridProtocol_PPMOSI_SS
{

class DramDirectoryCntlr
{
public:
   DramDirectoryCntlr(MemoryManager* memory_manager,
                      AddressHomeLookup* dram_home_lookup,
                      UInt32 directory_total_entries,
                      UInt32 directory_associativity,
                      UInt32 cache_line_size,
                      UInt32 directory_max_num_sharers,
                      UInt32 directory_max_hw_sharers,
                      string dram_directory_type_str,
                      UInt32 num_directory_slices,
                      UInt64 directory_access_delay);
   ~DramDirectoryCntlr();

   void handleMsgFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);
   void handleMsgFromDramCntlr(tile_id_t sender, ShmemMsg* dram_reply);

   DirectoryCache* getDramDirectory() { return _directory; }
  
   void enable() { _enabled = true; }
   void disable() { _enabled = false; }

   void outputSummary(ostream& out);
   static void dummyOutputSummary(ostream& out);

private:
   MemoryManager* _memory_manager;
   DirectoryCache* _directory;
   AddressHomeLookup* _dram_home_lookup;

   // Type of directory - (full_map, limited_broadcast, limited_no_broadcast, ackwise, limitless)
   DirectoryType _directory_type;

   HashMapQueue<IntPtr,BufferedReq*>* _buffered_req_queue_list;

   bool _enabled;

   // Utilities
   UInt32 getCacheLineSize();
   MemoryManager* getMemoryManager()      { return _memory_manager; }
   ShmemPerfModel* getShmemPerfModel();
   tile_id_t getTileID();

   // Process next req from L2 cache in the order in which they were received
   void processNextReqFromL2Cache(IntPtr address);
 
   // Allocate shared and exclusive cache lines in the L2 cache 
   bool allocateExclusiveCacheLine(IntPtr address, ShmemMsg::Type directory_req_type, tile_id_t cached_location, Mode::Type mode,
                                   DirectoryEntry* directory_entry, tile_id_t requester, bool modeled);
   bool allocateSharedCacheLine(IntPtr address, ShmemMsg::Type directory_req_type, tile_id_t cached_location, Mode::Type mode,
                                DirectoryEntry* directory_entry, tile_id_t requester, bool modeled);
   bool nullifyCacheLine(IntPtr address, DirectoryEntry* directory_entry, tile_id_t requester, bool modeled);
   
   // Req to allocate a directory entry if it is not present for a specific address
   // May involve invalidating all sharers corresponding to the directory entry that is evicted
   DirectoryEntry* processDirectoryEntryAllocationReq(BufferedReq* buffered_req);

   // Req to invalidate all the sharers of an address since its directory entry is getting evicted
   void processNullifyReq(BufferedReq* buffered_req);

   // Process a request to the directory from the L2 cache
   // The protocol type is unknown when the request is made, hence the prefix 'UNIFIED_'
   void processUnifiedReqFromL2Cache(BufferedReq* buffered_req);

   // Process the request to the directory
   void processDirectoryReq(BufferedReq* buffered_req, DirectoryEntry* directory_entry);

   // Process an L2 cache request for a cache line that is in PR_L1_PR_L2_MOSI directory mode
   void processPrivateReqFromL2Cache(BufferedReq* buffered_req, DirectoryEntry* directory_entry);
   
   // Process Ex/Sh Req from the L2 cache to a specific cache line
   void processExReqFromL2Cache(BufferedReq* buffered_req, DirectoryEntry* directory_entry);
   void processShReqFromL2Cache(BufferedReq* buffered_req, DirectoryEntry* directory_entry);
   
   // Process an L2 cache request for a cache line that is in REMOTE_ACCESS mode
   void processRemoteAccessReqFromL2Cache(BufferedReq* buffered_req, DirectoryEntry* directory_entry);
   void processRemoteAccessReplyFromL2Cache(tile_id_t sender, ShmemMsg* remote_reply);

   // Write unlock request
   void processWriteUnlockReqFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);

   // Process the reply to a remote access request from the L2 cache
   void processRemoteReplyFromL2Cache(tile_id_t sender, ShmemMsg* remote_reply);
   
   bool retrieveDataAndSendToL2Cache(IntPtr address, tile_id_t cached_location,
                                     ShmemMsg::Type reply_type, tile_id_t requester, bool modeled);

   // process Inv/Flush/WB replies from the L2 cache to the directory
   void processInvReplyFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);
   void processFlushReplyFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);
   void processWbReplyFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);

   // Reply from dram in response to a read request from the directory
   void processDramFetchReply(tile_id_t sender, ShmemMsg* dram_reply);

   // Transition to remote mode
   bool transitionToRemoteMode(BufferedReq* buffered_req, DirectoryEntry* directory_entry, Mode::Type mode);

   void sendShmemMsg(ShmemMsg::Type send_msg_type, IntPtr address,
                     tile_id_t single_receiver, bool all_tiles_sharers, vector<tile_id_t>& sharers_list,
                     tile_id_t requester, bool msg_modeled);

   // Send a message to the memory controller requesting to fetch/store data from/to dram
   void fetchDataFromDram(IntPtr address, tile_id_t requester, bool modeled);
   void storeDataInDram(IntPtr address, Byte* data_buf, tile_id_t requester, bool modeled);

   // Get the location of the memory controller corresponding to a specific address
   tile_id_t getDramHome(IntPtr address);

   // Restart the processing of a request after a reply is received from one of the sharers or the remote core
   void restartDirectoryReqIfReady(BufferedReq* buffered_req, DirectoryEntry* directory_entry,
                                   tile_id_t last_msg_sender, ShmemMsg* last_msg);
};

}
