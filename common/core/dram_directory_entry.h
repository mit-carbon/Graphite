#ifndef DRAM_DIRECTORY_ENTRY_H
#define DRAM_DIRECTORY_ENTRY_H

#include <vector>
#include <string>
#include <algorithm>

class DramDirectoryEntry
{
public:
	enum dstate_t {
		UNCACHED, 
        EXCLUSIVE,
        SHARED,
        NUM_STATES
	};
	
	DramDirectoryEntry();
	virtual ~DramDirectoryEntry();
	
	bool addSharer(unsigned int sharer_rank);
	bool addExclusiveSharer(unsigned int sharer_rank);
	
	//FIXME: return string (don't cout)
	void toString();
	
private:
	dstate_t dstate;
	vector<unsigned int> sharers;
		
};

#endif
