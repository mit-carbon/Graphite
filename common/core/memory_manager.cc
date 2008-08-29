#include "debug.h"
#include "memory_manager.h"

MemoryManager::MemoryManager(Core *the_core_arg, OCache *ocache_arg) {

   the_core = the_core_arg;
	ocache = ocache_arg;
	
	//FIXME; need to add infrastructure for specifying core architecture details (e.g. DRAM size)
	// this also assumes 1 dram per core

	// assume 4GB / dCacheLineSize  bytes/line 
	int total_num_cache_lines = (int) (pow(2,32) / ocache->dCacheLineSize()); 
	
	int dram_lines_per_core = total_num_cache_lines / the_core->getNumCores();
	assert( (dram_lines_per_core * the_core->getNumCores()) == total_num_cache_lines );

	// TODO: verify correct semantics with Jonathan
   assert( ocache != NULL );
	int num_cache_lines_per_core = ocache->dCacheSize() / ocache->dCacheLineSize();

	dram_dir = new DramDirectory(dram_lines_per_core, ocache->dCacheLineSize(), the_core_arg->getRank(), the_core_arg->getNumCores());
	cache_dir = new CacheDirectory(num_cache_lines_per_core, ocache->dCacheLineSize(), the_core_arg->getRank());
	addr_home_lookup = new AddressHomeLookup(total_num_cache_lines, the_core->getNumCores(), ocache->dCacheLineSize());
}

MemoryManager::~MemoryManager()
{
}

bool action_readily_permissable(CacheDirectoryEntry* cache_dir_entry_ptr, shmem_req_t shmem_req_type)
{
	bool ret;
	if ( shmem_req_type == READ )
	{
		ret = cache_dir_entry_ptr->readable();
	}
	else
	{
		if ( shmem_req_type == WRITE )
		{
			ret = cache_dir_entry_ptr->writable();
		}
		else 
		{
			throw("action_readily_permissable: unsupported memory transaction type.");
		}
	}
	return ret;
}


