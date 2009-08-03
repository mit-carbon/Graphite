#ifndef DRAM_DIRECTORY_H
#define DRAM_DIRECTORY_H

#include "shmem_req_types.h"
#include "memory_manager.h"
#include "network.h"
#include "dram_directory_entry.h"
#include "fixed_types.h"

#include "dram_directory_perf_model_base.h"
#include "dram_perf_model.h"
#include "shmem_perf_model.h"

#include <map>
#include <queue>
#include <cassert>

enum shmem_req_t;


class SingleDramRequest
{
   private:

      IntPtr m_address;
      shmem_req_t m_shmem_req_type;
      UInt32 m_requestor;
      UInt64 m_time;

   public:
      SingleDramRequest() : 
         m_address(0), 
         m_shmem_req_type(READ), 
         m_requestor(0),
         m_time(0)
      {}

      SingleDramRequest(IntPtr address, shmem_req_t shmem_req_type, UInt32 requestor, UInt64 time) :
         m_address(address),
         m_shmem_req_type(shmem_req_type),
         m_requestor(requestor),
         m_time(time)
      {}

      ~SingleDramRequest() {}

      IntPtr getAddress() { return m_address; }
      shmem_req_t getShmemReqType() { return m_shmem_req_type; }
      UInt32 getRequestor() { return m_requestor; }
      UInt64 getTime() { return m_time; }

      void setTime(UInt64 time) { m_time = time; }
};

class DramRequestsForSingleAddress
{
   private:

      DramDirectoryEntry::dstate_t m_old_dstate;
      UInt32 m_num_acks_to_recv;

      std::queue<SingleDramRequest*> m_requests;

   public:

      void setCurrRequestAttributes(
         DramDirectoryEntry::dstate_t old_dstate,
         UInt32 num_acks_to_recv
      )
      {
         m_old_dstate = old_dstate;
         m_num_acks_to_recv = num_acks_to_recv;
      }

      void decNumAcksToRecv() { m_num_acks_to_recv --; }

      DramDirectoryEntry::dstate_t getOldDState() { return (m_old_dstate); }
      UInt32 getNumAcksToRecv()  { return (m_num_acks_to_recv); }

      void enqueueRequest(SingleDramRequest* single_dram_req)
      {
         m_requests.push(single_dram_req);
      }
      SingleDramRequest* dequeueRequest()
      {
         assert(m_requests.size() > 0);
         SingleDramRequest* single_dram_req = m_requests.front();
         m_requests.pop();
         return (single_dram_req);
      }
      SingleDramRequest* getRequest()
      {
         assert(m_requests.size() > 0);
         return m_requests.front();
      }
      SInt32 numRequests()
      {
         return (m_requests.size());
      }

};


/* Actual Directory Starts Here */

class DramDirectory
{
   private:
      Network* m_network;
      
      SInt32 m_core_id;
      UInt32 m_total_cores;
      UInt32 m_cache_line_size;
      UInt32 m_limitless_hw_sharer_count;
      UInt32 m_limitless_software_trap_penalty;
      
      std::map<IntPtr, DramDirectoryEntry*> dram_directory_entries;
      //state for re-entering the Directory
      std::map<IntPtr, DramRequestsForSingleAddress*> dram_request_list;

      DramDirectoryPerfModelBase* m_dram_directory_perf_model;
      DramPerfModel* m_dram_perf_model;
      ShmemPerfModel* m_shmem_perf_model;
     
      // Private Functions
      DramDirectoryEntry* getEntry(IntPtr address);
      
      void runDramPerfModel(void);
      void putDataToDram(DramDirectoryEntry* dram_dir_entry, Byte* data_buffer);
      void getDataFromDram(DramDirectoryEntry* dram_dir_entry, Byte* data_buffer);

      // Send Invalidations and Downgrade messages
      void startDemoteOwner(DramDirectoryEntry* dram_dir_entry, CacheState::cstate_t new_cstate);
      void startInvalidateAllSharers(DramDirectoryEntry* dram_dir_entry);
      void startInvalidateSingleSharer(DramDirectoryEntry* dram_dir_entry, UInt32 sharer_id);

      // Process Acknowledgements
      void processDemoteOwnerAck(UInt32 sender, DramDirectoryEntry* dram_dir_entry, MemoryManager::AckPayload* ack_payload_v, Byte *data_buffer, CacheState::cstate_t new_cstate);
      void processInvalidateSharerAck(UInt32 sender, DramDirectoryEntry* dram_dir_entry, MemoryManager::AckPayload* ack_payload_v);

      // Send Data to Requester
      void sendDataLine(DramDirectoryEntry* dram_dir_entry, UInt32 requestor, CacheState::cstate_t new_cstate);

      // Processing Shared Memory Requests
      void processSharedMemRequest(UInt32 requestor, shmem_req_t shmem_req_type, IntPtr address, DramRequestsForSingleAddress* dram_reqs);
      void finishSharedMemRequest(DramRequestsForSingleAddress* dram_reqs);

      // Packet Constructor
      NetPacket makePacket(PacketType packet_type, Byte* payload_buffer, UInt32 payload_size, SInt32 sender_rank, SInt32 receiver_rank);

   public:

      DramDirectory(SInt32 core_id, Network* network, ShmemPerfModel* shmem_perf_model, DramPerfModel* dram_perf_model);
      virtual ~DramDirectory();

      // Receive and process request for memory_block
      void startSharedMemRequest(NetPacket& req_packet);
      void processAck(NetPacket& ack_packet);
      void processWriteBack(NetPacket& req_packet);

      DramPerfModel* getDramPerformanceModel()
      {
         return m_dram_perf_model;
      }
};


#endif
