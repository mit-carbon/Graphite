#include "memory_manager.h"
//#define MMU_DEBUG
//#define MMU_CACHEHIT_DEBUG
//#define ADDR_HOME_LOOKUP_DEBUG

using namespace std;

/* TODO for data sharing integration
 *
 * 1. put data into update-payload (requests)
 *    put data into ack-payload (when we sent invalidate/demote to an exclusive, need to do WB)
 *
 * 2. write back to DRAM
 *			WB on evictions
 *			WB on demotions (E->{S,I}) (owner sends data back)
 *
 */

MemoryManager::MemoryManager(Core *the_core_arg, OCache *ocache_arg) {

   the_core = the_core_arg;
	ocache = ocache_arg;
	
	//FIXME; need to add infrastructure for specifying core architecture details (e.g. DRAM size)
	// this also assumes 1 dram per core

	/* ================================================================= */
	/* Added by George */
	dramAccessCost = 0;
	/* ================================================================= */

	// assume 4GB / dCacheLineSize  bytes/line 
	int total_num_dram_lines = (int) (pow(2,32) / ocache->dCacheLineSize()); 
	
	int dram_lines_per_core = total_num_dram_lines / the_core->getNumCores();
	assert( (dram_lines_per_core * the_core->getNumCores()) == total_num_dram_lines );

   assert( ocache != NULL );

	//TODO can probably delete "dram_lines_per_core" b/c it not necessary.
	dram_dir = new DramDirectory(dram_lines_per_core, ocache->dCacheLineSize(), the_core_arg->getRank(), the_core_arg->getNumCores());

	
	/**** Data Passing Stuff ****/
	eviction_buffer = new char[g_knob_line_size];
	fill_buffer = new char[g_knob_line_size];
	
	//TODO bug: this may not gracefully handle cache lines that spill over from one core's dram to another
	addr_home_lookup = new AddressHomeLookup(the_core->getNumCores(), g_knob_ahl_param.Value(), the_core->getRank());
#ifdef ADDR_HOME_LOOKUP_DEBUG
	cerr << "Creating New Addr Home Lookup Structure: " << the_core->getNumCores() << ", " << g_knob_ahl_param.Value() << ", " << the_core->getRank() << "\n";
#endif
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
	//TODO BUG this code doesn't work b/c it gets deallocated before the network copies it
	cerr << "Starting adding Request Payload;" << endl;
	MemoryManager::RequestPayload payload;
	payload.request_type = shmem_req_type;
	payload.request_address = address;  // TODO: cache line align?
	payload.request_num_bytes = size_bytes;

	packet->data = (char *)(&payload);
	cerr << "Finished adding Request Payload;" << endl;
}

void addAckPayload(NetPacket* packet, ADDRINT address, CacheState::cstate_t new_cstate)
{
	MemoryManager::AckPayload payload;
	payload.ack_new_cstate = new_cstate;
	payload.ack_address = address; //only sent for debugging purposes

	packet->data = (char *)(&payload);
}

void addUpdatePayload(NetPacket* packet, ADDRINT address, CacheState::cstate_t new_cstate)
{
	MemoryManager::UpdatePayload payload;
	payload.update_new_cstate = new_cstate;
	payload.update_address= address;
	
	packet->data = (char *)(&payload);
}

NetPacket makePacket(PacketType packet_type, UINT32 payload_size, int sender_rank, int receiver_rank )
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

	return ret;
}


//bool MemoryManager::initiateSharedMemReq(ADDRINT address, UINT32 size, shmem_req_t shmem_req_type)

