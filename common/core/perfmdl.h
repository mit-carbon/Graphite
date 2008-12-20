// Jonathan Eastep (eastep@mit.edu) 
// 04.07.08
//
// This file contains classes and structs for modeling performance in the 
// simulator. It collects stats from the various other models that are
// running (e.g. the cache model) and loosely models microarchitectural 
// execution to keep a clock updated. To minimize overhead in Pin, as few 
// as possible additional instructions are instrumented exclusively for 
// performance modeling; instead, performance modeling tries to piggy-back
// on the instrumentation the other models required. Because models don't
// end up instrumenting every instruction, the performance model uses
// time warp and batches up as much of the modeling it needs to do as 
// possible. 


#ifndef PERFMDL_H
#define PERFMDL_H

#include <vector>
#include <utility>
#include <iostream>

#include "pin.H"

#include "perfmdl_interval_stat.h"

/* ===================================================================== */
/* External References */
/* ===================================================================== */

extern LEVEL_BASE::KNOB<bool> g_knob_enable_performance_modeling;


/* ===================================================================== */
/* Performance Modeler Classes */
/* ===================================================================== */


// JME. FIXME. many of these members should be private.



class PerfModel {

   private:
      // does not include stalls
      UINT64 microop_issue_count;

      // this is the local clock for the core
      UINT64 cycle_count;    

      // this is used for finding dependencies on loaded data
      vector<UINT64> scoreboard;

      // set for debugging purposes
      string name;

      // Lock for atomically updating the clock
      PIN_LOCK m_clock_lock;
    
      // methods
      UINT32 getInsMicroOpsCount(const INS& ins);

   public:

      PerfModel(string n);

      // The following two methods atomically update the cycle count
      void updateCycleCount(UINT64 new_cycle_count);
      void addToCycleCount(UINT64 cycles);

      UINT64 getCycleCount() { return cycle_count; }
      UINT64 getMicroOpCount() { return microop_issue_count; }


      // These functions are for logging modeling events for which the performance impact
      // may be lazily evaluated later when the performance model is next run. 

      VOID logICacheLoadAccess(PerfModelIntervalStat *stats, bool hit)
      { 
	 // stats->icache_load_miss_history.push_back( !hit ); 
	 stats->logICacheLoadAccess(hit);
      }
     
      VOID logDCacheLoadAccess(PerfModelIntervalStat *stats, bool hit)
      {
	 stats->logDCacheLoadAccess(hit);
      }

      VOID logDCacheStoreAccess(PerfModelIntervalStat *stats, bool hit)
      { 
	 // stats->dcache_store_miss_history.push_back( !hit ); 
	 stats->logDCacheStoreAccess(hit);
      }

      VOID logBranchPrediction(PerfModelIntervalStat *stats, bool correct)
      {  // stats->branch_mispredict = !correct; 
	 stats->logBranchPrediction(correct);
      }


      // Called at first encounter of an interval. Fills out stats for the interval

      PerfModelIntervalStat* analyzeInterval(const string& parent_routine, 
                                             const INS& start_ins, const INS& end_ins);

   
      // Pin inserts a call to one of the following functions when instrumenting 
      // instructions.

      // the vanilla run method.
      VOID run(PerfModelIntervalStat *interval_stats, bool firstCallInIntrvl);

      // run method which accounts for load data dependency stalls
      VOID run(PerfModelIntervalStat *interval_stats, REG *reads, UINT32 num_reads, bool firstCallInIntrvl);

      // run method which registers destination registers in the scoreboard
      VOID run(PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
               REG *writes, UINT32 num_writes, bool firstCallInIntrvl);


      // this method is called at the end of simulation
      //FIXME: implement this function
      VOID fini(int code, VOID *v, ofstream& out)
      { }

};


#endif
