#include "dram_directory.h"
//#define DRAM_DEBUG

DramDirectory::DramDirectory(UINT32 num_lines_arg, UINT32 bytes_per_cache_line_arg, UINT32 dram_id_arg, UINT32 num_of_cores_arg, Network* network_arg)
{
	the_network = network_arg;
	num_lines = num_lines_arg;
	number_of_cores = num_of_cores_arg;
	dram_id = dram_id_arg;
   cerr << "Init Dram with num_lines: " << num_lines << endl;
   cerr << "   bytes_per_cache_linew: " << bytes_per_cache_line_arg << endl;
   assert( num_lines >= 0 );
   bytes_per_cache_line = bytes_per_cache_line_arg;

   /* Added by George */
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
DramDirectoryEntry* DramDirectory::getEntry(ADDRINT address)
{
	// note: the directory is a map key'ed by cache line. so, first we need to determine the associated cache line
	UINT32 data_line_index = (address / bytes_per_cache_line);
  
#ifdef DRAM_DEBUG
	printf(" DRAM_DIR: getEntry: address        = 0x %x\n" ,address );
	cerr << " DRAM_DIR: getEntry: cachline_index = " << data_line_index << endl;
#endif
  
	assert( data_line_index >= 0);

	DramDirectoryEntry* entry_ptr = dram_directory_entries[data_line_index];
  
	//TODO FIXME we need to handle the data_buffer correctly! what do we fill it with?!
	if( entry_ptr == NULL ) {
//		debugPrint(dram_id, "DRAMDIR", "Making a new dram entry");
		UINT32 memory_line_address = ( address / bytes_per_cache_line ) * bytes_per_cache_line;
		dram_directory_entries[data_line_index] =  new DramDirectoryEntry( memory_line_address
																								, number_of_cores);
//		this->print();
	}

	return dram_directory_entries[data_line_index];
}

//George's stuff for modeling dram access costs
//TODO can I actually have all dram requests go through the directory?
void DramDirectory::runDramAccessModel () {
	dramAccessCost += g_knob_dram_access_cost.Value();
}

UINT64 DramDirectory::getDramAccessCost() {
	return (dramAccessCost);
}

// TODO: implement DramRequest 
// if cache lookup is not a hit, we want to model dram request.
// and when we push around data, this function will deal with this
bool issueDramRequest(ADDRINT d_addr, shmem_req_t mem_req_type)
{
  cerr << "TODO: implement me: dram_directory.cc issueDramRequest"<< endl;
  return true;
}

void DramDirectory::copyDataToDram(ADDRINT address, char* data_buffer) //, UINT32 data_size)
{

	//TODO provide a unique key, 
	UINT32 data_line_index = ( address / bytes_per_cache_line );
	DramDirectoryEntry* entry_ptr = dram_directory_entries[data_line_index];

	if( entry_ptr == NULL ) {
		dram_directory_entries[data_line_index] = new DramDirectoryEntry( address, number_of_cores, data_buffer );
		entry_ptr = dram_directory_entries[data_line_index];
		assert(entry_ptr != NULL );
	} 
	
	entry_ptr->fillDramDataLine(data_buffer); //TODO verify that data_buffer is correctly the size of the memory_line

}

void DramDirectory::processWriteBack(NetPacket wb_packet)
{
	cerr << endl << "processing writeback" << endl << endl;
	
	this->print();

	MemoryManager::AckPayload payload; //TODO writebacks must always be an Ackpayload
	char data_buffer[bytes_per_cache_line];

	MemoryManager::extractAckPayloadBuffer(&wb_packet, &payload, data_buffer); 
	
	copyDataToDram(payload.ack_address, data_buffer); //is data size needed? , data_size);
	
	if( payload.is_eviction ) {
		DramDirectoryEntry* dir_entry = getEntry(payload.ack_address);
		dir_entry->removeSharer( wb_packet.sender );
	}

	this->print();
	
	assert( payload.is_writeback );
	
	runDramAccessModel();
	
	cerr << endl;
	cerr << "finished processing writeback" << endl;
	cerr << endl;
}

/* ======================================================== */
/* The Network sends memory requests to the MMU.
 * The MMU serializes the requests, and forwards them
 * to the DRAM_Directory.
 * ======================================================== */

void DramDirectory::processSharedMemReq(NetPacket req_packet) 
{
	/* ============================================================== */
/*	
	MemoryManager::UpdatePayload payload;
	char* data_buffer;
	//TODO what is data_buffer used for?  I think nothing!
	MemoryManager::extractUpdatePayloadBuffer(&req_packet, &payload, data_buffer);
	DramDirectoryEntry* dram_entry = this->getEntry(address);
	 
	switch(req_d_state)
	{
		case WRITE: 
			if(current_d_state)
			{
				invalidate_owner();
				write_back(); //Dram cost model
			}
			else
			{
				invalidate_sharers();
			}
			break;

		case READ:
			if(current_d_state == DramDirectoryEntry::EXCLUSIVE)
			{
				demote_owner();
				write_back(); //Dram cost model
	      }
			else
			{
			 //nothing.
			}
			
			addSharer();
			setDState(Shared);

			break;
		default:
			cerr << "ERROR in ProcessSharedMemReq" << endl;
			assert( false );
			break;
	}
	 
	dram_dir_entry->addSharer(requestor);
	sendDataLine(); //Dram cost model
*/
	/* ============================================================== */
   stringstream ss;

  	// extract relevant values from incoming request packet
	shmem_req_t shmem_req_type = (shmem_req_t)((MemoryManager::RequestPayload*)(req_packet.data))->request_type;
  	ADDRINT address = ((MemoryManager::RequestPayload*)(req_packet.data))->request_address;
  	UINT32 requestor = req_packet.sender;
  
  	DramDirectoryEntry* dram_dir_entry = this->getEntry(address);
  	DramDirectoryEntry::dstate_t current_dstate = dram_dir_entry->getDState();
   	CacheState::cstate_t new_cstate;

	ss.str("");
	ss << "Requested Addr: " << hex << address << ",ReqType: " << ((shmem_req_type==WRITE) ? "WRITE" : "READ ") << ", CurrentDState: " << DramDirectoryEntry::dStateToString(current_dstate) << " ";
	debugPrint(dram_id, "DRAMDIR", ss.str());

	switch( shmem_req_type ) {
	
		case WRITE: 
			new_cstate = CacheState::EXCLUSIVE;

			if( current_dstate == DramDirectoryEntry::EXCLUSIVE )
			{
				//invalidate owner. receive write_back packet
				NetPacket wb_packet;
				wb_packet = demoteOwner(dram_dir_entry, CacheState::INVALID); 
				processWriteBack(wb_packet);
			}
			else
			{
//				cerr << "OH HAI SSTARTING Invalidating Sharers" << endl;
				invalidateSharers(dram_dir_entry);
				dram_dir_entry->setDState(DramDirectoryEntry::EXCLUSIVE);
//				cerr << "OH HAI FINISHED Invalidating Sharers" << endl;
			}
		break;

		case READ:
			new_cstate = CacheState::SHARED;
		{
			// handle the case where this line is in the exclusive state (so data has to be written back first and that entry is downgraded to SHARED)
			if(current_dstate == DramDirectoryEntry::EXCLUSIVE)
			{
				//demote exclusive owner
				NetPacket wb_packet;
				wb_packet = demoteOwner(dram_dir_entry, CacheState::SHARED);
				processWriteBack(wb_packet);
				dram_dir_entry->setDState(DramDirectoryEntry::SHARED);
			}
			else
			{
				//do nothing. no need to contact other sharers in this case.
				if(current_dstate == DramDirectoryEntry::UNCACHED)
					dram_dir_entry->setDState(DramDirectoryEntry::SHARED);
			}
		}
		break;
      
		default:
			throw("unsupported memory transaction type.");
		break;
   }
  
	// 3. return data back to requestor 
	dram_dir_entry->addSharer(requestor);
	sendDataLine(dram_dir_entry, requestor, new_cstate); //Dram cost model

#ifdef DRAM_DEBUG
	debugPrint(dram_id, "DRAM", "end of sharedMemReq function");
#endif
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
void DramDirectory::sendDataLine(DramDirectoryEntry* dram_dir_entry, UINT32 requestor, CacheState::cstate_t new_cstate)
{
   // initialize packet payload
	MemoryManager::UpdatePayload payload;

	payload.update_new_cstate = new_cstate;
	payload.update_address = dram_dir_entry->getMemLineAddress();
	
	stringstream ss;
	ss << " Sending Back UpdateAddr: " << hex << payload.update_address;
	debugPrint(dram_id, "DRAM: SendDataLine", ss.str());

	char data_buffer[bytes_per_cache_line];
	UINT32 data_size;
	dram_dir_entry->getDramDataLine(data_buffer, &data_size);
	
	assert (data_size == bytes_per_cache_line);
	UINT32 payload_size = sizeof(payload) + data_size;
	char payload_buffer[payload_size];
	payload.data_size = data_size;
	payload.is_writeback = false;
//	payload.is_eviction = false;
	MemoryManager::createUpdatePayloadBuffer(&payload, data_buffer, payload_buffer, payload_size);
	NetPacket packet = MemoryManager::makePacket(SHARED_MEM_UPDATE_EXPECTED, payload_buffer, payload_size, dram_id, requestor );
	
//	debugPrint(dram_id, "DRAM: SendDataLine", "sending update expected packet");
	the_network->netSend(packet);
	debugPrint(dram_id, "DRAM: SendDataLine", "finished sending update expected packet");

}

NetPacket DramDirectory::demoteOwner(DramDirectoryEntry* dram_dir_entry, CacheState::cstate_t new_cstate)
{
	//demote exclusive owner
	assert( dram_dir_entry->numSharers() == 1 );
	assert( new_cstate == CacheState::SHARED || new_cstate == CacheState::INVALID );

	unsigned int current_owner = dram_dir_entry->getExclusiveSharerRank();

	// reqeust a write back data payload and downgrade to new_dstate
//	MemoryManager::AckPayload payload; //this was orginally here, but i don't think ackpayload is correct.
	MemoryManager::UpdatePayload upd_payload;
	upd_payload.update_new_cstate = new_cstate;
	ADDRINT address = dram_dir_entry->getMemLineAddress();
	upd_payload.update_address= address;
	upd_payload.is_writeback = false;
//	upd_payload.is_eviction = false;
	upd_payload.data_size = 0;
	NetPacket packet = MemoryManager::makePacket(SHARED_MEM_UPDATE_UNEXPECTED, (char *)(&upd_payload), sizeof(MemoryManager::UpdatePayload), dram_id, current_owner);
	
	the_network->netSend(packet);

	// wait for the acknowledgement from the original owner that it downgraded itself to SHARED (from EXCLUSIVE)
	NetMatch net_match;
	net_match.sender = current_owner;
	net_match.sender_flag = true;
	net_match.type = SHARED_MEM_ACK;
	net_match.type_flag = true;
	
	NetPacket wb_packet = the_network->netRecv(net_match);
	
	// assert a few things in the ack packet (sanity checks)
	MemoryManager::AckPayload ack_payload;
	//null because we aren't interested in extracting the data_buffer here
	char data_buffer[(int) g_knob_line_size];
	MemoryManager::extractAckPayloadBuffer(&wb_packet, &ack_payload, data_buffer); //TODO the data_buffer here isn't used! 
	
	assert((unsigned int)(wb_packet.sender) == current_owner);
	assert(wb_packet.type == SHARED_MEM_ACK);

	ADDRINT received_address = ack_payload.ack_address; 
	assert(received_address == address);

	CacheState::cstate_t received_new_cstate = ack_payload.ack_new_cstate; 
	assert(received_new_cstate == new_cstate);
	
	// TODO DOES THIS CASE EVER HAPPEN? ie, an owner that had already been evicted
	// did the former owner invalidate it already? if yes, we should remove him from the sharers list
//	assert( ack_payload.remove_from_sharers == false );
	if( ack_payload.remove_from_sharers != false ) 
		cerr << "*** ERROR ***** DRAM DIRECTORY: DEMOTE OWNER.  OWNER HAS EVICTED ADDRESS, AND THE DRAM DIR WAS NOT UPDATED!!!!! ***** " << endl;
	
	if( new_cstate == CacheState::INVALID || ack_payload.remove_from_sharers )
		dram_dir_entry->removeSharer( current_owner );

	return wb_packet; //let the calling function perform the WriteBack so we don't implicitly do write backs
} //end demoteOwner()


//private utility function
//send invalidate messages to all of the sharers
//and wait on acks.
void DramDirectory::invalidateSharers(DramDirectoryEntry* dram_dir_entry)
{
   //invalidate current sharers
	//bool already_invalidated = false;
	
	ADDRINT address = dram_dir_entry->getMemLineAddress();
	//TODO I'm not positive this address is correctly calculated!
	vector<UINT32> sharers_list = dram_dir_entry->getSharersList();
	
	// send message to sharer to invalidate it
	for(UINT32 i = 0; i < sharers_list.size(); i++) 
	{
		MemoryManager::UpdatePayload payload;
		payload.update_new_cstate = CacheState::INVALID;
		payload.update_address = address;
		payload.data_size = 0;
		payload.is_writeback = false;
//		payload.is_eviction = false;
		NetPacket packet = MemoryManager::makePacket( SHARED_MEM_UPDATE_UNEXPECTED, (char *)(&payload), sizeof(MemoryManager::UpdatePayload), dram_id, sharers_list[i]);
		the_network->netSend(packet);
  }
  
  // receive invalidation acks from all sharers
  for(UINT32 i = 0; i < sharers_list.size(); i++)      
  {				
		// TODO: optimize this by receiving acks out of order
		// wait for all of the invalidation acknowledgements
		NetMatch net_match;
		net_match.sender = sharers_list[i]; //*sharers_iterator;
		net_match.sender_flag = true;
		net_match.type = SHARED_MEM_ACK;
		net_match.type_flag = true;
		NetPacket recv_packet = the_network->netRecv(net_match);
		 
		// assert a few things in the ack packet (sanity checks)
		assert((unsigned int)(recv_packet.sender) == sharers_list[i]); 
		ADDRINT received_address = ((MemoryManager::AckPayload*)(recv_packet.data))->ack_address;
		assert(received_address == address);
		 
		CacheState::cstate_t received_new_cstate = (CacheState::cstate_t)(((MemoryManager::AckPayload*)(recv_packet.data))->ack_new_cstate);
		assert(received_new_cstate == CacheState::INVALID);
	}

}

void DramDirectory::print()
{
	cerr << endl << endl << " <<<<<<<<<<<<<<< PRINTING DRAMDIRECTORY INFO [" << dram_id << "] >>>>>>>>>>>>>>>>> " << endl << endl;
	std::map<UINT32, DramDirectoryEntry*>::iterator iter = dram_directory_entries.begin();
	while(iter != dram_directory_entries.end())
	{
		cerr << "   ADDR (aligned): 0x" << hex << iter->second->getMemLineAddress() 
				<< "  DState: " << DramDirectoryEntry::dStateToString(iter->second->getDState());

		vector<UINT32> sharers = iter->second->getSharersList();
		cerr << "  SharerList <size= " << sharers.size() << " > = { ";
		
		for(unsigned int i = 0; i < sharers.size(); i++) {
			cerr << sharers[i] << " ";
		}
		
		cerr << "} "<< endl;
		iter++;
	}
	cerr << endl << " <<<<<<<<<<<<<<<<<<<<< ----------------- >>>>>>>>>>>>>>>>>>>>>>>>> " << endl << endl;
}

void DramDirectory::debugSetDramState(ADDRINT address, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list)
{

	UINT32 cache_line_index = (address / bytes_per_cache_line) - ( num_lines * dram_id );
  
	assert( cache_line_index >= 0);

	DramDirectoryEntry* entry_ptr = dram_directory_entries[cache_line_index];
  
	if( entry_ptr == NULL ) {
		UINT32 memory_line_address = ( address / bytes_per_cache_line ) * bytes_per_cache_line;
		dram_directory_entries[cache_line_index] =  new DramDirectoryEntry( memory_line_address
																								, number_of_cores);
		entry_ptr = dram_directory_entries[cache_line_index];
	}

	assert( entry_ptr != NULL );

//	entry_ptr->dirDebugPrint();
//	dram_directory_entries[cache_line_index]->setDState(dstate);
	entry_ptr->setDState(dstate);
	
	//set sharer's list
	entry_ptr->debugClearSharersList();
//	entry_ptr->dirDebugPrint();
	while(!sharers_list.empty())
	{
		assert( dstate != DramDirectoryEntry::UNCACHED );
		UINT32 new_sharer = sharers_list.back();
//		cerr << "ADDING SHARER-< " << new_sharer << " > " << endl;
		sharers_list.pop_back();
		entry_ptr->addSharer(new_sharer);
	}
//	entry_ptr->dirDebugPrint();
}

bool DramDirectory::debugAssertDramState(ADDRINT address, DramDirectoryEntry::dstate_t	expected_dstate, vector<UINT32> expected_sharers_vector)
{

	UINT32 cache_line_index = (address / bytes_per_cache_line) - ( num_lines * dram_id );
  
	assert( cache_line_index >= 0);

	DramDirectoryEntry* entry_ptr = dram_directory_entries[cache_line_index];
  
	if( entry_ptr == NULL ) {
		UINT32 memory_line_address = ( address / bytes_per_cache_line ) * bytes_per_cache_line;
		dram_directory_entries[cache_line_index] =  new DramDirectoryEntry( memory_line_address
																								, number_of_cores);
	}

//	entry_ptr->dirDebugPrint();
	DramDirectoryEntry::dstate_t actual_dstate = dram_directory_entries[cache_line_index]->getDState();
	bool is_assert_true = ( actual_dstate == expected_dstate ); 
	
	//copy STL vectors (which are just glorified stacks) and put data into array (for easier comparsion)
	bool* actual_sharers_array = new bool[number_of_cores];
	bool* expected_sharers_array = new bool[number_of_cores];
	
	for(int i=0; i < (int) number_of_cores; i++) {
		actual_sharers_array[i] = false;
		expected_sharers_array[i] = false;
	}

	vector<UINT32> actual_sharers_vector = entry_ptr->getSharersList();

	while(!actual_sharers_vector.empty())
	{
		UINT32 sharer = actual_sharers_vector.back();
		actual_sharers_vector.pop_back();
		assert( sharer >= 0);
		assert( sharer < number_of_cores );
		actual_sharers_array[sharer] = true;
//	  cerr << "Actual Sharers Vector Sharer-> Core# " << sharer << endl;
	}

	while(!expected_sharers_vector.empty())
	{
		UINT32 sharer = expected_sharers_vector.back();
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
	
	return is_assert_true;
}
