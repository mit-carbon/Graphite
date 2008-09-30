#include "chip.h"

// definitions

CAPI_return_t chipInit(int *rank)
{
   THREADID pin_tid = PIN_ThreadId();

	GetLock(&(g_chip->maps_lock), 1); 

   map<THREADID, int>::iterator e = g_chip->core_map.find(pin_tid);

   if ( e == g_chip->core_map.end() ) { 
      g_chip->tid_map[g_chip->prev_rank] = pin_tid;    
      g_chip->core_map.insert( make_pair(pin_tid, g_chip->prev_rank) );
      *rank = g_chip->prev_rank; 
      ++(g_chip->prev_rank);
   }   
   else {
      *rank = e->second;
   }
   
   ReleaseLock(&(g_chip->maps_lock));
   
   return 0;
};

CAPI_return_t chipRank(int *rank)
{
   THREADID pin_tid = PIN_ThreadId();
   map<THREADID, int>::iterator e = g_chip->core_map.find(pin_tid);
   *rank = ( e == g_chip->core_map.end() ) ? -1 : e->second;
   bool rank_ok = (*rank < g_chip->getNumModules());
   if (!rank_ok) {
     //printf("Bad rank: %d @ %p\n", *rank, rank);
     LOG("Bad rank: " + decstr(*rank) + " @ ptr: " + hexstr(rank) + "\n");

     LOG("  ThreadID: " + decstr(pin_tid));
     if ( e == g_chip->core_map.end() ) {
       LOG(" was NOT found in core_map!\n");
     } else {
       LOG(" was found in map: <" + decstr(e->first) + ", " + 
	   decstr(e->second) + ">\n");
     }

     LOG("  core_map:  <pin_tid, rank>\n             -----------------\n");
     map<THREADID, int>::iterator f;
     for (f = g_chip->core_map.begin(); f != g_chip->core_map.end(); f++) {
       LOG("               <" + decstr(f->first) + ", " + 
	   decstr(f->second) + ">\n");
     }
   }
   ASSERT(rank_ok, "Illegal rank value returned by chipRank!\n");

   return 0;
}

CAPI_return_t chipSendW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver, 
                        char *buffer, int size)
{
   return g_chip->core[sender].coreSendW(sender, receiver, buffer, size);
}

CAPI_return_t chipRecvW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver, 
                        char *buffer, int size)
{
   return g_chip->core[receiver].coreRecvW(sender, receiver, buffer, size);
}
 //FIXME hack, keep calling Network::netEntryTasks until all cores have finished running. Can we use an interupt instead? Also, I'm too lazy to figure out my_rank, so I'm just passing it in for now. cpc
CAPI_return_t chipHackFinish(int my_rank)
{
	cout << "FINISHED: CORE [" << my_rank << "] " << endl;
	bool volatile finished = false;
	
	//debugging shared memory
//  g_chip->core[my_rank]. 

	while(!finished) {
		g_chip->core[my_rank].getNetwork()->netCheckMessages();
		//sleep? and conditionally check if we should end loop?
		//
	}

   return 0;
}
// performance model wrappers

VOID perfModelRun(PerfModelIntervalStat *interval_stats)
{ 
   int rank; 
   chipRank(&rank);
   g_chip->core[rank].perfModelRun(interval_stats); 
}

VOID perfModelRun(PerfModelIntervalStat *interval_stats, REG *reads, 
                  UINT32 num_reads)
{ 
   int rank;
   chipRank(&rank);
   g_chip->core[rank].perfModelRun(interval_stats, reads, num_reads); 
}

VOID perfModelRun(PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
                  REG *writes, UINT32 num_writes)
{ 
   int rank;
   chipRank(&rank);
   g_chip->core[rank].perfModelRun(interval_stats, dcache_load_hit, writes, num_writes); 
}


PerfModelIntervalStat* perfModelAnalyzeInterval(const string& parent_routine, 
                                                const INS& start_ins, const INS& end_ins)
{ 
   int rank;
   chipRank(&rank);
   return g_chip->core[rank].perfModelAnalyzeInterval(parent_routine, start_ins, end_ins); 
}

VOID perfModelLogICacheLoadAccess(PerfModelIntervalStat *stats, bool hit)
{ 
   int rank;
   chipRank(&rank);
   g_chip->core[rank].perfModelLogICacheLoadAccess(stats, hit); 
}
     
VOID perfModelLogDCacheStoreAccess(PerfModelIntervalStat *stats, bool hit)
{ 
   int rank;
   chipRank(&rank);
   g_chip->core[rank].perfModelLogDCacheStoreAccess(stats, hit); 
}

VOID perfModelLogBranchPrediction(PerfModelIntervalStat *stats, bool correct)
{ 
   int rank;
   chipRank(&rank);
   g_chip->core[rank].perfModelLogBranchPrediction(stats, correct); 
}


// organic cache model wrappers

bool icacheRunLoadModel(ADDRINT i_addr, UINT32 size)
{ 
   int rank;
   chipRank(&rank);
   return g_chip->core[rank].icacheRunLoadModel(i_addr, size); 
}

bool dcacheRunLoadModel(ADDRINT d_addr, UINT32 size)
{ 
   int rank;
   chipRank(&rank);
//	assert( size <= 4 );
   return g_chip->core[rank].dcacheRunLoadModel(d_addr, size); 
}

bool dcacheRunStoreModel(ADDRINT d_addr, UINT32 size)
{ 
   int rank;
   chipRank(&rank);
   return g_chip->core[rank].dcacheRunStoreModel(d_addr, size); 
}

