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

	// Initializing request queue parameters
	processing_request_flag = false;
	incoming_requests_count = 0;
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
	// debugPrint (the_core->getRank(), "MMU", "Starting adding Request Payload");
	MemoryManager::RequestPayload payload;
	payload.request_type = shmem_req_type;
	payload.request_address = address;  
	payload.request_num_bytes = size_bytes;

	packet->data = (char *)(&payload);
	// debugPrint (the_core->getRank(), "MMU", "Finished adding Request Payload");
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
/*
void MemoryManager::readFillBuffer( UINT32 offset, char* data_buffer, UINT32 data_size)
{
	assert( (offset + data_size) <= ocache->dCacheLineSize() );
	
	for(unsigned int i=0; i < data_size; i++)
		data_buffer[i] = fill_buffer[offset + i];
}

void MemoryManager::writeFillBuffer(UINT32 offset, char* data_buffer, UINT32 data_size)
{
//	debugPrint(the_core->getRank(), "MMU", "writeFullBuffer");
	assert( (offset + data_size) <= ocache->dCacheLineSize() );

//	cerr << " Write Data   : ";
//	for(unsigned int i=0; i < data_size; i++)
//		cerr << hex << (int) data_buffer[i];
//	cerr << endl;

//	cerr << " Buffer Before: ";
//	for(unsigned int i=0; i < g_knob_line_size; i++)
//		cerr << hex << (int) fill_buffer[i];
//	cerr << endl;

	for(unsigned int i=0; i < data_size; i++)
		fill_buffer[offset+i] = data_buffer[i];
	
//	cerr << " Buffer After : ";
//	for(unsigned int i=0; i < g_knob_line_size; i++)
//		cerr << hex << (int) fill_buffer[i];
//	cerr << endl;

}
*/
pair<bool, CacheTag*> MemoryManager::getCacheLineInfo(ADDRINT address)
{
	return ocache->runDCachePeekModel(address);
}

//copy data at cache to data_buffer
void MemoryManager::accessCacheLineData(CacheBase::AccessType access_type, ADDRINT ca_address, UINT32 offset, char* data_buffer, UINT32 data_size)
{
	bool fail_need_fill = false;
  	bool eviction = false;
   ADDRINT evict_addr;
   char evict_buff[ocache->dCacheLineSize()];

   ADDRINT data_addr = ca_address + offset;
   pair<bool, CacheTag*> result;

	// FIXME: Hack
	result = ocache->accessSingleLine(data_addr, access_type,
										&fail_need_fill, NULL,
										data_buffer, data_size,
										&eviction, &evict_addr, evict_buff);
	
	assert(eviction == false);

//	if (access_type == CacheBase::k_ACCESS_TYPE_STORE) {
//		cerr << "accessCacheLineData: data_buffer: 0x";
//		for (UINT32 i = 0; i < data_size; i++)
//			cerr << hex << (UINT32) data_buffer[i];
//		cerr << dec << endl;

//		cerr << "accessCacheLineData: fill_buffer: 0x";
//		for (UINT32 i = 0; i < ocache->dCacheLineSize(); i++)
//			cerr << hex << (UINT32) fill_buffer[i];
//		cerr << dec << endl;
//	}

	if(fail_need_fill) {
		//note: fail_need_fill is known beforehand, 
		//so that fill_buffer can be fetched from DRAM
		result = ocache->accessSingleLine(data_addr, access_type,
											NULL, fill_buffer,
											data_buffer, data_size,
											&eviction, &evict_addr, evict_buff);
	
	}

	if(eviction) 
	{
		debugPrint(the_core->getRank(), "MMU", "accessCacheLineData: Evicting Line");
		
		//send write-back to dram
		UINT32 home_node_rank = addr_home_lookup->find_home_for_addr(evict_addr);
		AckPayload payload;
		payload.ack_address = evict_addr;
		payload.is_writeback = true;
		payload.is_eviction = true;
		UINT32 payload_size = sizeof(payload) + ocache->dCacheLineSize();
		payload.data_size = ocache->dCacheLineSize();
		char payload_buffer[payload_size];

		
		createAckPayloadBuffer(&payload, evict_buff, payload_buffer, payload_size);
		NetPacket packet = makePacket(SHARED_MEM_EVICT, payload_buffer, payload_size, the_core->getRank(), home_node_rank);

		(the_core->getNetwork())->netSend(packet);
	}

}

