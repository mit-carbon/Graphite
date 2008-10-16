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
//DEPRECATE THIS CODE... we should not be giving the MMU a pointer to a dram entry
//TODO
DramDirectoryEntry* DramDirectory::getEntry(ADDRINT address)
{
                              
	// note: the directory is a map key'ed by cache line. so, first we need to determine the associated cache line
	//TODO i think i can take out the ( - (num_lines*dram_id) ) since its just a key.
	//and, its not a cache line.
	// I really dont understand what this is..why do you need "dram_id" in any case ?? 
	//FIXME this is incorrect
//	UINT32 cache_line_index = (address / bytes_per_cache_line) - ( num_lines * dram_id );
	UINT32 data_line_index = (address / bytes_per_cache_line);
  
#ifdef DRAM_DEBUG
	printf(" DRAM_DIR: getEntry: address        = 0x %x\n" ,address );
	cout << " DRAM_DIR: getEntry: bytes_per_$line= " << bytes_per_cache_line << endl;
	cout << " DRAM_DIR: getEntry: cachline_index = " << data_line_index << endl;
	cout << " DRAM_DIR: getEntry: number of lines= " << num_lines << endl;
#endif
  
	assert( data_line_index >= 0);

	DramDirectoryEntry* entry_ptr = dram_directory_entries[data_line_index];
  
	if( entry_ptr == NULL ) {
		UINT32 memory_line_address = ( address / bytes_per_cache_line ) * bytes_per_cache_line;
#ifdef DRAM_DEBUG	  
		debugPrint(dram_id, "DRAM_DIR", "memory_line_address", memory_line_address );
#endif		
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

	//get data from packet
	//
	

	char* data_buffer;

	copyDataToDram(data_buffer, data_size);

}


/* ======================================================== */
/* The Network sends memory requests to the MMU.
 * The MMU serializes the requests, and forwards them
 * to the DRAM_Directory.
 * ======================================================== */

//TODO deallocate the network packet buffers
void MemoryManager::processSharedMemReq(NetPacket req_packet) 
{
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
				
				NetPacket packet = makePacket(SHARED_MEM_UPDATE_UNEXPECTED, my_rank, current_owner, sizeof(UpdatePayload));

				// initialize packet payload for downgrade
/*				int payload[SH_MEM_UPDATE_NUM_INTS_SIZE];
				payload[SH_MEM_UPDATE_IDX_NEW_CSTATE] = CacheState::SHARED;
				payload[SH_MEM_UPDATE_IDX_ADDRESS] = address;               // TODO: cache line align?
*/
				UpdatePayload payload;
				payload.update_new_cstate = CacheState::SHARED;
   			payload.update_address= address;
				packet.data = (char *)(&payload);
				
#ifdef MMU_DEBUG
				ss.str("");
				ss << "   START netSend to <" << packet.receiver << "> ";
//				debugPrint(the_core->getRank(), "MMU::processSMemReq", ss.str());
#endif
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
				
#ifdef MMU_DEBUG
				ss.str("");
				ss << "   START netRecv: from Core <" << net_match.sender << "> " ; 
//				debugPrint(the_core->getRank(), "MMU::processSMemReq", ss.str());
#endif

				NetPacket recv_packet = (the_core->getNetwork())->netRecv(net_match);
//				debugPrint(the_core->getRank(), "MMU::processSMemReq", "   END   netRecv");
		
				// assert a few things in the ack packet (sanity checks)
				assert((unsigned int)(recv_packet.sender) == current_owner);
				assert(recv_packet.type == SHARED_MEM_ACK);
		
				ADDRINT received_address = ((AckPayload*)(recv_packet.data))->ack_address;
				stringstream ss;
				//TODO bug here b/c we're servicing multiple outstanding requests (and netMatch gives us the wrong packet)
				// How is this possible? We wait for our request to be serviced. right??
				if(received_address != address) {
					//print out info before we crash
					ss.str("");
					ss << "EXCLUSIVE OWNER: " << current_owner << ", ADDR: " << hex << address << "  , Received ADDR: " << hex << received_address << ", MemRequestType: " << sMemReqTypeToString( shmem_req_type ); 
//					debugPrint(my_rank, "MMU", ss.str());
				}	
				assert(received_address == address);
		
				CacheState::cstate_t received_new_cstate = (CacheState::cstate_t)(((AckPayload*)(recv_packet.data))->ack_new_cstate);
				assert(received_new_cstate == CacheState::SHARED);
				
            // did the former owner invalidate it already? if yes, we should remove him from the sharers list
				if( ((AckPayload*)(recv_packet.data))->remove_from_sharers )
				{
					dram_dir_entry->removeSharer( current_owner );
				}
				/* ========================================================================== */
				/* Modified by George */
				else {
					 // In this case, the former owner needs to write back to the DRAM before the current requestor
					 // can read from it - so, we need to add to the cost once
					 runDramAccessModel();
				}
				/* =========================================================================== */

			}
			
			// TODO: is there a race condition here in the case when the directory gets updated and then
			// this thread gets swapped out? should we atomize the state update and the response message
			// this get executed no matter what state the dram directory entry was in

			/* =================================================================== */
			/* Modified by George */
			// FIXME: One memory to cache transfer always needs to take place
			// Dont know whether this is the right place to put this in
			runDramAccessModel();
			/* =================================================================== */

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
		  	bool already_invalidated = false;
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
				ss << "Receiving Invalidation Ack Message from Sharer [" << sharers_list[i] << "]";
				debugPrint(the_core->getRank(), "MMU", ss.str());
#endif
				 
				// wait for all of the invalidation acknowledgements
				NetMatch net_match;
				net_match.sender = sharers_list[i]; //*sharers_iterator;
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

				 /* =========================================================== */
				 /* Modified by George */
				 // FIXME: Verify that this is correct
				 if (((AckPayload*)(recv_packet.data))->remove_from_sharers) {
					  // Remove this core from the sharers list
					  already_invalidated = true;
				 }
				 /* =========================================================== */

			}
			
		  	/* ============================================================= */
		  	/* Modified by George */
		    // FIXME: Verify that this is correct
			// TODO: Test my understanding of runDCachePeekModel() and runDCache[(Load)/(Store)]Model()
		  	if (dram_dir_entry->getDState() == DramDirectoryEntry::EXCLUSIVE) {
				// We need to write back this cache line to the DRAM 	
				if (! already_invalidated) {
					runDramAccessModel();
				}

		  	}
			
			// An call to the access model has to be there anyways for getting the data from the DRAM
			runDramAccessModel();
		  	/* ============================================================= */
		  
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
	switch (dram_dir_entry->getDState()) {
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
	debugPrint(the_core->getRank(), "MMU", "end of sharedMemReq function");
#endif
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
