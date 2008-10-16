#ifndef DRAM_DIRECTORY_ENTRY_H
#define DRAM_DIRECTORY_ENTRY_H

#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

#include "assert.h"
#include "bit_vector.h"
#include "debug.h"

using namespace std;

extern LEVEL_BASE::KNOB<UINT32> g_knob_cache_line_size;

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
  DramDirectoryEntry(UINT32 mem_line_addr, UINT32 number_of_cores);
  DramDirectoryEntry(UINT32 mem_line_addr, UINT32 number_of_cores, char* data_buffer);
  virtual ~DramDirectoryEntry();
  
  dstate_t getDState();
  void setDState(dstate_t new_dstate);
  
  bool addSharer(UINT32 sharer_rank);
  void addExclusiveSharer(UINT32 sharer_rank);
  
  //the directory may remove someone if they've invalidated it from their cache
  void removeSharer(UINT32 sharer_rank);
  void debugClearSharersList();
  
  int numSharers();
  UINT32 getExclusiveSharerRank();
	
  vector<UINT32> getSharersList();

  void fillDramDataLine(char* fill_buffer);
  //input NULL fill_buffer, line_size
  //function fills the buffer and line_size values
  void getDramDataLine(char* fill_buffer, int* line_size);
  
  
  
  void dirDebugPrint();
  static string dStateToString(dstate_t dstate);
  UINT32 getMemLineAddress() { return memory_line_address; }
  
 private:
  dstate_t dstate;
  BitVector* sharers;
  UINT32 exclusive_sharer_rank;
  char* memory_line; //store the actual data in this buffer
  UINT32 memory_line_size; //assume to be the same size of the cache_line_size
  UINT32 number_of_sharers;
  UINT32 memory_line_address;	//only here for debugging purposes (its nice to know it)
										//(address / cache_line_size) * cache_line_size = memory_line_address
										//this is a memory line aligned address (the above operation shaves off
										//the insignificant digits)
  
};

#endif