//FIXME deal with the size argument (ie, rename the darn thing)
bool MemoryManager::initiateSharedMemReq(ADDRINT address, UINT32 size, shmem_req_t shmem_req_type)
{
	debugPrint(the_core->getRank(), "MMU", "initiateSharedMemReq ++++++++++++++++++++");
   dram_dir->print();
   unsigned int my_rank = the_core->getRank();
   bool native_cache_hit;  // independent of shared memory, is the line available in the cache?
   
  cache_dir->print(); 
	
	cout << " SHMEM Request Type: " << shmem_req_type << endl;
	if ( shmem_req_type == READ )
   {
	  native_cache_hit = ocache->runDCacheLoadModel(address, size);
	  if(native_cache_hit) {
		  cout << "NATIVE CACHE HIT" << endl;
	  } else {
		  cout << "not a NATIVE CACHE HIT" << endl;
	  }
   }
   else
   {
	  if ( shmem_req_type == WRITE )
  	     native_cache_hit = ocache->runDCacheStoreModel(address, size);
      else
		  throw("unsupported memory transaction type.");
   }
   
   if ( !native_cache_hit )
   {
	   // simulate going to get it (cache tags updated automagically). need to update directory state
	   // TODO: deal with the case where the address is homed on another core
	   // simulate miss
	   
   }
 
   // FIXME: turn this into a cache method which standardizes the parsing of addresses into indeces
   int cache_index = address / ocache->dCacheLineSize();

   PacketType req_msg_type, resp_msg_type;
	
   // first, check local cache
   debugPrint(the_core->getRank(), "MMU", "getting $Entry (intiateSharedMemReq)");
	CacheDirectoryEntry* cache_dir_entry_ptr = cache_dir->getEntry(cache_index);
   debugPrint(the_core->getRank(), "MMU", "retrieved $Entry (initiateSharedMemReq)");
                      
   req_msg_type = SHARED_MEM_REQ;
   resp_msg_type = SHARED_MEM_UPDATE_EXPECTED;

   while( !action_readily_permissable(cache_dir_entry_ptr, shmem_req_type) )
   {
     // it was not readable in the cache, so find out where it should be, and send a read request to the home directory
     UINT32 home_node_rank = addr_home_lookup->find_home_for_addr(address);

#ifdef SMEM_DEBUG
	char value_str[20];
	char line[80];
	sprintf(value_str, "%x", address); //convert int to string (with hex formatting)
	sprintf(line, "address           : %s\n", value_str);

	 debugPrint(the_core->getRank(), "MMU", line);
	 debugPrint(the_core->getRank(), "MMU", "home_node_rank ", home_node_rank);
#endif
	 
	 assert(home_node_rank >= 0 && home_node_rank < (UINT32)(the_core->getNumCores()));
	   
	   // send message here to home node to request data
	   NetPacket packet;
	   packet.type = req_msg_type;
	   packet.sender = my_rank;
	   packet.receiver = home_node_rank;
	   packet.length = sizeof(int) * SH_MEM_REQ_NUM_INTS_SIZE;

	   // initialize packet payload
	   int payload[SH_MEM_REQ_NUM_INTS_SIZE];
	   payload[SH_MEM_REQ_IDX_REQ_TYPE] = shmem_req_type;
	   payload[SH_MEM_REQ_IDX_ADDR] = address;                         // TODO: cache line align?
	   payload[SH_MEM_REQ_IDX_NUM_BYTES_REQ] = ocache->dCacheLineSize();  // TODO: make sure getLineSize returns bytes
	   packet.data = (char *)(payload);
	   (the_core->getNetwork())->netSend(packet);

	   // receive the requested data (blocking receive)
	   NetMatch net_match;
	   net_match.sender = home_node_rank;
	   net_match.sender_flag = true;
	   net_match.type = resp_msg_type;
	   net_match.type_flag = true;
	   NetPacket recv_packet = (the_core->getNetwork())->netRecv(net_match);

	   // NOTE: we don't actually send back the data because we're just modeling performance (for now)
	   int* recv_payload = (int *)(recv_packet.data);

	   // TODO: properly cast from int to cstate_t type. may need a helper conversion method
	   CacheDirectoryEntry::cstate_t resp_c_state = (CacheDirectoryEntry::cstate_t)(recv_payload[SH_MEM_UPDATE_IDX_NEW_CSTATE]);
	   
	   assert(recv_packet.type == SHARED_MEM_UPDATE_EXPECTED);
	   
	   ADDRINT incoming_starting_addr = recv_payload[SH_MEM_UPDATE_IDX_ADDRESS];
	   // TODO: remove starting addr from data. this is just in there temporarily to debug
	   assert(incoming_starting_addr == address);
	   
	   // update cache_entry
	   // TODO: add support for actually updating data when we move to systems with software shared memory         
	   //BUG FIXME updating the copy
		cache_dir_entry_ptr->setCState(resp_c_state);         
	   
	   // TODO: update performance model

   }
   // if the while loop is never entered, the line is already in the cache in an appropriate state.
   // do nothing shared mem related

	dram_dir->print();
	debugPrint(the_core->getRank(), "MMU", "end of initiateSharedMemReq -------------");
   return native_cache_hit;
}


/*
 * this function is called by the "interrupt handler" when a shared memory request message is destined for this node
 * this function would be called on the home node (owner) of the address specified in the arguments
 * the shared request type indicates either a read or a write
 */

