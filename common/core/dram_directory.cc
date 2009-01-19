#include "dram_directory.h"
//#define DRAM_DEBUG

#include "pin.H"

//TODO i don't think this is used
extern LEVEL_BASE::KNOB<UInt32> g_knob_dram_access_cost;

//LIMITED_DIRECTORY Flag
//Dir(i)NB ; i = number of pointers
//if MAX_SHARERS >= total number of cores, then the directory
//collaspes into the full-mapped case.
//TODO use a knob to set this instead
//(-dms) : directory_max_sharers
//TODO provide easy mechanism to initiate a broadcast invalidation
//	static const UInt32 MAX_SHARERS = 2;
extern LEVEL_BASE::KNOB<UInt32> g_knob_dir_max_sharers;

//TODO LIST (ccelio)
//add support for limited directory scheme.
//supply MAX_SHARERS, evict one (LRU? Random?) to add new sharers.

DramDirectory::DramDirectory(UInt32 num_lines_arg, UInt32 bytes_per_cache_line_arg, UInt32 dram_id_arg, UInt32 num_of_cores_arg, Network* network_arg)
{
	the_network = network_arg;
	num_lines = num_lines_arg;
	number_of_cores = num_of_cores_arg;
	dram_id = dram_id_arg;
   bytes_per_cache_line = bytes_per_cache_line_arg;

/*
#ifdef DRAM_DEBUG
   debugPrintStart (dram_id, "DRAMDIR", "Init Dram with num_lines", num_lines);
   debugPrintStart (dram_id, "DRAMDIR", "bytes_per_cache_linew", bytes_per_cache_line);
#endif
 */
   assert( num_lines >= 0 );

   dramAccessCost = 0;
}

DramDirectory::~DramDirectory()
{
//FIXME is this correct way to delete a std::map? and is std::map going to deallocate all of the entries?
//  delete dram_directory_entries;
}

/*
 * returns the associated DRAM directory entry given a memory address
 */
DramDirectoryEntry* DramDirectory::getEntry(IntPtr address)
{
	// note: the directory is a map key'ed by cache line. so, first we need to determine the associated cache line
	UInt32 data_line_index = (address / bytes_per_cache_line);
  
	assert( data_line_index >= 0);

	DramDirectoryEntry* entry_ptr = dram_directory_entries[data_line_index];
  
	// TODO FIXME we need to handle the data_buffer correctly! what do we fill it with?!
	// I guess this is the place we need to call the code for transferring data from the host machine to the simulated machine
	if( entry_ptr == NULL ) {
		UInt32 memory_line_address = ( address / bytes_per_cache_line ) * bytes_per_cache_line;
		dram_directory_entries[data_line_index] =  new DramDirectoryEntry( memory_line_address, number_of_cores);
	}

	return dram_directory_entries[data_line_index];
}

void DramDirectory::setDramMemoryLine(IntPtr addr, char* data_buffer, UInt32 data_size)
{
	assert( data_size == bytes_per_cache_line );
	copyDataToDram(addr, data_buffer);
}

//George's stuff for modeling dram access costs
//TODO can I actually have all dram requests go through the directory?
void DramDirectory::runDramAccessModel () {
	dramAccessCost += g_knob_dram_access_cost.Value();
}

UInt64 DramDirectory::getDramAccessCost() {
	return (dramAccessCost);
}

// TODO: implement DramRequest 
// if cache lookup is not a hit, we want to model dram request.
// and when we push around data, this function will deal with this
bool issueDramRequest(IntPtr d_addr, shmem_req_t mem_req_type)
{
  debugPrint(-1, "DRAM", "TODO: implement me: dram_directory.cc issueDramRequest");
  return true;
}

