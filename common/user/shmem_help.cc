
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

