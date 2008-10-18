#include "dram_directory.h"
#include "debug.h"

DramDirectory::DramDirectory(UINT32 num_lines_arg, UINT32 bytes_per_cache_line_arg, UINT32 dram_id_arg, UINT32 num_of_cores_arg)
{
	num_lines = num_lines_arg;
	number_of_cores = num_of_cores_arg;
	dram_id = dram_id_arg;
  cout << "Init Dram with num_lines: " << num_lines << endl;
  cout << "   bytes_per_cache_linew: " << bytes_per_cache_line_arg << endl;
  assert( num_lines >= 0 );
  bytes_per_cache_line = bytes_per_cache_line_arg;
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
	cout << " DRAM_DIR: getEntry: cachline_index = " << data_line_index << endl;
#endif
  
	assert( data_line_index >= 0);

	DramDirectoryEntry* entry_ptr = dram_directory_entries[data_line_index];
  
	//TODO FIXME we need to handle the data_buffer correctly! what do we fill it with?!
	if( entry_ptr == NULL ) {
		UINT32 memory_line_address = ( address / bytes_per_cache_line ) * bytes_per_cache_line;
		dram_directory_entries[data_line_index] =  new DramDirectoryEntry( memory_line_address
																								, number_of_cores);
	}

	return dram_directory_entries[data_line_index];
}

//George's stuff for modeling dram access costs
//TODO can I actually have all dram requests go through the directory?
void MemoryManager::runDramAccessModel () {
	dramAccessCost += g_knob_dram_access_cost.Value();
}

