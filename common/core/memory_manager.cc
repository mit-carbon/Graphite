#include "memory_manager.h"
#include "log.h"
#define LOG_DEFAULT_RANK   (the_core->getRank())
#define LOG_DEFAULT_MODULE MMU

extern LEVEL_BASE::KNOB<BOOL> g_knob_simarch_has_shared_mem;
extern LEVEL_BASE::KNOB<UInt32> g_knob_ahl_param;
extern LEVEL_BASE::KNOB<UInt32> g_knob_dram_access_cost;
extern LEVEL_BASE::KNOB<UInt32> g_knob_line_size;

using namespace std;

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

      default:
         // whoops
         assert(false);
      };
}

MemoryManager::MemoryManager(Core *the_core_arg, OCache *ocache_arg) 
{
   the_core = the_core_arg;
   ocache = ocache_arg;

   //FIXME; need to add infrastructure for specifying core architecture details (e.g. DRAM size)
   // this also assumes 1 dram per core

   // assume 4GB / dCacheLineSize  bytes/line 
   int total_num_dram_lines = (int) (pow(2,32) / ocache->dCacheLineSize()); 
   
   int dram_lines_per_core = total_num_dram_lines / the_core->getNumCores();
   // assert( (dram_lines_per_core * the_core->getNumCores()) == total_num_dram_lines );

   assert( ocache != NULL );

   //TODO can probably delete "dram_lines_per_core" b/c it not necessary.
   dram_dir = new DramDirectory(dram_lines_per_core, ocache->dCacheLineSize(), the_core->getRank(), the_core->getNumCores(), the_core->getNetwork());

   //TODO bug: this may not gracefully handle cache lines that spill over from one core's dram to another
   addr_home_lookup = new AddressHomeLookup(the_core->getNumCores(), g_knob_ahl_param.Value(), the_core->getRank());
   LOG_PRINT("Creating New Addr Home Lookup Structure: %i, %i, %i", the_core->getNumCores(), g_knob_ahl_param.Value(), the_core->getRank());

   Network *net = the_core->getNetwork();
   net->registerCallback(SHARED_MEM_REQ, MemoryManagerNetworkCallback, this);
   net->registerCallback(SHARED_MEM_EVICT, MemoryManagerNetworkCallback, this);
   net->registerCallback(SHARED_MEM_UPDATE_UNEXPECTED, MemoryManagerNetworkCallback, this);
   net->registerCallback(SHARED_MEM_ACK, MemoryManagerNetworkCallback, this);
   net->registerCallback(SHARED_MEM_RESPONSE, MemoryManagerNetworkCallback, this);

}

MemoryManager::~MemoryManager()
{
   Network *net = the_core->getNetwork();
   net->unregisterCallback(SHARED_MEM_REQ);
   net->unregisterCallback(SHARED_MEM_EVICT);
   net->unregisterCallback(SHARED_MEM_UPDATE_UNEXPECTED);
   net->unregisterCallback(SHARED_MEM_ACK);
}


void MemoryManager::debugPrintReqPayload(RequestPayload payload)
{
   LOG_PRINT("RequestPayload - RequestType (%s) ADDR (%x)", sMemReqTypeToString(payload.request_type).c_str(), payload.request_address);
}

void addRequestPayload(NetPacket* packet, shmem_req_t shmem_req_type, IntPtr address, UInt32 size_bytes)
{
   //TODO BUG this code doesn't work b/c it gets deallocated before the network copies it
   LOG_PRINT_EXPLICIT(-1, MMU, "Starting adding Request Payload;"); 
   MemoryManager::RequestPayload payload;
   payload.request_type = shmem_req_type;
   payload.request_address = address;  
   payload.request_num_bytes = size_bytes;

   packet->data = (char *)(&payload);
   LOG_PRINT_EXPLICIT(-1, MMU, "Finished adding Request Payload;"); 
}

void addAckPayload(NetPacket* packet, IntPtr address, CacheState::cstate_t new_cstate)
{
   MemoryManager::AckPayload payload;
   payload.ack_new_cstate = new_cstate;
   payload.ack_address = address; //only sent for debugging purposes

   packet->data = (char *)(&payload);
}

void addUpdatePayload(NetPacket* packet, IntPtr address, CacheState::cstate_t new_cstate)
{
   MemoryManager::UpdatePayload payload;
   payload.update_new_cstate = new_cstate;
   payload.update_address = address;
   
   packet->data = (char *)(&payload);
}

