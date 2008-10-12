#include "memory_manager.h"
//#define MMU_DEBUG
//#define MMU_CACHEHIT_DEBUG
using namespace std;

MemoryManager::MemoryManager(Core *the_core_arg, OCache *ocache_arg) {

   the_core = the_core_arg;
	ocache = ocache_arg;
	
	//FIXME; need to add infrastructure for specifying core architecture details (e.g. DRAM size)
	// this also assumes 1 dram per core

	// assume 4GB / dCacheLineSize  bytes/line 
	int total_num_dram_lines = (int) (pow(2,32) / ocache->dCacheLineSize()); 
	
	int dram_lines_per_core = total_num_dram_lines / the_core->getNumCores();
	assert( (dram_lines_per_core * the_core->getNumCores()) == total_num_dram_lines );

   assert( ocache != NULL );

	//TODO can probably delete "dram_lines_per_core" b/c it not necessary.
	dram_dir = new DramDirectory(dram_lines_per_core, ocache->dCacheLineSize(), the_core_arg->getRank(), the_core_arg->getNumCores());

	//TODO bug: this may not gracefully handle cache lines that spill over from one core's dram to another
	addr_home_lookup = new AddressHomeLookup(the_core->getNumCores(), the_core->getRank());
}

MemoryManager::~MemoryManager()
{
}


void MemoryManager::debugPrintReqPayload(RequestPayload payload)
{
	stringstream ss;
	ss << " RequestPayload - RequestType(" << sMemReqTypeToString(payload.request_type)
		<< ") ADDR( " << hex << payload.request_address << ")"; 
	debugPrint(the_core->getRank(), "MMU", ss.str());
}


void addRequestPayload(NetPacket* packet, shmem_req_t shmem_req_type, ADDRINT address, UINT32 size_bytes)
{
	//TODO BUG this shit doesn't work b/c it gets deallocated before the network copies it
	cerr << "Starting adding Request Payload;" << endl;
	RequestPayload payload;
	payload.request_type = shmem_req_type;
	payload.request_address = address;  // TODO: cache line align?
	payload.request_num_bytes = size_bytes;

	packet->data = (char *)(&payload);
	cerr << "Finished adding Request Payload;" << endl;
}

void addAckPayload(NetPacket* packet, ADDRINT address, CacheState::cstate_t new_cstate)
{
	AckPayload payload;
	payload.ack_new_cstate = new_cstate;
	payload.ack_address = address; //only sent for debugging purposes

	packet->data = (char *)(&payload);
}

void addUpdatePayload(NetPacket* packet, ADDRINT address, CacheState::cstate_t new_cstate)
{
	UpdatePayload payload;
	payload.update_new_cstate = new_cstate;
	payload.update_address= address;
	
	packet->data = (char *)(&payload);
}

NetPacket makePacket(PacketType packet_type, int sender_rank, int receiver_rank, UINT32 payload_size)
{
	NetPacket packet;
	packet.type = packet_type;
	packet.sender = sender_rank;
	packet.receiver = receiver_rank;
	packet.length = payload_size;

	return packet;
}

//assumes we want to match both sender_rank and packet_type
NetMatch makeNetMatch(PacketType packet_type, int sender_rank)
{
	   NetMatch net_match;
	   net_match.sender = sender_rank;
	   net_match.sender_flag = true;
	   net_match.type = packet_type;
	   net_match.type_flag = true;

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
		   cerr << "ERROR in Actionreadily permissiable " << endl;
			throw("action_readily_permissable: unsupported memory transaction type.");
	      break;
	}

/*	if ( shmem_req_type == READ )
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
*/
	return ret;
}


//FIXME deal with the size argument (ie, rename the darn thing)
//TODO take out size, b/c we're only accessing a single cache line anyways
bool MemoryManager::initiateSharedMemReq(ADDRINT address, UINT32 size, shmem_req_t shmem_req_type)
{

#ifdef MMU_DEBUG
	debugPrint(the_core->getRank(), "MMU", "initiateSharedMemReq +++++++++");
//   dram_dir->print();
	debugPrintString(the_core->getRank(), "MMU", " SHMEM Request Type: ", MemoryManager::sMemReqTypeToString(shmem_req_type));
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
		  cerr << "ERROR: shmem_req_type is incorrect! (switch statement failure) " << endl;
		  throw("unsupported memory transaction type.");
		break;
   }
	
	stringstream ss;
   
