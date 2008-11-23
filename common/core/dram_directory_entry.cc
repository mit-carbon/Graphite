#include "dram_directory_entry.h"

DramDirectoryEntry::DramDirectoryEntry(): 
																 dstate(UNCACHED), 
																 exclusive_sharer_rank(0), 
																 number_of_sharers(0), 
																 MAX_SHARERS(g_knob_dir_max_sharers), 
																 memory_line_size(g_knob_line_size),
																 memory_line_address(0)
{
  stat_min_sharers = 0;
  stat_max_sharers = 0;
  stat_access_count = 0;
  stat_avg_sharers = 0; 
}                                                                          

DramDirectoryEntry::DramDirectoryEntry(UINT32 cache_line_addr, UINT32 number_of_cores): 
																 dstate(UNCACHED),
																 exclusive_sharer_rank(0),
																 number_of_sharers(0),
																 MAX_SHARERS(g_knob_dir_max_sharers), 
																 memory_line_size(g_knob_line_size),
																 memory_line_address(cache_line_addr)
{
	sharers = new BitVector(number_of_cores);
	memory_line = new char[memory_line_size];

	//clear memory_line
	memset (memory_line, '\0', memory_line_size);

  stat_min_sharers = 0;
  stat_max_sharers = 0;
  stat_access_count = 0;
  stat_avg_sharers = 0; 
}                                                                         

DramDirectoryEntry::DramDirectoryEntry(UINT32 cache_line_addr, UINT32 number_of_cores, char* data_buffer): 
																	dstate(UNCACHED),
																	exclusive_sharer_rank(0),
																	number_of_sharers(0),
																	MAX_SHARERS(g_knob_dir_max_sharers), 
																	memory_line_size(g_knob_line_size),
																	memory_line_address(cache_line_addr)
{
	sharers = new BitVector(number_of_cores);
	memory_line = new char[memory_line_size];

	//copy memory_line
	memcpy (memory_line, data_buffer, memory_line_size);
  
   stat_min_sharers = 0;
   stat_max_sharers = 0;
   stat_access_count = 0;
   stat_avg_sharers = 0; 

}                                                                          

DramDirectoryEntry::~DramDirectoryEntry()
{
	if(sharers != NULL)
		delete sharers;
	delete [] memory_line;

}


void DramDirectoryEntry::fillDramDataLine(char* input_buffer)
{
	assert( input_buffer != NULL );
	assert( memory_line != NULL );
	
	memcpy (memory_line, input_buffer, memory_line_size);

}

void DramDirectoryEntry::getDramDataLine(char* fill_buffer, UINT32* line_size)
{
	assert( fill_buffer != NULL );
	assert( memory_line != NULL );
	
	*line_size = memory_line_size;

	memcpy (fill_buffer, memory_line, memory_line_size);

}

//returns <eviction, eviction_id> pair for use with limited_directories
//bool is true if an eviction occurred
//eviction_id is set to the evicted sharer.
//the directory (that calls this function) is responsible for dealing with
//the actual invalidation
//must also keep track of exclusive sharer and number of sharers
//FIXME: is it necessary to track exclusive sharer rank here?
pair<bool, UINT32> DramDirectoryEntry::addSharer(UINT32 sharer_rank)
{
	bool eviction = false;
	UINT32 eviction_id = -1;
	
	if(!sharers->at(sharer_rank)) {
		if(number_of_sharers == MAX_SHARERS) {
//			cerr << "Number of Sharers: " << number_of_sharers << endl;
//			cerr << "MAX_SHARERS:       " << MAX_SHARERS << endl;
//			cerr << "g_knob_dir_max_shr:" << g_knob_dir_max_sharers << endl;
			eviction = true;
			eviction_id = getEvictedId();
			assert( sharers->at(eviction_id) );
			sharers->clear(eviction_id);
			number_of_sharers--;
		}
		sharers->set(sharer_rank);
		exclusive_sharer_rank = sharer_rank;
		number_of_sharers++;
		return pair<bool,UINT32>(eviction,eviction_id);
	} else {
		//sharer already present.
		return pair<bool,UINT32>(false,-1);
	}
}

UINT32 DramDirectoryEntry::getEvictedId() 
{
	vector<UINT32> sharers_list = getSharersList();
	assert(sharers_list.size() > 0);
	return sharers_list[0];
}

void DramDirectoryEntry::removeSharer(UINT32 sharer_rank)
{
	if(sharers->at(sharer_rank)) {
		assert( number_of_sharers != 0 );
		sharers->clear(sharer_rank);
		number_of_sharers--;
	}
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
  

	if( (new_dstate == UNCACHED) && (number_of_sharers != 0) ) {
		cerr << "UH OH!  Settingi to UNCached.... number_of_sharers == " << number_of_sharers << endl;
		dirDebugPrint();
	}
	assert( (new_dstate == UNCACHED) ? (number_of_sharers == 0) : true );
  
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

	int i = 0;
	while( (new_sharer = sharers->find()) != -1) {
		sharers_list[i] = new_sharer;
		++i;
	}
	
	return sharers_list;
}

void DramDirectoryEntry::dirDebugPrint()
{
	cerr << " -== DramDirectoryEntry ==-" << endl;
	cerr << "     Addr= " << hex << memory_line_address << dec;
	cerr << "     state= " << dStateToString(dstate);
   cerr << "; sharers = { ";
	for(unsigned int i=0; i < sharers->getSize(); i++) {
		if(sharers->at(i)) {
			cerr << i << ", ";
		}
	}
	cerr << "}" << endl << endl;
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
