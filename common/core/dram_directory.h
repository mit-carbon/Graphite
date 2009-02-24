#ifndef DRAM_DIRECTORY_H
#define DRAM_DIRECTORY_H


#include "dram_directory_entry.h"
#include "network.h"
#include "memory_manager.h"
#include <map>
#include <queue>

enum shmem_req_t;


class SingleDramRequest
{

   public:

      IntPtr address;
      shmem_req_t shmem_req_type;
      UInt32 requestor;

      SingleDramRequest() : address(0), shmem_req_type(READ), requestor(0) {}

      SingleDramRequest(IntPtr address, shmem_req_t shmem_req_type, UInt32 requestor)
      {
         this->address = address;
         this->shmem_req_type = shmem_req_type;
         this->requestor = requestor;
      }
};

class DramRequest
{

   private:

      SingleDramRequest single_dram_req;
      DramDirectoryEntry::dstate_t old_dstate;
      UInt32 num_acks_to_recv;

      std::queue<SingleDramRequest> waiting_requests;

   public:

      void setRequestAttributes(
         SingleDramRequest& single_dram_req,
         DramDirectoryEntry::dstate_t old_dstate,
         UInt32 num_acks_to_recv
      )
      {
         this->single_dram_req.address = single_dram_req.address;
         this->single_dram_req.shmem_req_type = single_dram_req.shmem_req_type;
         this->single_dram_req.requestor = single_dram_req.requestor;
         this->old_dstate = old_dstate;
         this->num_acks_to_recv = num_acks_to_recv;
      }

      void decNumAcksToRecv()
      {
         num_acks_to_recv --;
      }

      shmem_req_t getShmemReqType()
      {
         return (single_dram_req.shmem_req_type);
      }

      UInt32 getRequestor()
      {
         return (single_dram_req.requestor);
      }

      DramDirectoryEntry::dstate_t getOldDState()
      {
         return (old_dstate);
      }

      UInt32 getNumAcksToRecv()
      {
         return (num_acks_to_recv);
      }

      SingleDramRequest getNextRequest()
      {
         assert(waiting_requests.size() != 0);
         SingleDramRequest single_dram_req = waiting_requests.front();
         waiting_requests.pop();
         return (single_dram_req);
      }

      SInt32 numWaitingRequests()
      {
         return (waiting_requests.size());
      }

      void addRequestToQueue(SingleDramRequest& single_dram_req)
      {
         waiting_requests.push(single_dram_req);
      }


};


/* Actual Directory Starts Here */

class DramDirectory
{
   private:
      //assumption: each dram_directory is tied to a given network (node), and is addressed at dram_id
      Network* m_network;
      UInt32 num_lines;
      UInt32 bytes_per_cache_line;
      UInt32 number_of_cores;
      //key dram entries on cache_line (assumes cache_line is 1:1 to dram memory lines)
      std::map<UInt32, DramDirectoryEntry*> dram_directory_entries;
      UInt32 dram_id;

      //state for re-entering the Directory
      std::map<IntPtr, DramRequest*> dram_request_list;

      /* Added by George */
      UInt64 dramAccessCost;

      //TODO debugAssertValidStates();
      //scan the directory occasionally for invalid state configurations.
      //can that be done here? or only in the MMU since we may need cache access.

   public:
      //is this a needed function?
      DramDirectoryEntry* getEntry(IntPtr address);

      DramDirectory(UInt32 num_lines, UInt32 bytes_per_cache_line, UInt32 dram_id_arg, UInt32 num_of_cores, Network* network_arg);
      virtual ~DramDirectory();

      /***************************************/

      //receive and process request for memory_block
      void startSharedMemRequest(NetPacket& req_packet);
      void finishSharedMemRequest(IntPtr address);
      void startNextSharedMemRequest(IntPtr address);

      void processSharedMemRequest(UInt32 requestor, shmem_req_t shmem_req_type, IntPtr address);

      void processAck(NetPacket& ack_packet);

      void startDemoteOwner(DramDirectoryEntry* dram_dir_entry, CacheState::cstate_t new_cstate);
      void startInvalidateAllSharers(DramDirectoryEntry* dram_dir_entry);
      void startInvalidateSingleSharer(DramDirectoryEntry* dram_dir_entry, UInt32 sharer_id);

      void processDemoteOwnerAck(UInt32 sender, DramDirectoryEntry* dram_dir_entry, void* /*MemoryManager::AckPayload&*/ ack_payload_v, char *data_buffer, DramDirectoryEntry::dstate_t new_dstate);

      void processInvalidateSharerAck(UInt32 sender, DramDirectoryEntry* dram_dir_entry, void* /*MemoryManager::AckPayload&*/ ack_payload_v, char *data_buffer);

      //make many of these private functions
      void copyDataToDram(IntPtr address, char* data_buffer);
      //sending another memory line to another core. rename.
      void sendDataLine(DramDirectoryEntry* dram_dir_entry, UInt32 requestor, CacheState::cstate_t new_cstate);

      void processWriteBack(NetPacket& req_packet);

      void setNumberOfLines(UInt32 number_of_lines) { num_lines = number_of_lines; }

      void runDramAccessModel();
      UInt64 getDramAccessCost();

      /***************************************/

      void debugSetDramState(IntPtr addr, DramDirectoryEntry::dstate_t dstate, vector<UInt32> sharers_list, char *d_data);
      bool debugAssertDramState(IntPtr addr, DramDirectoryEntry::dstate_t dstate, vector<UInt32> sharers_list, char *d_data);

      /***************************************/

      void setDramMemoryLine(IntPtr addr, char* data_buffer, UInt32 data_size);

      //for debug purposes
      void print();
};


#endif