#ifdef MMU_CACHEHIT_DEBUG                                    
	if(native_cache_hit) {
		//god i hate c++
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

	//TODO take this out, b/c its just debugging info
	if( cache_model_results.second == NULL) {
		//before we crash let's print out some important state info
		ss.str("");
		ss << "  Address: " << hex << address << " , type: " << sMemReqTypeToString(shmem_req_type) << ", size= " << size;
		debugPrint(my_rank, "MMU", ss.str());
	}
	assert( cache_model_results.second != NULL );
   
	
	//I think this should just be an "if" statement...
	while( !action_readily_permissable(cache_model_results.second->getCState(), shmem_req_type) )
   {
     // it was not readable in the cache, so find out where it should be, and send a read request to the home directory
     
      UINT32 home_node_rank = addr_home_lookup->find_home_for_addr(address);

		stringstream ss;
#ifdef MMU_DEBUG
		ss << "address           : " << hex << address << endl;
		debugPrint(the_core->getRank(), "MMU", ss.str());
		debugPrint(the_core->getRank(), "MMU", "home_node_rank ", home_node_rank);
#endif
	 
		assert(home_node_rank >= 0 && home_node_rank < (UINT32)(the_core->getNumCores()));
	   
	   // send message here to home node to request data
		//packet.type, sender, receiver, packet.length
	   NetPacket packet = makePacket(req_msg_type, my_rank, home_node_rank, sizeof(RequestPayload)); 
		
		// initialize packet payload
//		addRequestPayload(&packet, shmem_req_type, address, ocache->dCacheLineSize());
		//payload is on the stack, so it gets de-allocated once we exit this loop
		RequestPayload payload;
		payload.request_type = shmem_req_type;
		payload.request_address = address;  // TODO: cache line align? ie, why are we requesting entire cache line unless it *is* cache line aligned
		payload.request_num_bytes = ocache->dCacheLineSize(); //TODO this should actually be "size" from the inputs I believe (though size should be the same as cacheLineSize()

		packet.data = (char *)(&payload);

#ifdef MMU_DEBUG
		ss.clear();
		ss << "Addr: " << hex << address 
				<< " Payload.Addr: " << hex << payload.request_address
				<< " Packet.data: " << hex << ((RequestPayload*) packet.data)->request_address;
//		debugPrint(the_core->getRank(), "MMU", ss.str());
		debugPrintReqPayload(payload);

		ss.str("");
      ss << "   START netSend: to Tile<" << home_node_rank << "> " ;
		debugPrint(the_core->getRank(), "MMU::initiateSMemReq", ss.str());
#endif

		(the_core->getNetwork())->netSend(packet);

#ifdef MMU_DEBUG
		debugPrint(the_core->getRank(), "MMU::initiateSMemReq", "   END   netSend ");
#endif

	   // receive the requested data (blocking receive)
		NetMatch net_match = makeNetMatch( resp_msg_type, home_node_rank );
#ifdef MMU_DEBUG
		ss.str("");
		ss << "   START netRecv: from Tile <" << net_match.sender << "> " ; 
		debugPrint(the_core->getRank(), "MMU::initiateSMemReq", ss.str());
#endif
		NetPacket recv_packet = (the_core->getNetwork())->netRecv(net_match);
#ifdef MMU_DEBUG
		debugPrint(the_core->getRank(), "MMU::initiateSMemReq", "   END   netRecv");
#endif
	   // TODO: we don't actually send back the data because we're just modeling performance (for now)
	   UpdatePayload* recv_payload = (UpdatePayload*)(recv_packet.data);

	   //TODO: properly cast from int to cstate_t type. may need a helper conversion method
	   CacheState::cstate_t resp_c_state = (CacheState::cstate_t)(recv_payload->update_new_cstate);
	   
	   assert(recv_packet.type == SHARED_MEM_UPDATE_EXPECTED);
	   
	   ADDRINT incoming_starting_addr = recv_payload->update_address;
	   //TODO: remove starting addr from data. this is just in there temporarily to debug
		assert(incoming_starting_addr == address);
	   
	   // update cache_entry
	   //TODO: add support for actually updating data when we move to systems with software shared memory         
		//TODO: is there a better way to deal with null cache_model_results.second?
		assert( cache_model_results.second != NULL );
		cache_model_results.second->setCState(resp_c_state);         
	   
	   //TODO: update performance model

   }
   // if the while loop is never entered, the line is already in the cache in an appropriate state.

#ifdef MMU_DEBUG
//	dram_dir->print();
	debugPrint(the_core->getRank(), "MMU", "end of initiateSharedMemReq -------");
#endif
   return native_cache_hit;
}


