#ifndef DRAM_DIRECTORY_ENTRY_H
#define DRAM_DIRECTORY_ENTRY_H

#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

#include "assert.h"
#include "bit_vector.h"

using namespace std;

class DramDirectoryEntry
{
   public:

      enum dstate_t
      {
         UNCACHED,
         EXCLUSIVE,
         SHARED,
         NUM_DSTATE_STATES
      };

      DramDirectoryEntry();
      DramDirectoryEntry(UInt32 mem_line_addr, UInt32 number_of_cores);
      DramDirectoryEntry(UInt32 mem_line_addr, UInt32 number_of_cores, char* data_buffer);
      
      ~DramDirectoryEntry();

      dstate_t getDState();
      void setDState(dstate_t new_dstate);

      bool addSharer(UInt32 sharer_rank);
      UInt32 getEvictionId();
      void addExclusiveSharer(UInt32 sharer_rank);

      void removeSharer(UInt32 sharer_rank);
      bool hasSharer(UInt32 sharer_rank);

      int numSharers();
      UInt32 getExclusiveSharerRank();

      vector<UInt32> getSharersList();

      void fillDramDataLine(char* fill_buffer);
      //input NULL fill_buffer, line_size
      //function fills the buffer and line_size values
      void getDramDataLine(char* fill_buffer, UInt32* line_size);

      static string dStateToString(dstate_t dstate);
      UInt32 getMemLineAddress() { return memory_line_address; }
      UInt32 getMemLineSize() {return memory_line_size; }

      // For Unit testing purposes
      void debugClearSharersList();

   private:
      dstate_t dstate;

      BitVector* sharers;
      UInt32 exclusive_sharer_rank;
      UInt32 number_of_sharers;
      unsigned int MAX_SHARERS; //for limited directories

      char* memory_line; //store the actual data in this buffer
      UInt32 memory_line_size; //assume to be the same size of the cache_line_size
      UInt32 memory_line_address;

      UInt32 lock;

      /*** STAT TRACKING ***/
      //TODO
      UInt32 stat_min_sharers;
      UInt32 stat_max_sharers;
      UInt32 stat_access_count;  //when do we increment this?
      UInt32 stat_avg_sharers;

};

#endif
