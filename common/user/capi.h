// Harshad Kasture
//
// This file declares the user level message passing functions used in multithreaded applications 
// for the multicore simulator.

#ifndef CAPI_H
#define CAPI_H

#include <iostream>
#include <string>
#include "pin.H"
#include "dram_directory_entry.h"
#include "cache_state.h"

typedef int CAPI_return_t;

typedef int CAPI_endpoint_t;

// externed so the names don't get name-mangled
extern "C" {

   CAPI_return_t CAPI_Initialize(int *rank);

   CAPI_return_t CAPI_rank(int * rank);

   CAPI_return_t CAPI_message_send_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint, 
                                     char * buffer, int size);

   CAPI_return_t CAPI_message_receive_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint, 
                                        char * buffer, int size);
                                        
	//these two are for unit testing shared memory from user program (setMemState and assertMemState) provides hooks into the chip
	CAPI_return_t CAPI_debugSetMemState( ADDRINT address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1 , vector<UINT32> sharers_list);
	
	CAPI_return_t CAPI_debugAssertMemState( ADDRINT address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, string test_code, string error_string);

	/*
	CAPI_return_t CAPI_setDramBoundaries( vector< pair< ADDRINT, ADDRINT> > addr_boundaries);
	*/
}
	
	//FIXME this is a temp hack function
	CAPI_return_t CAPI_Finish(int my_rank);
#endif