// Chip class method definitions

Chip::Chip(int num_mods): num_modules(num_mods), prev_rank(0)
{
   proc_time = new UINT64[num_mods];
   tid_map = new THREADID [num_mods];
   core = new Core[num_mods];

   for(int i = 0; i < num_mods; i++) 
   {
      proc_time[i] = 0;
      tid_map[i] = 0;
      core[i].coreInit(this, i, num_mods);
   }

   InitLock(&maps_lock);
}

VOID Chip::fini(int code, VOID *v)
{
   ofstream out( g_knob_output_file.Value().c_str() );

   for(int i = 0; i < num_modules; i++) 
   {
      cout << "*** Core[" << i << "] summary ***" << endl;
      core[i].fini(code, v, out); 
      cout << endl;
   }

   out.close();
}

void Chip::debugSetInitialMemConditions(ADDRINT address, vector< pair<INT32, DramDirectoryEntry::dstate_t> > dram_vector, vector< pair<INT32, CacheState::cstate_t> > cache_vector, vector<UINT32> sharers_list)
{
	INT32 temp_dram_id, temp_cache_id;
	DramDirectoryEntry::dstate_t temp_dstate;
	CacheState::cstate_t temp_cstate;

	while(!dram_vector.empty()) 
	{  //TODO does this assume 1:1 core/dram allocation?
		temp_dram_id = dram_vector.back().first;
		temp_dstate = dram_vector.back().second;
      dram_vector.pop_back();

		core[temp_dram_id].debugSetDramState(address, temp_dstate, sharers_list);
   }

	while(!cache_vector.empty()) 
	{
		temp_cache_id = cache_vector.back().first;
		temp_cstate = cache_vector.back().second;
      cache_vector.pop_back();

		core[temp_cache_id].debugSetCacheState(address, temp_cstate);
   }

}

bool Chip::debugAssertMemConditions(ADDRINT address, vector< pair<INT32, DramDirectoryEntry::dstate_t> > dram_vector, vector< pair<INT32, CacheState::cstate_t> > cache_vector, vector<UINT32> sharers_list, string test_code, string error_string)
{

//	cout << "    ## Asserting Memory Conditions ## [" << test_code <<" ] " << endl;
   bool all_asserts_passed = true; //return false if any assertions fail
	
	INT32 temp_dram_id, temp_cache_id;
	DramDirectoryEntry::dstate_t temp_dstate;
	CacheState::cstate_t temp_cstate;

	while(!dram_vector.empty()) 
	{
		temp_dram_id = dram_vector.back().first;
		temp_dstate = dram_vector.back().second;
      dram_vector.pop_back();

		if(!core[temp_dram_id].debugAssertDramState(address, temp_dstate, sharers_list))
			all_asserts_passed = false;
   }

	while(!cache_vector.empty()) 
	{
		temp_cache_id = cache_vector.back().first;
		temp_cstate = cache_vector.back().second;
      cache_vector.pop_back();

		if(!core[temp_cache_id].debugAssertCacheState(address, temp_cstate))
			all_asserts_passed = false;
   }

	if(!all_asserts_passed) 
	{
 //  	cout << "    *** ASSERTION FAILED *** : " << error_string << endl;
	}

	return all_asserts_passed;
}

void Chip::setDramBoundaries( vector< pair< ADDRINT, ADDRINT> > addr_boundaries) 
{

	cout << " CHIP: setting Dram Boundaries " << endl; 
   for(int i = 0; i < num_modules; i++) 
   {
      core[i].setDramBoundaries(addr_boundaries);
   }

	//static function that should affect all AHL's on every core
	cout << " CHIP: Finished Dram Boundaries " << endl; 
}
/*user program calls get routed through this */
CAPI_return_t chipDebugSetMemState(ADDRINT address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list) 
{
	vector< pair<INT32, DramDirectoryEntry::dstate_t> > dram_vector;
	vector< pair<INT32, CacheState::cstate_t> > cache_vector;

	dram_vector.push_back( pair<INT32, DramDirectoryEntry::dstate_t>(dram_address_home_id, dstate) );

	cache_vector.push_back( pair<INT32, CacheState::cstate_t>(0, cstate0) );
	cache_vector.push_back( pair<INT32, CacheState::cstate_t>(1, cstate1) );
	
	g_chip->debugSetInitialMemConditions(address,  dram_vector, cache_vector, sharers_list);

	return 0;
}

CAPI_return_t chipDebugAssertMemState(ADDRINT address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, string test_code, string error_code) 
{
//	cout << " [Chip] Asserting Mem State " << endl;
	vector< pair<INT32, DramDirectoryEntry::dstate_t> > dram_vector;
	vector< pair<INT32, CacheState::cstate_t> > cache_vector;
			
	dram_vector.push_back( pair<INT32, DramDirectoryEntry::dstate_t>(dram_address_home_id, dstate) );
					
	cache_vector.push_back( pair<INT32, CacheState::cstate_t>(1, cstate1) );
	cache_vector.push_back( pair<INT32, CacheState::cstate_t>(0, cstate0) );

	//TODO return true/false based on whether assert is true or not?
	if(g_chip->debugAssertMemConditions(address, dram_vector, cache_vector, sharers_list, test_code, error_code))
	{
		return 1;
	} else {
		return 0;
	}
	

}

CAPI_return_t chipSetDramBoundaries(vector< pair<ADDRINT, ADDRINT> > addr_boundaries)
{
	g_chip->setDramBoundaries(addr_boundaries);
	return 0;
}
