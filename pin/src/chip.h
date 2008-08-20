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
#include "perfmdl.h"
#include "ocache.h"

// external variables

// JME: not entirely sure why these are needed...
class Chip;
class Core;

extern Chip *g_chip;
extern LEVEL_BASE::KNOB<string> g_knob_output_file;



// prototypes

// FIXME: if possible, these shouldn't be globals. Pin callbacks may need them to be. 

CAPI_return_t chipInit(int *rank);

CAPI_return_t chipRank(int *rank);

CAPI_return_t chipSendW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                        char *buffer, int size);

CAPI_return_t chipRecvW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                        char *buffer, int size);
CAPI_return_t chipHackFinish(int my_rank);

// performance model wrappers

VOID perfModelRun(PerfModelIntervalStat *interval_stats);

VOID perfModelRun(PerfModelIntervalStat *interval_stats, REG *reads, 
                  UINT32 num_reads);

VOID perfModelRun(PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
                  REG *writes, UINT32 num_writes);

PerfModelIntervalStat* perfModelAnalyzeInterval(const string& parent_routine, 
                                                const INS& start_ins, const INS& end_ins);

VOID perfModelLogICacheLoadAccess(PerfModelIntervalStat *stats, bool hit);
     
VOID perfModelLogDCacheStoreAccess(PerfModelIntervalStat *stats, bool hit);

VOID perfModelLogBranchPrediction(PerfModelIntervalStat *stats, bool correct);


// organic cache model wrappers

bool icacheRunLoadModel(ADDRINT i_addr, UINT32 size);

bool dcacheRunLoadModel(ADDRINT d_addr, UINT32 size);

bool dcacheRunStoreModel(ADDRINT d_addr, UINT32 size);


// chip class

class Chip 
{

   // wrappers
   public:

      // messaging wrappers

      friend CAPI_return_t chipInit(int *rank); 
      friend CAPI_return_t chipRank(int *rank);
      
	  //FIXME: this is a hack function 
	  friend CAPI_return_t chipHackFinish(int my_rank);
      
	  friend CAPI_return_t chipSendW(CAPI_endpoint_t sender, 
                                     CAPI_endpoint_t receiver, char *buffer, 
                                     int size);
      friend CAPI_return_t chipRecvW(CAPI_endpoint_t sender, 
                                     CAPI_endpoint_t receiver, char *buffer, 
                                     int size);

      // performance modeling wrappers
 
      friend VOID perfModelRun(PerfModelIntervalStat *interval_stats);
      friend VOID perfModelRun(PerfModelIntervalStat *interval_stats, REG *reads, 
                               UINT32 num_reads);
      friend VOID perfModelRun(PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
                               REG *writes, UINT32 num_writes);
      friend PerfModelIntervalStat* perfModelAnalyzeInterval(const string& parent_routine, 
                                                             const INS& start_ins, 
                                                             const INS& end_ins);
      friend VOID perfModelLogICacheLoadAccess(PerfModelIntervalStat *stats, bool hit);
      friend VOID perfModelLogDCacheStoreAccess(PerfModelIntervalStat *stats, bool hit);
      friend VOID perfModelLogBranchPrediction(PerfModelIntervalStat *stats, bool correct);      

      
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

      VOID fini(int code, VOID *v);

};

#endif