UINT64 MemoryManager::getDramAccessCost() {
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

void copyDataToDram(ADDRINT address, char* data_buffer, UINT32 data_size)
{

	//TODO provide a unique key, 
	UINT32 data_line_index = ( address / bytes_per_cache_line );

	if( entry_ptr == NULL ) {
		dram_directory_entries[data_line_index] = new DramDirectoryEntry( memory_line_address, number_of_cores, data_buffer );
		entry_ptr = dram_directory_entries[data_line_index];
		assert(entry_ptr != NULL );
	} 
	
	entry_ptr->addData(data_buffer, data_size);

}

void DramDirectory::processWriteBack(NetPacket req_packet)
{
	AckPayload payload; //TODO is it always acks? i think not! :(
	char data_buffer[cache_line_size];

	extractAckPayload(wb_packet, &payload, &data_buffer); 
	copyDataToDram(data_buffer, data_size);
	
	runDramAccessModel();
}


/* ======================================================== */
/* The Network sends memory requests to the MMU.
 * The MMU serializes the requests, and forwards them
 * to the DRAM_Directory.
 * ======================================================== */

void MemoryManager::processSharedMemReq(NetPacket req_packet) 
{
	/* ============================================================== */
	extract(req_packet);
	DramDirectoryEntry* dram_entry = getEntry(address);
	 
	switch(req_d_state)
	{
		WRITE: 
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

		READ:
			if(current_d_state==EXCLUSIVE)
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
	 
	addToSharers(requestor);
	sendDataLine(); //Dram cost model

	/* ============================================================== */

	stringstream ss;
#ifdef MMU_DEBUG
   dram_dir->print();
   debugPrint(the_core->getRank(), "DRAM_DIR", "Processing shared memory request.");
#endif
   
  	// extract relevant values from incoming request packet
	shmem_req_t shmem_req_type = (shmem_req_t)((RequestPayload*)(req_packet.data))->request_type;
  	ADDRINT address = ((RequestPayload*)(req_packet.data))->request_address;
  	unsigned int requestor = req_packet.sender;
  
  	DramDirectoryEntry* dram_dir_entry = dram_dir->getEntry(address);
  	dstate_t current_dstate = dram_dir_entry->getDState();

	switch( shmem_req_type ) {
	
		case WRITE: 
		{	
			if( current_dstate == EXCLUSIVE )
			{
				//invalidate owner. receive write_back packet
				NetPacket wb_packet;
				wb_packet = demoteOwner(dram_dir_entry, INVALID); //CHRIS TODO write demote owner, and process write-back
				processWriteBack(wb_packet);
			}
			else
			{
				invalidateSharers(dram_dir_entry);
			}
		}
		break;

		case READ:
		{
			// handle the case where this line is in the exclusive state (so data has to be written back first and that entry is downgraded to SHARED)
			if(current_dstate == DramDirectoryEntry::EXCLUSIVE) i
			{
				//demote exclusive owner
				NetPacket wb_packet;
				wb_packet = demoteOwner(dram_dir_entry, SHARED);
				processWriteBack(wb_packet);
			}
			else
			{
				//do nothing. no need to contact other sharers in this case.
			}
		}
		break;
      
		default:
			throw("unsupported memory transaction type.");
		break;
   }
  
	// 3. return data back to requestor 
	addToSharers(requestor);
	sendDataLine(dram_dir_entry, requestor, new_cstate); //Dram cost model

#ifdef MMU_DEBUG
	debugPrint(the_core->getRank(), "MMU", "end of sharedMemReq function");
#endif
}

//TODO this is a "untouched copy of process shared Mem Req
void MemoryManager::processSharedMemReq(NetPacket req_packet) 
{
	 
	/* ============================================================== */
	
	extract(req_packet);
	DramDirectoryEntry* dram_entry = getEntry(address);
	 
	switch(req_d_state)
	{
		WRITE: 
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

		READ:
			if(current_d_state==EXCLUSIVE)
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
	 
	addToSharers(requestor);
	sendDataLine(); //Dram cost model

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

	stringstream ss;
#ifdef MMU_DEBUG
  dram_dir->print();
  debugPrint(the_core->getRank(), "DRAM_DIR", "Processing shared memory request.");
#endif
   
  	// extract relevant values from incoming request packet
	shmem_req_t shmem_req_type = (shmem_req_t)((RequestPayload*)(req_packet.data))->request_type;
  	ADDRINT address = ((RequestPayload*)(req_packet.data))->request_address;
  	unsigned int requestor = req_packet.sender;
  
   // 0. get the DRAM directory entry associated with this address
  	DramDirectoryEntry* dram_dir_entry = dram_dir->getEntry(address);

  	// 1. based on requested operation (read or write), make necessary updates to current owners
  	// 2. update directory state in DRAM and sharers array
	
	switch( shmem_req_type ) {
	
		case READ:
		{
			// handle the case where this line is in the exclusive state (so data has to be written back first and that entry is downgraded to SHARED)
			if(dram_dir_entry->getDState() == DramDirectoryEntry::EXCLUSIVE) i
			{
				//demote exclusive owner
				assert(dram_dir_entry->numSharers() == 1);
		
				unsigned int current_owner = dram_dir_entry->getExclusiveSharerRank();
		
				// reqeust a data write back data and downgrade to shared
				NetPacket packet = makePacket(SHARED_MEM_UPDATE_UNEXPECTED, dram_id, current_owner, sizeof(UpdatePayload));
				payload.update_new_cstate = CacheState::SHARED;
   			payload.update_address= address;
				packet.data = (char *)(&payload);
				
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
		
				ADDRINT received_address = ((AckPayload*)(recv_packet.data))->ack_address;
				assert(received_address == address);
		
				CacheState::cstate_t received_new_cstate = (CacheState::cstate_t)(((AckPayload*)(recv_packet.data))->ack_new_cstate);
				assert(received_new_cstate == CacheState::SHARED);
				
            // did the former owner invalidate it already? if yes, we should remove him from the sharers list
				if( ((AckPayload*)(recv_packet.data))->remove_from_sharers )
				{
					dram_dir_entry->removeSharer( current_owner );
				}
				else 
				{
					 // In this case, the former owner needs to write back to the DRAM before the current requestor
					 // can read from it - so, we need to add to the cost once
//					 runDramAccessModel();
				}

			}
			
			/* Modified by George */
			// FIXME: One memory to cache transfer always needs to take place
			// Dont know whether this is the right place to put this in
//			runDramAccessModel();

//			dram_dir_entry->addSharer(requestor);
//			dram_dir_entry->setDState(DramDirectoryEntry::SHARED);
	   } 
		break;
      
		case WRITE: 
		{	
		  // invalidate current sharers
		  	bool already_invalidated = false;
			vector<UINT32> sharers_list = dram_dir_entry->getSharersList();
			
			for(UINT32 i = 0; i < sharers_list.size(); i++) 
			{
			 
#ifdef MMU_DEBUG
				ss.clear();
				ss << "Sending Invalidation Message to Sharer [" << sharers_list[i] << "]";
				debugPrint(the_core->getRank(), "MMU", "Sending Invalidation Message to Sharer");
#endif			 
				// send message to sharer to invalidate it
				NetPacket packet = makePacket( SHARED_MEM_UPDATE_UNEXPECTED, my_rank, sharers_list[i], sizeof(UpdatePayload));
				 
				/* format of shared memory request packet data
				   req_type | starting addr | length (in bytes) requested
				*/
				 
				UpdatePayload payload;
				payload.update_new_cstate = CacheState::INVALID;
				payload.update_address = address;
				packet.data = (char *)(&payload);
				(the_core->getNetwork())->netSend(packet);
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
				NetPacket recv_packet = (the_core->getNetwork())->netRecv(net_match);
				 
				// assert a few things in the ack packet (sanity checks)
				assert((unsigned int)(recv_packet.sender) == sharers_list[i]); // I would hope so
				 
				ADDRINT received_address = ((AckPayload*)(recv_packet.data))->ack_address;
#ifdef MMU_DEBUG
				ss << " Recieving Invalidation Ack Messages.... Data Addr: " << hex << ((int *)(recv_packet.data)) << ", Received Addr: " << hex << received_address << "  Addr: " << hex << address;
				debugPrint(my_rank, "MMU", ss.str());
#endif				
				assert(received_address == address);
				 
				 CacheState::cstate_t received_new_cstate = (CacheState::cstate_t)(((AckPayload*)(recv_packet.data))->ack_new_cstate);
				 assert(received_new_cstate == CacheState::INVALID);

				 /* Modified by George */
				 // FIXME: Verify that this is correct
				 if (((AckPayload*)(recv_packet.data))->remove_from_sharers) {
					  // Remove this core from the sharers list
					  already_invalidated = true;
				 }

			}
			
		  	/* Modified by George */
		    // FIXME: Verify that this is correct
		  	if (dram_dir_entry->getDState() == DramDirectoryEntry::EXCLUSIVE) {
				// We need to write back this cache line to the DRAM 	
				if (! already_invalidated) {
					runDramAccessModel();
				}

		  	}
			
			// An call to the access model has to be there anyways for getting the data from the DRAM
			runDramAccessModel();
		  
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
  
  // 3. return data back to requestor 
  NetPacket ret_packet;
  ret_packet.type = SHARED_MEM_UPDATE_EXPECTED;
  ret_packet.sender = my_rank;
  ret_packet.receiver = requestor;
  ret_packet.length = sizeof(UpdatePayload);
  
  // initialize packet payload
	UpdatePayload payload;
	CacheState::cstate_t new_cstate;

	//we need to map from dstate to cstate
	switch (dram_dir_entry->getDState()) {
		case DramDirectoryEntry::UNCACHED:
			assert ( 0 ); 
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
	
	char data_buffer[cache_line_size];
	getDramMemoryLine(some_address, &data_buffer); //GEORGE <- put your dramAccessCost into this function?

	int data_size;
	MemoryManager->createUpdatePayloadBuffer(payload, data_buffer, &data_size);
	NetPacket ret_packet = makePacket(SHARED_MEM_UPDATE_EXPECTED, my_rank, requestor, data_buffer);
	(the_core->getNetwork())->netSend(ret_packet);

#ifdef MMU_DEBUG
	debugPrint(the_core->getRank(), "MMU", "end of sharedMemReq function");
#endif
}

//TODO go through this code again. i think a lot of it is unnecessary.
//TODO rename this to something more descriptive about what it does (sending a memory line to another core)
void sendDataLine(DramDirectoryEntry* dram_dir_entry, UINT32 requestor, CacheState::cstate_t new_cstate)
{

   // initialize packet payload
	UpdatePayload payload;

	//we need to map from dstate to cstate
	switch (dram_dir_entry->getDState()) {
		case DramDirectoryEntry::UNCACHED:
			assert ( 0 ); 
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
	
	char data_buffer[cache_line_size];
	getDramMemoryLine(some_address, &data_buffer); //GEORGE <- put your dramAccessCost into this function?

	int data_size;
	MemoryManager->createUpdatePayloadBuffer(payload, data_buffer, &data_size);
	NetPacket packet = makePacket(SHARED_MEM_UPDATE_EXPECTED, my_rank, requestor, data_buffer);
	packet.type = SHARED_MEM_UPDATE_EXPECTED;
	packet.sender = dram_id;
	packet.receiver = requestor;
//	packet.length = sizeof(UpdatePayload); //TODO remove this. not correct.
	(the_core->getNetwork())->netSend(packet);

}

NetPacket demoteOwner(DramDirectoryEntry* dram_dir_entry, cstate_t new_cstate)
{
	//demote exclusive owner
	assert( dram_dir_entry->numSharers() == 1 );
	assert( new_cstate == SHARED || new_cstate == INVALID );

	unsigned int current_owner = dram_dir_entry->getExclusiveSharerRank();

	// reqeust a write back data payload and downgrade to new_dstate
	NetPacket packet = makePacket(SHARED_MEM_UPDATE_UNEXPECTED, dram_id, current_owner, sizeof(UpdatePayload));
	payload.update_new_cstate = new_cstate;
	payload.update_address= address;
	packet.data = (char *)(&payload);
	
	(the_core->getNetwork())->netSend(packet);

	// wait for the acknowledgement from the original owner that it downgraded itself to SHARED (from EXCLUSIVE)
	NetMatch net_match;
	net_match.sender = current_owner;
	net_match.sender_flag = true;
	net_match.type = SHARED_MEM_ACK;
	net_match.type_flag = true;
	
	NetPacket wb_packet = (the_core->getNetwork())->netRecv(net_match);
	
	// assert a few things in the ack packet (sanity checks)
	AckPayload payload;
	//null because we aren't interested in extracting the data_buffer here
	extractAckPayloadBuffer(wb_packet, &payload, NULL);
	
//	processWriteBack(wb_packet);

	assert((unsigned int)(wb_packet.sender) == current_owner);
	assert(wb_packet.type == SHARED_MEM_ACK);

	ADDRINT received_address = payload.ack_address; 
	assert(received_address == address);

	CacheState::cstate_t received_new_cstate = payload.ack_new_cstate; 
	assert(received_new_cstate == new_cstate);
	
	// TODO DOES THIS CASE EVER HAPPEN? ie, an owner that had already been evicted
	// did the former owner invalidate it already? if yes, we should remove him from the sharers list
	assert( payload.remove_from_sharers == false );
	if( new_cstate == INVALID || payload.remove_from_sharers )
		dram_dir_entry->removeSharer( current_owner );

	return wb_packet; //let the calling function perform the WriteBack so we don't implicitly do write backs
} //end demoteOwner()


//private utility function
//send invalidate messages to all of the sharers
//and wait on acks.
void invalidateSharers(DramDirectoryEntry* dram_dir_entry)
{
   //invalidate current sharers
	//bool already_invalidated = false;
	vector<UINT32> sharers_list = dram_dir_entry->getSharersList();
	
	for(UINT32 i = 0; i < sharers_list.size(); i++) 
	{
		// send message to sharer to invalidate it
		NetPacket packet = makePacket( SHARED_MEM_UPDATE_UNEXPECTED, my_rank, sharers_list[i], sizeof(UpdatePayload));
		 
		UpdatePayload payload;
		payload.update_new_cstate = CacheState::INVALID;
		payload.update_address = address;
		packet.data = (char *)(&payload);
		(the_core->getNetwork())->netSend(packet);
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
		NetPacket recv_packet = (the_core->getNetwork())->netRecv(net_match);
		 
		// assert a few things in the ack packet (sanity checks)
		assert((unsigned int)(recv_packet.sender) == sharers_list[i]); 
		ADDRINT received_address = ((AckPayload*)(recv_packet.data))->ack_address;
		assert(received_address == address);
		 
		CacheState::cstate_t received_new_cstate = (CacheState::cstate_t)(((AckPayload*)(recv_packet.data))->ack_new_cstate);
		assert(received_new_cstate == CacheState::INVALID);
	}

}

void DramDirectory::print()
{
	cout << endl << endl << " <<<<<<<<<<<<<<< PRINTING DRAMDIRECTORY INFO [" << dram_id << "] >>>>>>>>>>>>>>>>> " << endl << endl;
	std::map<UINT32, DramDirectoryEntry*>::iterator iter = dram_directory_entries.begin();
	while(iter != dram_directory_entries.end())
	{
		cout << "   ADDR (aligned): 0x" << hex << iter->second->getMemLineAddress() 
				<< "  DState: " << DramDirectoryEntry::dStateToString(iter->second->getDState());

		vector<UINT32> sharers = iter->second->getSharersList();
		cout << "  SharerList <size= " << sharers.size() << " > = { ";
		
		for(unsigned int i = 0; i < sharers.size(); i++) {
			cout << sharers[i] << " ";
		}
		
		cout << "} "<< endl;
		iter++;
	}
	cout << endl << " <<<<<<<<<<<<<<<<<<<<< ----------------- >>>>>>>>>>>>>>>>>>>>>>>>> " << endl << endl;
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
//		cout << "ADDING SHARER-< " << new_sharer << " > " << endl;
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
//	  cout << "Actual Sharers Vector Sharer-> Core# " << sharer << endl;
	}

	while(!expected_sharers_vector.empty())
	{
		UINT32 sharer = expected_sharers_vector.back();
		expected_sharers_vector.pop_back();
		assert( sharer >= 0);
		assert( sharer < number_of_cores );
		expected_sharers_array[sharer] = true;
//		cout << "Expected Sharers Vector Sharer-> Core# " << sharer << endl;
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
