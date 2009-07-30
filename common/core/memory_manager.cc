#include <string.h>

#include "memory_manager.h"
#include "simulator.h"
#include "log.h"

using namespace std;

void MemoryManagerNetworkCallback(void *obj, NetPacket packet);

MemoryManager::MemoryManager(SInt32 core_id, Core *core, Network *network, Cache *dcache, ShmemPerfModel* shmem_perf_model)
{
   LOG_ASSERT_ERROR(!Config::getSingleton()->isSimulatingSharedMemory() || Config::getSingleton()->getEnableDCacheModeling(), "Must set dcache modeling on (-mdc) to use shared memory model.");

   m_core_id = core_id;
   m_core = core;
   m_network = network;
   m_dcache = dcache;

   assert(m_dcache != NULL);

   // Performance Models
   m_mmu_perf_model = MMUPerfModelBase::createModel(MMUPerfModelBase::MMU_PERF_MODEL);
   m_dram_perf_model = new DramPerfModel(m_core);
   m_shmem_perf_model = shmem_perf_model;
   
   try
   {
      m_cache_line_size = Sim()->getCfg()->getInt("l2_cache/line_size");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Could not read l2_cache/line_size from config");
   }

   m_dram_dir = new DramDirectory(m_core_id, m_network, m_shmem_perf_model, m_dram_perf_model);
   m_addr_home_lookup = new AddressHomeLookup(m_core_id);

   // Register Callbacks when a packet arrives
   m_network->registerCallback(SHARED_MEM_REQ, MemoryManagerNetworkCallback, this);
   m_network->registerCallback(SHARED_MEM_EVICT, MemoryManagerNetworkCallback, this);
   m_network->registerCallback(SHARED_MEM_UPDATE_UNEXPECTED, MemoryManagerNetworkCallback, this);
   m_network->registerCallback(SHARED_MEM_ACK, MemoryManagerNetworkCallback, this);
   m_network->registerCallback(SHARED_MEM_RESPONSE, MemoryManagerNetworkCallback, this);
   m_network->registerCallback(SHARED_MEM_INIT_REQ, MemoryManagerNetworkCallback, this);
   m_network->registerCallback(RESET_CACHE_COUNTERS, MemoryManagerNetworkCallback, this);
   m_network->registerCallback(DISABLE_CACHE_COUNTERS, MemoryManagerNetworkCallback, this);

   // Some private variables
   cache_locked = false;
}

MemoryManager::~MemoryManager()
{
   m_network->unregisterCallback(SHARED_MEM_REQ);
   m_network->unregisterCallback(SHARED_MEM_EVICT);
   m_network->unregisterCallback(SHARED_MEM_UPDATE_UNEXPECTED);
   m_network->unregisterCallback(SHARED_MEM_ACK);
   m_network->unregisterCallback(SHARED_MEM_RESPONSE);
   m_network->unregisterCallback(SHARED_MEM_INIT_REQ);
   m_network->unregisterCallback(RESET_CACHE_COUNTERS);
   m_network->unregisterCallback(DISABLE_CACHE_COUNTERS);

   delete m_dram_dir;
   delete m_addr_home_lookup;

   delete m_dram_perf_model;
}

void MemoryManagerNetworkCallback(void *obj, NetPacket packet)
{
   MemoryManager *mm = (MemoryManager*) obj;
   assert(mm != NULL);

   mm->getShmemPerfModel()->setCycleCount(packet.time);

   switch (packet.type)
   {
   case SHARED_MEM_REQ:
      mm->addMemRequest(packet);
      return;

   case SHARED_MEM_EVICT:
      mm->forwardWriteBackToDram(packet);
      return;

   case SHARED_MEM_UPDATE_UNEXPECTED:
      mm->processUnexpectedSharedMemUpdate(packet);
      return;

   case SHARED_MEM_ACK:
      mm->processAck(packet);
      return;

   case SHARED_MEM_RESPONSE:
      mm->processSharedMemResponse(packet);
      return;

   case SHARED_MEM_INIT_REQ:
      mm->processSharedMemInitialReq(packet);
      return;

   case RESET_CACHE_COUNTERS:
      mm->resetCacheCounters(packet);
      return;

   case DISABLE_CACHE_COUNTERS:
      mm->disableCacheCounters(packet);
      break;

   default:
      assert(false);
   };
}

void MemoryManager::resetCacheCounters(NetPacket pkt)
{
   m_mmu_lock.acquire();
   m_dcache->resetCounters();
   m_mmu_lock.release();
}

