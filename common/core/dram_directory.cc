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
	//TODO i think i can take out the ( - (num_lines*dram_id) ) since its just a key.
	//and, its not a cache line.
	// I really dont understand what this is..why do you need "dram_id" in any case ?? 
	UINT32 cache_line_index = (address / bytes_per_cache_line) - ( num_lines * dram_id );
  
#ifdef DRAM_DEBUG
	printf(" DRAM_DIR: getEntry: address        = 0x %x\n" ,address );
	cout << " DRAM_DIR: getEntry: bytes_per_$line= " << bytes_per_cache_line << endl;
	cout << " DRAM_DIR: getEntry: cachline_index = " << cache_line_index << endl;
	cout << " DRAM_DIR: getEntry: number of lines= " << num_lines << endl;
#endif
  
//	assert( cache_line_index < num_lines);
	assert( cache_line_index >= 0);

	DramDirectoryEntry* entry_ptr = dram_directory_entries[cache_line_index];
  
	if( entry_ptr == NULL ) {
		UINT32 memory_line_address = ( address / bytes_per_cache_line ) * bytes_per_cache_line;
#ifdef DRAM_DEBUG	  
		debugPrint(dram_id, "DRAM_DIR", "memory_line_address", memory_line_address );
#endif		
		dram_directory_entries[cache_line_index] =  new DramDirectoryEntry( memory_line_address
																								, number_of_cores);
	}

	return dram_directory_entries[cache_line_index];
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
