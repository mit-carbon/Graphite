#include "chip.h"

// definitions

//TODO: c++-ize this, please oh please!
CAPI_return_t chipInit(int *rank)
{
   THREADID pin_tid = PIN_ThreadId();

	GetLock(&(g_chip->maps_lock), 1); 

   map<THREADID, int>::iterator e = g_chip->core_map.find(pin_tid);

   if ( e == g_chip->core_map.end() ) { 
      assert(0 <= g_chip->prev_rank && g_chip->prev_rank < g_chip->num_modules);
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

   // FIXME: This could be a big slow-down. Might want to do a full
   // read/write lock.
   GetLock(&(g_chip->maps_lock), 1);

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

   ReleaseLock(&(g_chip->maps_lock));
   ASSERT(rank_ok, "Illegal rank value returned by chipRank!\n");

   return 0;
}

CAPI_return_t commRank(int *rank)
{
   int my_tid;
   chipRank(&my_tid);
   // FIXME: The network nodes should be moved up from within a core to
   //  directly within a chip.  When this happens, this will have to change.
   *rank = g_chip->core[my_tid].coreCommID();

   return 0;
}

CAPI_return_t chipSendW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver, 
                        char *buffer, int size)
{
   int rank;
   chipRank(&rank);
   return g_chip->core[rank].coreSendW(sender, receiver, buffer, size);
}

CAPI_return_t chipRecvW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver, 
                        char *buffer, int size)
{
   int rank;
   chipRank(&rank);
   /*
   cout << "Starting receive on core " << rank << " (from:" << sender 
	<< ", to:" << receiver << ", ptr:" << hex << &buffer << dec
	<< ", size:" << size << ")" << endl;
   */ 
   return g_chip->core[rank].coreRecvW(sender, receiver, buffer, size);
}
 //FIXME hack, keep calling Network::netEntryTasks until all cores have finished running. Can we use an interupt instead? Also, I'm too lazy to figure out my_rank, so I'm just passing it in for now. cpc