void MemoryManager::forwardWriteBackToDram(NetPacket wb_packet)
{
	debugPrint(the_core->getRank(), "MMU", "Forwarding WriteBack to DRAM");
	dram_dir->processWriteBack(wb_packet);
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
//	debugPrint(the_core->getRank(), "MMU", "start of requestPermission");	
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
	debugPrint(the_core->getRank(), "MMU", "requestPermission - netRecv start");
	NetPacket recv_packet = (the_core->getNetwork())->netRecv(net_match);
	debugPrint(the_core->getRank(), "MMU", "requestPermission - netRecv finished");
	
	/* ===================================================== */
	/* ============== Handle Update Payload ================ */
	/* ============ Put Data Into Fill Buffer ============= */
	/* ===================================================== */
	
//	debugPrint(the_core->getRank(), "MMU", "requestPermission before extract update");	
	UpdatePayload recv_payload;
	extractUpdatePayloadBuffer(&recv_packet, &recv_payload, fill_buffer);
//	debugPrint(the_core->getRank(), "MMU", "requestPermission end of extract update");	
	
	*new_cstate = (CacheState::cstate_t)(recv_payload.update_new_cstate);
	
	//debug check
	assert(recv_packet.type == SHARED_MEM_UPDATE_EXPECTED);
	ADDRINT incoming_starting_addr = recv_payload.update_address;
	if(incoming_starting_addr != ca_address) {
		debugPrintHex (the_core->getRank(), "MMU", "Incoming Address", incoming_starting_addr);
     	debugPrintHex (the_core->getRank(), "MMU", "CA Address", ca_address);;
	}
	assert(incoming_starting_addr == ca_address);
	
	/* ==================================================== */
	/* ==================================================== */
	
	//if a read occurs, then it reads the fill buffer and then puts fill buffer into cache
	//if a write ocurs, then it modifies the fill buffer and then puts the fill buffer into cache
//	debugPrint(the_core->getRank(), "MMU", "end of requestPermission");	
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
			READ: readCacheLineData()							
			WRITE: writeCacheLineData()						
	}
*/
   /* ========================================================================= */
	
#ifdef MMU_DEBUG	
   stringstream ss;
	ss << ((shmem_req_type==READ) ? " READ " : " WRITE " ) << " - start : REQUESTING ADDR: " << hex << ca_address;
	debugPrint(the_core->getRank(), "MMU", ss.str());
#endif

	assert(buffer_size > 0);

	pair<bool, CacheTag*> cache_model_results;  
	//first-> is the line available in the cache? (Native cache hit)
	//second-> pointer to cache tag to update cache state
	bool native_cache_hit;  
	
	cache_model_results = getCacheLineInfo(ca_address); //check hit status, get CState
	native_cache_hit = cache_model_results.first;
	CacheState::cstate_t curr_cstate = (cache_model_results.second != NULL) 
															? cache_model_results.second->getCState() 
															: CacheState::INVALID;
   
	CacheBase::AccessType access_type = (shmem_req_type == READ) 
															? CacheBase::k_ACCESS_TYPE_LOAD 
															: CacheBase::k_ACCESS_TYPE_STORE;

	if ( action_readily_permissable(curr_cstate, shmem_req_type) )
   {
		assert( native_cache_hit == true );
		
		accessCacheLineData(access_type, ca_address, addr_offset, data_buffer, buffer_size); 
		
		return native_cache_hit;
	}
	else
	{
		//requested dram data is written to the fill_buffer
		CacheState::cstate_t new_cstate;
		requestPermission(shmem_req_type, ca_address, &new_cstate);
		
		accessCacheLineData(access_type, ca_address, addr_offset, data_buffer, buffer_size);
		setCacheLineInfo(ca_address, new_cstate);                                        
	
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
//TODO this such that is describes that it affects the CACHE state, and is not an update to the directory?
//ie, write_backs need to go elsewhere!
void MemoryManager::processUnexpectedSharedMemUpdate(NetPacket update_packet) 
{
  // TODO: This kind of argument passing is bad for performance. Try to pass by reference or pass pointers

  // verify packet type is correct
  assert(update_packet.type == SHARED_MEM_UPDATE_UNEXPECTED);
  
  char data_buffer[ocache->dCacheLineSize()];

  UpdatePayload update_payload;
  extractUpdatePayloadBuffer(&update_packet, &update_payload, data_buffer );

  // extract relevant values from incoming request packet
   CacheState::cstate_t new_cstate = update_payload.update_new_cstate;
   ADDRINT address = update_payload.update_address;
  
#ifdef MMU_DEBUG
  	stringstream ss;
	ss << "Processing Unexpected: address: " << hex << address << ", new CState: " << CacheState::cStateToString(new_cstate);
	debugPrint(the_core->getRank(), "MMU", ss.str());
#endif

	pair<bool, CacheTag*> cache_model_results = ocache->runDCachePeekModel(address);
   // if it is null, it means the address has been invalidated
   CacheState::cstate_t current_cstate = ( cache_model_results.second != NULL ) ?
																						cache_model_results.second->getCState() :
																						CacheState::INVALID;
																							
   // send back acknowledgement of receiveing this message
   AckPayload payload;
   payload.ack_new_cstate = new_cstate; //verify you set it to the correct cstate
	payload.ack_address = address; //only sent for debugging purposes
	UINT32 line_size = ocache->dCacheLineSize();
   char writeback_data[line_size]; 
	
	UINT32 payload_size = 0;
	char payload_buffer[line_size];

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
			
			if (new_cstate == CacheState::INVALID) 
				invalidateCacheLine(address);
			
			break;
		
		case CacheState::INVALID:
			//THIS can happen due to race conditions where core evalidates at the same time dram sends demotion message
			//address has been invalidated -> tell directory to remove us from sharers' list
			payload_size = sizeof(payload);
			payload.remove_from_sharers = true; 
			createAckPayloadBuffer(&payload, NULL, payload_buffer, payload_size);
			
			break;
		
		default:
			debugPrint (the_core->getRank(), "MMU", "ERROR in MMU switch statement");
			break;
	}

   NetPacket packet = makePacket( SHARED_MEM_ACK, payload_buffer, payload_size, the_core->getRank(), update_packet.sender );
   
  (the_core->getNetwork())->netSend(packet);
  
#ifdef MMU_DEBUG	
	debugPrint(the_core->getRank(), "MMU", "end of processUnexpectedSharedMemUpdate");
#endif	

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

void MemoryManager::debugSetDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list, char *d_data)
{
   // Assume d_data is a pointer to the entire memory block
	dram_dir->debugSetDramState(addr, dstate, sharers_list, d_data);	
}

