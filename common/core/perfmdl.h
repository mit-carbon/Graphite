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


/* ===================================================================== */
/* External References */
/* ===================================================================== */

extern LEVEL_BASE::KNOB<bool> g_knob_enable_performance_modeling;


/* ===================================================================== */
/* Performance Modeler Classes */
/* ===================================================================== */

#define k_PERFMDL_CYCLE_INVALID  (~ ((UINT64) 0) )
#define k_PERFMDL_INT_STATE_SIZE 5

// JME. FIXME. many of these members should be private.

class PerfModelIntervalStat {
   private:

      // keeps track of miss status for icache and dcache loads and stores
      // set true for miss
      // too expensive in terms of memory footprint
      // list<bool> icache_load_miss_history;
      // list<bool> dcache_load_miss_history;
      // list<bool> dcache_store_miss_history;

      bool icache_load_miss_history[k_PERFMDL_INT_STATE_SIZE];
      bool dcache_load_miss_history[k_PERFMDL_INT_STATE_SIZE];
      bool dcache_store_miss_history[k_PERFMDL_INT_STATE_SIZE];
      UINT32 icache_load_miss_history_index;
      UINT32 dcache_load_miss_history_index;
      UINT32 dcache_store_miss_history_index;

   public:
      // holds instruction addresses and sizes 
      vector< pair<ADDRINT, UINT32> > inst_trace;



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
	 icache_load_miss_history_index(0), dcache_load_miss_history_index(0), dcache_store_miss_history_index(0),
         inst_trace(trace), 
         microops_count(uops), cycles_subtotal(cyc_subtotal), 
         branch_mispredict(false), parent_routine(parent)
      {
      }

      VOID logICacheLoadAccess(bool hit)
      {
	 assert( icache_load_miss_history_index < k_PERFMDL_INT_STATE_SIZE );
 	 icache_load_miss_history[icache_load_miss_history_index++] = !hit;
      }      

      UINT32 getICacheLoadAccessCount()
      {  return icache_load_miss_history_index; }

      bool getICacheLoadAccessMissStatus(UINT32 which)
      {  return icache_load_miss_history[which]; }


      VOID logDCacheLoadAccess(bool hit)
      {
         assert( dcache_load_miss_history_index < k_PERFMDL_INT_STATE_SIZE );
         dcache_load_miss_history[dcache_load_miss_history_index++] = !hit;
      }

      UINT32 getDCacheLoadAccessCount()
      {  return dcache_load_miss_history_index; }

      bool getDCacheLoadAccessMissStatus(UINT32 which)
      {  return dcache_load_miss_history[which]; }


      VOID logDCacheStoreAccess(bool hit)
      {
	 assert( dcache_store_miss_history_index < k_PERFMDL_INT_STATE_SIZE );
	 dcache_store_miss_history[dcache_store_miss_history_index++] = !hit;
      }

      UINT32 getDCacheStoreAccessCount()
      {  return dcache_store_miss_history_index; }

      bool getDCacheStoreAccessMissStatus(UINT32 which)
      {  return dcache_store_miss_history[which]; }


      VOID logBranchPrediction(bool correct)
      {
         branch_mispredict = !correct; 
      }

      VOID reset()
      {
         // resets everything but inst_trace and parent

         // changed because lists were too memory intensive
         // dcache_load_miss_history.resize(0);
         // dcache_store_miss_history.resize(0);
         // icache_load_miss_history.resize(0);
	 icache_load_miss_history_index = 0;
         dcache_load_miss_history_index = 0;
         dcache_store_miss_history_index = 0;

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
