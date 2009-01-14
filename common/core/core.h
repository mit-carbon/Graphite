// Harshad Kasture
//

#ifndef CORE_H
#define CORE_H

#include <iostream>
#include <fstream>
#include <string.h>

// some forward declarations for cross includes
class Network;
class MemoryManager;
class SyscallMdl;
class SyncClient;
class OCache;
class PerfModel;
class Network;

#include "pin.H"
#include "config.h"

#include "cache_state.h"
#include "dram_directory_entry.h"
#include "perfmdl.h"

// externally defined vars

extern LEVEL_BASE::KNOB<bool> g_knob_enable_performance_modeling;
extern LEVEL_BASE::KNOB<bool> g_knob_enable_dcache_modeling;
extern LEVEL_BASE::KNOB<bool> g_knob_enable_icache_modeling;

extern LEVEL_BASE::KNOB<UINT32> g_knob_cache_size;
extern LEVEL_BASE::KNOB<UINT32> g_knob_line_size;
extern LEVEL_BASE::KNOB<UINT32> g_knob_associativity;
extern LEVEL_BASE::KNOB<UINT32> g_knob_mutation_interval;
extern LEVEL_BASE::KNOB<UINT32> g_knob_dcache_threshold_hit;
extern LEVEL_BASE::KNOB<UINT32> g_knob_dcache_threshold_miss;
extern LEVEL_BASE::KNOB<UINT32> g_knob_dcache_size;
extern LEVEL_BASE::KNOB<UINT32> g_knob_dcache_associativity;
extern LEVEL_BASE::KNOB<UINT32> g_knob_dcache_max_search_depth;
extern LEVEL_BASE::KNOB<UINT32> g_knob_icache_threshold_hit;
extern LEVEL_BASE::KNOB<UINT32> g_knob_icache_threshold_miss;
extern LEVEL_BASE::KNOB<UINT32> g_knob_icache_size;
extern LEVEL_BASE::KNOB<UINT32> g_knob_icache_associativity;
extern LEVEL_BASE::KNOB<UINT32> g_knob_icache_max_search_depth; 

class Core
{
   private:
      int core_tid;
      int core_num_mod;
      MemoryManager *memory_manager;
      Network *network;
      PerfModel *perf_model;
      OCache *ocache;
      SyscallMdl *syscall_model;
      SyncClient *sync_client;


   public:

      enum mem_operation_t 
      {
         LOAD,
         STORE
      };

      int getRank() 
         { return core_tid; }
      int getId() const { return core_tid; }

      int getNumCores()
         { return core_num_mod; }

      
      int coreInit(int tid, int num_mod);

      // Return the communication endpoint ID for this core
      int coreCommID();

      int coreSendW(int sender, int receiver, char *buffer, int size);

      int coreRecvW(int sender, int receiver, char *buffer, int size);

      MemoryManager* getMemoryManager(void)
         { return memory_manager; }
      
      //performance model wrappers
		SyscallMdl *getSyscallMdl() { return syscall_model; }
      SyncClient *getSyncClient() { return sync_client; }
      
      VOID fini(int code, VOID *v, ofstream& out);
	
      // organic cache wrappers
	
      bool icacheRunLoadModel(ADDRINT i_addr, UINT32 size);
	
      bool dcacheRunModel(mem_operation_t operation, ADDRINT d_addr, char* data_buffer, UINT32 data_size);

      // FIXME: Debug Functions. Debug hook to smash cache state
      void debugSetCacheState(ADDRINT addr, CacheState::cstate_t cstate, char *c_data);
      //return true if assertion is true  (that's a good thing)
      bool debugAssertCacheState(ADDRINT addr, CacheState::cstate_t cstate, char *c_data);

      void debugSetDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list, char *d_data);
      bool debugAssertDramState(ADDRINT addr, DramDirectoryEntry::dstate_t dstate, vector<UINT32> sharers_list, char *d_data);
	
      //performance model wrappers

      VOID perfModelRun(PerfModelIntervalStat *interval_stats, bool firstCallInIntrvl)
         { perf_model->run(interval_stats, firstCallInIntrvl); }

      VOID perfModelRun(PerfModelIntervalStat *interval_stats, REG *reads, 
                        UINT32 num_reads, bool firstCallInIntrvl)
         { perf_model->run(interval_stats, reads, num_reads, firstCallInIntrvl); }

      VOID perfModelRun(PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
                        REG *writes, UINT32 num_writes, bool firstCallInIntrvl)
         { perf_model->run(interval_stats, dcache_load_hit, writes, num_writes, firstCallInIntrvl); }

      PerfModelIntervalStat* perfModelAnalyzeInterval(const string& parent_routine,
                                                      const INS& start_ins,
                                                      const INS& end_ins)
         { return perf_model->analyzeInterval(parent_routine, start_ins, end_ins); }

      VOID perfModelLogICacheLoadAccess(PerfModelIntervalStat *stats, bool hit)
         { perf_model->logICacheLoadAccess(stats, hit); }

      VOID perfModelLogDCacheStoreAccess(PerfModelIntervalStat *stats, bool hit)
         { perf_model->logDCacheStoreAccess(stats, hit); }

      VOID perfModelLogBranchPrediction(PerfModelIntervalStat *stats, bool correct)
         { perf_model->logBranchPrediction(stats, correct); }
	

      // network accessor since network is private
      Network *getNetwork() { return network; }
      PerfModel *getPerfModel() { return perf_model; }
};

#endif