/*
 * this function is called by the "interrupt handler" when a shared memory request message is destined for this node
 * this function would be called on the home node (owner) of the address specified in the arguments
 * the shared request type indicates either a read or a write
 */

//TODO make this a PQueue
void MemoryManager::addRequestToQueue(NetPacket packet)
{
	++incoming_requests_count;
	request_queue.push(packet);
}

NetPacket MemoryManager::getNextRequest()
{
	assert( incoming_requests_count > 0 );
	
	--incoming_requests_count;
	NetPacket packet = request_queue.front();
	request_queue.pop();

	return packet;
}

void MemoryManager::addMemRequest(NetPacket req_packet)
{
//	bool first_time = true; //for debugging purposes

	addRequestToQueue(req_packet);
	
	while( incoming_requests_count > 0 && !processing_request_flag )
	{
		//"Lock" the processRequest code
		processing_request_flag = true;
		
//		if( first_time ) {
//			debug_counter++;
//			first_time = false;
//		}
		
		processSharedMemReq(getNextRequest());
		
		//"Release" the processRequest code
		processing_request_flag = false;
	}
}

/*
 * this function is called by the "interrupt handler" when a shared memory request message is destined for this node
 * this function would be called on the home node (owner) of the address specified in the arguments
 * the shared request type indicates either a read or a write
 */