void DramDirectory::copyDataToDram(IntPtr address, char* data_buffer) 
{
	runDramAccessModel();

	UInt32 data_line_index = ( address / bytes_per_cache_line );
	DramDirectoryEntry* entry_ptr = dram_directory_entries[data_line_index];

	assert(entry_ptr != NULL);
	if( entry_ptr == NULL ) {
		dram_directory_entries[data_line_index] = new DramDirectoryEntry( address, number_of_cores, data_buffer );
		entry_ptr = dram_directory_entries[data_line_index];
		assert(entry_ptr != NULL );
	} 
	
	entry_ptr->fillDramDataLine(data_buffer); //TODO verify that data_buffer is correctly the size of the memory_line

}

// FIXME: In case this was a writeback, we need to set the state bits also properly in addition to the sharers list
void DramDirectory::processWriteBack(NetPacket& wb_packet)
{
	// Only evictions come here from now on
	MemoryManager::AckPayload payload; 
	char data_buffer[bytes_per_cache_line];

	MemoryManager::extractAckPayloadBuffer(&wb_packet, &payload, data_buffer);

	DramDirectoryEntry* dir_entry = getEntry(payload.ack_address);

#ifdef DRAM_DEBUG
	stringstream ss;
	ss << "Got Eviction for Address: 0x" << hex << (UInt32) payload.ack_address << dec << ", Sharer = " << wb_packet.sender << ", numSharers = " << dir_entry->numSharers() << ", DState = " << DramDirectoryEntry::dStateToString(dir_entry->getDState());
	debugPrint (dram_id, "DRAM_DIR", ss.str());
#endif

	dir_entry->removeSharer(wb_packet.sender);

	// cerr << "DRAM Directory: Address = 0x" << hex << (UInt32) payload.ack_address << dec << ", Sharer = " << wb_packet.sender << ", numSharers = " << dir_entry->numSharers() << endl;

	if(dir_entry->numSharers() == 0) {
		dir_entry->setDState(DramDirectoryEntry::UNCACHED);
	}

	if (payload.is_writeback) {
	
		assert (payload.data_size == bytes_per_cache_line);
		copyDataToDram(payload.ack_address, data_buffer); //TODO verify that all wb_packets would give us the entire cache_line
	}
	
}


/* ======================================================== *
 * The Dram Directory serializes requests with the same address.
 * ======================================================== */

void DramDirectory::startSharedMemRequest(NetPacket& req_packet) {

	// This function adds the shared memory request on the queue and starts processing it if possible
	IntPtr address = ((MemoryManager::RequestPayload*)(req_packet.data))->request_address;
	shmem_req_t shmem_req_type = ((MemoryManager::RequestPayload*)(req_packet.data))->request_type;
	UInt32 requestor = req_packet.sender;

	DramRequest* dram_req = dram_request_list[address];

#ifdef DRAM_DEBUG
	stringstream ss;
	string start_now = (dram_req == NULL) ? "YES" : "NO";
	string request_type = (shmem_req_type == READ) ? "READ" : "WRITE";
	ss << "Got Shared Memory Request for address: 0x" << hex << (UInt32) address << dec << ", Req_Type = " << request_type << ", Start Now - " << start_now;
	debugPrint (dram_id, "DRAM_DIR", ss.str());
#endif

	if (dram_req == NULL) {

		// No requests for this address exist currently
		dram_request_list[address] = new DramRequest;

		dram_req = dram_request_list[address];
		assert (dram_req != NULL);
		
		SingleDramRequest* single_dram_req = new SingleDramRequest(address, shmem_req_type, requestor);
		dram_req->addRequestToQueue (*single_dram_req);
		delete single_dram_req;

		startNextSharedMemRequest(address);
	}
	
	else {

		// There exists some requests for this address already
		SingleDramRequest* single_dram_req = new SingleDramRequest(address, shmem_req_type, requestor);
		dram_req->addRequestToQueue (*single_dram_req);
		delete single_dram_req;
	}
}

