
#include "dram_directory_entry.h"
#include "cache_state.h"
//these two are for unit testing shared memory from user program (setMemState and assertMemState) provides hooks into the chip

CAPI_return_t CAPI_debugSetMemState( IntPtr address, SInt32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1 , vector<UInt32> sharers_list, char *d_data, char *c_data);

CAPI_return_t CAPI_debugAssertMemState( IntPtr address, SInt32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UInt32> sharers_list, char *d_data, char *c_data, string test_code, string error_string);

//FIXME this is a temp hack function
CAPI_return_t CAPI_Finish(int my_rank);

//i believe locks in cout are screwing up out-of-band messages, 
//so let's not instrument print outs.

CAPI_return_t CAPI_Print(string s);


// FIXME: A hack
CAPI_return_t CAPI_alias(IntPtr address, addr_t addrType, UInt32 num);

