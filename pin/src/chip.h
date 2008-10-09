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
#include "syscall_model.h"
#include "syscall_server.h"


// external variables

// JME: not entirely sure why these are needed...
class Chip;
class Core;

extern Chip *g_chip;
extern SyscallServer *g_syscall_server;

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

void perfModelRun(PerfModelIntervalStat *interval_stats);

void perfModelRun(PerfModelIntervalStat *interval_stats, REG *reads, 
                  UINT32 num_reads);

void perfModelRun(PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
                  REG *writes, UINT32 num_writes);

PerfModelIntervalStat* perfModelAnalyzeInterval(const string& parent_routine, 
                                                const INS& start_ins, const INS& end_ins);

void perfModelLogICacheLoadAccess(PerfModelIntervalStat *stats, bool hit);
     
void perfModelLogDCacheStoreAccess(PerfModelIntervalStat *stats, bool hit);

void perfModelLogBranchPrediction(PerfModelIntervalStat *stats, bool correct);


// organic cache model wrappers

bool icacheRunLoadModel(ADDRINT i_addr, UINT32 size);

bool dcacheRunLoadModel(ADDRINT d_addr, UINT32 size);

bool dcacheRunStoreModel(ADDRINT d_addr, UINT32 size);

// syscall model wrappers
void syscallEnterRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
void syscallExitRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);

// syscall server wrappers
void syscallServerRun();


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
 
      friend void perfModelRun(PerfModelIntervalStat *interval_stats);
      friend void perfModelRun(PerfModelIntervalStat *interval_stats, REG *reads, 
                               UINT32 num_reads);
      friend void perfModelRun(PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
                               REG *writes, UINT32 num_writes);
      friend PerfModelIntervalStat* perfModelAnalyzeInterval(const string& parent_routine, 
                                                             const INS& start_ins, 
                                                             const INS& end_ins);
      friend void perfModelLogICacheLoadAccess(PerfModelIntervalStat *stats, bool hit);
      friend void perfModelLogDCacheStoreAccess(PerfModelIntervalStat *stats, bool hit);
      friend void perfModelLogBranchPrediction(PerfModelIntervalStat *stats, bool correct);      

      // syscall modeling wrapper
      friend void syscallEnterRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      friend void syscallExitRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);

      
      // organic cache modeling wrappers
      friend bool icacheRunLoadModel(ADDRINT i_addr, UINT32 size);
      friend bool dcacheRunLoadModel(ADDRINT d_addr, UINT32 size);
      friend bool dcacheRunStoreModel(ADDRINT d_addr, UINT32 size);      

   private:

      int num_modules;

      UINT64 *proc_time;

      // tid_map takes core # to pin thread id
      // core_map takes pin thread id to core # (it's the reverse map)
      THREADID *tid_map;
      map<THREADID, int> core_map;
      int prev_rank;
      PIN_LOCK maps_lock;

      Core *core;


   public:

      UINT64 getProcTime(int module) { return proc_time[module]; }

      void setProcTime(int module, unsigned long long new_time) 
      { proc_time[module] = new_time; }

      int getNumModules() { return num_modules; }

      Chip(int num_mods);

      void fini(int code, void *v);

};

#endif
