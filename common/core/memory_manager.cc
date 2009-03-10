#include "memory_manager.h"
#include "simulator.h"
#include "log.h"

using namespace std;

UInt32 MemoryManager::m_knob_ahl_param;
UInt32 MemoryManager::m_knob_dram_access_cost;
UInt32 MemoryManager::m_knob_line_size;

void MemoryManagerNetworkCallback(void *obj, NetPacket packet);

MemoryManager::MemoryManager(SInt32 core_id, Network *network, OCache *ocache)
{
   try
   {
   m_knob_ahl_param = Sim()->getCfg()->getInt("dram/ahl_param");
   m_knob_dram_access_cost = Sim()->getCfg()->getInt("dram/dram_access_cost");
   m_knob_line_size = Sim()->getCfg()->getInt("ocache/line_size");
   }
   catch(...)
   {
      LOG_ASSERT_ERROR(false, "MemoryManager obtained a bad value from config.");
   }

   LOG_ASSERT_ERROR(!Config::getSingleton()->isSimulatingSharedMemory() || Config::getSingleton()->getEnableDCacheModeling(),
                    "Must set dcache modeling on (-mdc) to use shared memory model.");

   m_core_id = core_id;
   m_network = network;
   m_ocache = ocache;

   assert(m_ocache != NULL);

   m_cache_line_size = Config::getSingleton()->getCacheLineSize();

   m_dram_dir = new DramDirectory(m_core_id, m_network);
   m_addr_home_lookup = new AddressHomeLookup(m_core_id);

   m_network->registerCallback(SHARED_MEM_REQ, MemoryManagerNetworkCallback, this);
   m_network->registerCallback(SHARED_MEM_EVICT, MemoryManagerNetworkCallback, this);
   m_network->registerCallback(SHARED_MEM_UPDATE_UNEXPECTED, MemoryManagerNetworkCallback, this);
   m_network->registerCallback(SHARED_MEM_ACK, MemoryManagerNetworkCallback, this);
   m_network->registerCallback(SHARED_MEM_RESPONSE, MemoryManagerNetworkCallback, this);
   m_network->registerCallback(SHARED_MEM_INIT_REQ, MemoryManagerNetworkCallback, this);

}

MemoryManager::~MemoryManager()
{
   m_network->unregisterCallback(SHARED_MEM_REQ);
   m_network->unregisterCallback(SHARED_MEM_EVICT);
   m_network->unregisterCallback(SHARED_MEM_UPDATE_UNEXPECTED);
   m_network->unregisterCallback(SHARED_MEM_ACK);
   m_network->unregisterCallback(SHARED_MEM_RESPONSE);
   m_network->unregisterCallback(SHARED_MEM_INIT_REQ);
}

void MemoryManagerNetworkCallback(void *obj, NetPacket packet)
{
   MemoryManager *mm = (MemoryManager*) obj;
   assert(mm != NULL);

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

   default:
      assert(false);
   };
}


void MemoryManager::debugPrintReqPayload(RequestPayload payload)
{
   LOG_PRINT("RequestPayload - RequestType (%s) ADDR (%x)", sMemReqTypeToString(payload.request_type).c_str(), payload.request_address);
}