bool MemoryManager::debugAssertDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list, char *d_data)
{
	return dram_dir->debugAssertDramState(addr, dstate, sharers_list, d_data);	
}

void MemoryManager::debugSetCacheState(ADDRINT address, CacheState::cstate_t cstate, char *c_data) {
	
	//using Load Model, so that way we garuntee the tag isn't null
	// Assume that address is always cache aligned
	pair<bool, CacheTag*> cache_result;

	bool fail_need_fill;
	// char buff[ocache->dCacheLineSize()];
	bool eviction;
	ADDRINT evict_addr;
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
					// Actually, this is OK !!
					cerr << "**** Problem ****: Some data has been evicted !!!\n";
				}
			}
			
			cache_result.second->setCState(cstate);
			// Now I have set the state as well as data
			break;
		default:
			cerr << "ERROR in switch for Core::debugSetCacheState" << endl;
	}
}

bool MemoryManager::debugAssertCacheState(ADDRINT address, CacheState::cstate_t expected_cstate, char *expected_data) {

	//	pair<bool,CacheTag*> cache_result = ocache->runDCachePeekModel(address, 1);
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
		for (UINT32 i = 0; i < ocache->dCacheLineSize(); i++)
			cerr << hex << (UINT32) actual_data[i];
		cerr << endl;
		
		cerr << "Expected Data: 0x";
		for (UINT32 i = 0; i < ocache->dCacheLineSize(); i++)
			cerr << hex << (UINT32) expected_data[i];
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

void MemoryManager::createUpdatePayloadBuffer (UpdatePayload* send_payload, char* data_buffer, char* payload_buffer, UINT32 payload_size)
{
	// Create a new buffer of size : sizeof(send_payload) + cache_line_size
	assert( payload_buffer != NULL );
	
	//copy send_payload
	memcpy ( (void*) payload_buffer, (void*) send_payload, sizeof(*send_payload) );

	//this is very important on the recieving end, so the extractor knows how big data_size is
   assert( send_payload->data_size == (payload_size - sizeof(*send_payload)) );
	
	//copy data_buffer over
	if(send_payload->data_size > g_knob_line_size) {
		// debugPrint(the_core->getRank(), "MMU", "****ERROR **** dataSize > g_knob_line_size: ...... data_size = ", send_payload->data_size);
		cerr << "CreateUpdatePayloadBuffer: Error\n";
	}

	assert (send_payload->data_size <= g_knob_line_size);

	//copy data_buffer
	if(data_buffer != NULL) 
		memcpy ((void*) (payload_buffer + sizeof(*send_payload)), (void*) data_buffer, payload_size - sizeof(*send_payload));
	
}

void MemoryManager::createAckPayloadBuffer (AckPayload* send_payload, char* data_buffer, char* payload_buffer, UINT32 payload_size)
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


