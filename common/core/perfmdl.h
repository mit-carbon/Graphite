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

#include <list>
#include <vector>
#include <utility>
#include <iostream>

#include "pin.H"


/* ===================================================================== */
/* External References */
/* ===================================================================== */

extern LEVEL_BASE::KNOB<bool> g_knob_enable_performance_modeling;


/* ===================================================================== */
/* Performance Modeler Classes */
/* ===================================================================== */

#define k_PERFMDL_CYCLE_INVALID  (~ ((UINT64) 0) )


// JME. I feel like this class is exempt from some of the style guidelines
// since it's a class that's more or less used like a struct 

class PerfModelIntervalStat {
   public:
      // holds instruction addresses and sizes 
      vector< pair<ADDRINT, UINT32> > inst_trace;

      // keeps track of miss status for icache and dcache loads and stores
      // set true for miss
      list<bool> icache_load_miss_history;
      list<bool> dcache_load_miss_history;
      list<bool> dcache_store_miss_history;

      // set when instrumenting the code to add calls to analysis
      UINT32 microops_count;
      UINT32 cycles_subtotal;

      // set true if interval had branch mispredict   
      bool branch_mispredict;

      // set for use in debugging
      string parent_routine;


      // methods
      PerfModelIntervalStat(const string& parent, const vector< pair<ADDRINT, UINT32> >& trace, 
                            UINT32 uops, UINT32 cyc_subtotal): 
         inst_trace(trace), microops_count(uops), cycles_subtotal(cyc_subtotal), 
         branch_mispredict(false), parent_routine(parent)
      {
      }

      VOID reset()
      {
         // resets everything but inst_trace and parent
         dcache_load_miss_history.resize(0);
         dcache_store_miss_history.resize(0);
         icache_load_miss_history.resize(0);
         branch_mispredict = false; 
         microops_count = 0;
         cycles_subtotal = 0;
      }
};


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

    
      // methods
 
      UINT32 getInsMicroOpsCount(const INS& ins);

   public:

      PerfModel(string n): 
         microop_issue_count(0), cycle_count(0), 
	 scoreboard(LEVEL_BASE::REG_LAST, k_PERFMDL_CYCLE_INVALID), name(n)
      { }

      UINT64 getCycleCount() { return cycle_count; }
      UINT64 getMicroOpCount() { return microop_issue_count; }


      // These functions are for logging modeling events for which the performance impact
      // may be lazily evaluated later when the performance model is next run. 

      VOID logICacheLoadAccess(PerfModelIntervalStat *stats, bool hit)
      { stats->icache_load_miss_history.push_back( !hit ); }

      VOID logDCacheStoreAccess(PerfModelIntervalStat *stats, bool hit)
      { stats->dcache_store_miss_history.push_back( !hit ); }

      VOID logBranchPrediction(PerfModelIntervalStat *stats, bool correct)
      { stats->branch_mispredict = !correct; }


      // Called at first encounter of an interval. Fills out stats for the interval

      PerfModelIntervalStat* analyzeInterval(const string& parent_routine, 
                                             const INS& start_ins, const INS& end_ins);

   
      // Pin inserts a call to one of the following functions when instrumenting 
      // instructions.

      // the vanilla run method.
      VOID run(PerfModelIntervalStat *interval_stats);

      // run method which accounts for load data dependency stalls
      VOID run(PerfModelIntervalStat *interval_stats, REG *reads, UINT32 num_reads);

      // run method which registers destination registers in the scoreboard
      VOID run(PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
               REG *writes, UINT32 num_writes);


      // this method is called at the end of simulation
      //FIXME: implement this function
      VOID fini(int code, VOID *v, ofstream& out)
      { }

};


#endif