void DramDirectory::finishSharedMemRequest(IntPtr address) 
{
	// We need to process the next request on the queue here
	DramRequest* dram_req = dram_request_list[address];
	assert (dram_req != NULL);
 	
#ifdef DRAM_DEBUG
	stringstream ss;
	string start_next = (dram_req->numWaitingRequests() == 0) ? "NO" : "YES";
	ss << "Finished DRAM Request for address: 0x" << hex << (UInt32) address << dec << ", Start Next - " << start_next;
	debugPrint (dram_id, "DRAM_DIR", ss.str());
#endif

	if (dram_req->numWaitingRequests() == 0) {
		delete dram_req;
		dram_request_list.erase(address);
	}
	else {
		startNextSharedMemRequest (address);
	}
	
	
}

void DramDirectory::startNextSharedMemRequest(IntPtr address) {
	
	// This function is ONLY called when there is another request on the queue and it is ready to be processed
	DramRequest* dram_req = dram_request_list[address];
	assert (dram_req != NULL);
	assert (dram_req->numWaitingRequests() != 0);

	// Extract the next request from the queue
	SingleDramRequest single_dram_req = dram_req->getNextRequest ();
	
	DramDirectoryEntry* dram_dir_entry = getEntry(address);
	assert (dram_dir_entry != NULL);
	
	DramDirectoryEntry::dstate_t old_dstate = dram_dir_entry->getDState();
	UInt32 num_acks_to_recv = dram_dir_entry->numSharers();
		
	dram_req->setRequestAttributes (single_dram_req, old_dstate, num_acks_to_recv);

	processSharedMemRequest (single_dram_req.requestor, single_dram_req.shmem_req_type, single_dram_req.address);

}

void DramDirectory::processSharedMemRequest (UInt32 requestor, shmem_req_t shmem_req_type, IntPtr address) {

	// This function should not depend on the state of the DRAM directory
	// The relevant arguments must be passed in from the calling function

	DramDirectoryEntry* dram_dir_entry = getEntry(address);
	DramDirectoryEntry::dstate_t curr_dstate = dram_dir_entry->getDState();

#ifdef DRAM_DEBUG
	stringstream ss;
	string request_type = (shmem_req_type == READ) ? "READ" : "WRITE";
	ss << "Start Processing DRAM request for address: 0x" << hex << (UInt32) address << dec << ", Req_Type = " << request_type << ", <- " << requestor << ", Current State = " << DramDirectoryEntry::dStateToString(curr_dstate);
	debugPrint (dram_id, "DRAM_DIR", ss.str());
#endif

	switch (shmem_req_type) {
	
		case WRITE: 
			if (curr_dstate == DramDirectoryEntry::EXCLUSIVE)
			{
				// assert (dram_dir_entry->numSharers() == 1);
				startDemoteOwner(dram_dir_entry, CacheState::INVALID); 
			}
			else if (curr_dstate == DramDirectoryEntry::SHARED) // Number of sharers > 0
			{
				// assert (dram_dir_entry->numSharers() > 0);
				startInvalidateAllSharers(dram_dir_entry);
			}
			else if (curr_dstate == DramDirectoryEntry::UNCACHED)
			{
				// assert (dram_dir_entry->numSharers == 0);
				// Return the data to the requestor
				dram_dir_entry->addExclusiveSharer (requestor);
				dram_dir_entry->setDState (DramDirectoryEntry::EXCLUSIVE);

				sendDataLine (dram_dir_entry, requestor, CacheState::EXCLUSIVE);
				finishSharedMemRequest (address);
			}
			else 
			{
				assert (false);	// We should not reach here
			}
		break;

		case READ:
			// handle the case where this line is in the exclusive state (so data has to be written back first and that entry is downgraded to SHARED)
			if (curr_dstate == DramDirectoryEntry::EXCLUSIVE)
			{
				// assert (dram_dir_entry->numSharers() == 1);
				startDemoteOwner(dram_dir_entry, CacheState::SHARED);
			}
			else if (
					   (curr_dstate == DramDirectoryEntry::SHARED)  ||
						(curr_dstate == DramDirectoryEntry::UNCACHED)
					  )
			{
				// Check if the maximum limit on the number of sharers has been reached
				if (! dram_dir_entry->addSharer (requestor)) {
					// Success, I can now just return the data to the requestor
					dram_dir_entry->setDState (DramDirectoryEntry::SHARED);
					sendDataLine (dram_dir_entry, requestor, CacheState::SHARED);
					finishSharedMemRequest (address);
				}
				else {
					// We need to remove another sharer before we can add this sharer
					UInt32 eviction_id = dram_dir_entry->getEvictionId ();

					startInvalidateSingleSharer (dram_dir_entry, eviction_id);
				}
			}
			else  
			{
				// We should not reach here
				assert(false);
			}
		break;
      
		default:
			throw("unsupported memory transaction type.");
		break;
   }
  
}


	/* ============================================================== */
	//
	//
	 /* ============================================================== */
	 /* Added by George */
	 
	 // XXX: Here, I add the DRAM access cost
	 // TODO: This is just a temporary hack. The actual DRAM access cost has to incorporated 
	 //       while adding data storage to all the coherence messages
	 //
	 // 1) READ request:
	 //    a) Case (EXCLUSIVE):
	 //       There is only 1 AckPayload message we need to look at 
	 //	    i) AckPayload::remove_from_sharers = false
	 //	      * Do '2' runDramAccessModel()
	 //	    ii) AckPayload::remove_from_sharers = true
	 //	      * Do '1' runDramAccessModel()
	 //	 b) Case (SHARED or INVALID):
	 //	   * Do '1' runDramAccessModel()
	 //
	 // 2) WRITE request
	 //    a) Case (EXCLUSIVE):
	 //    	 There is only 1 AckPayload message we need to look at
	 //       i) AckPayload::remove_from_sharers = false
	 //         * Do '2' runDramAccessModel()
	 //       ii) AckPayload::remove_from_sharers = true
	 //         * Do '1' runDramAccessModel()
	 //    b) Case (SHARED or INVALID):
	 //      * Do '1' runDramAccessModel()
	 //

	 /* =============================================================== */