NetPacket MemoryManager::makePacket(PacketType packet_type, char* payload_buffer, UInt32 payload_size, int sender_rank, int receiver_rank )
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
NetMatch MemoryManager::makeNetMatch(PacketType packet_type, int sender_rank)
{
   NetMatch net_match;
   net_match.senders.push_back(sender_rank);
   net_match.types.push_back(packet_type);

   return net_match;
}

bool action_readily_permissable(CacheState cache_state, shmem_req_t shmem_req_type)
{
   bool ret;
   
   switch( shmem_req_type ) {
      case READ:
         ret = cache_state.readable();
         break;
      case WRITE:
         ret = cache_state.writable();
         break;
      default:
         LOG_NOTIFY_ERROR ();
         LOG_PRINT_EXPLICIT(-1, MMU, "ERROR in Actionreadily permissiable");
         assert(false);
         exit(-1);
         break;
   }

   return ret;
}

void MemoryManager::setCacheLineInfo(IntPtr ca_address, CacheState::cstate_t new_cstate)
{
   pair<bool, CacheTag*> results = ocache->runDCachePeekModel(ca_address);
   assert( results.first ); //it should already be in the cache for us to change it!
   assert( results.second != NULL ); 

   results.second->setCState(new_cstate);         

}

pair<bool, CacheTag*> MemoryManager::getCacheLineInfo(IntPtr address)
{
   return ocache->runDCachePeekModel(address);
}

//copy data at cache to data_buffer
void MemoryManager::accessCacheLineData(CacheBase::AccessType access_type, IntPtr ca_address, UInt32 offset, char* data_buffer, UInt32 data_size)
{
   bool fail_need_fill = false;
   bool eviction = false;
   IntPtr evict_addr;
   char evict_buff[ocache->dCacheLineSize()];

   IntPtr data_addr = ca_address + offset;
   pair<bool, CacheTag*> result;

   // FIXME: Hack
   result = ocache->accessSingleLine(data_addr, access_type,
                              &fail_need_fill, NULL,
                              data_buffer, data_size,
                              &eviction, &evict_addr, evict_buff);
   
   assert(eviction == false);
   assert(fail_need_fill == false);

// if (access_type == CacheBase::k_ACCESS_TYPE_STORE) {
//    cerr << "accessCacheLineData: data_buffer: 0x";
//    for (UInt32 i = 0; i < data_size; i++)
//       cerr << hex << (UInt32) data_buffer[i];
//    cerr << dec << endl;

//    cerr << "accessCacheLineData: fill_buffer: 0x";
//    for (UInt32 i = 0; i < ocache->dCacheLineSize(); i++)
//       cerr << hex << (UInt32) fill_buffer[i];
//    cerr << dec << endl;
// }
}

void MemoryManager::fillCacheLineData(IntPtr ca_address, char* fill_buffer)
{
   char data_buffer[ocache->dCacheLineSize()];
   bool fail_need_fill;

   bool eviction;
   IntPtr evict_addr;
   char evict_buff[ocache->dCacheLineSize()];

   pair<bool, CacheTag*> result;

   result = ocache->accessSingleLine(ca_address, CacheBase::k_ACCESS_TYPE_LOAD,
                              &fail_need_fill, NULL,
                              data_buffer, ocache->dCacheLineSize(),
                              &eviction, &evict_addr, evict_buff);
   
   assert (fail_need_fill == true);
   assert (eviction == false);
   
   if (fail_need_fill) 
   {
      //note: fail_need_fill is known beforehand, 
      //so that fill_buffer can be fetched from DRAM
      result = ocache->accessSingleLine(ca_address, CacheBase::k_ACCESS_TYPE_LOAD,
                                 NULL, fill_buffer,
                                 data_buffer, ocache->dCacheLineSize(),
                                 &eviction, &evict_addr, evict_buff);
   
      if (eviction) 
      {
         //send write-back to dram
         // TODO: We need a way to find differentiate between the CLEAN and the DIRTY states
         // of a cache line. This is because we need to write back the cache block only when
         // it was DIRTY. The 'accessSingleLine()' interface should tell us whether the 
         // cache block was dirty or clean
         UInt32 home_node_rank = addr_home_lookup->find_home_for_addr(evict_addr);
         AckPayload payload;
         payload.ack_address = evict_addr;
         payload.is_writeback = true;
         payload.data_size = ocache->dCacheLineSize();
         UInt32 payload_size = sizeof(payload) + ocache->dCacheLineSize();
         char payload_buffer[payload_size];

         createAckPayloadBuffer(&payload, evict_buff, payload_buffer, payload_size);
         NetPacket packet = makePacket(SHARED_MEM_EVICT, payload_buffer, payload_size, the_core->getRank(), home_node_rank);

         (the_core->getNetwork())->netSend(packet);
      }
   }

}

