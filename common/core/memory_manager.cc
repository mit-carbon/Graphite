#include "memory_manager.h"

MemoryManager::MemoryManager(Core *the_core_arg, OCache *ocache_arg) {

	the_core = the_core_arg;
	ocache = ocache_arg;
	
	//FIXME; need to add infrastructure for specifying core architecture details (e.g. DRAM size)
	// this also assumes 1 dram per core
	
	// assume 4GB / 64 bytes/line = 67108864 
	int total_num_cache_lines = 67108864;
	
	int dram_lines_per_core = total_num_cache_lines / the_core->getNumCores();
	assert( (num_dram_lines * the_core->getNumCores()) == total_num_cache_lines );
	
	int num_cache_lines_per_core = ocache->getCacheSize() / ocache->getLineSize();

	dram_dir = new DramDirectory(dram_lines_per_core);
	cache_dir = new CacheDirectory(num_cache_lines_per_core);
	addr_home_lookup = new AddressHomeLookup(total_num_cache_lines, the_core->GetNumNodes());
}

MemoryManager::~MemoryManager()
{
}

void MemoryManager::initiateSharedMemReq(int address, shmem_req_t shmem_req_type)
{

   unsigned int my_rank = the_core->getRank();
   bool native_cache_hit;  // independent of shared memory, is the line available in the cache?
   if ( shmem_req_type == READ )
   {
	  native_cache_hit = ocache->runDCacheLoadModel(d_addr, size);
   }
   else
   {
	  if ( shmem_req_type == WRITE )
  	     native_cache_hit = ocache->runDCacheStoreModel(d_addr, size);
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
   int cache_index = address / ocache->getLineSize();

   Network::PacketType req_msg_type, resp_msg_type;
   bool (*action_readily_permissable_fn)();
	
   // first, check local cache
   CacheDirectoryEntry& cache_dir_entry = cache_dir.getEntry(cache_index);

   req_msg_type = SHARED_MEM_REQ;
   resp_msg_type = SHARED_MEM_UPDATE_EXPECTED;
   if ( shmem_req_type == READ )
   {
	   action_readily_permissable_fn = cache_dir_entry.readable;
   }
   else
   {
	   if ( shmem_req_type == WRITE )
	   {
		   action_readily_permissable_fn = cache_dir_entry.writable;
		   break;
	   }
	   else 
	   {
		   throw("unsupported memory transaction type.");
	   }
   }
   
   
   while( !action_readily_permissable_fn() )
   {
	   // it was not readable in the cache, so find out where it should be, and send a read request to the home directory
	   unsigned int home_node_rank = addr_home_lookup.find_home_for_addr(address);
	   assert(home_node_rank >= 0 && home_node_rank < the_core->getNumCores());

	   // TODO: optimize for case when home node is self? what are the assumptions about where DRAM exists?
	   
	   // send message here to home node to request data
	   Network::NetPacket packet;
	   packet.type = req_msg_type;
	   packet.sender = my_rank;
	   packet.receiver = home_node_rank;
	   packet.length = sizeof(int) * SH_MEM_REQ_NUM_INTS_SIZE;

	   // initialize packet payload
	   int payload[SH_MEM_REQ_NUM_INTS_SIZE];
	   payload[SH_MEM_REQ_IDX_REQ_TYPE] = shmem_req_type;
	   payload[SH_MEM_REQ_IDX_ADDR] = address;                         // TODO: cache line align?
	   payload[SH_MEM_REQ_IDX_NUM_BYTES_REQ] = ocache->getLineSize();  // TODO: make sure getLineSize returns bytes
	   packet.data = (char *)(payload);
	   the_core->network->netSend(packet);

	   // receive the requested data (blocking receive)
	   Network::NetMatch net_match;
	   net_match.sender = home_node_rank;
	   net_match.sender_flag = true;
	   net_match.type = resp_msg_type;
	   net_match.type_flag = true;
	   Network::NetPacket recv_packet = the_core->network->netRecv(net_match);

	   // NOTE: we don't actually send back the data because we're just modeling performance (for now)

	   int* recv_payload = (int *)(recv_packet.data);
	   cstate_t resp_c_state = recv_payload[SH_MEM_UPDATE_IDX_NEW_CSTATE];

	   assert(resp_c_state == SHARED_MEM_UPDATE_EXPECTED);
	   
	   int incoming_starting_addr = recv_payload[SH_MEM_UPDATE_IDX_ADDRESS];
	   // TODO: remove starting addr from data. this is just in there temporarily to debug
	   assert(incoming_starting_addr == address);
	   
	   // update cache_entry
	   // TODO: add support for actually updating data when we move to systems with software shared memory         
	   cache_entry.setCState(resp_c_state);         
	   
	   // TODO: update performance model

   }
   // if the while loop is never entered, the line is already in the cache in an appropriate state.
   // do nothing shared mem related
   
}


/*
 * this function is called by the "interrupt handler" when a shared memory request message is destined for this node
 * this function would be called on the home node (owner) of the address specified in the arguments
 * the shared request type indicates either a read or a write
 */

void MemoryManager::processSharedMemReq(Network::NetPacket req_packet) {
	
	// extract relevant values from incoming request packet
	shmem_req_t shmem_req_type = ((int *)(packet.data))[SH_MEM_REQ_IDX_REQ_TYPE];
	int address = ((int *)(packet.data))[SH_MEM_REQ_IDX_ADDR];
	unsigned int requestor = packet.sender;
	unsigned int my_rank = the_core->getRank();
	
	// 0. get the DRAM directory entry associated with this address
	DramDirectoryEntry dram_dir_entry = dram_dir.getEntry(address);
	
	// 1. based on requested operation (read or write), make necessary updates to current owners
    // 2. update directory state in DRAM and sharers array
	if ( shmem_req_type == READ )
	{
		// handle the case where this line is in the exclusive state (so data has to be written back first and that entry is downgraded to SHARED)
		if(dram_dir_entry.dstate == EXCLUSIVE) {

			// make sure there is only one sharerer since this dram_directory_entry is in the exclusive state
			assert(dram_dir_entry.sharers.size() == 1);

			unsigned int current_owner = dram_dir_entry.sharers[0];
				
			// reqeust a data write back data and downgrade to shared
			Network::NetPacket packet;
			packet.type = SHARED_MEM_UPDATE_UNEXPECTED;
			packet.sender = my_rank;
			packet.receiver = current_owner;
			packet.length = sizeof(int) * SH_MEM_UPDATE_NUM_INTS_SIZE;
			
			// initialize packet payload for downgrade
			int payload[SH_MEM_UPDATE_NUM_INTS_SIZE];
			payload[SH_MEM_UPDATE_IDX_NEW_CSTATE] = CacheDirectoryEntry::SHARED;
			payload[SH_MEM_UPDATE_IDX_ADDRESS] = address;               // TODO: cache line align?
			packet.data = (char *)(payload);
			the_core->network->netSend(packet);
			
			// wait for the acknowledgement from the original owner that it downgraded itself to SHARED (from EXCLUSIVE)
			Network::NetMatch net_match;
			net_match.sender = current_owner;
			net_match.sender_flag = true;
			net_match.type = SH_MEM_ACK;
			net_match.type_flag = true;
			Network::NetPacket recv_packet = the_core->network->netRecv(net_match);

			// assert a few things in the ack packet (sanity checks)
			assert(recv_packet.sender == current_owner);
			assert(recv_packet.type == SHARED_MEM_ACK);
			
			int received_address = ((int *)(recv_packet.data))[SH_MEM_ACK_IDX_ADDRESS];
			assert(received_address == address);
			
			cstate_t recieved_new_cstate = ((int *)(recv_packet.data))[SH_MEM_ACK_IDX_NEW_CSTATE];
			assert(received_new_cstate == CacheDirectoryEntry::SHARED);
		}

		// TODO: is there a race condition here in the case when the directory gets updated and then
		// this thread gets swapped out? should we atomize the state update and the response message
		// this get executed no matter what state the dram directory entry was in
		dram_dir_entry.addSharer(requestor);
		dram_dir_entry.dstate = SHARED;
	}
	else
	{
		if ( shmem_req_type == WRITE )
		{
			// invalidate current sharers
			vector<unsigned int>::iterator sharers_iterator;
			sharers_iterator = sharers.begin();
			while( sharers_iterator != sharers.end()) {
				
				// send message to sharer to invalidate it
				Network::NetPacket packet;
				packet.type = SHARED_MEM_UPDATE_UNEXPECTED;
				packet.sender = my_rank;
				packet.receiver = *sharers_iterator;
				packet.length = sizeof(int) * SH_MEM_UPDATE_NUM_INTS_SIZE;
				
				/* format of shared memory request packet data
				   req_type | starting addr | length (in bytes) requested
				*/
				
				// initialize packet payload for invalidation 
				int payload[SH_MEM_UPDATE_NUM_INTS_SIZE];
				payload[SH_MEM_UPDATE_IDX_NEW_CSTATE] = CacheDirectoryEntry::INVALID;
				payload[SH_MEM_UPDATE_IDX_ADDRESS] = address;               // TODO: cache line align?
				packet.data = (char *)(payload);
				the_core->network->netSend(packet);
								
				sharers_iterator++;
			}

			// receive invalidation acks from all sharers
			sharers_iterator = sharers.begin();
			while( sharers_iterator != sharers.end()) {

				// TODO: optimize this by receiving acks out of order
				
				// wait for all of the invalidation acknowledgements
				Network::NetMatch net_match;
				net_match.sender = *sharers_iterator;
				net_match.sender_flag = true;
				net_match.type = SH_MEM_ACK;
				net_match.type_flag = true;
				Network::NetPacket recv_packet = the_core->network->netRecv(net_match);
				
				// assert a few things in the ack packet (sanity checks)
				assert(recv_packet.sender == *sharers_iterator);  // I would hope so
			
				int received_address = ((int *)(recv_packet.data))[SH_MEM_ACK_IDX_ADDRESS];
				assert(received_address == address);
				
				cstate_t recieved_new_cstate = ((int *)(recv_packet.data))[SH_MEM_ACK_IDX_NEW_CSTATE];
				assert(received_new_cstate == CacheDirectoryEntry::INVALID);
				
				sharers_iterator++;
			}
			
			dram_dir_entry.addExclusiveSharer(requestor);
			dram_dir_entry.dstate = EXCLUSIVE;
		}
		else
		{
			throw("unsupported memory transaction type.");
		}
	}
	
    // 3. return data back to requestor (note: actual data doesn't have to be sent at this time (6/08))
	Network::NetPacket packet;
	packet.type = SHARED_MEM_UPDATE_EXPECTED;
	packet.sender = my_rank;
	packet.receiver = requestor;
	packet.length = sizeof(int) * SH_MEM_UPDATE_NUM_INTS_SIZE;
	
	// initialize packet payload
	int payload[SH_MEM_UPDATE_NUM_INTS_SIZE];
	payload[SH_MEM_UPDATE_IDX_NEW_CSTATE] = dram_dir_entry.dstate; // TODO: make proper mapping to cstate
	payload[SH_MEM_UPDATE_DIX_ADDRESS] = address;                  // TODO: cache line align?
	packet.data = (char *)(payload);
	the_core->network->netSend(packet);
}


/*
 * this function is called by the "interrupt handler" when an unexpected shared memory update arrives
 * (for example, an invalidation message). "expected" shared memory update messages are processed
 * in band by explicit receive messages
 */ 
void MemoryManager::processUnexpectedSharedMemUpdate(Network::NetPacket update_packet) {

	// verify packet type is correct
	assert(update_packet.type == SHARED_MEM_UPDATE_UNEXPECTED);
		   
	// extract relevant values from incoming request packet
	CacheDirectoryEntry::cstate_t new_cstate = ((int *)(update_packet.data))[SH_MEM_UPDATE_IDX_NEW_CSTATE];
	int address = ((int *)(update_packet.data))[SH_MEM_UPDATE_IDX_ADDRESS];

	// FIXME: turn this into a cache method which standardizes the parsing of addresses into indeces
	int cache_index = address / ocache->getLineSize();
	CacheDirectoryEntry& cache_dir_entry = cache_dir.getDirectorEntry(cache_index);
	cache_entry.setCState(new_cstate);
	
	// send back acknowledgement of receiveing this message
	Network::NetPacket packet;
	packet.type = SHARED_MEM_ACK;
	packet.sender = the_core->getRank();
	packet.receiver = update_packet.sender;
	packet.length = sizeof(int) * SH_MEM_ACK_NUM_INTS_SIZE;
	
	// initialize packet payload for downgrade
	int payload[SH_MEM_UPDATE_NUM_INTS_SIZE];
	payload[SH_MEM_ACK_IDX_NEW_CSTATE] = new_cstate;
	payload[SH_MEM_ACK_IDX_ADDRESS] = address;               // TODO: cache line align?
	packet.data = (char *)(payload);
	the_core->network->netSend(packet);

	// TODO: invalidate/flush from cache? Talk to Jonathan
}

bool MemoryManager::runDCacheLoadModel(ADDRINT d_addr, UINT32 size)
{
   if ( g_knob_simarch_has_shared_mem )
   {
	  if ( !ocache->runDCacheLoadModel(d_addr, size))
	  {
		  memRequest(d_addr, READ);
      }
   }
   else
   {
	  // non shared-memory
      return ocache->runDCacheLoadModel(d_addr, size);
   }
}

bool MemoryManager::runDCacheStoreModel(ADDRINT d_addr, UINT32 size)
{
   if ( g_knob_simarch_has_shared_mem )
   {
	  if ( !ocache->runDCacheLoadModel(d_addr, size) )
	  {
 		  memRequest(d_addr, WRITE);
      }
   }
   else
   {
	  // non shared-memory
      return ocache->runDCacheStoreModel(d_addr, size);
   }
}



/*
 * Private Methods
 */