void MemoryManager::processSharedMemReq(NetPacket req_packet) {

  dram_dir->print();

#ifdef SMEM_DEBUG
  debugPrint(the_core->getRank(), "MMU", "Processing shared memory request.");
#endif
   
  // extract relevant values from incoming request packet
  shmem_req_t shmem_req_type = (shmem_req_t)((int *)(req_packet.data))[SH_MEM_REQ_IDX_REQ_TYPE];
  int address = ((int *)(req_packet.data))[SH_MEM_REQ_IDX_ADDR];
  unsigned int requestor = req_packet.sender;
  unsigned int my_rank = the_core->getRank();
  
  // 0. get the DRAM directory entry associated with this address
  debugPrint(the_core->getRank(), "MMU", "getting $Entry (processSMemReq)");
  DramDirectoryEntry* dram_dir_entry = dram_dir->getEntry(address);
  debugPrint(the_core->getRank(), "MMU", "retrieved $Entry (processSMemReq)");


  printf("Addr: %x  DState: %d, shmem_req_type %d\n", address, dram_dir_entry->getDState(), shmem_req_type);
  debugPrint(the_core->getRank(), "MMU", "printed dstate (processSMemReq)");
	
  // 1. based on requested operation (read or write), make necessary updates to current owners
  // 2. update directory state in DRAM and sharers array
	if ( shmem_req_type == READ ) {
  
		printf("Addr: %x  DState: %d, shmem_req_type %d\n", address, dram_dir_entry->getDState(), shmem_req_type);
    
		// handle the case where this line is in the exclusive state (so data has to be written back first and that entry is downgraded to SHARED)
      if(dram_dir_entry->getDState() == DramDirectoryEntry::EXCLUSIVE) {
	
			// make sure there is only one sharerer since this dram_directory_entry is in the exclusive state
			assert(dram_dir_entry->numSharers() == 1);
	
			unsigned int current_owner = dram_dir_entry->getExclusiveSharerRank();
	
			// reqeust a data write back data and downgrade to shared
			NetPacket packet;
			packet.type = SHARED_MEM_UPDATE_UNEXPECTED;
			packet.sender = my_rank;
			packet.receiver = current_owner;
			packet.length = sizeof(int) * SH_MEM_UPDATE_NUM_INTS_SIZE;
	
			// initialize packet payload for downgrade
			int payload[SH_MEM_UPDATE_NUM_INTS_SIZE];
			payload[SH_MEM_UPDATE_IDX_NEW_CSTATE] = CacheDirectoryEntry::SHARED;
			payload[SH_MEM_UPDATE_IDX_ADDRESS] = address;               // TODO: cache line align?
			packet.data = (char *)(payload);
			(the_core->getNetwork())->netSend(packet);
	
			// wait for the acknowledgement from the original owner that it downgraded itself to SHARED (from EXCLUSIVE)
			NetMatch net_match;
			net_match.sender = current_owner;
			net_match.sender_flag = true;
			net_match.type = SHARED_MEM_ACK;
			net_match.type_flag = true;
			NetPacket recv_packet = (the_core->getNetwork())->netRecv(net_match);
	
			// assert a few things in the ack packet (sanity checks)
			assert((unsigned int)(recv_packet.sender) == current_owner);
			assert(recv_packet.type == SHARED_MEM_ACK);
	
			int received_address = ((int *)(recv_packet.data))[SH_MEM_ACK_IDX_ADDRESS];
			assert(received_address == address);
	
			CacheDirectoryEntry::cstate_t received_new_cstate = (CacheDirectoryEntry::cstate_t)(((int *)(recv_packet.data))[SH_MEM_ACK_IDX_NEW_CSTATE]);
			assert(received_new_cstate == CacheDirectoryEntry::SHARED);
		}
      
      // TODO: is there a race condition here in the case when the directory gets updated and then
      // this thread gets swapped out? should we atomize the state update and the response message
      // this get executed no matter what state the dram directory entry was in
		
		debugPrint(the_core->getRank(), "MMU", "addSharer");
      dram_dir_entry->addSharer(requestor);
		debugPrint(the_core->getRank(), "MMU", "SetDState");
      dram_dir_entry->setDState(DramDirectoryEntry::SHARED);
		debugPrint(the_core->getRank(), "MMU", "finished mesing with dram_entry");
	}
	else
	{
      if ( shmem_req_type == WRITE ) {
		
	  // invalidate current sharers

      //FIXME!!! I'm taking out this and replacing sharers with a BitVector
		//need to address this code - CELIO
		
//		vector<unsigned int>::iterator sharers_iterator = dram_dir_entry.getSharersIterator();
//	  while( sharers_iterator != dram_dir_entry.getSharersSentinel() ) bracketwenthere
		cout << "Getting Sharrers List" << endl;		
      vector<UINT32> sharers_list = dram_dir_entry->getSharersList();
		
		for(UINT32 i = 0; i < sharers_list.size(); i++) {
	    
		 // send message to sharer to invalidate it
	    NetPacket packet;
	    packet.type = SHARED_MEM_UPDATE_UNEXPECTED;
	    packet.sender = my_rank;
	    packet.receiver = sharers_list[i];//*sharers_iterator;
	    packet.length = sizeof(int) * SH_MEM_UPDATE_NUM_INTS_SIZE;
	    
	    /* format of shared memory request packet data
	       req_type | starting addr | length (in bytes) requested
	    */
	    
	    // initialize packet payload for invalidation 
	    int payload[SH_MEM_UPDATE_NUM_INTS_SIZE];
	    payload[SH_MEM_UPDATE_IDX_NEW_CSTATE] = CacheDirectoryEntry::INVALID;
	    payload[SH_MEM_UPDATE_IDX_ADDRESS] = address;               // TODO: cache line align?
	    packet.data = (char *)(payload);
	    (the_core->getNetwork())->netSend(packet);
	    
//	    sharers_iterator++;
	  }
	  
	  // receive invalidation acks from all sharers
//	  sharers_iterator = dram_dir_entry.getSharersIterator();
//	  while( sharers_iterator != dram_dir_entry.getSharersSentinel()) bracketgoeshere
		for(UINT32 i = 0; i < sharers_list.size(); i++) {     
	    // TODO: optimize this by receiving acks out of order
	    
	    // wait for all of the invalidation acknowledgements
	    NetMatch net_match;
	    net_match.sender = sharers_list[i];//*sharers_iterator;
	    net_match.sender_flag = true;
	    net_match.type = SHARED_MEM_ACK;
	    net_match.type_flag = true;
	    NetPacket recv_packet = (the_core->getNetwork())->netRecv(net_match);
	    
	    // assert a few things in the ack packet (sanity checks)
	    assert((unsigned int)(recv_packet.sender) == sharers_list[i]); //*sharers_iterator);  // I would hope so
	    
	    int received_address = ((int *)(recv_packet.data))[SH_MEM_ACK_IDX_ADDRESS];
	    assert(received_address == address);
	    
	    CacheDirectoryEntry::cstate_t received_new_cstate = (CacheDirectoryEntry::cstate_t)(((int *)(recv_packet.data))[SH_MEM_ACK_IDX_NEW_CSTATE]);
       assert(received_new_cstate == CacheDirectoryEntry::INVALID);
	    
//	    sharers_iterator++;
	  }
	  
	  dram_dir_entry->addExclusiveSharer(requestor);
	  dram_dir_entry->setDState(DramDirectoryEntry::EXCLUSIVE);
	 }
      else
		{
			throw("unsupported memory transaction type.");
		}
   }
  
  // 3. return data back to requestor (note: actual data doesn't have to be sent at this time (6/08))
  NetPacket ret_packet;
  ret_packet.type = SHARED_MEM_UPDATE_EXPECTED;
  ret_packet.sender = my_rank;
  ret_packet.receiver = requestor;
  ret_packet.length = sizeof(int) * SH_MEM_UPDATE_NUM_INTS_SIZE;
  
  // initialize packet payload
  int payload[SH_MEM_UPDATE_NUM_INTS_SIZE];
  payload[SH_MEM_UPDATE_IDX_NEW_CSTATE] = dram_dir_entry->getDState();  // TODO: make proper mapping to cstate
  payload[SH_MEM_UPDATE_IDX_ADDRESS] = address;                  // TODO: cache line align?
  ret_packet.data = (char *)(payload);
  (the_core->getNetwork())->netSend(ret_packet);

	dram_dir->print();
	debugPrint(the_core->getRank(), "MMU", "end of sharedMemReq function");

}