void MemoryManager::forwardWriteBackToDram(NetPacket wb_packet)
{
// LOG_PRINT("Forwarding WriteBack to DRAM");
   dram_dir->processWriteBack(wb_packet);
}

void MemoryManager::invalidateCacheLine(IntPtr address)
{
   bool hit = ocache->invalidateLine(address);
   assert( hit );
   //we shouldn't be invalidating lines unless we know it's in the cache
}

//send request to DRAM Directory to request a given address, for a certain operation
//does NOT touch the cache, but instead writes the data to the global "fill_buffer"
//sets what the new_cstate should be set to on the receiving end
void MemoryManager::requestPermission(shmem_req_t shmem_req_type, IntPtr ca_address)
{
   UInt32 home_node_rank = addr_home_lookup->find_home_for_addr(ca_address);

   assert(home_node_rank >= 0 && home_node_rank < (UInt32)(the_core->getNumCores()));
   
   /* ==================================================== */
   /* =========== Send Request & Recv Update ============= */
   /* ==================================================== */
   
   // initialize packet payload
   //Request the entire cache-line
   RequestPayload payload;
   payload.request_type = shmem_req_type;
   payload.request_address = ca_address;  
   payload.request_num_bytes = ocache->dCacheLineSize(); 
   
   // send message here to home directory node to request data
   //packet.type, sender, receiver, packet.length
   NetPacket packet = makePacket(SHARED_MEM_REQ, (char *)(&payload), sizeof(RequestPayload), the_core->getRank(), home_node_rank); 
   (the_core->getNetwork())->netSend(packet);

}

//buffer_size is the size in bytes of the char* data_buffer
//we may give the cache less than a cache_line of data, but never more than a cache_line_size of data
//ca_address is "cache-aligned" address
//addr_offset provides the offset that points to the requested address
//TODO this will not work correctly for multi-line requests!
//TODO what is "return bool" used for?  cache hits? or immediately permissable?
bool MemoryManager::initiateSharedMemReq(shmem_req_t shmem_req_type, IntPtr ca_address, UInt32 addr_offset, char* data_buffer, UInt32 buffer_size)
{
   assert(buffer_size > 0);
   
   CacheBase::AccessType access_type = (shmem_req_type == READ) 
                                          ? CacheBase::k_ACCESS_TYPE_LOAD 
                                          : CacheBase::k_ACCESS_TYPE_STORE;
   LOG_PRINT("%s - start : REQUESTING ADDR: %x", ((shmem_req_type==READ) ? " READ " : " WRITE "), ca_address);
   
   mmu_cond.acquire();

   while(1) 
   {

      pair<bool, CacheTag*> cache_model_results;  
      //first-> is the line available in the cache? (Native cache hit)
      //second-> pointer to cache tag to update cache state
   
      cache_model_results = getCacheLineInfo(ca_address); //check hit status, get CState
      bool native_cache_hit = cache_model_results.first;
      CacheState::cstate_t curr_cstate = (cache_model_results.second != NULL) 
                                             ? cache_model_results.second->getCState() 
                                             : CacheState::INVALID;
   
      if ( action_readily_permissable(curr_cstate, shmem_req_type) )
      {
         assert( native_cache_hit == true );
      
         accessCacheLineData(access_type, ca_address, addr_offset, data_buffer, buffer_size); 
         
         LOG_PRINT ("%s - FINISHED(cache_hit) : REQUESTING ADDR: 0x%x", (shmem_req_type==READ) ? " READ " : " WRITE ",ca_address);

         mmu_cond.release();
      
         return (true);
      }
      else
      {
         // request permission from the directory to access data in the correct state
         requestPermission(shmem_req_type, ca_address);

         // Wait till the cache miss has been processed by the network thread
         mmu_cond.wait();
      }

   }

   assert (false);   // Should not reach here !!
   return (false);

}

