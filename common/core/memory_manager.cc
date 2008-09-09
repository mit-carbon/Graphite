#include "memory_manager.h"

using namespace std;

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

	dram_dir = new DramDirectory(dram_lines_per_core, ocache->dCacheLineSize(), the_core_arg->getRank(), the_core_arg->getNumCores());
	addr_home_lookup = new AddressHomeLookup(total_num_cache_lines, the_core->getNumCores(), ocache->dCacheLineSize());
}

MemoryManager::~MemoryManager()
{
}

bool action_readily_permissable(CacheState cache_state, shmem_req_t shmem_req_type)
{
	bool ret;
	if ( shmem_req_type == READ )
	{
		ret = cache_state.readable();
	}
	else
	{
		if ( shmem_req_type == WRITE )
		{
			ret = cache_state.writable();
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
//	dram_dir->print();
#ifdef MMU_DEBUG
	debugPrint(the_core->getRank(), "MMU", "initiateSharedMemReq ++++++++++++++++++++");
   dram_dir->print();
	debugPrintString(my_rank, "MMU", " SHMEM Request Type: ", MemoryManager::sMemReqTypeToString(shmem_req_type));
#endif

   unsigned int my_rank = the_core->getRank();
   bool native_cache_hit;  // independent of shared memory, is the line available in the cache?
	//first-> is the line available in the cache? (Native cache hit)
	//second-> pointer to cache tag to update cache state
   pair<bool, CacheTag*> cache_model_results;  // independent of shared memory, is the line available in the cache?
															  
	switch( shmem_req_type ) {
		
		case READ:
			cache_model_results = ocache->runDCacheLoadModel(address, size);
			native_cache_hit = cache_model_results.first;
		break;
		
		case WRITE:
  	     cache_model_results = ocache->runDCacheStoreModel(address, size);
  	     native_cache_hit = cache_model_results.first;
		break;
		
		default: 
		  throw("unsupported memory transaction type.");
		break;
   }
   
#ifdef MMU_CACHEHIT_DEBUG                                    
			if(native_cache_hit) {
				//god i hate c++
				stringstream ss;
				ss << "NATIVE CACHE (HIT) : ADDR: = " <<  hex << address 
					<< " - CState: "<< CacheState::cStateToString(cache_model_results.second->getCState());
				debugPrint(my_rank,"MMU", ss.str()); 
			} else {
				stringstream ss;
				ss << "NATIVE CACHE (MISS): ADDR: = " << hex << address 
					<< " - CState: " << CacheState::cStateToString(cache_model_results.second->getCState());
				debugPrint(my_rank,"MMU", ss.str());
			}
#endif
   
	if ( !native_cache_hit )
   {
	   // simulate going to get it (cache tags updated automagically). need to update directory state
	   // TODO: deal with the case where the address is homed on another core
	   // simulate miss
	   
   }
 
   PacketType req_msg_type, resp_msg_type;
	
   // first, check local cache
   req_msg_type = SHARED_MEM_REQ;
   resp_msg_type = SHARED_MEM_UPDATE_EXPECTED;

	assert( cache_model_results.second != NULL );
   while( !action_readily_permissable(cache_model_results.second->getCState(), shmem_req_type) )
   {
     // it was not readable in the cache, so find out where it should be, and send a read request to the home directory
     UINT32 home_node_rank = addr_home_lookup->find_home_for_addr(address);

#ifdef MMU_DEBUG
	stringstream ss;
	ss << "address           : " << hex << address << endl;
	debugPrint(the_core->getRank(), "MMU", ss.str());
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
	   CacheState::cstate_t resp_c_state = (CacheState::cstate_t)(recv_payload[SH_MEM_UPDATE_IDX_NEW_CSTATE]);
	   
	   assert(recv_packet.type == SHARED_MEM_UPDATE_EXPECTED);
	   
	   ADDRINT incoming_starting_addr = recv_payload[SH_MEM_UPDATE_IDX_ADDRESS];
	   // TODO: remove starting addr from data. this is just in there temporarily to debug
	   assert(incoming_starting_addr == address);
	   
	   // update cache_entry
	   // TODO: add support for actually updating data when we move to systems with software shared memory         
		//TODO: is there a better way to deal with null cache_model_results.second?
		assert( cache_model_results.second != NULL );
		cache_model_results.second->setCState(resp_c_state);         
	   
	   // TODO: update performance model

   }
   // if the while loop is never entered, the line is already in the cache in an appropriate state.
   // do nothing shared mem related

//	dram_dir->print();
#ifdef MMU_DEBUG
	dram_dir->print();
	debugPrint(the_core->getRank(), "MMU", "end of initiateSharedMemReq -------------");
#endif
   return native_cache_hit;
}


/*
 * this function is called by the "interrupt handler" when a shared memory request message is destined for this node
 * this function would be called on the home node (owner) of the address specified in the arguments
 * the shared request type indicates either a read or a write
 */

void MemoryManager::processSharedMemReq(NetPacket req_packet) {

#ifdef MMU_DEBUG
  dram_dir->print();
  debugPrint(the_core->getRank(), "MMU", "Processing shared memory request.");
#endif
   
  // extract relevant values from incoming request packet
  shmem_req_t shmem_req_type = (shmem_req_t)((int *)(req_packet.data))[SH_MEM_REQ_IDX_REQ_TYPE];
  int address = ((int *)(req_packet.data))[SH_MEM_REQ_IDX_ADDR];
  unsigned int requestor = req_packet.sender;
  unsigned int my_rank = the_core->getRank();
  
  // 0. get the DRAM directory entry associated with this address
#ifdef MMU_DEBUG
  debugPrint(the_core->getRank(), "MMU", "getting $Entry (processSMemReq)");
#endif
  DramDirectoryEntry* dram_dir_entry = dram_dir->getEntry(address);
#ifdef MMU_DEBUG
  debugPrint(the_core->getRank(), "MMU", "retrieved $Entry (processSMemReq)");

  printf("Addr: %x  DState: %d, shmem_req_type %d\n", address, dram_dir_entry->getDState(), shmem_req_type);
  debugPrint(the_core->getRank(), "MMU", "printed dstate (processSMemReq)");
#endif

  // 1. based on requested operation (read or write), make necessary updates to current owners
  // 2. update directory state in DRAM and sharers array
	switch( shmem_req_type ) {
	
		case READ:
		{
	  
#ifdef MMU_DEBUG
			printf("Addr: %x  DState: %d, shmem_req_type %d\n", address, dram_dir_entry->getDState(), shmem_req_type);
#endif		
		 
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

#ifdef MMU_DEBUG
			debugPrint(the_core->getRank(), "MMU", "addSharer & Set DState");
#endif
			dram_dir_entry->addSharer(requestor);
			dram_dir_entry->setDState(DramDirectoryEntry::SHARED);
	   } 
		break;
      
		case WRITE: 
		{	
		  // invalidate current sharers
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
			 
		  }
		  
		  // receive invalidation acks from all sharers
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
				assert((unsigned int)(recv_packet.sender) == sharers_list[i]); // I would hope so
				 
				int received_address = ((int *)(recv_packet.data))[SH_MEM_ACK_IDX_ADDRESS];
				stringstream ss;
				ss << "Data Addr: " << hex << ((int *)(recv_packet.data)) << ", Received Addr: " << hex << received_address << "  Addr: " << hex << address;
				debugPrint(my_rank, "MMU", ss.str());
				assert(received_address == address);
				 
				 CacheDirectoryEntry::cstate_t received_new_cstate = (CacheDirectoryEntry::cstate_t)(((int *)(recv_packet.data))[SH_MEM_ACK_IDX_NEW_CSTATE]);
				 assert(received_new_cstate == CacheDirectoryEntry::INVALID);
		  }
		  
		  dram_dir_entry->addExclusiveSharer(requestor);
		  dram_dir_entry->setDState(DramDirectoryEntry::EXCLUSIVE);
		}
		break;

		default:
		{
			throw("unsupported memory transaction type.");
		}
		break;
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

#ifdef MMU_DEBUG
	dram_dir->print();
	debugPrint(the_core->getRank(), "MMU", "end of sharedMemReq function");
#endif
}

/*
 * this function is called by the "interrupt handler" when an unexpected shared memory update arrives
 * (for example, an invalidation message). "expected" shared memory update messages are processed
 * in band by explicit receive messages
 */ 
void MemoryManager::processUnexpectedSharedMemUpdate(NetPacket update_packet) {
  
#ifdef MMU_DEBUG
	debugPrint(the_core->getRank(), "MMU", "processUnexpectedSharedMemUpdate $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
 
//	dram_dir->print();
#endif
	dram_dir->print();

 // verify packet type is correct
  assert(update_packet.type == SHARED_MEM_UPDATE_UNEXPECTED);
  
  // extract relevant values from incoming request packet
  CacheState::cstate_t new_cstate = (CacheState::cstate_t)(((int *)(update_packet.data))[SH_MEM_UPDATE_IDX_NEW_CSTATE]);
  int address = ((int *)(update_packet.data))[SH_MEM_UPDATE_IDX_ADDRESS];

	stringstream ss;
	ss << "Unexpected: address: " << hex << address;
	debugPrint(the_core->getRank(), "MMU", ss.str());
  // FIXME: turn this into a cache method which standardizes the parsing of addresses into indeces
//  int cache_index = address / ocache->dCacheLineSize();
//  debugPrint(the_core->getRank(), "MMU", "getting $Entry (processUnexpectedSMemUpdate)");
//  CacheDirectoryEntry* cache_dir_entry_ptr = cache_dir->getEntry(cache_index);
//  debugPrint(the_core->getRank(), "MMU", "getting $Entry (processUnexpectedSMemUpdate)");
  
  //FIXME: adding in Jonathan's new state stuff in the ocache......
  //is it okay to model the ocache here, required to get a pointer
	pair<bool, CacheTag*> cache_model_results = ocache->runDCacheLoadModel(address, 4); //FIXME what the hell is size??
//  cache_dir_entry_ptr->setCState(new_cstate);
	assert( cache_model_results.second != NULL );
	cache_model_results.second->setCState(new_cstate);
  
  // send back acknowledgement of receiveing this message
  NetPacket packet;
  packet.type = SHARED_MEM_ACK;
  packet.sender = the_core->getRank();
  packet.receiver = update_packet.sender;
  //FIXME is this gonna choke on 64bit machines?
  packet.length = sizeof(int) * SH_MEM_ACK_NUM_INTS_SIZE;
  
  // initialize packet payload for downgrade
  // FIXME make this malloced? in which case we need to free it!
  int* payload;
  payload = new int[SH_MEM_UPDATE_NUM_INTS_SIZE];
  assert( payload != NULL );
 //TODO FREE THE PAYLOAD
//  int payload[SH_MEM_UPDATE_NUM_INTS_SIZE];
  payload[SH_MEM_ACK_IDX_NEW_CSTATE] = new_cstate;
  payload[SH_MEM_ACK_IDX_ADDRESS] = address;               // TODO: cache line align?
  packet.data = (char *)(payload);
  
  ss.str("");
  ss << " Payload Attached: data Addr: " << hex << (int) packet.data << ", Addr: " << hex << ((int*) (packet.data))[SH_MEM_ACK_IDX_ADDRESS];
  debugPrint(the_core->getRank(), "MMU", ss.str());

  (the_core->getNetwork())->netSend(packet);
  
  // TODO: invalidate/flush from cache? Talk to Jonathan
#ifdef MMU_DEBUG	
	dram_dir->print();
	debugPrint(the_core->getRank(), "MMU", "end of processUnexpectedSharedMemUpdate");
#endif	
//	dram_dir->print();
}

// TODO: implement DramRequest 
// if cache lookup is not a hit, we want to model dram request.
// and when we push around data, this function will deal with this
bool issueDramRequest(ADDRINT d_addr, shmem_req_t mem_req_type)
{
  cout << "TODO: implement me: MemoryManager.cc issueDramRequest"<< endl;
  return true;
}

string MemoryManager::sMemReqTypeToString(shmem_req_t type)
{
	switch(type) {
		case READ: return	"READ      ";
		case WRITE: return "WRITE     ";
		case INVALIDATE: return "INVALIDATE";
		default: return "ERROR SMEMREQTYPE";
	}
	return "ERROR SMEMREQTYPE";
}