/*
 * this function is called by the "interrupt handler" when an unexpected shared memory update arrives
 * (for example, an invalidation message). "expected" shared memory update messages are processed
 * in band by explicit receive messages
 */ 
void MemoryManager::processUnexpectedSharedMemUpdate(NetPacket update_packet) {
  
 debugPrint(the_core->getRank(), "MMU", "processUnexpectedSharedMemUpdate $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
 
	dram_dir->print();
 
 // verify packet type is correct
  assert(update_packet.type == SHARED_MEM_UPDATE_UNEXPECTED);
  
  // extract relevant values from incoming request packet
  CacheDirectoryEntry::cstate_t new_cstate = (CacheDirectoryEntry::cstate_t)(((int *)(update_packet.data))[SH_MEM_UPDATE_IDX_NEW_CSTATE]);
  int address = ((int *)(update_packet.data))[SH_MEM_UPDATE_IDX_ADDRESS];
  
  // FIXME: turn this into a cache method which standardizes the parsing of addresses into indeces
  int cache_index = address / ocache->dCacheLineSize();
  debugPrint(the_core->getRank(), "MMU", "getting $Entry (processUnexpectedSMemUpdate)");
  CacheDirectoryEntry* cache_dir_entry_ptr = cache_dir->getEntry(cache_index);
  debugPrint(the_core->getRank(), "MMU", "getting $Entry (processUnexpectedSMemUpdate)");
  cache_dir_entry_ptr->setCState(new_cstate);
  
  // send back acknowledgement of receiveing this message
  NetPacket packet;
  packet.type = SHARED_MEM_ACK;
  packet.sender = the_core->getRank();
  packet.receiver = update_packet.sender;
  //FIXME is this gonna choke on 64bit machines?
  packet.length = sizeof(int) * SH_MEM_ACK_NUM_INTS_SIZE;
  
  // initialize packet payload for downgrade
  int payload[SH_MEM_UPDATE_NUM_INTS_SIZE];
  payload[SH_MEM_ACK_IDX_NEW_CSTATE] = new_cstate;
  payload[SH_MEM_ACK_IDX_ADDRESS] = address;               // TODO: cache line align?
  packet.data = (char *)(payload);
  (the_core->getNetwork())->netSend(packet);
  
  // TODO: invalidate/flush from cache? Talk to Jonathan
	dram_dir->print();
	debugPrint(the_core->getRank(), "MMU", "end of processUnexpectedSharedMemUpdate");
}

// TODO: implement me
bool issueDramRequest(ADDRINT d_addr, shmem_req_t mem_req_type)
{
  cout << "TODO: implement me: MemoryManager.cc issueDramRequest"<< endl;
  return true;
}

/* 
 *
 * FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME 
 * FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME 
 *
 *
 */
/*
bool MemoryManager::runDCacheLoadModel(ADDRINT d_addr, UINT32 size)
{
 
   //entry point
   cout << "Fix Me: MMU: runDCacheLoadModel" << endl;
   return false;
}

bool MemoryManager::runDCacheStoreModel(ADDRINT d_addr, UINT32 size)
{
   cout << "Fix Me: MMU: runDCacheLoadModel" << endl;
  return false;
}


#ifdef I_DONT_UNDERSTAND_THIS_MODEL

bool MemoryManager::runDCacheLoadModel(ADDRINT d_addr, UINT32 size)
{
  bool ret;
  if( g_knob_simarch_has_shared_mem )
    {
      if( !ocache->runDCacheLoadModel(d_addr, size) )
	{
	  // not a hit. model this in the performance model
	  // TODO: Jonathan
	  ret = issueDramRequest(d_addr, READ);
	}
    } else
      {
	// non shared-memory
	ret = ocache->runDCacheLoadModel(d_addr, size);
      }
  return ret;
}


bool MemoryManager::runDCacheStoreModel(ADDRINT d_addr, UINT32 size)
{
  if ( g_knob_simarch_has_shared_mem )
    {
      if ( !ocache->runDCacheLoadModel(d_addr, size) )
	{
	  // not a hit. model this in the performance model
	  // TODO: Jonathan
	  return issueDramRequest(d_addr, WRITE);
	}
    }
  else
    {
      // non shared-memory
      return ocache->runDCacheStoreModel(d_addr, size);
    }
}



#endif
*/
