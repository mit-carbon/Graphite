#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
using namespace std;

// Forward Decls
namespace PrL1PrL2DramDirectoryMOSI
{
   class MemoryManager;
}

#include "directory.h"
#include "shmem_perf_model.h"

namespace PrL1PrL2DramDirectoryMOSI
{
   class DramDirectoryCache
   {
      private:
         MemoryManager* m_memory_manager;
         Directory* m_directory;
         vector<DirectoryEntry*> m_replaced_directory_entry_list;

         UInt32 m_total_entries;
         UInt32 m_associativity;

         UInt32 m_num_sets;
         UInt32 m_cache_block_size;
         UInt32 m_log_num_sets;
         UInt32 m_log_cache_block_size;

         UInt32 m_log_num_dram_cntlrs;
         UInt32 m_log_num_cores;

         static const UInt32 ADDRESS_THRESHOLD = 22;

         // Collect replacement statistics
         UInt64* histogram;
         map<IntPtr, SInt32> m_replaced_address_list;

         UInt32 m_dram_directory_cache_access_time;
         ShmemPerfModel* m_shmem_perf_model;

         MemoryManager* getMemoryManager() { return m_memory_manager; }
         ShmemPerfModel* getShmemPerfModel() { return m_shmem_perf_model; }

         void splitAddress(IntPtr address, IntPtr& tag, UInt32& set_index);
         UInt32 selectBits(IntPtr address, UInt32 low, UInt32 high);

         UInt32 getCacheBlockSize() { return m_cache_block_size; }
         UInt32 getLogCacheBlockSize() { return m_log_cache_block_size; }
         UInt32 getNumSets() { return m_num_sets; }
         UInt32 getLogNumSets() { return m_log_num_sets; }
         UInt32 getLogNumDramCntlrs() { return m_log_num_dram_cntlrs; }
         UInt32 getLogNumCores() { return m_log_num_cores; }
     
         void aggregateStatistics(UInt64*, UInt64&, UInt64&, UInt64&, SInt32& max_index, IntPtr& max_replaced_address, SInt32& max_replaced_times);

         // Bookkeeping of the random bits in an address
         set<IntPtr> m_global_address_set;
         vector<UInt64> m_address_histogram_one;
         vector<UInt64> m_address_histogram;

         void initializeRandomBitsTracker();
         void processRandomBits(IntPtr address);
         void printRandomBitsHistogram();
         // --------------------------------------------

      public:

         DramDirectoryCache(MemoryManager* memory_manager,
               string directory_type_str,
               UInt32 total_entries,
               UInt32 associativity,
               UInt32 cache_block_size,
               UInt32 max_hw_sharers,
               UInt32 max_num_sharers,
               UInt32 num_dram_cntlrs,
               UInt32 dram_directory_cache_access_time,
               ShmemPerfModel* shmem_perf_model);
         ~DramDirectoryCache();

         DirectoryEntry* getDirectoryEntry(IntPtr address);
         DirectoryEntry* replaceDirectoryEntry(IntPtr replaced_address, IntPtr address);
         void invalidateDirectoryEntry(IntPtr address);
         void getReplacementCandidates(IntPtr address, vector<DirectoryEntry*>& replacement_candidate_list); 

         void outputSummary(ostream& out);
         static void dummyOutputSummary(ostream& out);
   };
}
