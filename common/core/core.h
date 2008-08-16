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


#include "memory_manager.h"
#include "pin.H"
#include "chip.h"
#include "network.h"
#include "perfmdl.h"
#include "ocache.h"

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
	Chip *the_chip;
	int core_tid;
	int core_num_mod;
	Network *network;
	PerfModel *perf_model;
	
	// JP: shouldn't this be only a part of MemoryManager?
	OCache *ocache;

	MemoryManager *memory_manager;
	
   public:

      int getRank() 
         { return core_tid; }

      int getNumCores()
         { return core_num_mod; }

      int coreInit(Chip *chip, int tid, int num_mod);

      int coreSendW(int sender, int receiver, char *buffer, int size);

      int coreRecvW(int sender, int receiver, char *buffer, int size);

      // network accessor since network is private
      Network* getNetwork(void);

      VOID fini(int code, VOID *v, ofstream& out);
	
	inline VOID perfModelRun(PerfModelIntervalStat *interval_stats);
	
	inline VOID perfModelRun(PerfModelIntervalStat *interval_stats, REG *reads, 
							 UINT32 num_reads);
	
	inline VOID perfModelRun(PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
							 REG *writes, UINT32 num_writes);
	
	inline PerfModelIntervalStat* perfModelAnalyzeInterval(const string& parent_routine, 
														   const INS& start_ins, 
														   const INS& end_ins);
	
	inline VOID perfModelLogICacheLoadAccess(PerfModelIntervalStat *stats, bool hit);
	
	inline VOID perfModelLogDCacheStoreAccess(PerfModelIntervalStat *stats, bool hit);
	
	inline VOID perfModelLogBranchPrediction(PerfModelIntervalStat *stats, bool correct);
	
	
	// organic cache wrappers
	
	inline bool icacheRunLoadModel(ADDRINT i_addr, UINT32 size);
	
	inline bool dcacheRunLoadModel(ADDRINT d_addr, UINT32 size);
	
	inline bool dcacheRunStoreModel(ADDRINT d_addr, UINT32 size);
	
};

#endif
