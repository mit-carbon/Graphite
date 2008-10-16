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

bool dcacheRunLoadModel(ADDRINT d_addr, UINT32 size)
{ 
   int rank;
   chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   return g_chip->core[rank].dcacheRunLoadModel(d_addr, size); 
}

bool dcacheRunStoreModel(ADDRINT d_addr, UINT32 size)
{ 
   int rank;
   chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   return g_chip->core[rank].dcacheRunStoreModel(d_addr, size); 
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

void MCPFinish()
{
   assert(g_MCP != NULL);
   g_MCP->finish();
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
