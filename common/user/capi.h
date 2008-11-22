// Harshad Kasture
//
// This file declares the user level message passing functions used in multithreaded applications 
// for the multicore simulator.

#ifndef CAPI_H
#define CAPI_H


#include "dram_directory_entry.h"
#include "cache_state.h"


typedef int CAPI_return_t;

typedef int CAPI_endpoint_t;
// FIXME: Hack for debugging purposes
enum addr_t
{
	DRAM_0,
	DRAM_1,
	DRAM_00,
	DRAM_01,
	DRAM_10,
	DRAM_11,
	NUM_ADDR_TYPES
};


#ifdef __cplusplus
// externed so the names don't get name-mangled
extern "C" {
#endif

   CAPI_return_t CAPI_Initialize(int rank);
   CAPI_return_t CAPI_Initialize_FreeRank(int *rank);

   CAPI_return_t CAPI_rank(int *rank);

   CAPI_return_t CAPI_message_send_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint, 
                                     char * buffer, int size);

   CAPI_return_t CAPI_message_receive_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint, 
                                        char * buffer, int size);
                                        
	//these two are for unit testing shared memory from user program (setMemState and assertMemState) provides hooks into the chip
	
	CAPI_return_t CAPI_debugSetMemState( ADDRINT address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1 , vector<UINT32> sharers_list, char *d_data, char *c_data);
	
	CAPI_return_t CAPI_debugAssertMemState( ADDRINT address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, char *d_data, char *c_data, string test_code, string error_string);
	
	//FIXME this is a temp hack function
	CAPI_return_t CAPI_Finish(int my_rank);

	//i believe locks in cout are screwing up out-of-band messages, 
	//so let's not instrument print outs.
	
	CAPI_return_t CAPI_Print(string s);
	

	// FIXME: A hack
	CAPI_return_t CAPI_alias(ADDRINT address, addr_t addrType, UINT32 num);
	

#ifdef __cplusplus
}
#endif


#endif
