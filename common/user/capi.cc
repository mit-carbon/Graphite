#include "capi.h"

// Stub implementations of user level message passing functions declared in capi.h
// These functions are replaced by the pintool with functions that implement the 
// corresponding message passing primitives.

CAPI_return_t CAPI_Initialize(int rank)
{ 
   rank = 0;
   // cout << "Running Application Without Pintool in Uniprocessor mode" << endl;
   return 0; 
}

CAPI_return_t CAPI_Initialize_FreeRank(int *rank)
{ 
   *rank = 0;
   return 0; 
}

CAPI_return_t CAPI_rank(int *rank)
{
   *rank = 0;
   // cout << "Running Application Without Pintool in Uniprocessor mode" << endl;
   return 0;
}

CAPI_return_t CAPI_message_send_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint, 
                                  char * buffer, int size)
{ 
   // cout << "Running Application Without Pintool in Uniprocessor mode" << endl;
   return 0; 
}

CAPI_return_t CAPI_message_receive_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint, 
                                     char * buffer, int size)
{ 
   // cout << "Running Application Without Pintool in Uniprocessor mode" << endl;
   return 0; 
}

//FIXME this is a hacked stub to get around network/message issue
CAPI_return_t CAPI_Finish(int my_rank)
{
   // cout << "Running Application Without Pintool in Uniprocessor mode" << endl;
	return 0;
}


CAPI_return_t CAPI_Print(string s)
{
   cout << "Running Application Without Pintool in Uniprocessor mode" << endl;
   cout << s;
	return 0;
}


//FIXME how to deal with pin types? i can't see this here
//CAPI_return_t CAPI_debugSetMemState( ADDRINT address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1 ) 
CAPI_return_t CAPI_debugSetMemState( ADDRINT address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1 , vector<UINT32> sharers_list, char *d_data, char *c_data ) 
{
	return 0;
}

CAPI_return_t CAPI_debugAssertMemState( ADDRINT address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, char *d_data, char *c_data, string test_code, string error_string) 
{
	return 0;
}


CAPI_return_t CAPI_alias (ADDRINT address, addr_t addrType, UINT32 num) {
	 return 0;
}

