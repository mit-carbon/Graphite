#ifndef DRAM_DIRECTORY_ENTRY_H
#define DRAM_DIRECTORY_ENTRY_H

#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

#include "assert.h"
#include "utils.h"
#include "debug.h"

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
  DramDirectoryEntry(UINT32 cache_line_addr, UINT32 number_of_cores);
  virtual ~DramDirectoryEntry();
  
  dstate_t getDState();
  void setDState(dstate_t new_dstate);
  
  bool addSharer(UINT32 sharer_rank);
  void addExclusiveSharer(UINT32 sharer_rank);
  
  int numSharers();
  UINT32 getExclusiveSharerRank();
	
	vector<UINT32> getSharersList();

  //FIXME: return string (don't cout)
  void dirDebugPrint();
  
 private:
  dstate_t dstate;
  BitVector* sharers;
  UINT32 number_of_sharers;
  UINT32 exclusive_sharer_rank;
  UINT32 cache_line_address;	//only here for debugging purposes (its nice to know it)
										//(address / cache_line_size) = cache_line_address
										//called cache_line_index, and used as the key to find this entry
  
};

#endif
