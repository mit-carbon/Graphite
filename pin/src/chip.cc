#include "chip.h"

// definitions

//TODO: c++-ize this, please oh please!
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
   return g_chip->core[rank].dcacheRunLoadModel(d_addr, size); 
}

bool dcacheRunStoreModel(ADDRINT d_addr, UINT32 size)
{ 
   int rank;
   chipRank(&rank);
   return g_chip->core[rank].dcacheRunStoreModel(d_addr, size); 
}


// syscall model wrappers
void syscallRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   int rank;
   chipRank(&rank);
   g_chip->syscall_model.run(rank, ctx, syscall_standard);
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