void MemoryManager::processSharedMemResponse(NetPacket rep_packet) {

   assert (rep_packet.type == SHARED_MEM_RESPONSE);

   UpdatePayload rep_payload;
   char fill_buffer[ocache->dCacheLineSize()];
   
   extractUpdatePayloadBuffer(&rep_packet, &rep_payload, fill_buffer);

   IntPtr address = rep_payload.update_address;
   CacheState::cstate_t new_cstate = rep_payload.update_new_cstate;

   mmu_cond.acquire();

   fillCacheLineData(address, fill_buffer);
   setCacheLineInfo(address, new_cstate);                                        
   
   mmu_cond.release();
   mmu_cond.signal();

   LOG_PRINT("FINISHED(cache_miss) : REQUESTING ADDR: %x", address);
}

//only the first request called drops into the while loop
//all additional requests add to the request_queue, 
//but then skip over the while loop.
void MemoryManager::addMemRequest(NetPacket req_packet)
{
   dram_dir->startSharedMemRequest (req_packet);
}

void MemoryManager::processAck(NetPacket req_packet)
{
   dram_dir->processAck(req_packet);
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
   extractUpdatePayloadBuffer(&update_packet, &update_payload, NULL );

   // extract relevant values from incoming request packet
   CacheState::cstate_t new_cstate = update_payload.update_new_cstate;
   IntPtr address = update_payload.update_address;
  
   LOG_PRINT("Processing Unexpected: address: %x, new CState: %s", address, CacheState::cStateToString(new_cstate).c_str());

   mmu_cond.acquire();

   pair<bool, CacheTag*> cache_model_results = ocache->runDCachePeekModel(address);
   // if it is null, it means the address has been invalidated
   CacheState::cstate_t current_cstate = ( cache_model_results.second != NULL ) ?
                                                                  cache_model_results.second->getCState() :
                                                                  CacheState::INVALID;

   // cerr << "MMU: [" << the_core->getRank() << "] Address = 0x" << hex << address << dec << ", Current State = " << CacheState::cStateToString(current_cstate) << endl;
                                                                     
   // send back acknowledgement of receiveing this message
   AckPayload payload;
   payload.ack_new_cstate = new_cstate; //verify you set it to the correct cstate
   payload.ack_address = address; //only sent for debugging purposes
   UInt32 line_size = ocache->dCacheLineSize();
   char writeback_data[line_size]; 
   
   UInt32 payload_size = 0;
   char payload_buffer[sizeof(payload) + line_size];

   assert ( (new_cstate == CacheState::INVALID)  ||  (new_cstate == CacheState::SHARED) );
   switch( current_cstate ) {

      case CacheState::EXCLUSIVE: 
         cache_model_results.second->setCState(new_cstate);
         
         //send data for write-back
         //TODO this is going to trigger a read-statistic for the cache. 
         accessCacheLineData(CacheBase::k_ACCESS_TYPE_LOAD, address, 0, writeback_data, line_size);
         payload_size = sizeof(payload) + line_size;
         payload.is_writeback = true;
         payload.data_size = line_size;
         createAckPayloadBuffer(&payload, writeback_data, payload_buffer, payload_size);
         
         if (new_cstate == CacheState::INVALID) 
            invalidateCacheLine(address);
         
         break;
      
      case CacheState::SHARED:
         cache_model_results.second->setCState(new_cstate);
         
         payload_size = sizeof(payload);
         createAckPayloadBuffer(&payload, NULL, payload_buffer, payload_size);
         
         assert (new_cstate == CacheState::INVALID);

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
         LOG_NOTIFY_ERROR();
         LOG_PRINT("ERROR in MMU switch statement.");
         assert(false);
         break;
   }

   //TODO we can move this earlier for performance
   mmu_cond.release();

   NetPacket packet = makePacket( SHARED_MEM_ACK, payload_buffer, payload_size, the_core->getRank(), update_packet.sender );
   
   (the_core->getNetwork())->netSend(packet);
  
   LOG_PRINT("end of processUnexpectedSharedMemUpdate");
}

string MemoryManager::sMemReqTypeToString(shmem_req_t type)
{
   switch(type) {
      case READ: return "READ      ";
      case WRITE: return "WRITE     ";
      case INVALIDATE: return "INVALIDATE";
      default: return "ERROR SMEMREQTYPE";
   }
   return "ERROR SMEMREQTYPE";
}