void MemoryManager::disableCacheCounters(NetPacket pkt)
{
   m_mmu_lock.acquire();
   m_dcache->disableCounters();
   m_mmu_lock.release();
}

void MemoryManager::debugPrintReqPayload(RequestPayload payload)
{
   LOG_PRINT("RequestPayload - RequestType (%s) ADDR (%x)", sMemReqTypeToString(payload.request_type).c_str(), payload.request_address);
}

NetPacket MemoryManager::makePacket(PacketType packet_type, Byte* payload_buffer, UInt32 payload_size, SInt32 sender_rank, SInt32 receiver_rank)
{
   NetPacket packet;
   packet.time = getShmemPerfModel()->getCycleCount();
   packet.type = packet_type;
   packet.sender = sender_rank;
   packet.receiver = receiver_rank;
   packet.data = payload_buffer;
   packet.length = payload_size;

   return packet;
}

//assumes we want to match both sender_rank and packet_type
NetMatch MemoryManager::makeNetMatch(PacketType packet_type, SInt32 sender_rank)
{
   NetMatch net_match;
   net_match.senders.push_back(sender_rank);
   net_match.types.push_back(packet_type);

   return net_match;
}

bool MemoryManager::actionPermissable(IntPtr addr, CacheState cache_state, shmem_req_t shmem_req_type, bool modeled, SInt32 access_num)
{
   // TODO: This has to be kind of redesigned.
   // This is not the best way of updating cache counters
   // TODO:
   // 1) Capture conflict misses
   // 2) Capture true and false sharing misses
   // 3) Try to distinguish between upgrade misses and sharing misses
   switch (shmem_req_type)
   {
   case READ:
      if (m_shmem_perf_model->isEnabled() && modeled && (access_num == 0))
      {
         m_dcache->updateCounters(addr, cache_state, CacheBase::ACCESS_TYPE_LOAD);
      }
      return cache_state.readable();
   
   case READ_EX:
   case WRITE:
      if (m_shmem_perf_model->isEnabled() && modeled && (access_num == 0))
      {
         m_dcache->updateCounters(addr, cache_state, CacheBase::ACCESS_TYPE_STORE);
      }
      return cache_state.writable();
   
   default:
      LOG_PRINT_ERROR("In Actionreadily permissiable");
      return false;
   }
}

void MemoryManager::setCacheLineInfo(IntPtr address, CacheState::cstate_t new_cstate)
{
   CacheBlockInfo* cache_block_info = m_dcache->peekSingleLine(address);
   assert(cache_block_info);   //it should already be in the cache for us to change it!

   cache_block_info->setCState(new_cstate);
}

CacheBlockInfo* MemoryManager::getCacheLineInfo(IntPtr address)
{
   return m_dcache->peekSingleLine(address);
}

//copy data at cache to data_buffer
void MemoryManager::accessCacheLineData(CacheBase::AccessType access_type, IntPtr ca_address, UInt32 offset, Byte* data_buffer, UInt32 data_size)
{
   IntPtr address = ca_address + offset;
   __attribute(__unused__) CacheBlockInfo* cache_block_info = m_dcache->accessSingleLine(address, access_type, data_buffer, data_size);

   assert(cache_block_info);
}

void MemoryManager::fillCacheLineData(IntPtr ca_address, Byte* fill_buffer)
{
   bool eviction;
   IntPtr evict_addr;
   CacheBlockInfo evict_block_info;
   Byte evict_buff[m_cache_line_size];

   m_dcache->insertSingleLine(ca_address, fill_buffer,
         &eviction, &evict_addr, &evict_block_info, evict_buff);

   if (eviction)
   {
      UInt32 home_node_rank = m_addr_home_lookup->find_home_for_addr(evict_addr);
      AckPayload payload;
      payload.ack_address = evict_addr;
      if (evict_block_info.isDirty())
      {
         payload.is_writeback = true;
         payload.data_size = m_cache_line_size;
      }
      else
      {
         payload.is_writeback = false;
         payload.data_size = 0;
      }
      UInt32 payload_size = sizeof(payload) + payload.data_size;
      Byte payload_buffer[payload_size];

      createAckPayloadBuffer(&payload, evict_buff, payload_buffer, payload_size);
      NetPacket packet = makePacket(SHARED_MEM_EVICT, payload_buffer, payload_size, m_core_id, home_node_rank);

      m_network->netSend(packet);
   }
}