//TODO go through this code again. i think a lot of it is unnecessary.
//TODO rename this to something more descriptive about what it does (sending a memory line to another core)
void DramDirectory::sendDataLine(DramDirectoryEntry* dram_dir_entry, UInt32 requestor, CacheState::cstate_t new_cstate)
{
   // initialize packet payload
	MemoryManager::UpdatePayload payload;

	payload.update_new_cstate = new_cstate;
	payload.update_address = dram_dir_entry->getMemLineAddress();
	
	char data_buffer[bytes_per_cache_line];
	UInt32 data_size;
	dram_dir_entry->getDramDataLine(data_buffer, &data_size);
	
	assert (data_size == bytes_per_cache_line);
	UInt32 payload_size = sizeof(payload) + data_size;
	char payload_buffer[payload_size];
	payload.data_size = data_size;
	
	MemoryManager::createUpdatePayloadBuffer(&payload, data_buffer, payload_buffer, payload_size);
	// This is the response to a shared memory request
	NetPacket packet = MemoryManager::makePacket(SHARED_MEM_RESPONSE, payload_buffer, payload_size, dram_id, requestor );
	
	the_network->netSend(packet);

}

void DramDirectory::startDemoteOwner(DramDirectoryEntry* dram_dir_entry, CacheState::cstate_t new_cstate)
{
	//demote exclusive owner
	assert( dram_dir_entry->numSharers() == 1 );
	assert( dram_dir_entry->getDState() == DramDirectoryEntry::EXCLUSIVE );
	assert( new_cstate == CacheState::SHARED || new_cstate == CacheState::INVALID );

	UInt32 owner = dram_dir_entry->getExclusiveSharerRank();

	IntPtr address = dram_dir_entry->getMemLineAddress();

	// cerr << "DRAM_DIR: [" << dram_id << "] Owner = " << owner << ", Address = 0x" << hex << address << dec << endl;
	// print();
	// reqeust a write back data payload and downgrade to new_dstate
	MemoryManager::UpdatePayload upd_payload;
	upd_payload.update_new_cstate = new_cstate;
	upd_payload.update_address = address;
	upd_payload.data_size = 0;
	NetPacket packet = MemoryManager::makePacket(SHARED_MEM_UPDATE_UNEXPECTED, (char *)(&upd_payload), sizeof(MemoryManager::UpdatePayload), dram_id, owner);

	the_network->netSend(packet);
}