void MemoryManager::debugSetDramState(IntPtr addr, DramDirectoryEntry::dstate_t dstate, vector<UInt32> sharers_list, char *d_data)
{
   // Assume d_data is a pointer to the entire memory block
   dram_dir->debugSetDramState(addr, dstate, sharers_list, d_data);  
}

bool MemoryManager::debugAssertDramState(IntPtr addr, DramDirectoryEntry::dstate_t dstate, vector<UInt32> sharers_list, char *d_data)
{
   return dram_dir->debugAssertDramState(addr, dstate, sharers_list, d_data); 
}

void MemoryManager::debugSetCacheState(IntPtr address, CacheState::cstate_t cstate, char *c_data) {
   
   //using Load Model, so that way we garuntee the tag isn't null
   // Assume that address is always cache aligned
   pair<bool, CacheTag*> cache_result;

   pair<bool, CacheTag*> cache_model_results;
   CacheState::cstate_t current_cstate;


   bool fail_need_fill;
   // char buff[ocache->dCacheLineSize()];
   bool eviction;
   IntPtr evict_addr;
   char evict_buff[ocache->dCacheLineSize()];

   switch(cstate) {
      case CacheState::INVALID:
         ocache->dCacheInvalidateLine(address);
         break;
      case CacheState::SHARED:
         // cache_result = ocache->runDCacheLoadModel(address,1);
         // Falls through. Hope this is fine
      case CacheState::EXCLUSIVE:
         // cache_result = ocache->runDCacheLoadModel(address,1);
         assert ( (cstate == CacheState::SHARED) || (cstate == CacheState::EXCLUSIVE) );
         cache_result = ocache->accessSingleLine (address, CacheBase::k_ACCESS_TYPE_STORE,   
                           &fail_need_fill, NULL, 
                           c_data, ocache->dCacheLineSize(),  
                           &eviction, &evict_addr, evict_buff);
         // Make sure that this is the first time this cache block is being written into
         // assert (fail_need_fill == true);
         if (fail_need_fill) {
            // Note 'c_data' is a pointer to the entire cache line
            cache_result = ocache->accessSingleLine (address, CacheBase::k_ACCESS_TYPE_STORE, 
                           NULL, c_data, 
                           NULL, 0, 
                           &eviction, &evict_addr, evict_buff);

            if (eviction) {
               cerr << "[" << the_core->getRank() << "] MMU: Some data has been evicted\n";
               cerr << "[" << the_core->getRank() << "] MMU: Evicted Address = 0x" << hex << evict_addr << dec << endl;
            }

         }
         
         cache_result.second->setCState(cstate);
         cerr << "MMU: [" << the_core->getRank() << "] DebugSet: Setting State to: " << CacheState::cStateToString(cstate) << endl;

         cache_model_results = ocache->runDCachePeekModel(address);
         // if it is null, it means the address has been invalidated
         current_cstate = ( cache_model_results.second != NULL ) ?
                                 cache_model_results.second->getCState() : CacheState::INVALID;

         cerr << "MMU: [" << the_core->getRank() << "] Address = 0x" << hex << address << dec << ", Current State (Debug) = " << CacheState::cStateToString(current_cstate) << endl;

         // Now I have set the state as well as data
         break;
      
      default:
         cerr << "[" << the_core->getRank() << "] MMU: ERROR in switch for Core::debugSetCacheState";
   }
}

