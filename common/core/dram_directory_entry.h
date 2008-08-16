#ifndef DRAM_DIRECTORY_ENTRY_H
#define DRAM_DIRECTORY_ENTRY_H

#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

#include "assert.h"

using namespace std;

class DramDirectoryEntry
{
 public:
  enum dstate_t {
    UNCACHED, 
    EXCLUSIVE,
    SHARED,
    NUM_DSTATE_STATES
  };
  
  DramDirectoryEntry();
  virtual ~DramDirectoryEntry();
  
  dstate_t getDState();
  void setDState(dstate_t new_dstate);
  
  bool addSharer(unsigned int sharer_rank);
  void addExclusiveSharer(unsigned int sharer_rank);
  
  int numSharers();
  unsigned int getExclusiveSharerRank();
  vector<unsigned int>::iterator getSharersIterator();
  vector<unsigned int>::iterator getSharersSentinel(); // useful for a sentinel value in iterators
  
  //FIXME: return string (don't cout)
  void toString();
  
 private:
  dstate_t dstate;
  vector<unsigned int> sharers;
  
};

#endif