CAPI_return_t chipHackFinish(int my_rank)
{
	cerr << "HACK---- please remove chipHackFInish...... " << endl;
	cerr << "FINISHED: CORE [" << my_rank << "] " << endl;
	
	/* ========================================================================== */
	/* Added by George */
	cerr << "Total DRAM access cost = " << ((g_chip->core[my_rank]).getMemoryManager()->getDramDirectory())->getDramAccessCost() << endl;
//	cerr << "Total DRAM access cost = " << ((g_chip->core[my_rank]).getMemoryManager())->getDramAccessCost() << endl;
	/* ========================================================================== */

	bool volatile finished = false;
	
	//debugging shared memory
//  g_chip->core[my_rank]. 

	while(!finished) {
		g_chip->core[my_rank].getNetwork()->netCheckMessages();
//		cout << "FINISHED lawls [" << my_rank << "] " << endl;
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
   assert(0 <= rank && rank < g_chip->num_modules);
   g_chip->core[rank].perfModelRun(interval_stats); 
}

VOID perfModelRun(PerfModelIntervalStat *interval_stats, REG *reads, 
                  UINT32 num_reads)
{ 
   int rank;
   chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   g_chip->core[rank].perfModelRun(interval_stats, reads, num_reads); 
}

VOID perfModelRun(PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
                  REG *writes, UINT32 num_writes)
{ 
   int rank;
   chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   g_chip->core[rank].perfModelRun(interval_stats, dcache_load_hit, writes, num_writes); 
}


PerfModelIntervalStat** perfModelAnalyzeInterval(const string& parent_routine, 
                                                 const INS& start_ins, const INS& end_ins)
{ 
   // using zero is a dirty hack 
   // assumes its safe to use core zero to generate perfmodels for all cores
   assert(g_chip->num_modules > 0);
   PerfModelIntervalStat* *array = new PerfModelIntervalStat*[g_chip->num_modules];

   for (INT32 i = 0; i < g_chip->num_modules; i++)
      array[i] = g_chip->core[0].perfModelAnalyzeInterval(parent_routine, start_ins, end_ins);
 
   return array; 
}

VOID perfModelLogICacheLoadAccess(PerfModelIntervalStat *stats, bool hit)
{ 
   int rank;
   chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   g_chip->core[rank].perfModelLogICacheLoadAccess(stats, hit); 
}
     
VOID perfModelLogDCacheStoreAccess(PerfModelIntervalStat *stats, bool hit)
{ 
   int rank;
   chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   g_chip->core[rank].perfModelLogDCacheStoreAccess(stats, hit); 
}

VOID perfModelLogBranchPrediction(PerfModelIntervalStat *stats, bool correct)
{ 
   int rank;
   chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   g_chip->core[rank].perfModelLogBranchPrediction(stats, correct); 
}


// organic cache model wrappers

bool icacheRunLoadModel(ADDRINT i_addr, UINT32 size)
{ 
   int rank;
   chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   return g_chip->core[rank].icacheRunLoadModel(i_addr, size); 
}

bool dcacheRunModel(CacheBase::AccessType access_type, ADDRINT d_addr, char* data_buffer, UINT32 data_size)
{

   int rank;
   chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
	//TODO make everything use the cachebase::accesstype enum
	//TODO just passing in dummy data for now
	for(unsigned int i = 0; i < data_size; i++)
		data_buffer[i] = (char) i;
	if( access_type == CacheBase::k_ACCESS_TYPE_LOAD)
		return g_chip->core[rank].dcacheRunModel(Core::LOAD, d_addr, data_buffer, data_size); 
	else
		return g_chip->core[rank].dcacheRunModel(Core::STORE, d_addr, data_buffer, data_size); 
}

bool dcacheRunLoadModel(ADDRINT d_addr, UINT32 size)
{ 
   int rank;
   chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
//   return g_chip->core[rank].dcacheRunLoadModel(d_addr, size); 
	char data_buffer[size];
   //TODO load data from cache into data_buffer. should buffer be on the stack?
	return g_chip->core[rank].dcacheRunModel(Core::LOAD, d_addr, data_buffer, size); 
}

bool dcacheRunStoreModel(ADDRINT d_addr, UINT32 size)
{ 
   int rank;
   chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
//   return g_chip->core[rank].dcacheRunStoreModel(d_addr, size); 
	//TODO just passing in dummy data for now
	char data_buffer[size];
	for(unsigned int i = 0; i < size; i++)
		data_buffer[i] = (char) i;
   return g_chip->core[rank].dcacheRunModel(Core::STORE, d_addr, data_buffer, size); 
}

// syscall model wrappers
void syscallEnterRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   int rank;
   chipRank(&rank);
   if(rank >= 0)
      g_chip->core[rank].getSyscallMdl()->runEnter(rank, ctx, syscall_standard);
}

void syscallExitRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   int rank;
   chipRank(&rank);
   if(rank >= 0)
      g_chip->core[rank].getSyscallMdl()->runExit(rank, ctx, syscall_standard);
}

// MCP wrappers
void MCPRun()
{
   assert(g_MCP != NULL);
   g_MCP->run();
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
   InitLock(&dcache_lock);
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

/*
void Chip::setDramBoundaries( vector< pair< ADDRINT, ADDRINT> > addr_boundaries) 
{

	cerr << " CHIP: setting Dram Boundaries " << endl; 
   for(int i = 0; i < num_modules; i++) 
   {
      cerr << " CHIP: core[" << i << "] of [" << num_modules << "] setting dram boundaries " << endl;
		core[i].setDramBoundaries(addr_boundaries);
   }

	//static function that should affect all AHL's on every core?
	cerr << " CHIP: Finished Dram Boundaries " << endl; 
}
*/
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

/*
CAPI_return_t chipSetDramBoundaries(vector< pair<ADDRINT, ADDRINT> > addr_boundaries)
{
	g_chip->setDramBoundaries(addr_boundaries);
	return 0;
}
*/


//TODO this are here as a temporary fix to the dcache synro bug (code hangs if we don't serilize cache accesses).  i suspect this will NOT work once we go across clusters.
// rank only used for debug output
void Chip::getDCacheModelLock(int rank)
{
//	cerr << "[" << rank << "] Getting Lock " << endl;
	GetLock(&dcache_lock, 1);
//	cerr << "[" << rank << "] GOTTEN Lock " << endl;
	
}

void Chip::releaseDCacheModelLock(int rank)
{
//	cerr << "[" << rank << "] about to release Lock " << endl;
	ReleaseLock(&dcache_lock);       
//	cerr << "[" << rank << "] RELASED Lock " << endl;
}