void MemoryManager::forwardWriteBackToDram(NetPacket wb_packet)
{
   LOG_PRINT("Forwarding WriteBack to DRAM");
   m_dram_dir->processWriteBack(wb_packet);
}

void MemoryManager::invalidateCacheLine(IntPtr address)
{
   __attribute(__unused__) bool hit = m_dcache->invalidateSingleLine(address);
   assert(hit);
}

//send request to DRAM Directory to request a given address, for a certain operation
//does NOT touch the cache, but instead writes the data to the global "fill_buffer"
//sets what the new_cstate should be set to on the receiving end
void MemoryManager::requestPermission(shmem_req_t shmem_req_type, IntPtr ca_address)
{
   // initialize packet payload
   //Request the entire cache-line
 
   RequestPayload payload;
 
   if (shmem_req_type == READ_EX)
   {
      // I want to get the block in exclusive state
      // FIXME: Clean up the terminology later
      // It should be GetEX and GetSH
      payload.request_type = WRITE;
   }
   else // (shmem_req_type == READ) || (shmem_req_type == WRITE)
   {
      payload.request_type = shmem_req_type;
   }

   payload.request_address = ca_address;
   payload.request_num_bytes = m_cache_line_size;

   // send message here to home directory node to request data
   //packet.type, sender, receiver, packet.length
   NetPacket packet = makePacket(SHARED_MEM_INIT_REQ, (Byte *)(&payload), sizeof(RequestPayload), m_core_id, m_core_id);
   m_network->netSend(packet);

}

// ca_address is "cache-aligned" address
// addr_offset provides the offset that points to the requested address
// Returns 'true' if there was a cache hit and 'false' otherwise
bool MemoryManager::initiateSharedMemReq(Core::lock_signal_t lock_signal, shmem_req_t shmem_req_type, IntPtr ca_address, UInt32 addr_offset, Byte* data_buffer, UInt32 buffer_size, bool modeled)
{
   bool cache_hit = true;

   assert(buffer_size > 0);

   CacheBase::AccessType access_type;
   
   if (shmem_req_type == READ)
   {
      access_type = CacheBase::ACCESS_TYPE_LOAD;
   }
   else if (shmem_req_type == READ_EX)
   {
      access_type = CacheBase::ACCESS_TYPE_LOAD;
   }
   else if (shmem_req_type == WRITE)
   {
      access_type = CacheBase::ACCESS_TYPE_STORE;
   }
   else
   {
      LOG_ASSERT_ERROR(false, "Got an INVALID shared memory request type: %u", shmem_req_type);
      return false;
   }
      
   LOG_PRINT("%s - start : REQUESTING ADDR: %x", ((shmem_req_type==READ) ? "READ" : "WRITE"), ca_address);

   SInt32 access_num = 0;

   m_mmu_lock.acquire();

   while (1)
   {
      CacheBlockInfo* cache_block_info;
      //first-> is the line available in the cache? (Native cache hit)
      //second-> pointer to cache tag to update cache state

      cache_block_info = getCacheLineInfo(ca_address); //check hit status, get CState
      CacheState::cstate_t curr_cstate = (cache_block_info != NULL)
                                         ? cache_block_info->getCState()
                                         : CacheState::INVALID;

      // Since 'actionPermissable()' is called multiple times on a cache
      // miss, we want it to update the cache counters only on the
      // first access
      bool permissible = actionPermissable(ca_address, curr_cstate, shmem_req_type, modeled, access_num);
      if (permissible)
      {
         accessCacheLineData(access_type, ca_address, addr_offset, data_buffer, buffer_size);

         LOG_PRINT("%s - FINISHED(cache_hit) : REQUESTING ADDR: 0x%x", (shmem_req_type==READ) ? " READ " : " WRITE ", ca_address);

         // Lock or Unlock the cache
         if (lock_signal == Core::LOCK)
         {
            LOG_ASSERT_ERROR(shmem_req_type == READ_EX, "Shared Memory Request is not READ_EX");
            lockCache();
         }
         else if (lock_signal == Core::UNLOCK)
         {
            LOG_ASSERT_ERROR(shmem_req_type == WRITE, "Shared Memory Request is not WRITE");
            unlockCache();

            m_mmu_cond.broadcast();
         }

         m_mmu_lock.release();

         return (cache_hit);
      }
      else
      {
         // I should not make a shared memory request when the cache is locked
         // because the network thread may be waiting on the condition variable
         LOG_ASSERT_ERROR (!isCacheLocked(), "Cache Miss when cache is locked not supported");

         cache_hit = false;

         // request permission from the directory to access data in the correct state
         requestPermission(shmem_req_type, ca_address);
         m_received_reply = false;

         // Wait till the cache miss has been processed by the network thread
         while (! m_received_reply)
         {
            LOG_PRINT ("Entering Wait on Condition Variable: ADDR: 0x%x", ca_address);
            m_mmu_cond.wait(m_mmu_lock);
            LOG_PRINT ("Finished Wait on Condition Variable: ADDR: 0x%x", ca_address);
         }
      }
      
      access_num ++;

   }

   assert(false);    // Should not reach here !!
   return (false);

}