//TODO this should be put into the directory code? Is this when the directory gets a message requesting a dram memory line? I am receiving a Request Packet.
void MemoryManager::processSharedMemReq(NetPacket req_packet) 
{

	stringstream ss;
#ifdef MMU_DEBUG
  dram_dir->print();
  debugPrint(the_core->getRank(), "MMU", "Processing shared memory request.");
#endif
   
  // extract relevant values from incoming request packet
  shmem_req_t shmem_req_type = (shmem_req_t)((RequestPayload*)(req_packet.data))->request_type;
  ADDRINT address = ((RequestPayload*)(req_packet.data))->request_address;
  unsigned int requestor = req_packet.sender;
  unsigned int my_rank = the_core->getRank();
  
  // 0. get the DRAM directory entry associated with this address
#ifdef MMU_DEBUG
  debugPrint(the_core->getRank(), "MMU", "getting $Entry (processSMemReq)");
#endif
  DramDirectoryEntry* dram_dir_entry = dram_dir->getEntry(address);
#ifdef MMU_DEBUG
  debugPrint(the_core->getRank(), "MMU", "retrieved $Entry (processSMemReq)");

  printf("Process SharedMemReq (received a packet with this info->) Addr: %x  DState: %d, shmem_req_type %d\n", address, dram_dir_entry->getDState(), shmem_req_type);
  debugPrint(the_core->getRank(), "MMU", "printed dstate (processSMemReq)");
#endif

  // 1. based on requested operation (read or write), make necessary updates to current owners
  // 2. update directory state in DRAM and sharers array
	
	switch( shmem_req_type ) {
	
		case READ:
		{
	  
#ifdef MMU_DEBUG
			ss.str("");
			ss << "Addr: " << hex << address << "  DState: "  << DramDirectoryEntry::dStateToString( dram_dir_entry->getDState() ) << " shmem_req_type: " << sMemReqTypeToString( shmem_req_type );
			debugPrint(my_rank, "MMU", ss.str());
#endif		
		 
			// handle the case where this line is in the exclusive state (so data has to be written back first and that entry is downgraded to SHARED)
			if(dram_dir_entry->getDState() == DramDirectoryEntry::EXCLUSIVE) {
		
				// make sure there is only one sharerer since this dram_directory_entry is in the exclusive state
				assert(dram_dir_entry->numSharers() == 1);
		
				unsigned int current_owner = dram_dir_entry->getExclusiveSharerRank();
		
				// reqeust a data write back data and downgrade to shared
//				NetPacket packet;
//				packet.type = SHARED_MEM_UPDATE_UNEXPECTED;
//				packet.sender = my_rank;
//				packet.receiver = current_owner;
//				packet.length = sizeof(UpdatePayload);
				NetPacket packet = makePacket( SHARED_MEM_UPDATE_UNEXPECTED, my_rank, current_owner, sizeof(UpdatePayload));

				// initialize packet payload for downgrade
/*				int payload[SH_MEM_UPDATE_NUM_INTS_SIZE];
				payload[SH_MEM_UPDATE_IDX_NEW_CSTATE] = CacheState::SHARED;
				payload[SH_MEM_UPDATE_IDX_ADDRESS] = address;               // TODO: cache line align?
*/
				UpdatePayload payload;
				payload.update_new_cstate = CacheState::SHARED;
   			payload.update_address= address;
				packet.data = (char *)(&payload);
				ss.str("");
				ss << "   START netSend to <" << packet.receiver << "> ";
//				debugPrint(the_core->getRank(), "MMU::processSMemReq", ss.str());
				(the_core->getNetwork())->netSend(packet);
//				debugPrint(the_core->getRank(), "MMU::processSMemReq", "   END   netSend");
		

				/******* MAGIC HAPPENS HERE ******** 
				 * I think I'm receiving the wrong packet back (failed assert below for receive_addr != addr)
				 * the stupid ccrash that is */


				// wait for the acknowledgement from the original owner that it downgraded itself to SHARED (from EXCLUSIVE)
				NetMatch net_match;
				net_match.sender = current_owner;
				net_match.sender_flag = true;
				net_match.type = SHARED_MEM_ACK;
				net_match.type_flag = true;
				ss.str("");
				ss << "   START netRecv: from Core <" << net_match.sender << "> " ; 
//				debugPrint(the_core->getRank(), "MMU::processSMemReq", ss.str());
				NetPacket recv_packet = (the_core->getNetwork())->netRecv(net_match);
//				debugPrint(the_core->getRank(), "MMU::processSMemReq", "   END   netRecv");
		
				// assert a few things in the ack packet (sanity checks)
				assert((unsigned int)(recv_packet.sender) == current_owner);
				assert(recv_packet.type == SHARED_MEM_ACK);
		
				ADDRINT received_address = ((AckPayload*)(recv_packet.data))->ack_address;
				stringstream ss;
				//TODO bug here b/c we're servicing multiple outstanding requests (and netMatch gives us the wrong packet)
				if(received_address != address) {
					//print out info before we crash
					ss.str("");
					ss << "EXCLUSIVE OWNER: " << current_owner << ", ADDR: " << hex << address << "  , Received ADDR: " << hex << received_address << ", MemRequestType: " << sMemReqTypeToString( shmem_req_type ); 
//					debugPrint(my_rank, "MMU", ss.str());
				}	
				assert(received_address == address);
		
				CacheState::cstate_t received_new_cstate = (CacheState::cstate_t)(((AckPayload*)(recv_packet.data))->ack_new_cstate);
				assert(received_new_cstate == CacheState::SHARED);
				
            //did the former owner invalidate it already? if yes, we should remove him from the sharers list
				if( ((AckPayload*)(recv_packet.data))->remove_from_sharers )
				{
					dram_dir_entry->removeSharer( current_owner );
				}

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
			
#ifdef MMU_DEBUG
			ss.clear();
			ss << "ProcessSharedMemReq: Write Case, Addr: " << hex << address;
			debugPrint(the_core->getRank(), "MMU", ss.str());
#endif			 
			
			for(UINT32 i = 0; i < sharers_list.size(); i++) 
			{
			 
#ifdef MMU_DEBUG
				ss.clear();
				ss << "Sending Invalidation Message to Sharer [" << sharers_list[i] << "]";
				debugPrint(the_core->getRank(), "MMU", "Sending Invalidation Message to Sharer");
#endif			 
				// send message to sharer to invalidate it
				NetPacket packet = makePacket( SHARED_MEM_UPDATE_UNEXPECTED, my_rank, sharers_list[i], sizeof(UpdatePayload));
//				NetPacket packet;
//				packet.type = SHARED_MEM_UPDATE_UNEXPECTED;
//				packet.sender = my_rank;
//				packet.receiver = sharers_list[i];//*sharers_iterator;
	//			packet.length = sizeof(int) * SH_MEM_UPDATE_NUM_INTS_SIZE;
//				packet.length = sizeof(UpdatePayload);
				 
				/* format of shared memory request packet data
				   req_type | starting addr | length (in bytes) requested
				*/
				 
				// initialize packet payload for invalidation 
	/*			int payload[SH_MEM_UPDATE_NUM_INTS_SIZE];
				payload[SH_MEM_UPDATE_IDX_NEW_CSTATE] = CacheState::INVALID;
				payload[SH_MEM_UPDATE_IDX_ADDRESS] = address;               // TODO: cache line align?
	 */  		 
				 
				//TODO does receiver use the correct payload type?
				UpdatePayload payload;
				payload.update_new_cstate = CacheState::INVALID;
				payload.update_address = address;
				packet.data = (char *)(&payload);
				ss.str("");
				ss << "   START netSend to <" << packet.receiver << "> ";
//				debugPrint(the_core->getRank(), "MMU::processSMemReq", ss.str());
				(the_core->getNetwork())->netSend(packet);
//				debugPrint(the_core->getRank(), "MMU::processSMemReq", "   END   netSend");
		  }
		  
		  // receive invalidation acks from all sharers
		  for(UINT32 i = 0; i < sharers_list.size(); i++)      
		  {				
				// TODO: optimize this by receiving acks out of order
#ifdef MMU_DEBUG				
				ss.clear();
				ss << "Recieving Invalidation Ack Message from Sharer [" << sharers_list[i] << "]";
				debugPrint(the_core->getRank(), "MMU", ss.str());
#endif
				 
				// wait for all of the invalidation acknowledgements
				NetMatch net_match;
				net_match.sender = sharers_list[i];//*sharers_iterator;
				net_match.sender_flag = true;
				net_match.type = SHARED_MEM_ACK;
				net_match.type_flag = true;
				ss.str("");
				ss << "   START netRecv1 ( awaiting ACK from <" << net_match.sender << "> )";
//				debugPrint(the_core->getRank(), "MMU::processSMemReq", ss.str());
				NetPacket recv_packet = (the_core->getNetwork())->netRecv(net_match);
//				debugPrint(the_core->getRank(), "MMU::processSMemReq", "   END   netRecv1");
				 
				// assert a few things in the ack packet (sanity checks)
				assert((unsigned int)(recv_packet.sender) == sharers_list[i]); // I would hope so
				 
//				ADDRINT received_address = ((int *)(recv_packet.data))[SH_MEM_ACK_IDX_ADDRESS];
				ADDRINT received_address = ((AckPayload*)(recv_packet.data))->ack_address;
#ifdef MMU_DEBUG
				ss << " Recieving Invalidation Ack Messages.... Data Addr: " << hex << ((int *)(recv_packet.data)) << ", Received Addr: " << hex << received_address << "  Addr: " << hex << address;
				debugPrint(my_rank, "MMU", ss.str());
#endif				
				assert(received_address == address);
				 
//				 CacheState::cstate_t received_new_cstate = (CacheState::cstate_t)(((int *)(recv_packet.data))[SH_MEM_ACK_IDX_NEW_CSTATE]);
				 CacheState::cstate_t received_new_cstate = (CacheState::cstate_t)(((AckPayload*)(recv_packet.data))->ack_new_cstate);
				 assert(received_new_cstate == CacheState::INVALID);
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
  // 3. TODO send actual data here!
  NetPacket ret_packet;
  ret_packet.type = SHARED_MEM_UPDATE_EXPECTED;
  ret_packet.sender = my_rank;
  ret_packet.receiver = requestor;
//  ret_packet.length = sizeof(int) * SH_MEM_UPDATE_NUM_INTS_SIZE;
  ret_packet.length = sizeof(UpdatePayload);
  
  // initialize packet payload
/*  int payload[SH_MEM_UPDATE_NUM_INTS_SIZE];
  payload[SH_MEM_UPDATE_IDX_NEW_CSTATE] = dram_dir_entry->getDState();  // TODO: make proper mapping to cstate
  payload[SH_MEM_UPDATE_IDX_ADDRESS] = address;                  // TODO: cache line align?
*/

	UpdatePayload payload;
	CacheState::cstate_t new_cstate;

	//we need to map from dstate to cstate
	switch(dram_dir_entry->getDState()) {
		case DramDirectoryEntry::UNCACHED:
			assert ( 0 ); //so this situation ever happen?
			new_cstate = CacheState::INVALID;
			break;
		case DramDirectoryEntry::SHARED:
			new_cstate = CacheState::SHARED;
			break;
		case DramDirectoryEntry::EXCLUSIVE:
			new_cstate = CacheState::EXCLUSIVE;
			break;
		default:
			assert( 0 ); //something else must've gone wrong!
	}

	payload.update_new_cstate = new_cstate;
	payload.update_address = address;
	ret_packet.data = (char *)(&payload);
	ss.str("");
	ss << "   START netSend to <" << ret_packet.receiver << "> ";
//	debugPrint(the_core->getRank(), "MMU::processSMemReq", ss.str());
	(the_core->getNetwork())->netSend(ret_packet);
//	debugPrint(the_core->getRank(), "MMU::processSMemReq", "   END   netSend");

#ifdef MMU_DEBUG
	dram_dir->print();
	debugPrint(the_core->getRank(), "MMU", "end of sharedMemReq function");
#endif
//	debugPrint(the_core->getRank(), "MMU", "end of sharedMemReq function");
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
  
#ifdef MMU_DEBUG
	debugPrint(the_core->getRank(), "MMU", "processUnexpectedSharedMemUpdate $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
 
	dram_dir->print();
#endif
//	debugPrint(the_core->getRank(), "MMU", "processUnexpectedSharedMemUpdate $$$$$");

 // verify packet type is correct
  assert(update_packet.type == SHARED_MEM_UPDATE_UNEXPECTED);
  
  // extract relevant values from incoming request packet
  CacheState::cstate_t new_cstate = (CacheState::cstate_t)(((UpdatePayload*)(update_packet.data))->update_new_cstate);
  int address = ((UpdatePayload*)(update_packet.data))->update_address;

	stringstream ss;
#ifdef MMU_DEBUG
	ss << "Unexpected: address: " << hex << address;
	debugPrint(the_core->getRank(), "MMU", ss.str());
#endif

	pair<bool, CacheTag*> cache_model_results = ocache->runDCachePeekModel(address);
  // if it is null, it means the address has been invalidated
	
  // send back acknowledgement of receiveing this message
  NetPacket packet = makePacket( SHARED_MEM_ACK, the_core->getRank(), update_packet.sender, sizeof(AckPayload) );
//  NetPacket packet;
//  packet.type = SHARED_MEM_ACK;
//  packet.sender = the_core->getRank();
//  packet.receiver = update_packet.sender;
//  packet.length = sizeof(AckPayload);
  
  // initialize packet payload for downgrade
  AckPayload payload;
  payload.ack_new_cstate = new_cstate;
  payload.ack_address = address; //only sent for debugging purposes

  packet.data = (char *)(&payload);

  if( cache_model_results.second != NULL ) { 
		//downgrade the cache state 
		cache_model_results.second->setCState(new_cstate);
		payload.remove_from_sharers = false;
  } else {
		//address has been invalidated -> tell directory to remove us from sharers' list
		payload.remove_from_sharers = true;
  }
  
#ifdef MMU_DEBUG
  ss.str("");
  ss << " Payload Attached: data Addr: " << hex << (int) packet.data << ", Addr: " << hex << ((AckPayload*) (packet.data))->ack_address << " packet.length = " << sizeof(AckPayload);
  debugPrint(the_core->getRank(), "MMU", ss.str());
#endif

	ss.str("");
	ss << "   START netSend to <" << update_packet.sender << "> ";
//  debugPrint(the_core->getRank(), "MMU::processUnexpectedUpdate", ss.str());
  (the_core->getNetwork())->netSend(packet);
//  debugPrint(the_core->getRank(), "MMU::processUnexpectedUpdate", "   END   netSend");
  
  // TODO: invalidate/flush from cache? Talk to Jonathan
#ifdef MMU_DEBUG	
	dram_dir->print();
	debugPrint(the_core->getRank(), "MMU", "end of processUnexpectedSharedMemUpdate");
#endif	
	
//	debugPrint(the_core->getRank(), "MMU", "end of processUnexpectedSharedMemUpdate $$$$");

}

// TODO: implement DramRequest 
// if cache lookup is not a hit, we want to model dram request.
// and when we push around data, this function will deal with this
bool issueDramRequest(ADDRINT d_addr, shmem_req_t mem_req_type)
{
  cerr << "TODO: implement me: MemoryManager.cc issueDramRequest"<< endl;
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

void MemoryManager::debugSetDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list)
{
	dram_dir->debugSetDramState(addr, dstate, sharers_list);	
}

bool MemoryManager::debugAssertDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list)
{
	return dram_dir->debugAssertDramState(addr, dstate, sharers_list);	
}

void MemoryManager::setDramBoundaries( vector< pair<ADDRINT, ADDRINT> > addr_boundaries)
{
	cerr << " MMU: setting Dram Boundaries " << endl;
	if(addr_home_lookup == NULL) {
		cerr << "AddresHomeLookup is Null!" << endl;
	}
	if(addr_boundaries.size() == 0  ) {
		cerr << "add bounds are null!" << endl;
	}
	
	addr_home_lookup->setAddrBoundaries(addr_boundaries);
	cerr << " MMU: Finished setting Dram Boundaries " << endl;
}


