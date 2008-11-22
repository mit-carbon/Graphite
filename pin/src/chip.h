// Jonathan Eastep and Harshad Kasture
//

#ifndef CHIP_H
#define CHIP_H

#include <iostream>
#include <fstream>
#include <map>

#include "pin.H"
#include "capi.h"
#include "core.h"
#include "ocache.h"
#include "dram_directory_entry.h"
#include "cache_state.h"
#include "address_home_lookup.h"
#include "perfmdl.h"
#include "lockfree_hash.h"
#include "syscall_model.h"
#include "mcp.h"
#include "debug.h"

// external variables

// JME: not entirely sure why these are needed...
class Chip;
class Core;

extern Chip *g_chip;
extern MCP *g_MCP;
extern PIN_LOCK print_lock;

extern LEVEL_BASE::KNOB<string> g_knob_output_file;

// prototypes

// FIXME: if possible, these shouldn't be globals. Pin callbacks may need them to be. 

CAPI_return_t chipInit(int rank);
CAPI_return_t chipInitFreeRank(int *rank);

CAPI_return_t chipRank(int *rank);

CAPI_return_t commRank(int *commid);

CAPI_return_t chipSendW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                        char *buffer, int size);

CAPI_return_t chipRecvW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                        char *buffer, int size);
//harshad should replace this -cpc
CAPI_return_t chipFinish(int my_rank);

//deal with locks in cout
CAPI_return_t chipPrint(string s);

CAPI_return_t chipDebugSetMemState(ADDRINT address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, char *d_data, char *c_data);

CAPI_return_t chipDebugAssertMemState(ADDRINT address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, char *d_data, char *c_data, string test_code, string error_code);


CAPI_return_t chipSetDramBoundaries(vector< pair<ADDRINT, ADDRINT> > addr_boundaries);

// Stupid Hack
CAPI_return_t chipAlias (ADDRINT address0, addr_t addrType, UINT32 num);
ADDRINT createAddress (UINT32 num, UINT32 coreId, bool pack1, bool pack2);
UINT32 log(UINT32);
// performance model wrappers

void perfModelRun(int rank, PerfModelIntervalStat *interval_stats);

void perfModelRun(int rank, PerfModelIntervalStat *interval_stats, 
                  REG *reads, UINT32 num_reads);

void perfModelRun(int rank, PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
                  REG *writes, UINT32 num_writes);

PerfModelIntervalStat** perfModelAnalyzeInterval(const string& parent_routine, 
                                                 const INS& start_ins, const INS& end_ins);

void perfModelLogICacheLoadAccess(int rank, PerfModelIntervalStat *stats, bool hit);
     
void perfModelLogDCacheStoreAccess(int rank, PerfModelIntervalStat *stats, bool hit);

void perfModelLogBranchPrediction(int rank, PerfModelIntervalStat *stats, bool correct);


// organic cache model wrappers

bool icacheRunLoadModel(ADDRINT i_addr, UINT32 size);

bool dcacheRunModel(CacheBase::AccessType access_type, ADDRINT d_addr, char* data_buffer, UINT32 size);
//TODO removed these because it's unneccsary
//shared memory doesn't care, but for legacy sake, i'm leaving them here for now
//bool dcacheRunLoadModel(ADDRINT d_addr, UINT32 size);
//bool dcacheRunStoreModel(ADDRINT d_addr, UINT32 size);

// syscall model wrappers

void syscallEnterRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);

void syscallExitRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);

// sync wrappers

void SimMutexInit(carbon_mutex_t *mux);
void SimMutexLock(carbon_mutex_t *mux);
void SimMutexUnlock(carbon_mutex_t *mux);

void SimCondInit(carbon_cond_t *cond);
void SimCondWait(carbon_cond_t *cond, carbon_mutex_t *mux);
void SimCondSignal(carbon_cond_t *cond);
void SimCondBroadcast(carbon_cond_t *cond);

void SimBarrierInit(carbon_barrier_t *barrier, UINT32 count);
void SimBarrierWait(carbon_barrier_t *barrier);

// MCP server wrappers
void MCPRun();
void MCPFinish();
void *MCPThreadFunc(void *dummy);

// chip class

class Chip 
{
	public:
      // messaging wrappers

      friend CAPI_return_t chipInit(int rank); 
      friend CAPI_return_t chipInitFreeRank(int *rank); 
      friend CAPI_return_t chipRank(int *rank);
      
	  	//FIXME: this is a hack function 
	  	friend CAPI_return_t chipFinish(int my_rank);
	  