/*
 * lockCache():
 *    This is the function used to LOCK the cache.
 *    Once the cache is locked, all unexpected updates are delayed till the cache is unlocked again
 *
 * Arguments: None
 * Returns: void
 */
void MemoryManager::lockCache()
{
   cache_locked = true;
}

/*
 * unlockCache():
 *    This is the function used to UNLOCK the cache.
 *
 * Arguments: None
 * Returns: void
 */
void MemoryManager::unlockCache()
{
   cache_locked = false;
}

/*
 * isCacheLocked():
 *    Queries if the cache is locked.
 *
 * Arguments: None
 * Returns: bool
 */

bool MemoryManager::isCacheLocked()
{
   return (cache_locked);
}

void MemoryManager::processSharedMemInitialReq(NetPacket req_packet)
{
   assert (req_packet.type == SHARED_MEM_INIT_REQ);
  
   RequestPayload req_payload;
   extractRequestPayloadBuffer (&req_packet, &req_payload);

   IntPtr address = req_payload.request_address;
   
   LOG_PRINT ("Got Initial Request for ADDR 0x%x", address);
   
   UInt32 home_node_rank = m_addr_home_lookup->find_home_for_addr(address);

   assert(home_node_rank >= 0 && home_node_rank < (UInt32)(Config::getSingleton()->getTotalCores()));

   // Send Shared Memory Request & Recv Update
   // send message here to home directory node to request data
   // packet.type, sender, receiver, packet.length
   NetPacket packet = makePacket(SHARED_MEM_REQ, (Byte *)(&req_payload), sizeof(RequestPayload), m_core_id, home_node_rank); 
   m_network->netSend(packet);

}

void MemoryManager::processSharedMemResponse(NetPacket rep_packet)
{
   // Update the 'user thread' performance model
   getShmemPerfModel()->setCycleCount(ShmemPerfModel::_USER_THREAD, rep_packet.time);

   assert(rep_packet.type == SHARED_MEM_RESPONSE);

   UpdatePayload rep_payload;
   Byte fill_buffer[m_cache_line_size];

   extractUpdatePayloadBuffer(&rep_packet, &rep_payload, fill_buffer);

   IntPtr address = rep_payload.update_address;
   CacheState::cstate_t new_cstate = rep_payload.update_new_cstate;

   m_mmu_lock.acquire();

   LOG_ASSERT_ERROR(!isCacheLocked(), "Cache Miss when Cache is Locked is not supported");

   fillCacheLineData(address, fill_buffer);
   setCacheLineInfo(address, new_cstate);
   m_received_reply = true;

   m_mmu_lock.release();
   m_mmu_cond.broadcast();

   LOG_PRINT("FINISHED(cache_miss) : REQUESTING ADDR: 0x%x", address);
}

//only the first request called drops into the while loop
//all additional requests add to the request_queue,
//but then skip over the while loop.
void MemoryManager::addMemRequest(NetPacket req_packet)
{
   m_dram_dir->startSharedMemRequest(req_packet);
}

void MemoryManager::processAck(NetPacket req_packet)
{
   m_dram_dir->processAck(req_packet);
}

/*
 * this function is called by the "interrupt handler" when an unexpected shared memory update arrives
 * (for example, an invalidation message). "expected" shared memory update messages are processed
 * in band by explicit receive messages
 * Essentially, the Core is being told by someone else to set their cache_tag for a given address to something new.
 * However, that part is only done if the address is still in the cache.  But an ACK is sent either way.
 */
