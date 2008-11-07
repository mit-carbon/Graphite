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

	if( access_type == CacheBase::k_ACCESS_TYPE_LOAD)
		return g_chip->core[rank].dcacheRunModel(Core::LOAD, d_addr, data_buffer, data_size); 
	else
		return g_chip->core[rank].dcacheRunModel(Core::STORE, d_addr, data_buffer, data_size); 
}

/*
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
*/
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
   
	InitLock(&global_lock);

	// FIXME: A hack
	aliasEnable = false;
}

VOID Chip::fini(int code, VOID *v)
{
   ofstream out( g_knob_output_file.Value().c_str() );

   for(int i = 0; i < num_modules; i++)
   {
      cerr << "*** Core[" << i << "] summary ***" << endl;
      core[i].fini(code, v, out); 
      cerr << endl;
   }

   out.close();
}

void Chip::debugSetInitialMemConditions (vector<ADDRINT>& address_vector, 
		  											  vector< pair<INT32, DramDirectoryEntry::dstate_t> >& dram_vector, vector<vector<UINT32> >& sharers_list_vector, 
													  vector< vector< pair<INT32, CacheState::cstate_t> > >& cache_vector, 
		  											  vector<char*>& d_data_vector, 
													  vector<char*>& c_data_vector)
{
	vector<ADDRINT> temp_address_vector = address_vector;

	assert (d_data_vector.size() == c_data_vector.size());
	assert (d_data_vector.size() == dram_vector.size());
	assert (cache_vector.size() == dram_vector.size() );

	while (!dram_vector.empty())
	{  //TODO does this assume 1:1 core/dram allocation?

		ADDRINT curr_address = address_vector.back();
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

	while(!cache_vector.empty()) 
	{

		ADDRINT curr_address = address_vector.back();
		address_vector.pop_back();
		
		vector< pair<INT32, CacheState::cstate_t> > curr_cache_vector = cache_vector.back();
		cache_vector.pop_back();

		char *curr_c_data = c_data_vector.back();
		c_data_vector.pop_back();

		while (!curr_cache_vector.empty()) {
			 
			INT32 curr_cache_id = curr_cache_vector.back().first;
			CacheState::cstate_t curr_cstate = curr_cache_vector.back().second;
      	curr_cache_vector.pop_back();

			core[curr_cache_id].debugSetCacheState(curr_address, curr_cstate, curr_c_data);
		}
   }

}

bool Chip::debugAssertMemConditions (vector<ADDRINT>& address_vector, 
		  										 vector< pair<INT32, DramDirectoryEntry::dstate_t> >& dram_vector, vector<vector<UINT32> >& sharers_list_vector, 
												 vector< vector< pair<INT32, CacheState::cstate_t> > >& cache_vector, 
		  										 vector<char*>& d_data_vector, 
												 vector<char*>& c_data_vector,
												 string test_code, string error_string)
{
	bool all_asserts_passed = true;
	vector<ADDRINT> temp_address_vector = address_vector;

	assert (d_data_vector.size() == c_data_vector.size());
	assert (d_data_vector.size() == dram_vector.size());
	assert (cache_vector.size() == dram_vector.size() );

	while (!dram_vector.empty())
	{  //TODO does this assume 1:1 core/dram allocation?

		ADDRINT curr_address = address_vector.back();
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

	while(!cache_vector.empty()) 
	{

		ADDRINT curr_address = address_vector.back();
		address_vector.pop_back();
		
		vector< pair<INT32, CacheState::cstate_t> > curr_cache_vector = cache_vector.back();
		cache_vector.pop_back();

		char *curr_c_data = c_data_vector.back();
		c_data_vector.pop_back();

		while (!curr_cache_vector.empty()) {
			 
			INT32 curr_cache_id = curr_cache_vector.back().first;
			CacheState::cstate_t curr_cstate = curr_cache_vector.back().second;
      	curr_cache_vector.pop_back();

			if (! core[curr_cache_id].debugAssertCacheState(curr_address, curr_cstate, curr_c_data))
				 all_asserts_passed = false;
		}
   }

	if(!all_asserts_passed) 
	{
   	cerr << "    *** ASSERTION FAILED *** : " << error_string << endl;
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
CAPI_return_t chipDebugSetMemState(ADDRINT address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, char *d_data, char *c_data)
{
	vector<ADDRINT> address_vector;
	vector< pair<INT32, DramDirectoryEntry::dstate_t> > dram_vector;
	vector< vector <UINT32> > sharers_list_vector;
	vector< vector < pair<INT32, CacheState::cstate_t> > > cache_vector;
	vector< pair<INT32, CacheState::cstate_t> > curr_cache_vector;

	vector<char*> d_data_vector;
	vector<char*> c_data_vector;

	assert (g_chip->aliasMap.find(address) != g_chip->aliasMap.end());
	address = g_chip->aliasMap[address];
	address_vector.push_back(address);

	dram_vector.push_back( pair<INT32, DramDirectoryEntry::dstate_t>(dram_address_home_id, dstate) );
	sharers_list_vector.push_back(sharers_list);

	curr_cache_vector.push_back( pair<INT32, CacheState::cstate_t>(0, cstate0) );
	curr_cache_vector.push_back( pair<INT32, CacheState::cstate_t>(1, cstate1) );
	cache_vector.push_back(curr_cache_vector);

	d_data_vector.push_back(d_data);
	c_data_vector.push_back(c_data);

	cerr << "ChipDebug Set: d_data = 0x" << hex << (UINT32) d_data << ", c_data = 0x" << hex << (UINT32) c_data << endl; 
	
	g_chip->debugSetInitialMemConditions (address_vector, 
		  											  dram_vector, sharers_list_vector, 
													  cache_vector, 
		  											  d_data_vector, 
													  c_data_vector);

	return 0;
}

CAPI_return_t chipDebugAssertMemState(ADDRINT address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, char *d_data, char *c_data, string test_code, string error_code)
{
	vector<ADDRINT> address_vector;
	vector< pair<INT32, DramDirectoryEntry::dstate_t> > dram_vector;
	vector< vector <UINT32> > sharers_list_vector;
	vector< vector < pair<INT32, CacheState::cstate_t> > > cache_vector;
	vector< pair<INT32, CacheState::cstate_t> > curr_cache_vector;

	vector<char*> d_data_vector;
	vector<char*> c_data_vector;

	assert (g_chip->aliasMap.find(address) != g_chip->aliasMap.end());
	
	address = g_chip->aliasMap[address];
	address_vector.push_back(address);

	dram_vector.push_back( pair<INT32, DramDirectoryEntry::dstate_t>(dram_address_home_id, dstate) );
	sharers_list_vector.push_back(sharers_list);

	curr_cache_vector.push_back( pair<INT32, CacheState::cstate_t>(0, cstate0) );
	curr_cache_vector.push_back( pair<INT32, CacheState::cstate_t>(1, cstate1) );
	cache_vector.push_back(curr_cache_vector);

	d_data_vector.push_back(d_data);
	c_data_vector.push_back(c_data);
	
	cerr << "ChipDebug Assert: d_data = 0x" << hex << (UINT32) d_data << ", c_data = 0x" << hex << (UINT32) c_data << endl; 
	
	if (g_chip->debugAssertMemConditions (address_vector, 
		  											  dram_vector, sharers_list_vector, 
													  cache_vector, 
		  											  d_data_vector, 
													  c_data_vector,
													  test_code, error_code) )
	{
		return 1;
	}
	else
	{
		return 0;
	}

	/*
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
	*/

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

// FIXME: Stupid Hack Functions

CAPI_return_t chipAlias (ADDRINT address0, ADDRINT address1)
{
	// It is better to create an alias map here. An assciative array
	assert (g_chip->num_modules == 2);
	g_chip->aliasMap[address0] = (0 << g_knob_ahl_param);
	g_chip->aliasMap[address1] = (1 << g_knob_ahl_param);
	g_chip->aliasEnable = true;

	cerr << "Aliasing address 0x" << hex << address0 << "  ==>  Node 0"  << endl;  
	cerr << "Aliasing address 0x" << hex << address1 << "  ==>  Node 1"  << endl;  
	return 0;
}

bool isAliasEnabled () {
 	return (g_chip->aliasEnable);
}

