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

class DramRequestsForSingleAddress
{

   private:

      SingleDramRequest* single_dram_req;
      DramDirectoryEntry::dstate_t old_dstate;
      UInt32 num_acks_to_recv;

      std::queue<SingleDramRequest*> waiting_requests;

   public:

      void setCurrRequestAttributes(
         SingleDramRequest* single_dram_req,
         DramDirectoryEntry::dstate_t old_dstate,
         UInt32 num_acks_to_recv
      )
      {
         this->single_dram_req = single_dram_req;
         this->old_dstate = old_dstate;
         this->num_acks_to_recv = num_acks_to_recv;
      }

      void deleteCurrRequest()
      {
         delete this->single_dram_req;
      }

      void decNumAcksToRecv()
      {
         this->num_acks_to_recv --;
      }

      shmem_req_t getShmemReqType()
      {
         return (this->single_dram_req->shmem_req_type);
      }

      UInt32 getRequestor()
      {
         return (this->single_dram_req->requestor);
      }

      DramDirectoryEntry::dstate_t getOldDState()
      {
         return (this->old_dstate);
      }

      UInt32 getNumAcksToRecv()
      {
         return (this->num_acks_to_recv);
      }

      SingleDramRequest* getNextRequest()
      {
         assert(this->waiting_requests.size() != 0);
         SingleDramRequest* single_dram_req = this->waiting_requests.front();
         this->waiting_requests.pop();
         return (single_dram_req);
      }


      SInt32 numWaitingRequests()
      {
         return (this->waiting_requests.size());
      }

      void addRequestToQueue(SingleDramRequest* single_dram_req)
      {
         this->waiting_requests.push(single_dram_req);
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
      std::map<IntPtr, DramDirectoryEntry*> dram_directory_entries;
      UInt32 dram_id;

      //state for re-entering the Directory
      std::map<IntPtr, DramRequestsForSingleAddress*> dram_request_list;

      UInt32 m_knob_dram_access_cost;
      UInt32 m_knob_dir_max_sharers;

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

      void processDemoteOwnerAck(UInt32 sender, DramDirectoryEntry* dram_dir_entry, void* /*MemoryManager::AckPayload&*/ ack_payload_v, char *data_buffer, CacheState::cstate_t new_cstate);

      void processInvalidateSharerAck(UInt32 sender, DramDirectoryEntry* dram_dir_entry, void* /*MemoryManager::AckPayload&*/ ack_payload_v);

      //make many of these private functions
      void copyDataToDram(IntPtr address, char* data_buffer);
      //sending another memory line to another core. rename.
      void sendDataLine(DramDirectoryEntry* dram_dir_entry, UInt32 requestor, CacheState::cstate_t new_cstate);

      void processWriteBack(NetPacket& req_packet);

      void setNumberOfLines(UInt32 number_of_lines) { num_lines = number_of_lines; }

      /***************************************/

      void debugSetDramState(IntPtr addr, DramDirectoryEntry::dstate_t dstate, vector<UInt32> sharers_list, char *d_data);
      bool debugAssertDramState(IntPtr addr, DramDirectoryEntry::dstate_t dstate, vector<UInt32> sharers_list, char *d_data);

      /***************************************/

      //for debug purposes
      void print();
};


#endif