void MemoryManager::processUnexpectedSharedMemUpdate(NetPacket update_packet)
{
   // TODO: This kind of argument passing is bad for performance. Try to pass by reference or pass pointers

   // verify packet type is correct
   assert(update_packet.type == SHARED_MEM_UPDATE_UNEXPECTED);

   // We dont expect any data from the Dram Directory
   UpdatePayload update_payload;
   extractUpdatePayloadBuffer(&update_packet, &update_payload, NULL);

   // extract relevant values from incoming request packet
   CacheState::cstate_t new_cstate = update_payload.update_new_cstate;
   IntPtr address = update_payload.update_address;

   LOG_PRINT("Processing Unexpected: address: %x, new CState: %s", address, CacheState::cStateToString(new_cstate).c_str());

   m_mmu_lock.acquire();

   while (isCacheLocked())
   {
      // Wait here till the user thread signals the condition variable
      m_mmu_cond.wait(m_mmu_lock);
   }

   CacheBlockInfo* cache_block_info = getCacheLineInfo(address);
   // if it is null, it means the address has been invalidated
   CacheState::cstate_t current_cstate = (cache_block_info != NULL) ?
                                         cache_block_info->getCState() :
                                         CacheState::INVALID;

   // send back acknowledgement of receiveing this message
   AckPayload payload;
   payload.ack_new_cstate = new_cstate; //verify you set it to the correct cstate
   payload.ack_address = address; //only sent for debugging purposes
   Byte writeback_data[m_cache_line_size];

   UInt32 payload_size = 0;
   Byte payload_buffer[sizeof(payload) + m_cache_line_size];

   assert((new_cstate == CacheState::INVALID)  || (new_cstate == CacheState::SHARED));
   switch (current_cstate)
   {

   case CacheState::EXCLUSIVE:
      cache_block_info->setCState(new_cstate);

      //send data for write-back
      //TODO this is going to trigger a read-statistic for the cache.
      accessCacheLineData(CacheBase::ACCESS_TYPE_LOAD, address, 0, writeback_data, m_cache_line_size);
      payload_size = sizeof(payload) + m_cache_line_size;
      payload.is_writeback = true;
      payload.data_size = m_cache_line_size;
      createAckPayloadBuffer(&payload, writeback_data, payload_buffer, payload_size);

      if (new_cstate == CacheState::INVALID)
      {
         invalidateCacheLine(address);
      }

      break;

   case CacheState::SHARED:
      cache_block_info->setCState(new_cstate);

      payload_size = sizeof(payload);
      createAckPayloadBuffer(&payload, NULL, payload_buffer, payload_size);

      assert(new_cstate == CacheState::INVALID);

      invalidateCacheLine(address);

      break;

   case CacheState::INVALID:
      //THIS can happen due to race conditions where core evalidates at the same time dram sends demotion message
      //address has been invalidated -> tell directory to remove us from sharers' list
      payload_size = sizeof(payload);
      payload.ack_new_cstate = CacheState::INVALID;
      payload.remove_from_sharers = true;
      createAckPayloadBuffer(&payload, NULL, payload_buffer, payload_size);

      break;

   default:
      LOG_PRINT_ERROR("in MMU switch statement.");
      break;
   }

   //TODO we can move this earlier for performance
   m_mmu_lock.release();

   NetPacket packet = makePacket(SHARED_MEM_ACK, payload_buffer, payload_size, m_core_id, update_packet.sender);
   m_network->netSend(packet);

   LOG_PRINT("end of processUnexpectedSharedMemUpdate");
}

string MemoryManager::sMemReqTypeToString(shmem_req_t type)
{
   switch (type)
   {
   case READ:
      return "READ      ";
   case WRITE:
      return "WRITE     ";
   case INVALIDATE:
      return "INVALIDATE";
   default:
      return "ERROR SMEMREQTYPE";
   }
   return "ERROR SMEMREQTYPE";
}

void MemoryManager::createUpdatePayloadBuffer(UpdatePayload* send_payload, Byte* data_buffer, Byte* payload_buffer, UInt32 payload_size)
{
   // Create a new buffer of size : sizeof(send_payload) + m_cache_line_size
   assert(payload_buffer != NULL);

   //copy send_payload
   memcpy((void*) payload_buffer, (void*) send_payload, sizeof(*send_payload));

   //this is very important on the recieving end, so the extractor knows how big data_size is
   assert(send_payload->data_size == (payload_size - sizeof(*send_payload)));

   //copy data_buffer
   if (data_buffer != NULL)
      memcpy((void*)(payload_buffer + sizeof(*send_payload)), (void*) data_buffer, payload_size - sizeof(*send_payload));

}

