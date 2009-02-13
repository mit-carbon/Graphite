
void ShmemDebugHelper::AliasedReadModeling()
{
#if 0
   if (g_chip->aliasMap.find(dcache_ld_addr) != g_chip->aliasMap.end())
   {
      dcache_ld_addr = g_chip->aliasMap[dcache_ld_addr];

      assert(dcache_ld_size == sizeof(UINT32));
      char data_ld_buffer[dcache_ld_size];

      LOG_PRINT("Doing read modelling for address: %x", dcache_ld_addr);

      dcacheRunModel(CacheBase::k_ACCESS_TYPE_LOAD, dcache_ld_addr, data_ld_buffer, dcache_ld_size);

      LOG_PRINT("Contents of data_ld_buffer: %x %x %x %x", (UINT32) data_ld_buffer[0], (UINT32) data_ld_buffer[1], (UINT32) data_ld_buffer[2], (UINT32) data_ld_buffer[3]);

      assert(is_dual_read == false);
   }
   else
   {
      // Discard all other read requests without any modelling
   }
#endif
}

void ShmemDebugHelper::AliasWriteModeling()
{
#if 0
   if (g_chip->aliasMap.find(dcache_st_addr) != g_chip->aliasMap.end())
   {
      dcache_st_addr = g_chip->aliasMap[dcache_st_addr];

      assert(dcache_st_size == sizeof(UINT32));
      char data_st_buffer[dcache_st_size];

      /*
         if ((dcache_st_addr >> g_knob_ahl_param) & 0x1) {
         memset (data_st_buffer, 'C', sizeof(UINT32));
         }
         else {
         memset (data_st_buffer, 'A', sizeof(UINT32));
         }
         */

      memset(data_st_buffer, 'z', sizeof(UINT32));

      LOG_PRINT("Doing write modelling for address: %x", dcache_st_addr);

      LOG_PRINT("Contents of data_st_buffer: %x %x %x %x", (UINT32) data_st_buffer[0], (UINT32) data_st_buffer[1], (UINT32) data_st_buffer[2], (UINT32) data_st_buffer[3]);

      dcacheRunModel(CacheBase::k_ACCESS_TYPE_STORE, dcache_st_addr, data_st_buffer, dcache_st_size);

   }
   else
   {
      // Discard all other write requests without any modelling
   }
#endif
}

void ShmemDebugHelper::debugSetInitialMemConditions(vector<IntPtr>& address_vector,
      vector< pair<INT32, DramDirectoryEntry::dstate_t> >& dram_vector, vector<vector<UINT32> >& sharers_list_vector,
      vector< vector< pair<INT32, CacheState::cstate_t> > >& cache_vector,
      vector<char*>& d_data_vector,
      vector<char*>& c_data_vector)
{
   vector<IntPtr> temp_address_vector = address_vector;

   assert(d_data_vector.size() == c_data_vector.size());
   assert(d_data_vector.size() == dram_vector.size());
   assert(cache_vector.size() == dram_vector.size());

   while (!dram_vector.empty())
   {  //TODO does this assume 1:1 core/dram allocation?

      IntPtr curr_address = address_vector.back();
      address_vector.pop_back();

      INT32 curr_dram_id = dram_vector.back().first;
      DramDirectoryEntry::dstate_t curr_dstate = dram_vector.back().second;
      dram_vector.pop_back();

      vector<UINT32> curr_sharers_list = sharers_list_vector.back();
      sharers_list_vector.pop_back();

      char *curr_d_data = d_data_vector.back();
      d_data_vector.pop_back();

      core[curr_dram_id].debugSetDramState(curr_address, curr_dstate, curr_sharers_list, curr_d_data);
   }

   address_vector = temp_address_vector;

   while (!cache_vector.empty())
   {

      IntPtr curr_address = address_vector.back();
      address_vector.pop_back();

      vector< pair<INT32, CacheState::cstate_t> > curr_cache_vector = cache_vector.back();
      cache_vector.pop_back();

      char *curr_c_data = c_data_vector.back();
      c_data_vector.pop_back();

      while (!curr_cache_vector.empty())
      {

         INT32 curr_cache_id = curr_cache_vector.back().first;
         CacheState::cstate_t curr_cstate = curr_cache_vector.back().second;
         curr_cache_vector.pop_back();

         core[curr_cache_id].debugSetCacheState(curr_address, curr_cstate, curr_c_data);
      }
   }

}