// Send invalidate messages to all of the sharers and wait on acks.
// TODO: Add in ability to do a broadcast invalidateSharers if #of sharers is greater than #of pointers.
void DramDirectory::startInvalidateAllSharers (DramDirectoryEntry* dram_dir_entry)
{
   assert (dram_dir_entry->numSharers() >= 1);
	assert (dram_dir_entry->getDState() == DramDirectoryEntry::SHARED);
	 
	// invalidate current sharers
	vector<UInt32> sharers_list = dram_dir_entry->getSharersList();
	
	// send message to sharer to invalidate it
	for(UInt32 i = 0; i < sharers_list.size(); i++)
	{
		startInvalidateSingleSharer (dram_dir_entry, sharers_list[i]);
  	}
  
}

// Invalidate Only a single sharer
void DramDirectory::startInvalidateSingleSharer (DramDirectoryEntry* dram_dir_entry, UInt32 sharer_id) {

	assert (dram_dir_entry->numSharers() >= 1);
	assert (dram_dir_entry->getDState() == DramDirectoryEntry::SHARED);

	IntPtr address = dram_dir_entry->getMemLineAddress();

	MemoryManager::UpdatePayload payload;
	payload.update_new_cstate = CacheState::INVALID;
	payload.update_address = address;
	payload.data_size = 0;

	NetPacket packet = MemoryManager::makePacket( SHARED_MEM_UPDATE_UNEXPECTED, (char *)(&payload), sizeof(MemoryManager::UpdatePayload), dram_id, sharer_id);
	the_network->netSend(packet);
}


	

void DramDirectory::processAck (NetPacket& ack_packet)
{
	/*
	 * Create a map of the following form
	 * key :- address (cache aligned)
	 * value :- <shmem_req_type, requestor, numAcksToRecv, old_dstate>
	 */

	IntPtr address = ((MemoryManager::AckPayload*) (ack_packet.data))->ack_address;
	DramRequest* dram_req = dram_request_list[address];

	assert (dram_req != NULL);		// A request must have already been created
	shmem_req_t shmem_req_type = dram_req->getShmemReqType();
	UInt32 requestor = dram_req->getRequestor();
	DramDirectoryEntry::dstate_t old_dstate = dram_req->getOldDState();

	DramDirectoryEntry* dram_dir_entry = getEntry(address);

	assert(dram_dir_entry != NULL);

#ifdef DRAM_DEBUG
	stringstream ss;
	ss << "Got Ack <- " << ack_packet.sender << " for " << requestor << ", Address = 0x" << hex << (UInt32) address << dec << ", NumAcksToRecv = " << dram_req->getNumAcksToRecv();
	debugPrint (dram_id, "DRAM_DIR", ss.str());
#endif

	char data_buffer[bytes_per_cache_line];
	MemoryManager::AckPayload ack_payload;
	MemoryManager::extractAckPayloadBuffer(&ack_packet, &ack_payload, data_buffer); //TODO the data_buffer here isn't used! 

	switch (shmem_req_type) {

		case (WRITE):

			if (old_dstate == DramDirectoryEntry::EXCLUSIVE)
			{
				processDemoteOwnerAck (ack_packet.sender, dram_dir_entry, (void*) &ack_payload, data_buffer, DramDirectoryEntry::UNCACHED);
				processSharedMemRequest (requestor, WRITE, address);
			}
			else if (old_dstate == DramDirectoryEntry::SHARED)
			{
				processInvalidateSharerAck (ack_packet.sender, dram_dir_entry, (void*) &ack_payload, data_buffer);
				dram_req->decNumAcksToRecv();
				if (dram_req->getNumAcksToRecv() == 0) {
					processSharedMemRequest (requestor, WRITE, address);
				}
			}
			else 
			{
				// Should never reach here				
				assert(false);
			}

		break;

		case (READ):

			if (old_dstate == DramDirectoryEntry::EXCLUSIVE)
			{
				processDemoteOwnerAck (ack_packet.sender, dram_dir_entry, (void*) &ack_payload, data_buffer, DramDirectoryEntry::SHARED);
				processSharedMemRequest (requestor, READ, address);
			}
			else if (old_dstate == DramDirectoryEntry::SHARED)
			{
				// We only need to evict the cache line from 1 sharer
				processInvalidateSharerAck (ack_packet.sender, dram_dir_entry, (void*) &ack_payload, data_buffer);
				processSharedMemRequest (requestor, READ, address);
			}
			else
			{
				// Should never reach here				
				assert (false);
			}

		break;

		default:

			debugPrint (dram_id, "DRAM_DIR", "Invalid Shared Memory Request Type");
			assert(false);

		break;
	}

}