NetPacket MemoryManager::makePacket(PacketType packet_type, Byte* payload_buffer, UInt32 payload_size, SInt32 sender_rank, SInt32 receiver_rank)
{
   NetPacket packet;
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

bool MemoryManager::actionPermissable(CacheState cache_state, shmem_req_t shmem_req_type)
{
   bool ret;

   switch (shmem_req_type)
   {
   case READ:
      ret = cache_state.readable();
      break;
   case WRITE:
      ret = cache_state.writable();
      break;
   default:
      LOG_PRINT_ERROR("in Actionreadily permissiable");
      return false;
   }

   return ret;
}

void MemoryManager::setCacheLineInfo(IntPtr ca_address, CacheState::cstate_t new_cstate)
{
   pair<bool, CacheTag*> results = m_ocache->runDCachePeekModel(ca_address);
   assert(results.first);   //it should already be in the cache for us to change it!
   assert(results.second != NULL);

   results.second->setCState(new_cstate);
}

pair<bool, CacheTag*> MemoryManager::getCacheLineInfo(IntPtr address)
{
   return m_ocache->runDCachePeekModel(address);
}

//copy data at cache to data_buffer
void MemoryManager::accessCacheLineData(CacheBase::AccessType access_type, IntPtr ca_address, UInt32 offset, Byte* data_buffer, UInt32 data_size)
{
   bool fail_need_fill = false;
   bool eviction = false;
   IntPtr evict_addr;
   Byte evict_buff[m_cache_line_size];

   IntPtr data_addr = ca_address + offset;
   pair<bool, CacheTag*> result;

   // FIXME: Hack
   result = m_ocache->accessSingleLine(data_addr, access_type,
                                       &fail_need_fill, NULL,
                                       data_buffer, data_size,
                                       &eviction, &evict_addr, evict_buff);

   assert(eviction == false);
   assert(fail_need_fill == false);
}

void MemoryManager::fillCacheLineData(IntPtr ca_address, Byte* fill_buffer)
{
   Byte data_buffer[m_cache_line_size];
   bool fail_need_fill;

   bool eviction;
   IntPtr evict_addr;
   Byte evict_buff[m_cache_line_size];

   pair<bool, CacheTag*> result;

   // FIXME: There may be a problem here. Ask Jonathan about it
   result = m_ocache->accessSingleLine(ca_address, CacheBase::k_ACCESS_TYPE_LOAD,
                                       &fail_need_fill, NULL,
                                       data_buffer, m_cache_line_size,
                                       &eviction, &evict_addr, evict_buff);

   assert(fail_need_fill == true);
   assert(eviction == false);

   if (fail_need_fill)
   {
      //note: fail_need_fill is known beforehand,
      //so that fill_buffer can be fetched from DRAM
      result = m_ocache->accessSingleLine(ca_address, CacheBase::k_ACCESS_TYPE_LOAD,
                                          NULL, fill_buffer,
                                          data_buffer, m_cache_line_size,
                                          &eviction, &evict_addr, evict_buff);

      if (eviction)
      {
         //send write-back to dram
         // TODO: We need a way to find differentiate between the CLEAN and the DIRTY states
         // of a cache line. This is because we need to write back the cache block only when
         // it was DIRTY. The 'accessSingleLine()' interface should tell us whether the
         // cache block was dirty or clean
         UInt32 home_node_rank = m_addr_home_lookup->find_home_for_addr(evict_addr);
         AckPayload payload;
         payload.ack_address = evict_addr;
         payload.is_writeback = true;
         payload.data_size = m_cache_line_size;
         UInt32 payload_size = sizeof(payload) + m_cache_line_size;
         Byte payload_buffer[payload_size];

         createAckPayloadBuffer(&payload, evict_buff, payload_buffer, payload_size);
         NetPacket packet = makePacket(SHARED_MEM_EVICT, payload_buffer, payload_size, m_core_id, home_node_rank);

         m_network->netSend(packet);
      }
   }
}

void MemoryManager::forwardWriteBackToDram(NetPacket wb_packet)
{
   // LOG_PRINT("Forwarding WriteBack to DRAM");
   m_dram_dir->processWriteBack(wb_packet);
}

void MemoryManager::invalidateCacheLine(IntPtr address)
{
   bool hit = m_ocache->invalidateLine(address);
   assert(hit);
}

//send request to DRAM Directory to request a given address, for a certain operation
//does NOT touch the cache, but instead writes the data to the global "fill_buffer"
//sets what the new_cstate should be set to on the receiving end
void MemoryManager::requestPermission(shmem_req_t shmem_req_type, IntPtr ca_address)
{
   /* ==================================================== */
   /* =========== Send Request & Recv Update ============= */
   /* ==================================================== */

   // initialize packet payload
   //Request the entire cache-line
   RequestPayload payload;
   payload.request_type = shmem_req_type;
   payload.request_address = ca_address;
   payload.request_num_bytes = m_cache_line_size;

   // send message here to home directory node to request data
   //packet.type, sender, receiver, packet.length
   NetPacket packet = makePacket(SHARED_MEM_INIT_REQ, (Byte *)(&payload), sizeof(RequestPayload), m_core_id, m_core_id);
   m_network->netSend(packet);

}

// ca_address is "cache-aligned" address
// addr_offset provides the offset that points to the requested address
bool MemoryManager::initiateSharedMemReq(shmem_req_t shmem_req_type, IntPtr ca_address, UInt32 addr_offset, Byte* data_buffer, UInt32 buffer_size)
{
   assert(buffer_size > 0);

   CacheBase::AccessType access_type = (shmem_req_type == READ)
                                       ? CacheBase::k_ACCESS_TYPE_LOAD
                                       : CacheBase::k_ACCESS_TYPE_STORE;
   LOG_PRINT("%s - start : REQUESTING ADDR: %x", ((shmem_req_type==READ) ? " READ " : " WRITE "), ca_address);

   m_mmu_cond.acquire();

   while (1)
   {

      pair<bool, CacheTag*> cache_model_results;
      //first-> is the line available in the cache? (Native cache hit)
      //second-> pointer to cache tag to update cache state

      cache_model_results = getCacheLineInfo(ca_address); //check hit status, get CState
      bool native_cache_hit = cache_model_results.first;
      CacheState::cstate_t curr_cstate = (cache_model_results.second != NULL)
                                         ? cache_model_results.second->getCState()
                                         : CacheState::INVALID;

      if (actionPermissable(curr_cstate, shmem_req_type))
      {
         assert(native_cache_hit == true);

         accessCacheLineData(access_type, ca_address, addr_offset, data_buffer, buffer_size);

         LOG_PRINT("%s - FINISHED(cache_hit) : REQUESTING ADDR: 0x%x", (shmem_req_type==READ) ? " READ " : " WRITE ", ca_address);

         m_mmu_cond.release();

         return (true);
      }
      else
      {
         // request permission from the directory to access data in the correct state
         requestPermission(shmem_req_type, ca_address);
         m_received_reply = false;

         // Wait till the cache miss has been processed by the network thread
         while (! m_received_reply) 
         {
            LOG_PRINT ("Entering Wait on Condition Variable: ADDR: 0x%x", ca_address);
            m_mmu_cond.wait();
            LOG_PRINT ("Finished Wait on Condition Variable: ADDR: 0x%x", ca_address);
         }
      }

   }

   assert(false);    // Should not reach here !!
   return (false);

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

   /* ==================================================== */
   /* =========== Send Request & Recv Update ============= */
   /* ==================================================== */
   
   // send message here to home directory node to request data
   //packet.type, sender, receiver, packet.length
   NetPacket packet = makePacket(SHARED_MEM_REQ, (Byte *)(&req_payload), sizeof(RequestPayload), m_core_id, home_node_rank); 
   m_network->netSend(packet);

}

void MemoryManager::processSharedMemResponse(NetPacket rep_packet)
{
   assert(rep_packet.type == SHARED_MEM_RESPONSE);

   UpdatePayload rep_payload;
   Byte fill_buffer[m_cache_line_size];

   extractUpdatePayloadBuffer(&rep_packet, &rep_payload, fill_buffer);

   IntPtr address = rep_payload.update_address;
   CacheState::cstate_t new_cstate = rep_payload.update_new_cstate;

   m_mmu_cond.acquire();

   fillCacheLineData(address, fill_buffer);
   setCacheLineInfo(address, new_cstate);
   m_received_reply = true;

   m_mmu_cond.release();
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
//TODO this such that is describes that it affects the CACHE state, and is not an update to the directory?
//ie, write_backs need to go elsewhere!
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

   m_mmu_cond.acquire();

   pair<bool, CacheTag*> cache_model_results = m_ocache->runDCachePeekModel(address);
   // if it is null, it means the address has been invalidated
   CacheState::cstate_t current_cstate = (cache_model_results.second != NULL) ?
                                         cache_model_results.second->getCState() :
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
      cache_model_results.second->setCState(new_cstate);

      //send data for write-back
      //TODO this is going to trigger a read-statistic for the cache.
      accessCacheLineData(CacheBase::k_ACCESS_TYPE_LOAD, address, 0, writeback_data, m_cache_line_size);
      payload_size = sizeof(payload) + m_cache_line_size;
      payload.is_writeback = true;
      payload.data_size = m_cache_line_size;
      createAckPayloadBuffer(&payload, writeback_data, payload_buffer, payload_size);

      if (new_cstate == CacheState::INVALID)
         invalidateCacheLine(address);

      break;

   case CacheState::SHARED:
      cache_model_results.second->setCState(new_cstate);

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
   m_mmu_cond.release();

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

