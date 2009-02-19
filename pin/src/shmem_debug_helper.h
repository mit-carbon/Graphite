#ifndef SHMEM_DEBUG_HELPER_H
#define SHMEM_DEBUG_HELPER_H

#include "capi.h"

CAPI_return_t chipDebugSetMemState(IntPtr address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, char *d_data, char *c_data);
CAPI_return_t chipDebugAssertMemState(IntPtr address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, char *d_data, char *c_data, string test_code, string error_code);

class ShmemDebugHelper
{
   public:
      bool aliasEnabled() { return false; }
      void aliasReadModeling();
      void aliasWriteModeling();

      //input parameters:
      //an address to set the conditions for
      //a dram vector, with a pair for the id's of the dram directories to set, and the value to set it to
      //a cache vector, with a pair for the id's of the caches to set, and the value to set it to
      void debugSetInitialMemConditions(vector<IntPtr>& address_vector,
                                        vector< pair<SInt32, DramDirectoryEntry::dstate_t> >& dram_vector, vector<vector<UInt32> >& sharers_list_vector,
                                        vector< vector< pair<SInt32, CacheState::cstate_t> > >& cache_vector,
                                        vector<char*>& d_data_vector,
                                        vector<char*>& c_data_vector);
      bool debugAssertMemConditions(vector<IntPtr>& address_vector,
                                    vector< pair<SInt32, DramDirectoryEntry::dstate_t> >& dram_vector, vector<vector<UInt32> >& sharers_list_vector,
                                    vector< vector< pair<SInt32, CacheState::cstate_t> > >& cache_vector,
                                    vector<char*>& d_data_vector,
                                    vector<char*>& c_data_vector,
                                    string test_code, string error_string);
};

#endif