void DramDirectory::processDemoteOwnerAck(
		  													UInt32 sender, 
															DramDirectoryEntry* dram_dir_entry, 
															void* /*MemoryManager::AckPayload&*/ ack_payload_v, 
															char *data_buffer, 
															DramDirectoryEntry::dstate_t new_dstate
													  )
{
	// wait for the acknowledgement from the original owner that it downgraded itself to SHARED (from EXCLUSIVE)
	// The 'dram_dir_entry' was in the exclusive state before	
	
	MemoryManager::AckPayload* ack_payload = (MemoryManager::AckPayload*) ack_payload_v;
	
	if (ack_payload->remove_from_sharers) {

		assert (ack_payload->data_size == 0);
		assert (ack_payload->ack_new_cstate == CacheState::INVALID);
		assert (ack_payload->is_writeback == false);
		assert (dram_dir_entry->getDState() == DramDirectoryEntry::UNCACHED);
		assert (dram_dir_entry->numSharers() == 0);

		// We cant say anything about the exclusive owner
	}

	else {

		// There has been no eviction 
		assert (ack_payload->data_size == bytes_per_cache_line);
		// assert (ack_payload->ack_new_cstate == new_dstate);
		assert (ack_payload->is_writeback == true);
		assert (dram_dir_entry->getDState() == DramDirectoryEntry::EXCLUSIVE);
		assert (dram_dir_entry->numSharers() == 1);
		assert (dram_dir_entry->getExclusiveSharerRank() == sender);

		// Write Back the cache line
		// copyDataToDram(IntPtr address, char* data_buffer) 

		copyDataToDram (dram_dir_entry->getMemLineAddress(), data_buffer);
		if (new_dstate == DramDirectoryEntry::UNCACHED) {
			// It was a write request
			dram_dir_entry->removeSharer(sender);
		}
		dram_dir_entry->setDState(new_dstate);
	}
	 
} 

