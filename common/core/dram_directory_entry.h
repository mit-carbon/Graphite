#ifndef DRAM_DIRECTORY_ENTRY_H
#define DRAM_DIRECTORY_ENTRY_H

#include <vector>
#include <string>

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

      DramDirectoryEntry(IntPtr cache_line_address);
      DramDirectoryEntry(IntPtr cache_line_address, Byte* data_buffer);
      
      ~DramDirectoryEntry();
      
      // Static Functions - To set static variables
      static void setCacheLineSize(UInt32 cache_line_size_arg);
      static void setMaxSharers(UInt32 max_sharers_arg);
      static void setTotalCores(UInt32 total_cores_arg);

      dstate_t getDState();
      void setDState(dstate_t new_dstate);

      bool addSharer(UInt32 sharer_rank);
      UInt32 getEvictionId();
      void addExclusiveSharer(UInt32 sharer_rank);

      void removeSharer(UInt32 sharer_rank);
      bool hasSharer(UInt32 sharer_rank);

      SInt32 numSharers();
      UInt32 getExclusiveSharerRank();

      vector<UInt32> getSharersList();

      void fillDramDataLine(Byte* data_buffer);
      void getDramDataLine(Byte* data_buffer);

      static string dStateToString(dstate_t dstate);
      
      UInt32 getCacheLineAddress() { return cache_line_address; }

      // For Unit testing purposes
      void debugClearSharersList();

   private:
      
      static UInt32 cache_line_size;    // Assume to be the same size of the cache_line_size
      static UInt32 max_sharers;        // For limited directories
      static UInt32 total_cores;        // For storing total number of cores
      
      dstate_t dstate;

      BitVector* sharers;
      UInt32 exclusive_sharer_rank;
      UInt32 number_of_sharers;

      IntPtr cache_line_address;
      Byte* cache_line; 
      

      /*** STAT TRACKING ***/
      //TODO
      UInt32 stat_min_sharers;
      UInt32 stat_max_sharers;
      UInt32 stat_access_count;  
      UInt32 stat_avg_sharers;

};

#endif
