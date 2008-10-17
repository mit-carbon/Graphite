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
#include "perfmdl.h"
#include "lockfree_hash.h"
#include "syscall_model.h"
#include "mcp.h"


// external variables

// JME: not entirely sure why these are needed...
class Chip;
class Core;

extern Chip *g_chip;
extern MCP *g_MCP;

extern LEVEL_BASE::KNOB<string> g_knob_output_file;

// prototypes

// FIXME: if possible, these shouldn't be globals. Pin callbacks may need them to be. 

CAPI_return_t chipInit(int *rank);

CAPI_return_t chipRank(int *rank);

CAPI_return_t commRank(int *rank);

CAPI_return_t chipSendW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                        char *buffer, int size);

CAPI_return_t chipRecvW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                        char *buffer, int size);


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

bool icacheRunLoadModel(int rank, ADDRINT i_addr, UINT32 size);

bool dcacheRunLoadModel(int rank, ADDRINT d_addr, UINT32 size);

bool dcacheRunStoreModel(int rank, ADDRINT d_addr, UINT32 size);


// syscall model wrappers

void syscallEnterRunModel(int rank, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);

void syscallExitRunModel(int rank, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);


// MCP server wrappers
void MCPRun();
void MCPFinish();

// chip class

class Chip 
{

   // wrappers
   public:

      // messaging wrappers

      friend CAPI_return_t chipInit(int *rank); 
      friend CAPI_return_t chipRank(int *rank);
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
      friend void syscallEnterRunModel(int rank, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      friend void syscallExitRunModel(int rank, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);

      
      // organic cache modeling wrappers
      friend bool icacheRunLoadModel(int rank, ADDRINT i_addr, UINT32 size);
      friend bool dcacheRunLoadModel(int rank, ADDRINT d_addr, UINT32 size);
      friend bool dcacheRunStoreModel(int rank, ADDRINT d_addr, UINT32 size);      

   private:

      int num_modules;

      UINT64 *proc_time;

      // tid_map takes core # to pin thread id
      // core_map takes pin thread id to core # (it's the reverse map)
      THREADID *tid_map;
      map<THREADID, int> core_map;
      LockFreeHash core_map2;
      int prev_rank;
      PIN_LOCK maps_lock;

      Core *core;


   public:

      UINT64 getProcTime(int module) { assert(module < num_modules); return proc_time[module]; }

      void setProcTime(int module, unsigned long long new_time) 
      { assert(module < num_modules); proc_time[module] = new_time; }

      int getNumModules() { return num_modules; }

      Chip(int num_mods);

      void fini(int code, void *v);

};

#endif