void DramDirectory::processInvalidateSharerAck(
		  														UInt32 sender, 
																DramDirectoryEntry* dram_dir_entry, 
																void *  /*MemoryManager::AckPayload&*/ ack_payload_v, 
																char *data_buffer
															 )
{
	MemoryManager::AckPayload* ack_payload = (MemoryManager::AckPayload*) ack_payload_v;
		
	assert (ack_payload->ack_new_cstate == CacheState::INVALID);
	assert (ack_payload->data_size == 0);
	assert (ack_payload->is_writeback == false);

	if (ack_payload->remove_from_sharers) {
		assert ( (dram_dir_entry->getDState() == DramDirectoryEntry::SHARED)  ||  (dram_dir_entry->getDState() == DramDirectoryEntry::UNCACHED) );
	}
	
	else {
		assert (dram_dir_entry->getDState() == DramDirectoryEntry::SHARED);

		// Remove the sharer from the sharer's list
		dram_dir_entry->removeSharer(sender);
		if (dram_dir_entry->numSharers() == 0) {
			dram_dir_entry->setDState(DramDirectoryEntry::UNCACHED);
		}
	}

}

void DramDirectory::print()
{
	cerr << endl << endl << " <<<<<<<<<<<<<<< PRINTING DRAMDIRECTORY INFO [" << dram_id << "] >>>>>>>>>>>>>>>>> " << endl << endl;
	std::map<UInt32, DramDirectoryEntry*>::iterator iter = dram_directory_entries.begin();
	while(iter != dram_directory_entries.end())
	{
		cerr << "   ADDR (aligned): 0x" << hex << iter->second->getMemLineAddress() 
				<< "  DState: " << DramDirectoryEntry::dStateToString(iter->second->getDState());

		vector<UInt32> sharers = iter->second->getSharersList();
		cerr << "  SharerList <size= " << sharers.size() << " > = { ";
		
		for(unsigned int i = 0; i < sharers.size(); i++) {
			cerr << sharers[i] << " ";
		}
		
		cerr << "} "<< endl;
		iter++;
	}
	cerr << endl << " <<<<<<<<<<<<<<<<<<<<< ----------------- >>>>>>>>>>>>>>>>>>>>>>>>> " << endl << endl;
}

void DramDirectory::debugSetDramState(IntPtr address, DramDirectoryEntry::dstate_t dstate, vector<UInt32> sharers_list, char *d_data)
{
	UInt32 data_line_index = (address / bytes_per_cache_line); 
	assert( data_line_index >= 0);

	DramDirectoryEntry* entry_ptr = dram_directory_entries[data_line_index];
  
	if( entry_ptr == NULL ) {
		UInt32 memory_line_address = ( address / bytes_per_cache_line ) * bytes_per_cache_line;
		dram_directory_entries[data_line_index] =  new DramDirectoryEntry( memory_line_address
																								, number_of_cores
																								, d_data );
		entry_ptr = dram_directory_entries[data_line_index];
	}

	assert( entry_ptr != NULL );

	//set sharer's list
	entry_ptr->debugClearSharersList();
	entry_ptr->setDState(dstate);
	entry_ptr->fillDramDataLine(d_data);
	
	if (dstate == DramDirectoryEntry::EXCLUSIVE) {
		assert (sharers_list.size() == 1);
		entry_ptr->addExclusiveSharer (sharers_list.back());
		print();
		return;
	}

	while(!sharers_list.empty())
	{
		assert( dstate != DramDirectoryEntry::UNCACHED );
		UInt32 new_sharer = sharers_list.back();
		sharers_list.pop_back();
		entry_ptr->addSharer(new_sharer);
	}

	print();
}