void MemoryManager::createAckPayloadBuffer(AckPayload* send_payload, Byte* data_buffer, Byte* payload_buffer, UInt32 payload_size)
{
   // Create a new buffer of size : sizeof(send_payload) + m_cache_line_size
   assert(payload_buffer != NULL);

   //this is very important on the recieving end, so the extractor knows how big data_size is
   assert(send_payload->data_size == (payload_size - sizeof(*send_payload)));

   //copy send_payload
   memcpy((void*) payload_buffer, (void*) send_payload, sizeof(*send_payload));

   //copy data_buffer
   if (data_buffer != NULL)
      memcpy((void*)(payload_buffer + sizeof(*send_payload)), (void*) data_buffer, payload_size - sizeof(*send_payload));

}

void MemoryManager::extractUpdatePayloadBuffer(NetPacket* packet, UpdatePayload* payload, Byte* data_buffer)
{
   //copy packet->data to payload (extract payload)
   memcpy((void*) payload, (void*)(packet->data), sizeof(*payload));

   assert((payload->data_size == 0) == (data_buffer == NULL));
   if (payload->data_size > 0)
      memcpy((void*) data_buffer, (void*)(((Byte*) packet->data) + sizeof(*payload)), payload->data_size);
}

//TODO should we turn payloads from structs to classes so we don't have to have seperate methods to do this stuff?
void MemoryManager::extractAckPayloadBuffer(NetPacket* packet, AckPayload* payload, Byte* data_buffer)
{
   memcpy((void*) payload, (void*)(packet->data), sizeof(*payload));

   if (payload->data_size > 0)
      memcpy((void*) data_buffer, (void*)(((Byte*) packet->data) + sizeof(*payload)), payload->data_size);
}

void MemoryManager::extractRequestPayloadBuffer (NetPacket* packet, RequestPayload* payload)
{
   assert (packet->length == sizeof(*payload));
   memcpy ((void*) payload, (void*) (packet->data), sizeof(*payload));
}

carbon_reg_t MemoryManager::redirectMemOp (bool has_lock_prefix, IntPtr tgt_ea, IntPtr size, AccessType access_type)
{
   assert (access_type < NUM_ACCESS_TYPES);
   char *scratchpad = m_scratchpad [access_type];
   
   if ((access_type == ACCESS_TYPE_READ) || (access_type == ACCESS_TYPE_READ2))
   {
      shmem_req_t shmem_req_type;
      Core::lock_signal_t lock_signal;

      if (has_lock_prefix)
      {
         // FIXME: Now, when we have a LOCK prefix, we do an exclusive READ
         shmem_req_type = READ_EX;
         lock_signal = Core::LOCK;
      }
      else
      {
         shmem_req_type = READ;
         lock_signal = Core::NONE;
      }
       
      m_core->accessMemory (lock_signal, shmem_req_type, tgt_ea, scratchpad, size, true);

   }
   return (carbon_reg_t) scratchpad;
}

void MemoryManager::completeMemWrite (bool has_lock_prefix, IntPtr tgt_ea, IntPtr size, AccessType access_type)
{
   char *scratchpad = m_scratchpad [access_type];

   Core::lock_signal_t lock_signal = (has_lock_prefix) ? Core::UNLOCK : Core::NONE;
      
   m_core->accessMemory (lock_signal, WRITE, tgt_ea, scratchpad, size, true);
   
   return;
}

carbon_reg_t MemoryManager::redirectPushf ( IntPtr tgt_esp, IntPtr size )
{
   m_saved_esp = tgt_esp;
   return ((carbon_reg_t) m_scratchpad [ACCESS_TYPE_WRITE]) + size;
}

carbon_reg_t MemoryManager::completePushf ( IntPtr esp, IntPtr size )
{
   m_saved_esp -= size;
   completeMemWrite (false, (IntPtr) m_saved_esp, size, ACCESS_TYPE_WRITE);
   return m_saved_esp;
}

carbon_reg_t MemoryManager::redirectPopf (IntPtr tgt_esp, IntPtr size)
{
   m_saved_esp = tgt_esp;
   return redirectMemOp (false, m_saved_esp, size, ACCESS_TYPE_READ);
}

carbon_reg_t MemoryManager::completePopf (IntPtr esp, IntPtr size)
{
   return (m_saved_esp + size);
}