//buffer_size is the size in bytes of the char* data_buffer
//we may give the cache less than a cache_line of data, but never more than a cache_line_size of data
//ca_address is "cache-aligned" address
//addr_offset provides the offset that points to the requested address
//TODO this will not work correctly for multi-line requests!
bool MemoryManager::initiateSharedMemReq(shmem_req_t shmem_req_type, ADDRINT ca_address, UINT32 addr_offset, char* data_buffer, UINT32 buffer_size)
{

	/*
	 
    1. check cache for ca_address;

	 2. if permissable -> return

	 3. we need to request data and/or permission to use the ca_address
	 (can I assume we just need to reget the entire thing from dram?)

	 4. Send message to dram_directory and request address.

	 5. extract the data from the update_data_payload


	 * 
	 *
	 *
	 *
	 *
	 *
	 *
	 *
	 *
	 */
	
	
	stringstream ss;
   
#ifdef MMU_DEBUG
	debugPrint(the_core->getRank(), "MMU", "initiateSharedMemReq +++++++++");
	debugPrintString(the_core->getRank(), "MMU", " SHMEM Request Type: ", MemoryManager::sMemReqTypeToString(shmem_req_type));
#endif

	//data_buffer is null if req_type == READ
	assert( shmem_req_type == READ ? data_buffer == NULL : true ); 
	
	unsigned int my_rank = the_core->getRank();
   
   //TODO rename CacheTag as CacheLineInfo
	pair<bool, CacheTag*> cache_model_results;  
	//first-> is the line available in the cache? (Native cache hit)
	//second-> pointer to cache tag to update cache state
	bool native_cache_hit;  
															  
	CacheBase::AccessType access_type;
	bool fail_need_fill;
   bool eviction;
	ADDRINT evict_addr;
	char* scratch_line[g_knob_line_size];
	char* data_buffer[g_knob_line_size];	
   
	switch( shmem_req_type ) {
		case READ:	access_type = CacheBase::k_ACCESS_TYPE_LOAD; break;
		case WRITE:	access_type = CacheBase::k_ACCESS_TYPE_STORE; break;
		default: throw("unsupported memory transaction type."); break;
   }
	
	cache_model_results = getCacheLineInfo(); //check hit status, get CState
//	cache_model_results = ocache->accessSingleLine(address, access_type,
//  																&fail_need_fill, NULL,
//																	data_buffer, g_knob_line_size,
//																	&eviction, &evict_addr, eviction_buffer);
																	
	//TODO question: is fail_need_fill the same as cache_hit??
	native_cache_hit = cache_model_results.first;
	assert( native_cache_hit != fail_need_fill );
	assert( cache_model_results.second != NULL );
	
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

   PacketType req_msg_type, resp_msg_type;
   req_msg_type = SHARED_MEM_REQ;
   resp_msg_type = SHARED_MEM_UPDATE_EXPECTED;

	if( action_readily_permissable(cache_model_results.second->getCState(), shmem_req_type) )
   {
		assert( native_cache_hit == true );
		return native_cache_hit;
	}
	else
	{
     // it was not readable in the cache, so find out where it should be, and send a read request to the home directory
     
		UINT32 home_node_rank = addr_home_lookup->find_home_for_addr(address);

#ifdef ADDR_HOME_LOOKUP_DEBUG
	  cerr << "Addr_Home_lookup: Address = " << hex << address << ", Home Node = " << dec << home_node_rank << endl;
#endif
#ifdef MMU_DEBUG
		ss << "address           : " << hex << address << endl;
		debugPrint(the_core->getRank(), "MMU", ss.str());
		debugPrint(the_core->getRank(), "MMU", "home_node_rank ", home_node_rank);
#endif
	 
		assert(home_node_rank >= 0 && home_node_rank < (UINT32)(the_core->getNumCores()));
	   
	   // send message here to home node to request data
		//packet.type, sender, receiver, packet.length
	   NetPacket packet = makePacket(req_msg_type, sizeof(RequestPayload), my_rank, home_node_rank); 
		
		// initialize packet payload
		//payload is on the stack, so it gets de-allocated once we exit this loop
		//Request the entire cache-line
		RequestPayload payload;
		payload.request_type = shmem_req_type;
		payload.request_address = ca_address;  
		payload.request_num_bytes = ocache->dCacheLineSize(); 

		packet.data = (char *)(&payload);

#ifdef MMU_DEBUG
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
	   
		/* =================================================== */
		/* ============= Handle Update Payload =============== */
		/* =================================================== */
		
//	   UpdatePayload* recv_payload = (UpdatePayload*)(recv_packet.data);
	   UpdatePayload recv_payload;
		//TODO for George
		mmuExtractUpdatePayloadBuffer(recv_packet, &recv_payload);
		
	   assert(recv_packet.type == SHARED_MEM_UPDATE_EXPECTED);
	   
	   //TODO: remove starting addr from data. this is just in there temporarily to debug
	   ADDRINT incoming_starting_addr = recv_payload->update_address;
		assert(incoming_starting_addr == address);
	   
	   CacheState::cstate_t resp_c_state = (CacheState::cstate_t)(recv_payload->update_new_cstate);
	   
	   //TODO: add support for actually updating data when we move to systems with software shared memory         
		//TODO: is there a better way to deal with null cache_model_results.second?
//		assert( cache_model_results.second != NULL );
//		cache_model_results.second->setCState(resp_c_state);         

		ocache->fillCacheLine(ca_address, resp_c_state, recv_payload->data_buffer, recv_payload->data_size); 
		
		/* =================================================== */
		/* =========== End Handle Update Payload ============= */
		/* =================================================== */
		
		assert( action_readily_permissable() );

#ifdef MMU_DEBUG
		debugPrint(the_core->getRank(), "MMU", "end of initiateSharedMemReq -------");
#endif
   
		return native_cache_hit;
   }
}


/*
 * this function is called by the "interrupt handler" when a shared memory request message is destined for this node
 * this function would be called on the home node (owner) of the address specified in the arguments
 * the shared request type indicates either a read or a write
 */

//TODO make this a PQueue on the time stamp
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