bool ShmemDebugHelper::debugAssertMemConditions(vector<IntPtr>& address_vector,
      vector< pair<INT32, DramDirectoryEntry::dstate_t> >& dram_vector, vector<vector<UINT32> >& sharers_list_vector,
      vector< vector< pair<INT32, CacheState::cstate_t> > >& cache_vector,
      vector<char*>& d_data_vector,
      vector<char*>& c_data_vector,
      string test_code, string error_string)
{
   bool all_asserts_passed = true;
   vector<IntPtr> temp_address_vector = address_vector;

   assert(d_data_vector.size() == c_data_vector.size());
   assert(d_data_vector.size() == dram_vector.size());
   assert(cache_vector.size() == dram_vector.size());

   while (!dram_vector.empty())
   {  //TODO does this assume 1:1 core/dram allocation?

      IntPtr curr_address = address_vector.back();
      address_vector.pop_back();

      INT32 curr_dram_id = dram_vector.back().first;
      DramDirectoryEntry::dstate_t curr_dstate = dram_vector.back().second;
      dram_vector.pop_back();

      vector<UINT32> curr_sharers_list = sharers_list_vector.back();
      sharers_list_vector.pop_back();

      char *curr_d_data = d_data_vector.back();
      d_data_vector.pop_back();

      if (! core[curr_dram_id].debugAssertDramState(curr_address, curr_dstate, curr_sharers_list, curr_d_data))
         all_asserts_passed = false;
   }

   address_vector = temp_address_vector;

   while (!cache_vector.empty())
   {

      IntPtr curr_address = address_vector.back();
      address_vector.pop_back();

      vector< pair<INT32, CacheState::cstate_t> > curr_cache_vector = cache_vector.back();
      cache_vector.pop_back();

      char *curr_c_data = c_data_vector.back();
      c_data_vector.pop_back();

      while (!curr_cache_vector.empty())
      {

         INT32 curr_cache_id = curr_cache_vector.back().first;
         CacheState::cstate_t curr_cstate = curr_cache_vector.back().second;
         curr_cache_vector.pop_back();

         if (! core[curr_cache_id].debugAssertCacheState(curr_address, curr_cstate, curr_c_data))
            all_asserts_passed = false;
      }
   }

   if (!all_asserts_passed)
   {
      cerr << "    *** ASSERTION FAILED *** : " << error_string << endl;
   }

   return all_asserts_passed;
}

/*user program calls get routed through this */
CAPI_return_t chipDebugSetMemState(IntPtr address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, char *d_data, char *c_data)
{
   vector<IntPtr> address_vector;
   vector< pair<INT32, DramDirectoryEntry::dstate_t> > dram_vector;
   vector< vector <UINT32> > sharers_list_vector;
   vector< vector < pair<INT32, CacheState::cstate_t> > > cache_vector;
   vector< pair<INT32, CacheState::cstate_t> > curr_cache_vector;

   vector<char*> d_data_vector;
   vector<char*> c_data_vector;

   assert(aliasMap.find(address) != aliasMap.end());
   address = aliasMap[address];
   address_vector.push_back(address);

   dram_vector.push_back(pair<INT32, DramDirectoryEntry::dstate_t>(dram_address_home_id, dstate));
   sharers_list_vector.push_back(sharers_list);

   curr_cache_vector.push_back(pair<INT32, CacheState::cstate_t>(0, cstate0));
   curr_cache_vector.push_back(pair<INT32, CacheState::cstate_t>(1, cstate1));
   cache_vector.push_back(curr_cache_vector);

   d_data_vector.push_back(d_data);
   c_data_vector.push_back(c_data);

   // cerr << "ChipDebug Set: d_data = 0x" << hex << (UINT32) d_data << ", c_data = 0x" << hex << (UINT32) c_data << endl;

   debugSetInitialMemConditions(address_vector,
                                dram_vector, sharers_list_vector,
                                cache_vector,
                                d_data_vector,
                                c_data_vector);

   return 0;
}

CAPI_return_t chipDebugAssertMemState(IntPtr address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, char *d_data, char *c_data, string test_code, string error_code)
{
   vector<IntPtr> address_vector;
   vector< pair<INT32, DramDirectoryEntry::dstate_t> > dram_vector;
   vector< vector <UINT32> > sharers_list_vector;
   vector< vector < pair<INT32, CacheState::cstate_t> > > cache_vector;
   vector< pair<INT32, CacheState::cstate_t> > curr_cache_vector;

   vector<char*> d_data_vector;
   vector<char*> c_data_vector;

   assert(aliasMap.find(address) != aliasMap.end());

   address = aliasMap[address];
   address_vector.push_back(address);

   dram_vector.push_back(pair<INT32, DramDirectoryEntry::dstate_t>(dram_address_home_id, dstate));
   sharers_list_vector.push_back(sharers_list);

   curr_cache_vector.push_back(pair<INT32, CacheState::cstate_t>(0, cstate0));
   curr_cache_vector.push_back(pair<INT32, CacheState::cstate_t>(1, cstate1));
   cache_vector.push_back(curr_cache_vector);

   d_data_vector.push_back(d_data);
   c_data_vector.push_back(c_data);

   // cerr << "ChipDebug Assert: d_data = 0x" << hex << (UINT32) d_data << ", c_data = 0x" << hex << (UINT32) c_data << endl;

   if (debugAssertMemConditions(address_vector,
                                dram_vector, sharers_list_vector,
                                cache_vector,
                                d_data_vector,
                                c_data_vector,
                                test_code, error_code))
   {
      return 1;
   }
   else
   {
      return 0;
   }

}