bool MemoryManager::debugAssertCacheState(IntPtr address, CacheState::cstate_t expected_cstate, char *expected_data) {

   // pair<bool,CacheTag*> cache_result = ocache->runDCachePeekModel(address, 1);
   // pair<bool,CacheTag*> cache_result = ocache->runDCachePeekModel(address);
   // We cant run peek model because it does not give us the data
   // Instead, we do "accessSingleLine" using a STORE request
   
   bool is_assert_true;
   bool fail_need_fill;
   CacheState::cstate_t actual_cstate;
   char actual_data[ocache->dCacheLineSize()];

   pair <bool, CacheTag*> cache_result;

   // assert ( (expected_cstate == CacheState::INVALID) == (expected_data == NULL) );

   cache_result = ocache->accessSingleLine(address, CacheBase::k_ACCESS_TYPE_LOAD, 
                     &fail_need_fill, NULL, 
                     actual_data, ocache->dCacheLineSize(), 
                     NULL, NULL, NULL); 
   
   // Note: (cache_result.second == NULL)  =>  (fail_need_fill == true)
   assert ( (cache_result.second == NULL) == (fail_need_fill == true) );
   // assert ( (cache_result.second == NULL) == (actual_data == NULL) );
   // assert ( (fail_need_fill == true) == (actual_data == NULL) );
   
   if( cache_result.second != NULL ) {
      actual_cstate = cache_result.second->getCState();
      is_assert_true = ( (actual_cstate  == expected_cstate) &&
                         (memcmp(actual_data, expected_data, ocache->dCacheLineSize()) == 0) );
      // assert (is_assert_true == true);
      
      cerr << "Actual Data: 0x";
      for (UInt32 i = 0; i < ocache->dCacheLineSize(); i++)
         cerr << hex << (UInt32) actual_data[i];
      cerr << endl;
      
      cerr << "Expected Data: 0x";
      for (UInt32 i = 0; i < ocache->dCacheLineSize(); i++)
         cerr << hex << (UInt32) expected_data[i];
      cerr << endl;

   } else {
      actual_cstate = CacheState::INVALID;
      // There is no point in looking at the expected data here
      is_assert_true = ( actual_cstate  == expected_cstate );
   }
   
   cerr << "   Asserting Cache[" << dec << the_core->getRank() << "] : Expected: " << CacheState::cStateToString(expected_cstate) << " ,  Actual: " <<  CacheState::cStateToString(actual_cstate);
   
   if(is_assert_true) {
      cerr << "                    TEST PASSED " << endl;
   } else {
      cerr << "                    TEST FAILED ****** " << endl;
   }

   assert (is_assert_true == true);
   return is_assert_true;
   
}

void MemoryManager::createUpdatePayloadBuffer (UpdatePayload* send_payload, char* data_buffer, char* payload_buffer, UInt32 payload_size)
{
   // Create a new buffer of size : sizeof(send_payload) + cache_line_size
   assert( payload_buffer != NULL );
   
   //copy send_payload
   memcpy ( (void*) payload_buffer, (void*) send_payload, sizeof(*send_payload) );

   //this is very important on the recieving end, so the extractor knows how big data_size is
   assert( send_payload->data_size == (payload_size - sizeof(*send_payload)) );
   
   //copy data_buffer over
   if(send_payload->data_size > g_knob_line_size) {
      cerr << "CreateUpdatePayloadBuffer: Error\n";
   }

   assert (send_payload->data_size <= g_knob_line_size);

   //copy data_buffer
   if(data_buffer != NULL) 
      memcpy ((void*) (payload_buffer + sizeof(*send_payload)), (void*) data_buffer, payload_size - sizeof(*send_payload));
   
}

void MemoryManager::createAckPayloadBuffer (AckPayload* send_payload, char* data_buffer, char* payload_buffer, UInt32 payload_size)
{
   // Create a new buffer of size : sizeof(send_payload) + cache_line_size
   assert( payload_buffer != NULL );
   
   //this is very important on the recieving end, so the extractor knows how big data_size is
   assert( send_payload->data_size == (payload_size - sizeof(*send_payload)) );

   //copy send_payload
   memcpy ((void*) payload_buffer, (void*) send_payload, sizeof(*send_payload));
   
   //copy data_buffer
   if(data_buffer != NULL) 
      memcpy ((void*) (payload_buffer + sizeof(*send_payload)), (void*) data_buffer, payload_size - sizeof(*send_payload));
   
}

void MemoryManager::extractUpdatePayloadBuffer (NetPacket* packet, UpdatePayload* payload, char* data_buffer) 
{ 
   //copy packet->data to payload (extract payload)
   memcpy ((void*) payload, (void*) (packet->data), sizeof(*payload));
   
   //copy data_buffer over
   assert (payload->data_size <= g_knob_line_size);

   assert ( (payload->data_size == 0) == (data_buffer == NULL) );
   if (payload->data_size > 0)
      memcpy ((void*) data_buffer, (void*) ( ((char*) packet->data) + sizeof(*payload) ), payload->data_size);
}

//TODO should we turn payloads from structs to classes so we don't have to have seperate methods to do this stuff?
void MemoryManager::extractAckPayloadBuffer (NetPacket* packet, AckPayload* payload, char* data_buffer) 
{ 
   memcpy ((void*) payload, (void*) (packet->data), sizeof(*payload));
   
   assert( payload->data_size <= g_knob_line_size );
   
   if(payload->data_size > 0)
      memcpy ((void*) data_buffer, (void*) ( ((char*) packet->data) + sizeof(*payload) ), payload->data_size);
}