/* ======================================================== */
/* The Network sends memory requests to the MMU.
 * The MMU serializes the requests, and forwards them
 * to the DRAM_Directory.
 * ======================================================== */

//only the first request called drops into the while loop
//all additional requests add to the request_queue, 
//but then skip over the while loop.
void MemoryManager::addMemRequest(NetPacket req_packet)
{
	addRequestToQueue(req_packet);
	
	while( incoming_requests_count > 0 && !processing_request_flag )
	{
		//"Lock" the processRequest code
		processing_request_flag = true;
		
		dram_dir->processSharedMemReq(getNextRequest());
		
		//"Release" the processRequest code
		processing_request_flag = false;
	}
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

/* E, S I
 *
 * These are the different states we need to deal with
 * E->I = invalidate cache_line, set cache_state, send back data on ack (for WB in DRAM)
 *
 * E->S = set cache_state, send back data on ack (for WB in DRAM)
 *
 * S->I = no data payload (e/o for debugging)
 *
 * ***** This case never happens anymore, no more silent evictions ******
 * E|I->I = it got evicted (already WB). the DRAM will need to supply cache_line to the requestor.
 * 
 * E|I->S = it got evicted (already WB). the DRAM will need to supply cache_line to the requestor.
 * 
 * S|I->I
 */
	stringstream ss;
#ifdef MMU_DEBUG
	debugPrint(the_core->getRank(), "MMU", "processUnexpectedSharedMemUpdate $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
#endif

  // verify packet type is correct
  assert(update_packet.type == SHARED_MEM_UPDATE_UNEXPECTED);
  
  // extract relevant values from incoming request packet
  CacheState::cstate_t new_cstate = (CacheState::cstate_t)(((UpdatePayload*)(update_packet.data))->update_new_cstate);
  int address = ((UpdatePayload*)(update_packet.data))->update_address;

#ifdef MMU_DEBUG
	ss << "Unexpected: address: " << hex << address;
	debugPrint(the_core->getRank(), "MMU", ss.str());
#endif

	//TODO rename CacheTag to CacheLineInfo
	pair<bool, CacheTag*> cache_model_results = ocache->runDCachePeekModel(address);
   // if it is null, it means the address has been invalidated
	data_buffer = getCacheLine(address);

   CacheState::cstate_t current_cstate = ( cache_model_results.second != null ) ?
																							cache_model_results.second->getCState() :
																							CacheState::INVALID;
																							
   // send back acknowledgement of receiveing this message
   NetPacket packet = makePacket( SHARED_MEM_ACK, sizeof(AckPayload), the_core->getRank(), update_packet.sender );
  
   // initialize packet payload for downgrade
   AckPayload payload;
   payload.ack_new_cstate = new_cstate; //verify you set it to the correct cstate
   payload.ack_address = address; //only sent for debugging purposes

	switch( current_cstate ) {

		EXCLUSIVE: 
			cache_model_results.second->setCState(new_cstate);
			
			//send data for write-back
			payload.data_buffer = getCacheLine(address); 
			payload.data_size = ocache->getDCacheLineSize(); 
			payload.remove_from_sharers = false;
			
			if( new_cstate == INVALID ) 
				invalidateCacheLine();
			
			break;
		
		SHARED:
			cache_model_results.second->setCState(new_cstate);
			
			//send data for debugging purposes (no need for write-back)
			//TODO make payloads a class.  we can't have data_buffer as a pointer, b/c
			//the Network needs to copy ever payload byte by byte.
			payload.data_buffer = getCacheLine(address); 
			payload.data_size = ocache->getDCacheLineSize(); 
			payload.remove_from_sharers = false;
			
			if( new_cstate == INVALID ) 
				invalidateCacheLine();
			
			break;
		
		INVALID:
			//address has been invalidated -> tell directory to remove us from sharers' list
			payload.data_buffer = NULL; //only sent for debugging purposes
			payload.data_size = 0; 
			payload.remove_from_sharers = true;
			
			break;
		
		default:
			cerr << "ERROR in MMU switch statement." << endl;
			break;
	}

   packet.data = (char *)(&payload);
   
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
  
  // TODO: invalidate/flush from cache? 
#ifdef MMU_DEBUG	
	dram_dir->print();
	debugPrint(the_core->getRank(), "MMU", "end of processUnexpectedSharedMemUpdate");
#endif	

}

// TODO: implement DramRequest 
// if cache lookup is not a hit, we want to model dram request.
// and when we push around data, this function will deal with this
/*bool issueDramRequest(ADDRINT d_addr, shmem_req_t mem_req_type)
{
  cerr << "TODO: implement me: MemoryManager.cc issueDramRequest"<< endl;
  return true;
}

void MemoryManager::runDramAccessModel () {
	dramAccessCost += g_knob_dram_access_cost.Value();
}

UINT64 MemoryManager::getDramAccessCost() {
	return (dramAccessCost);
}
*/
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