bool DramDirectory::debugAssertDramState(IntPtr address, DramDirectoryEntry::dstate_t	expected_dstate, vector<UInt32> expected_sharers_vector, char *expected_data)
{
	
	UInt32 data_line_index = (address / bytes_per_cache_line); 
	
	assert( data_line_index >= 0);
	
	UInt32 memory_line_size;
	DramDirectoryEntry::dstate_t actual_dstate;
	char actual_data[bytes_per_cache_line];
  
	DramDirectoryEntry* entry_ptr = dram_directory_entries[data_line_index];
  
	assert (entry_ptr != NULL);
	// It cant be NULL
	/*
	if( entry_ptr == NULL ) {
		UInt32 memory_line_address = ( address / bytes_per_cache_line ) * bytes_per_cache_line;
		dram_directory_entries[data_line_index] =  new DramDirectoryEntry( memory_line_address
																								, number_of_cores);
	}
	*/

//	entry_ptr->dirDebugPrint();
//	DramDirectoryEntry::dstate_t actual_dstate = dram_directory_entries[cache_line_index]->getDState();
	
	actual_dstate = entry_ptr->getDState();
	entry_ptr->getDramDataLine (actual_data, &memory_line_size);

	assert (memory_line_size == bytes_per_cache_line);

	cerr << "Expected Data ptr = 0x" << hex << (UInt32) expected_data << endl;
	cerr << "Actual Data: 0x";
	for (UInt32 i = 0; i < memory_line_size; i++) {
		cerr << hex << (UInt32) actual_data[i];
	}
	cerr << endl;

	cerr << "Expected Data: 0x";
	for (UInt32 i = 0; i < memory_line_size; i++) {
		cerr << hex << (UInt32) expected_data[i];
	}
	cerr << dec << endl;

	cerr << "Actual State: " << DramDirectoryEntry::dStateToString(actual_dstate) << endl; 
	cerr << "Expected State: " << DramDirectoryEntry::dStateToString(expected_dstate) << endl; 

	bool is_assert_true = ( (actual_dstate == expected_dstate) &&
			  						(memcmp(actual_data, expected_data, bytes_per_cache_line) == 0) ); 
	

	//copy STL vectors (which are just glorified stacks) and put data into array (for easier comparsion)
	bool* actual_sharers_array = new bool[number_of_cores];
	bool* expected_sharers_array = new bool[number_of_cores];
	
	for(int i = 0; i < (int) number_of_cores; i++) {
		actual_sharers_array[i] = false;
		expected_sharers_array[i] = false;
	}

	vector<UInt32> actual_sharers_vector = entry_ptr->getSharersList();

	while(!actual_sharers_vector.empty())
	{
		UInt32 sharer = actual_sharers_vector.back();
		actual_sharers_vector.pop_back();
		assert( sharer >= 0);
		assert( sharer < number_of_cores );
		actual_sharers_array[sharer] = true;
//	  	cerr << "Actual Sharers Vector Sharer-> Core# " << sharer << endl;
	}

	while(!expected_sharers_vector.empty())
	{
		UInt32 sharer = expected_sharers_vector.back();
		expected_sharers_vector.pop_back();
		assert( sharer >= 0);
		assert( sharer < number_of_cores );
		expected_sharers_array[sharer] = true;
//		cerr << "Expected Sharers Vector Sharer-> Core# " << sharer << endl;
	}

	//do actual comparision of both arrays
	for( int i=0; i < (int) number_of_cores; i++)
	{
		if( actual_sharers_array[i] != expected_sharers_array[i])
		{
			is_assert_true = false;
		}
	}
	
	cerr << "   Asserting Dram     : Expected: " << DramDirectoryEntry::dStateToString(expected_dstate);
	cerr << ",  Actual: " <<  DramDirectoryEntry::dStateToString(actual_dstate);

	cerr << ", E {";

			for(int i=0; i < (int) number_of_cores; i++) 
			{
				if(expected_sharers_array[i]) {
					cerr << " " << i << " ";
				}
			}

	cerr << "}, A {";
			
			for(int i=0; i < (int) number_of_cores; i++) 
			{
				if(actual_sharers_array[i]) {
					cerr << " " << i << " ";
				}
			}
	cerr << "}";	
	
	//check sharers list
	//1. check that for every sharer expected, that he is set.
	//2. verify that no extra sharers are set. 
	if(is_assert_true) {
      cerr << " TEST PASSED " << endl;
	} else {
		cerr << " TEST FAILED ****** " << endl;
//		print();
	}
	
	// assert (is_assert_true == true);

	return is_assert_true;
}
