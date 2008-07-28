#include "dram_directory_entry.h"

DramDirectoryEntry::DramDirectoryEntry() : dstate(DramDirectoryEntry::UNCACHED), sharers(0)
{}

DramDirectoryEntry::~DramDirectoryEntry()
{}

// returns true if the sharer was added and wasn't already on the list. returns false if the sharer wasn't added since it was already there
bool DramDirectorEntry::addSharer(unsigned int sharer_rank)
{
	if(find(sharers.begin(), sharers,end(), sharer_rank) == sharers.end()) {
		// the new sharer doesn't already exist in the sharers array
		sharers.push_back(sharer_rank);
		return true;
	} 
	else 
	{
		return false;
	}
}

void DramDirectorEntry::addExclusiveSharer(unsigned int sharer_rank)
{
	sharers.clear();
	sharers.push_back(sharer_rank);
}

void DramDirectoryEntry::toString()
{
   vector<unsigned int>::iterator sharers_iterator;
   sharers_iterator = sharers.begin();
   cout << "state= " << dstate;
   cout << "; sharers={";
   while( sharers_iterator != sharers.end()) {
      cout << *sharers_iterator << ", ";
      sharers_iterator++;
   }
   cout << "}" << endl;
   return;
}
