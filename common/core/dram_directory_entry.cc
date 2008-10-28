#include "dram_directory_entry.h"

DramDirectoryEntry::DramDirectoryEntry(): 
																 dstate(UNCACHED), 
																 exclusive_sharer_rank(0), 
																 number_of_sharers(0), 
																 memory_line_size(g_knob_line_size),
																 memory_line_address(0)
{
}                                                                          

DramDirectoryEntry::DramDirectoryEntry(UINT32 cache_line_addr, UINT32 number_of_cores): 
																 dstate(UNCACHED),
																 exclusive_sharer_rank(0),
																 number_of_sharers(0),
																 memory_line_size(g_knob_line_size),
																 memory_line_address(cache_line_addr)
{
	sharers = new BitVector(number_of_cores);
	memory_line = new char[memory_line_size];

	//clear memory_line
	for(unsigned int i=0 ; i < memory_line_size; i++)
		memory_line[i] = (char) 0;

}                                                                          

DramDirectoryEntry::DramDirectoryEntry(UINT32 cache_line_addr, UINT32 number_of_cores, char* data_buffer): 
																	dstate(UNCACHED),
																	exclusive_sharer_rank(0),
																	number_of_sharers(0),
																	memory_line_size(g_knob_line_size),
																	memory_line_address(cache_line_addr)
{
	sharers = new BitVector(number_of_cores);
	memory_line = new char(memory_line_size);

	//copy memory_line
	//assumes data_buffer is the entire line
	for(unsigned int i=0 ; i < memory_line_size; i++)
		memory_line[i] = data_buffer[i];

}                                                                          

DramDirectoryEntry::~DramDirectoryEntry()
{
	if(sharers!=NULL)
		delete sharers;	

		//TODO memory leak deallocate memory_line
}


void DramDirectoryEntry::fillDramDataLine(char* input_buffer)
{
	assert( input_buffer != NULL );
	assert( memory_line != NULL );
	
	for( unsigned int i=0; i < memory_line_size; i++)
		memory_line[i] = input_buffer[i];

}

void DramDirectoryEntry::getDramDataLine(char* fill_buffer, UINT32* line_size)
{
	assert( fill_buffer != NULL );
	assert( memory_line != NULL );
	
	*line_size = memory_line_size;

	for( unsigned int i=0; i < memory_line_size; i++)
		fill_buffer[i] = memory_line[i];

}

// returns true if the sharer was added and wasn't already on the list. returns false if the sharer wasn't added since it was already there
//must also keep track of exclusive sharer and number of sharers
//FIXME: is it necessary to track exclusive sharer rank here?
bool DramDirectoryEntry::addSharer(UINT32 sharer_rank)
{
	if(!sharers->at(sharer_rank)) {
		sharers->set(sharer_rank);
		exclusive_sharer_rank = sharer_rank;
		number_of_sharers++;
		return true;
	} else {
		return false;
	}
}

void DramDirectoryEntry::removeSharer(UINT32 sharer_rank)
{
	sharers->clear(sharer_rank);
	number_of_sharers--;
}

void DramDirectoryEntry::addExclusiveSharer(UINT32 sharer_rank)
{
	sharers->reset();
	sharers->set(sharer_rank);
	number_of_sharers = 1;
	exclusive_sharer_rank = sharer_rank;
}

DramDirectoryEntry::dstate_t DramDirectoryEntry::getDState()
{
	return dstate;
}

void DramDirectoryEntry::setDState(dstate_t new_dstate)
{
  assert((int)(new_dstate) >= 0 && (int)(new_dstate) < NUM_DSTATE_STATES);
  dstate = new_dstate;
}

/* Return the number of cores currently sharing this entry */
int DramDirectoryEntry::numSharers()
{
  return number_of_sharers;
}

UINT32 DramDirectoryEntry::getExclusiveSharerRank()
{
	//FIXME is there a more efficient way of deducing exclusive sharer?
	//can i independently track exclusive rank and not slow everything else down?
  assert(numSharers() == 1);
  assert(dstate = EXCLUSIVE);
  //exclusive_sharer_rank is only valid if state is actually exclusive!
  //no garantee on value if not exclusive
  return exclusive_sharer_rank;
}

vector<UINT32> DramDirectoryEntry::getSharersList() 
{
	
	vector<UINT32> sharers_list(number_of_sharers);

	sharers->resetFind();

	int new_sharer = -1;

//	cout << "  Getting Sharers List<size= " << number_of_sharers << " > : (";
	
	int i = 0;
	while( (new_sharer = sharers->find()) != -1) {
//		cout << new_sharer << " ,  ";
      //is [ ] accessor faster?
//		sharers_list.push_back(new_sharer);
		sharers_list[i] = new_sharer;
		++i;
	}
	
//	cout << ") " << endl;
	return sharers_list;
}

void DramDirectoryEntry::dirDebugPrint()
{
	cout << " -== DramDirectoryEntry ==-" << endl;
	cout << "     Addr= " << hex << memory_line_address << dec;
	cout << "     state= " << dStateToString(dstate);
    cout << "; sharers = { ";
	for(unsigned int i=0; i < sharers->getSize(); i++) {
		if(sharers->at(i)) {
			cout << i << ", ";
		}
	}
	cout << "}" << endl << endl;
}

string DramDirectoryEntry::dStateToString(dstate_t dstate)
{
	switch(dstate) {
		case UNCACHED:	 return "UNCACHED  ";
		case EXCLUSIVE: return "EXCLUSIVE ";
		case SHARED :   return "SHARED    ";
		default: return "ERROR: DSTATETOSTRING";
	}

	return "ERROR: DSTATETOSTRING";
}

/* This function is used for unit testing dram,
 * in which we need to clear and set the sharers list
 */
void DramDirectoryEntry::debugClearSharersList()
{
	sharers->reset();
	number_of_sharers = 0;
	exclusive_sharer_rank = 0;
}
