//TODO also take care of write-backs on Ack payloads (processUnexpectedSharedMem)

#include "memory_manager.h"
#define MMU_DEBUG
//#define MMU_CACHEHIT_DEBUG
//#define ADDR_HOME_LOOKUP_DEBUG

using namespace std;

MemoryManager::MemoryManager(Core *the_core_arg, OCache *ocache_arg) {

   the_core = the_core_arg;
	ocache = ocache_arg;
	
	//FIXME; need to add infrastructure for specifying core architecture details (e.g. DRAM size)
	// this also assumes 1 dram per core

	/* ================================================================= */
	/* Added by George */
//	dramAccessCost = 0;
	/* ================================================================= */

	// assume 4GB / dCacheLineSize  bytes/line 
	int total_num_dram_lines = (int) (pow(2,32) / ocache->dCacheLineSize()); 
	
	int dram_lines_per_core = total_num_dram_lines / the_core->getNumCores();
	assert( (dram_lines_per_core * the_core->getNumCores()) == total_num_dram_lines );

   assert( ocache != NULL );

	//TODO can probably delete "dram_lines_per_core" b/c it not necessary.
	dram_dir = new DramDirectory(dram_lines_per_core, ocache->dCacheLineSize(), the_core_arg->getRank(), the_core_arg->getNumCores(), the_core_arg->getNetwork());

	
	/**** Data Passing Stuff ****/
	eviction_buffer = new char[g_knob_line_size];
	fill_buffer = new char[g_knob_line_size]; //dram writes to this buffer
	
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
	payload.request_address = address;  
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

NetPacket MemoryManager::makePacket(PacketType packet_type, char* payload_buffer, UINT32 payload_size, int sender_rank, int receiver_rank )
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

void MemoryManager::setCacheLineInfo(ADDRINT ca_address, CacheState::cstate_t new_cstate)
{
	pair<bool, CacheTag*> results = ocache->runDCachePeekModel(ca_address);
	assert( results.first ); //it should already be in the cache for us to change it!
	assert( results.second != NULL ); 

	results.second->setCState(new_cstate);         

}

//fill_buffer is a global inside the memory_manager
//the dram writes data to the fill_buffer
//before the fill_buffer is pushed to the cache
//the core can read or write the fill buffer
void MemoryManager::readFillBuffer( UINT32 offset, char* data_buffer, UINT32 data_size)
{
	assert( (offset + data_size) <= ocache->dCacheLineSize() );
	
	for(unsigned int i=0; i < data_size; i++)
		data_buffer[i] = fill_buffer[offset + i];
}

void MemoryManager::writeFillBuffer(UINT32 offset, char* data_buffer, UINT32 data_size)
{
	debugPrint(the_core->getRank(), "MMU", "writeFullBuffer");
	assert( (offset + data_size) < ocache->dCacheLineSize() );

	cerr << " Write Data   : ";
	for(unsigned int i=0; i < data_size; i++)
		cerr << hex << (int) data_buffer[i];
	cerr << endl;

	cerr << " Buffer Before: ";
	for(unsigned int i=0; i < g_knob_line_size; i++)
		cerr << hex << (int) fill_buffer[i];
	cerr << endl;

	for(unsigned int i=0; i < data_size; i++)
		fill_buffer[offset+i] = data_buffer[i];
	
	cerr << " Buffer After : ";
	for(unsigned int i=0; i < g_knob_line_size; i++)
		cerr << hex << (int) fill_buffer[i];
	cerr << endl;

}

//get cacheLineInfo
pair<bool, CacheTag*> MemoryManager::getCacheLineInfo(ADDRINT address)
{
	return ocache->runDCachePeekModel(address);
}

//copy data at cache to data_buffer
void MemoryManager::readCacheLineData(ADDRINT ca_address, UINT32 offset, char* data_buffer, UINT32 data_size)
{

	//assumes that the cache_line is already in the cache
   UINT32 line_size = ocache->dCacheLineSize();
	bool fail_need_fill;

	//TODO do i need this here? we're not doing a STORE
   bool eviction;
   ADDRINT evict_addr;
   char evict_buff[line_size];

   pair<bool, CacheTag*> result;
	ADDRINT addr = ca_address + offset;

	result = ocache->accessSingleLine(addr, CacheBase::k_ACCESS_TYPE_LOAD,
										&fail_need_fill, NULL,
										data_buffer, data_size,
										&eviction, &evict_addr, evict_buff);

	assert( result.first );
	assert( !fail_need_fill);
	assert( !eviction );

}

//TODO remember, the data_size isn't always the entire cache_line!
void MemoryManager::writeCacheLineData(ADDRINT ca_address, UINT32 offset, char* data_buffer, UINT32 data_size)
{

   	UINT32 line_size = ocache->dCacheLineSize();
	bool fail_need_fill;

  	bool eviction;
   	ADDRINT evict_addr;
   	char evict_buff[line_size];

   	ADDRINT addr = ca_address + offset;

   	pair<bool, CacheTag*> result;

	cerr << "CA ADDRESS: " << hex << ca_address << endl;
	cerr << "offset    : " << dec << ( ca_address & (ocache->dCacheLineSize() - 1) ) << endl;
	cerr << "line size : " << dec << ocache->dCacheLineSize() << endl;

	debugPrint(the_core->getRank(), "MMU", "Start Access (1)");
	result = ocache->accessSingleLine(addr, CacheBase::k_ACCESS_TYPE_STORE,
										&fail_need_fill, NULL,
										data_buffer, data_size,
										&eviction, &evict_addr, evict_buff);

	debugPrint(the_core->getRank(), "MMU", "End   Access (1)");
	
	cerr << "Fail_Need_Fill" << ( fail_need_fill ? " TRUE " : " FALSE ") << endl;
	
	if(fail_need_fill) {
		assert( data_size == ocache->dCacheLineSize() );
		debugPrint(the_core->getRank(), "MMU", "Start Access (2)");
		result = ocache->accessSingleLine(ca_address, CacheBase::k_ACCESS_TYPE_STORE,
											NULL, data_buffer,
											NULL, 0,
											&eviction, &evict_addr, evict_buff);
		debugPrint(the_core->getRank(), "MMU", "End   Access (2)");
	}


/*	result = ocache->accessSingleLine(addr, CacheBase::k_ACCESS_TYPE_STORE,
										NULL, data_buffer,
										data_buffer, data_size,
										&eviction, &evict_addr, evict_buff);
*/

	if(eviction) 
	{
		debugPrint(the_core->getRank(), "MMU", "writeCacheLineData: Evicting Line");
		
		//TODO can race conditions occur due to eviction messages?
		//send write-back to dram
		//TODO make sure I'm requesting the right size
		UINT32 home_node_rank = addr_home_lookup->find_home_for_addr(ca_address);
		UpdatePayload payload;
		payload.update_address = evict_addr;
		payload.is_writeback = true;
		UINT32 payload_size = sizeof(payload) + ocache->dCacheLineSize();
		payload.data_size = ocache->dCacheLineSize();
		char payload_buffer[payload_size];
		
		createUpdatePayloadBuffer(&payload, evict_buff, payload_buffer, payload_size);
		//TODO do i need to create a new network packetType for write-backs?
		NetPacket packet = makePacket(SHARED_MEM_UPDATE_UNEXPECTED, payload_buffer, payload_size, the_core->getRank(), home_node_rank);

		(the_core->getNetwork())->netSend(packet);

		//TODO do I need to wait for confirmation from DRAM directory that the write-back happened with no issues?
	}

}

void MemoryManager::invalidateCacheLine(ADDRINT address)
{
	bool hit = ocache->invalidateLine(address);
	assert( hit );
	//we shouldn't be invalidating lines unless we know it's in the cache
}

//send request to DRAM Directory to request a given address, for a certain operation
//does NOT touch the cache, but instead writes the data to the global "fill_buffer"
//sets what the new_cstate should be set to on the receiving end
void MemoryManager::requestPermission(shmem_req_t shmem_req_type, ADDRINT ca_address, CacheState::cstate_t* new_cstate)
{
	UINT32 home_node_rank = addr_home_lookup->find_home_for_addr(ca_address);

	assert(home_node_rank >= 0 && home_node_rank < (UINT32)(the_core->getNumCores()));
	
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

	// receive the requested data (blocking receive)
	NetMatch net_match = makeNetMatch( SHARED_MEM_UPDATE_EXPECTED, home_node_rank );
	NetPacket recv_packet = (the_core->getNetwork())->netRecv(net_match);
	
	/* ===================================================== */
	/* ============== Handle Update Payload ================ */
	/* ============ Put Data Into Fill Buffer ============= */
	/* ===================================================== */
	
	debugPrint(the_core->getRank(), "MMU", "requestPermission before extract update");	
	UpdatePayload recv_payload;
	extractUpdatePayloadBuffer(&recv_packet, &recv_payload, fill_buffer);
	debugPrint(the_core->getRank(), "MMU", "requestPermission end of extract update");	
	
	*new_cstate = (CacheState::cstate_t)(recv_payload.update_new_cstate);
	
	//debug check
	assert(recv_packet.type == SHARED_MEM_UPDATE_EXPECTED);
	ADDRINT incoming_starting_addr = recv_payload.update_address;
	if(incoming_starting_addr != ca_address) {
		cerr << "[" << the_core->getRank() << "] Incoming Address: " << hex << incoming_starting_addr << endl;
      	cerr << "CA Address         : " << hex << ca_address << endl;
	}
	assert(incoming_starting_addr == ca_address);
	
	/* ==================================================== */
	/* ==================================================== */
	
	//if a read occurs, then it reads the fill buffer and then puts fill buffer into cache
	//if a write ocurs, then it modifies the fill buffer and then puts the fill buffer into cache
	debugPrint(the_core->getRank(), "MMU", "end of requestPermission");	
}

//buffer_size is the size in bytes of the char* data_buffer
//we may give the cache less than a cache_line of data, but never more than a cache_line_size of data
//ca_address is "cache-aligned" address
//addr_offset provides the offset that points to the requested address
//TODO this will not work correctly for multi-line requests!
//TODO what is "return bool" used for?  cache hits? or immediately permissable?
bool MemoryManager::initiateSharedMemReq(shmem_req_t shmem_req_type, ADDRINT ca_address, UINT32 addr_offset, char* data_buffer, UINT32 buffer_size)
{
   /* ========================================================================= */
/*
   pair<bool, CacheTag*> cache_results = ocache->getCacheLineInfo(ca_address);
	bool hit cache_results.first;

	if(permissable()) 
	{
		READ: readCacheLineData(ca_address)
		WRITE: writeCacheLineData(ca_address)
	}
	else
	{
		request_address(ca_address, &fill_buffer);
		set cstate
			READ: readDataFromFillBuffer();
			WRITE: modifyFillBuffer();
		writeCacheLineData(ca_address, fill_buffer)
	}
*/
   /* ========================================================================= */
	
   	stringstream ss;
	ss << ((shmem_req_type==READ) ? " READ " : " WRITE " ) << " - start : REQUESTING ADDR: " << hex << ca_address;
	debugPrint(the_core->getRank(), "MMU", ss.str());


   	//TODO rename CacheTag as CacheLineInfo
	pair<bool, CacheTag*> cache_model_results;  
	//first-> is the line available in the cache? (Native cache hit)
	//second-> pointer to cache tag to update cache state
	bool native_cache_hit;  
	
	/*************** CACHE ACCESS *****************/
	//TODO side-effect junk. currently getCacheLineInfo only PEEKS at info, so if the
	//cache_line isn't there, CacheTag is null.  (meaning we can't change the cstate!)
	cache_model_results = getCacheLineInfo(ca_address); //check hit status, get CState
	native_cache_hit = cache_model_results.first;
	CacheState::cstate_t curr_cstate = (cache_model_results.second != NULL) ? 
																				cache_model_results.second->getCState() 
																				: CacheState::INVALID;

	if ( action_readily_permissable(curr_cstate, shmem_req_type) )
   	{
		ss.str("");
		ss << ((shmem_req_type==READ) ? " READ " : " WRITE " ) << " - action permissable, ADDR: " << hex << ca_address << " , offset: " << dec << addr_offset << ", buffer_size: " << dec << buffer_size;
		debugPrint(the_core->getRank(), "MMU", ss.str());

		assert( native_cache_hit == true );
		
		if(shmem_req_type==READ)
			readCacheLineData(ca_address, addr_offset, data_buffer, buffer_size); 
		else
			writeCacheLineData(ca_address, addr_offset, data_buffer, buffer_size);
		
		ss.str("");
		ss << ((shmem_req_type==READ) ? " READ " : " WRITE " ) << " - end (was permissable) ";
		debugPrint(the_core->getRank(), "MMU", ss.str());

		return native_cache_hit;
	}
	else
	{
		ss.str("");
		ss << ((shmem_req_type==READ) ? " READ " : " WRITE " ) << " - action is not permissable";
		debugPrint(the_core->getRank(), "MMU", ss.str());
		
		//requested dram data is written to the fill_buffer
		CacheState::cstate_t new_cstate;
		requestPermission(shmem_req_type, ca_address, &new_cstate);
		
		ss.str("");
		ss << ((shmem_req_type==READ) ? " READ " : " WRITE " ) << " - after permission";
		debugPrint(the_core->getRank(), "MMU", ss.str());
		
		if (shmem_req_type==READ) {
			readFillBuffer(addr_offset, data_buffer, buffer_size);
		} else {
			writeFillBuffer(addr_offset, data_buffer, buffer_size);
		}

		writeCacheLineData(ca_address, addr_offset, fill_buffer, ocache->dCacheLineSize());
		setCacheLineInfo(ca_address, new_cstate);                                        
	
		ss.str("");
		ss << ((shmem_req_type==READ) ? " READ " : " WRITE " ) << " - end (wasn't permissable) ";
		debugPrint(the_core->getRank(), "MMU", ss.str());

		return native_cache_hit;
	}
	
}

#if 0
bool MemoryManager::initiateSharedMemReq(ADDRINT address, UINT32 size, shmem_req_t shmem_req_type)
//buffer_size is the size in bytes of the char* data_buffer
//we may give the cache less than a cache_line of data, but never more than a cache_line_size of data
//ca_address is "cache-aligned" address
//addr_offset provides the offset that points to the requested address
{
	
	stringstream ss;
   
#ifdef MMU_DEBUG
	debugPrint(the_core->getRank(), "MMU", "initiateSharedMemReq +++++++++");
	debugPrintString(the_core->getRank(), "MMU", " SHMEM Request Type: ", MemoryManager::sMemReqTypeToString(shmem_req_type));
#endif

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
	
	cache_model_results = ocache->getCacheLineInfo(); //check hit status, get CState
//	cache_model_results = ocache->accessSingleLine(address, access_type,
//  																&fail_need_fill, NULL,
//																	data_buffer, g_knob_line_size,
//																	&eviction, &evict_addr, eviction_buffer);
																	
	native_cache_hit = cache_model_results.first;
	assert( native_cache_hit != fail_need_fill );
	assert( cache_model_results.second != NULL );
	
#ifdef MMU_CACHEHIT_DEBUG                                    
	if(native_cache_hit) {
		//god i hate c++
		ss << "NATIVE CACHE (HIT) : ADDR: = " <<  hex << address 
			<< " - CState: "<< CacheState::cStateToString(cache_model_results.second->getCState());
		debugPrint(the_core->getRank(),"MMU", ss.str()); 
	} else {
		stringstream ss;
		ss << "NATIVE CACHE (MISS): ADDR: = " << hex << address 
			<< " - CState: " << CacheState::cStateToString(cache_model_results.second->getCState());
		debugPrint(the_core->getRank(),"MMU", ss.str());
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
	   NetPacket packet = makePacket(req_msg_type, sizeof(RequestPayload), the_core->getRank(), home_node_rank); 
		
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
		extractUpdatePayloadBuffer(recv_packet, &recv_payload);
		
	   assert(recv_packet.type == SHARED_MEM_UPDATE_EXPECTED);
	   
	   ADDRINT incoming_starting_addr = recv_payload->update_address;
		assert(incoming_starting_addr == address);
	   
	   CacheState::cstate_t resp_c_state = (CacheState::cstate_t)(recv_payload->update_new_cstate);
	   
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

#endif

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
	// TODO: This kind of argument passing is bad for performance. Try to pass by reference or pass pointers

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
  	ADDRINT address = ((UpdatePayload*)(update_packet.data))->update_address;

#ifdef MMU_DEBUG
	ss << "Unexpected: address: " << hex << address;
	debugPrint(the_core->getRank(), "MMU", ss.str());
#endif

	//TODO rename CacheTag to CacheLineInfo
	pair<bool, CacheTag*> cache_model_results = ocache->runDCachePeekModel(address);
   	// if it is null, it means the address has been invalidated
   	CacheState::cstate_t current_cstate = ( cache_model_results.second != NULL ) ?
																							cache_model_results.second->getCState() :
																							CacheState::INVALID;
																							
   	// send back acknowledgement of receiveing this message
   	// initialize packet payload for downgrade
   	AckPayload payload;
   	payload.ack_new_cstate = new_cstate; //verify you set it to the correct cstate
   	payload.ack_address = address; //only sent for debugging purposes
   	char writeback_data[ocache->dCacheLineSize()];
	UINT32 payload_size = 0;
   	char* payload_buffer = NULL;

	switch( current_cstate ) {

		case CacheState::EXCLUSIVE: 
			cache_model_results.second->setCState(new_cstate);
			
			//send data for write-back
			readCacheLineData(address, 0, writeback_data, ocache->dCacheLineSize());
			payload_size = sizeof(payload) + ocache->dCacheLineSize();
			payload_buffer = new char[payload_size];
			// FIXME: Why dont you allocate this on the stack
			// char payload_buffer[payload_size]
			payload.data_size = ocache->dCacheLineSize();
			payload.remove_from_sharers = false;
			createAckPayloadBuffer(&payload, writeback_data, payload_buffer, payload_size);
			
			if (new_cstate == CacheState::INVALID) 
				invalidateCacheLine(address);
			
			break;
		
		case CacheState::SHARED:
			cache_model_results.second->setCState(new_cstate);
			
			payload_size = sizeof(payload);
			payload_buffer = new char[payload_size];
			// FIXME: Why dont you allocate this on the stack
			// char payload_buffer[payload_size]
			payload.data_size = 0;
			payload.remove_from_sharers = false;
			createAckPayloadBuffer(&payload, NULL, payload_buffer, payload_size);
			
			if (new_cstate == CacheState::INVALID) 
				invalidateCacheLine(address);
			
			break;
		
		case CacheState::INVALID:
			//address has been invalidated -> tell directory to remove us from sharers' list
			payload_size = sizeof(payload);
			payload_buffer = new char[payload_size];
			// FIXME: Why dont you allocate this on the stack
			// char payload_buffer[payload_size]
			payload.data_size = 0;
			payload.remove_from_sharers = true;
			createAckPayloadBuffer(&payload, NULL, payload_buffer, payload_size);
			
			break;
		
		default:
			cerr << "ERROR in MMU switch statement." << endl;
			break;
	}

   NetPacket packet = makePacket( SHARED_MEM_ACK, payload_buffer, payload_size, the_core->getRank(), update_packet.sender );
   
  (the_core->getNetwork())->netSend(packet);
  
  delete[] payload_buffer;

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

void MemoryManager::createUpdatePayloadBuffer (UpdatePayload* send_payload, char* data_buffer, char* payload_buffer, UINT32 payload_size)
{
	// Create a new buffer of size : sizeof(send_payload) + cache_line_size
	assert( payload_buffer != NULL );

	//copy send_payload
	memcpy ((void*) payload_buffer, (void*) send_payload, sizeof(*send_payload));
	
	//copy data_buffer
	if(data_buffer != NULL) 
		memcpy ((void*) (payload_buffer + sizeof(*send_payload)), (void*) data_buffer, payload_size - sizeof(*send_payload));
}

void MemoryManager::createAckPayloadBuffer (AckPayload* send_payload, char* data_buffer, char* payload_buffer, UINT32 payload_size)
{
	// Create a new buffer of size : sizeof(send_payload) + cache_line_size
	assert( payload_buffer != NULL );

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
	cerr << "Greetings in payload data size" << dec << payload->data_size << endl;
	//copy data_buffer over
	assert (payload->data_size <= g_knob_line_size);

	if (payload->data_size > 0)
		memcpy ((void*) data_buffer, (void*) ( ((char*) packet->data) + sizeof(*payload) ), payload->data_size);

	cerr << "Exiting in Extracdt" << endl;
}

//TODO should we turn payloads from structs to classes so we don't have to have seperate methods to do this stuff?
void MemoryManager::extractAckPayloadBuffer (NetPacket* packet, AckPayload* payload, char* data_buffer) 
{ 
	memcpy ((void*) payload, (void*) (packet->data), sizeof(*payload));
	
	assert( payload->data_size <= g_knob_line_size );
	
	if(payload->data_size > 0)
		memcpy ((void*) data_buffer, (void*) ( ((char*) packet->data) + sizeof(*payload) ), payload->data_size);

}


