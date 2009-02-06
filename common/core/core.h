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

#include "fixed_types.h"
#include "config.h"

#include "cache.h"
#include "cache_state.h"
#include "dram_directory_entry.h"
#include "perfmdl.h"

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
      int getRank()     { return core_tid; }
      int getNumCores() { return core_num_mod; }
      MemoryManager* getMemoryManager(void) { return memory_manager; }
      SyscallMdl *getSyscallMdl() { return syscall_model; }
      SyncClient *getSyncClient() { return sync_client; }

      int coreInit(int tid, int num_mod);

      int coreSendW(int sender, int receiver, char *buffer, int size);
      int coreRecvW(int sender, int receiver, char *buffer, int size);



      void fini(int code, void *v, ostream& out);

      // organic cache wrappers
      bool icacheRunLoadModel(IntPtr i_addr, UInt32 size);
      bool dcacheRunModel(CacheBase::AccessType operation, IntPtr d_addr, char* data_buffer, UInt32 data_size);

      // FIXME: Debug Functions. Debug hook to smash cache state
      void debugSetCacheState(IntPtr addr, CacheState::cstate_t cstate, char *c_data);
      //return true if assertion is true  (that's a good thing)
      bool debugAssertCacheState(IntPtr addr, CacheState::cstate_t cstate, char *c_data);

      void debugSetDramState(IntPtr addr, DramDirectoryEntry::dstate_t dstate, vector<UInt32> sharers_list, char *d_data);
      bool debugAssertDramState(IntPtr addr, DramDirectoryEntry::dstate_t dstate, vector<UInt32> sharers_list, char *d_data);

      //performance model wrappers

      void perfModelRun(PerfModelIntervalStat *interval_stats, bool firstCallInIntrvl)
         { perf_model->run(interval_stats, firstCallInIntrvl); }

      void perfModelRun(PerfModelIntervalStat *interval_stats, REG *reads, 
                        UInt32 num_reads, bool firstCallInIntrvl)
         { perf_model->run(interval_stats, reads, num_reads, firstCallInIntrvl); }

      void perfModelRun(PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
                        REG *writes, UInt32 num_writes, bool firstCallInIntrvl)
         { perf_model->run(interval_stats, dcache_load_hit, writes, num_writes, firstCallInIntrvl); }

      PerfModelIntervalStat* perfModelAnalyzeInterval(const string& parent_routine,
                                                      const INS& start_ins,
                                                      const INS& end_ins)
         { return perf_model->analyzeInterval(parent_routine, start_ins, end_ins); }

      void perfModelLogICacheLoadAccess(PerfModelIntervalStat *stats, bool hit)
         { perf_model->logICacheLoadAccess(stats, hit); }

      void perfModelLogDCacheStoreAccess(PerfModelIntervalStat *stats, bool hit)
         { perf_model->logDCacheStoreAccess(stats, hit); }

      void perfModelLogBranchPrediction(PerfModelIntervalStat *stats, bool correct)
         { perf_model->logBranchPrediction(stats, correct); }
	

      // network accessor since network is private
      Network *getNetwork() { return network; }
      PerfModel *getPerfModel() { return perf_model; }
};

#endif