	  	friend CAPI_return_t chipPrint(string s);
      
      friend CAPI_return_t commRank(int *rank);
      friend CAPI_return_t chipSendW(CAPI_endpoint_t sender, 
                                     CAPI_endpoint_t receiver, char *buffer, 
                                     int size);
      friend CAPI_return_t chipRecvW(CAPI_endpoint_t sender, 
                                     CAPI_endpoint_t receiver, char *buffer, 
                                     int size);

      // performance modeling wrappers
 
      friend void perfModelRun(int rank, PerfModelIntervalStat *interval_stats);
      friend void perfModelRun(int rank, PerfModelIntervalStat *interval_stats, 
                               REG *reads, UINT32 num_reads);
      friend void perfModelRun(int rank, PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
                               REG *writes, UINT32 num_writes);
      friend PerfModelIntervalStat** perfModelAnalyzeInterval(const string& parent_routine, 
                                                              const INS& start_ins, 
                                                              const INS& end_ins);
      friend void perfModelLogICacheLoadAccess(int rank, PerfModelIntervalStat *stats, bool hit);
      friend void perfModelLogDCacheStoreAccess(int rank, PerfModelIntervalStat *stats, bool hit);
      friend void perfModelLogBranchPrediction(int rank, PerfModelIntervalStat *stats, bool correct);      

      // syscall modeling wrapper
      friend void syscallEnterRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      friend void syscallExitRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);

      // sync wrappers
      friend void SimMutexInit(carbon_mutex_t *mux);
      friend void SimMutexLock(carbon_mutex_t *mux);
      friend void SimMutexUnlock(carbon_mutex_t *mux);

      friend void SimCondInit(carbon_cond_t *cond);
      friend void SimCondWait(carbon_cond_t *cond, carbon_mutex_t *mux);
      friend void SimCondSignal(carbon_cond_t *cond);
      friend void SimCondBroadcast(carbon_cond_t *cond);

      friend void SimBarrierInit(carbon_barrier_t *barrier, UINT32 count);
      friend void SimBarrierWait(carbon_barrier_t *barrier);
      
      // organic cache modeling wrappers
      friend bool icacheRunLoadModel(ADDRINT i_addr, UINT32 size);
      friend bool dcacheRunModel(CacheBase::AccessType access_type, ADDRINT d_addr, char* data_buffer, UINT32 data_size);
		//TODO deprecate these two bottom functions
      // friend bool dcacheRunLoadModel(ADDRINT d_addr, UINT32 size);
      // friend bool dcacheRunStoreModel(ADDRINT d_addr, UINT32 size);

		// FIXME: A hack for DEBUG purposes
		friend CAPI_return_t chipAlias(ADDRINT address0, addr_t addType, UINT32 num);
		friend bool isAliasEnabled(void);

   private:

      int num_modules;

      // tid_map takes core # to pin thread id
      // core_map takes pin thread id to core # (it's the reverse map)
      THREADID *tid_map;
      LockFreeHash core_map;
      int prev_rank;
      
      Core *core;

	 	bool* finished_cores;

   public:

		// FIXME: This is strictly a hack for testing data storage
		bool aliasEnable;
		std::map<ADDRINT,ADDRINT> aliasMap;
		//////////////////////////////////////////////////////////

      Chip(int num_mods);

      int getNumModules() { return num_modules; }
      Core *getCore(unsigned int id) { return &(core[id]); }
      void fini(int code, void *v);

		//input parameters: 
		//an address to set the conditions for
		//a dram vector, with a pair for the id's of the dram directories to set, and the value to set it to
		//a cache vector, with a pair for the id's of the caches to set, and the value to set it to
		void debugSetInitialMemConditions (vector<ADDRINT>& address_vector, 
		  											  vector< pair<INT32, DramDirectoryEntry::dstate_t> >& dram_vector, vector<vector<UINT32> >& sharers_list_vector, 
													  vector< vector< pair<INT32, CacheState::cstate_t> > >& cache_vector, 
		  											  vector<char*>& d_data_vector, 
													  vector<char*>& c_data_vector);
		bool debugAssertMemConditions (vector<ADDRINT>& address_vector, 
		  										 vector< pair<INT32, DramDirectoryEntry::dstate_t> >& dram_vector, vector<vector<UINT32> >& sharers_list_vector, 
												 vector< vector< pair<INT32, CacheState::cstate_t> > >& cache_vector, 
		  										 vector<char*>& d_data_vector, 
												 vector<char*>& c_data_vector,
												 string test_code, string error_string);
		
};

#endif
