#include "dram_directory_entry.h"

DramDirectoryEntry::DramDirectoryEntry() : dstate(DramDirectoryEntry::UNCACHED), sharers(0)
{}

DramDirectoryEntry::~DramDirectoryEntry()
{}

// returns true if the sharer was added and wasn't already on the list. returns false if the sharer wasn't added since it was already there
bool DramDirectoryEntry::addSharer(unsigned int sharer_rank)
{
  if(find(sharers.begin(), sharers.end(), sharer_rank) == sharers.end()) {
    // the new sharer doesn't already exist in the sharers array
    sharers.push_back(sharer_rank);
    return true;
  } 
  else 
    {
      return false;
    }
}

void DramDirectoryEntry::addExclusiveSharer(unsigned int sharer_rank)
{
  sharers.clear();
  sharers.push_back(sharer_rank);
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

int DramDirectoryEntry::numSharers()
{
  return sharers.size();
}

vector<unsigned int>::iterator DramDirectoryEntry::getSharersIterator()
{
  return sharers.begin();
}

// useful for a sentinel value in iterators
vector<unsigned int>::iterator DramDirectoryEntry::getSharersSentinel()
{
  return sharers.end();
}

unsigned int DramDirectoryEntry::getExclusiveSharerRank()
{
  assert(numSharers() == 1);
  return sharers[0];
}

void DramDirectoryEntry::toString()
{
  vector<unsigned int>::iterator sharers_iterator = getSharersIterator();
  cout << "state= " << dstate;
  cout << "; sharers={";
  while( sharers_iterator != getSharersSentinel()) {
    cout << *sharers_iterator << ", ";
    sharers_iterator++;
  }
  cout << "}" << endl;
  return;
}
