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
#include <string>

#include "fixed_types.h"
#include "perfmdl_interval_stat.h"
#include "lock.h"


/* ===================================================================== */
/* Performance Modeler Classes */
/* ===================================================================== */

// JME. FIXME. many of these members should be private.

class PerfModel
{
   public:

      PerfModel(std::string n);
      ~PerfModel();

      // The following two methods atomically update the cycle count
      void updateCycleCount(UInt64 new_cycle_count);
      void addToCycleCount(UInt64 cycles);

      UInt64 getCycleCount() { return m_cycle_count; }
      UInt64 getMicroOpCount() { return m_microop_issue_count; }

      // Pin inserts a call to one of the following functions when instrumenting
      // instructions.

      // the vanilla run method.
      void runComputationModel(PerfModelIntervalStat *interval_stats);
      void runICacheModel(PerfModelIntervalStat *interval_stats);

      // run method which accounts for load data dependency stalls
      void runDCacheWriteModel(PerfModelIntervalStat *interval_stats, carbon_reg_t *reads, UInt32 num_reads);

      // run method which registers destination registers in the scoreboard
      void runDCacheReadModel(PerfModelIntervalStat *interval_stats, bool dcache_load_hit,
               carbon_reg_t *writes, UInt32 num_writes);


      // this method is called at the end of simulation
      void outputSummary(std::ostream& out);

   private:
      // does not include stalls
      UInt64 m_microop_issue_count;

      // this is the local clock for the core
      UInt64 m_cycle_count;

      // this is used for finding dependencies on loaded data
      std::vector<UInt64> m_scoreboard;

      // set for debugging purposes
      std::string m_name;

      // Lock for atomically updating the clock
      Lock m_clock_lock;

      static const int REG_LAST = 4096;

};

#endif
