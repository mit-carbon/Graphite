#include "perfmdl.h"
#include <cassert>

PerfModel::PerfModel(string n)
   : microop_issue_count(0), 
   cycle_count(0), 
   scoreboard(LEVEL_BASE::REG_LAST, k_PERFMDL_CYCLE_INVALID), 
   name(n)
{
   InitLock(&m_clock_lock);
}

UINT32 PerfModel::getInsMicroOpsCount(const INS& ins)
{
   // FIXME: assumes that stack is not supported by special hardware; load and
   // store microops are assumed to be required.

   bool does_read  = INS_IsMemoryRead(ins);
   bool does_read2 = INS_HasMemoryRead2(ins);
   bool does_write = INS_IsMemoryWrite(ins);

   UINT32 count = 0;         

   // potentially load first operand from mem
   count += does_read ? 1 : 0;

   // potentially load second operand from mem
   count += does_read2 ? 1 : 0;

   // perform the op on the operands
   count += 1;

   // potentially store the result to mem
   count += does_write ? 1 : 0;

   return count;
}


PerfModelIntervalStat* PerfModel::analyzeInterval(const string& parent_routine, 
                                                  const INS& start_ins, const INS& end_ins)
{
   vector< pair<ADDRINT, UINT32> > inst_trace;
   UINT32 microop_count = 0;
   UINT32 cycles_subtotal = 0;

   // do some analysis to get the number of cycles (before mem, branch stalls)
   // fixme: for now we approximate with approx # x86 microops;
   // need to account for pipeline depth / instruction latencies

   for(INS ins = start_ins; ins!=end_ins; ins = INS_Next(ins))
   {
      // debug info
      // cout << hex << "0x" << INS_Address(BBL_InsTail(bbl)) << dec << ": " 
      //      << INS_Mnemonic(BBL_InsTail(bbl)) << endl;

      inst_trace.push_back( pair<ADDRINT, UINT32>(INS_Address(ins), INS_Size(ins)) );
      UINT32 micro_ops = getInsMicroOpsCount(ins);
      microop_count += micro_ops;                
      // FIXME
      cycles_subtotal += micro_ops;
   }

   // allocate struct for instructs in the basic block to write stats into.
   // NOTE: if a basic block gets split, this data may become redundant
   PerfModelIntervalStat *stats = new PerfModelIntervalStat(parent_routine, 
                                                            inst_trace,
                                                            microop_count, 
                                                            cycles_subtotal);
   return stats;
}


void PerfModel::run(PerfModelIntervalStat *interval_stats, bool firstCallInIntrvl)
{
   // NOTE: must function such that it can be called more than once per 
   // interval and still work
 
   UINT32 interval_cycle_count = 0;

   if ( firstCallInIntrvl )
      interval_cycle_count += interval_stats->cycles_subtotal;

   interval_cycle_count += (interval_stats->branch_mispredict ? 10 : 0);

   // Note: dcache load miss penalty is already 
   // accounted for by dependency stalling

   // Note: perfect dcache store queue assumed. 
   // store miss penalty assumed to be zero.

   // icache miss penalty
   //for (list<bool>::iterator it = interval_stats->icache_load_miss_history.begin(); 
   //     it != interval_stats->icache_load_miss_history.end();
   //     it++)
   //{ 
   //   // FIXME: this is not a constant. at minimum it should be a 
   //   // constant exposed to outside world
   //   interval_cycle_count += ((*it) ? 10 : 0); 
   // }
   for (UINT32 i = 0; i < interval_stats->getICacheLoadAccessCount(); i++)
   {
      interval_cycle_count += interval_stats->getICacheLoadAccessMissStatus(i) ? 10 : 0;
   }

   lockClock();
   {
      cycle_count += interval_cycle_count;
   }
   unlockClock();

   if ( firstCallInIntrvl )
      microop_issue_count += interval_stats->microops_count;

   // clear out values in case Run gets called again this interval
   interval_stats->reset();

   //cout << "made it here" << endl;
}


// run method which accounts for load data dependency stalls
void PerfModel::run(PerfModelIntervalStat *interval_stats, REG *reads, 
                    UINT32 numReads, bool firstCallInIntrvl)
{

   run(interval_stats, firstCallInIntrvl);

   UINT64 max = cycle_count;
   REG max_reg = LEVEL_BASE::REG_LAST;

   for ( UINT32 i = 0; i < numReads; i++ )
   {
      REG r = reads[i];
      assert((UINT32)r < scoreboard.size());
      UINT64 cycle = scoreboard[r];

      if ( cycle != k_PERFMDL_CYCLE_INVALID ) {
         if ( cycle > max ) {
            max = cycle;
            max_reg = r;
         }

         // first use encountered so release scoreboard slot
         scoreboard[r] = k_PERFMDL_CYCLE_INVALID;
         //cout << "removed " << REG_StringShort(r) << " from scoreboard: " 
         //     << cycle << endl;
      }
   } 
  
   if ( max != cycle_count ) {
      // cout << "stalled from " << cycle_count << " to " << max << " on " 
      //     << REG_StringShort(max_reg) << endl;
   }

   // stall for latest load dependency if needed
   lockClock();
   {
      cycle_count = max;
   }
   unlockClock();
}


void PerfModel::run(PerfModelIntervalStat *interval_stats, bool dcache_load_hit, REG *writes, 
                    UINT32 numWrites, bool firstCallInIntrvl)
{
   run(interval_stats, firstCallInIntrvl);

   //interval_stats->dcache_load_miss_history.push_back( !dcache_load_hit );
   interval_stats->logDCacheLoadAccess(dcache_load_hit);

   if ( g_knob_enable_performance_modeling && !dcache_load_hit ) {
      for (UINT32 i = 0; i < numWrites; i++) {
         REG w = writes[i];
	 assert((UINT32)w < scoreboard.size());
         scoreboard[w] = cycle_count + 100;  //FIXME: make this parameterizable
         // cout << "added " << REG_StringShort(w) << " to scoreboard: " 
         //     << cycle_count << " + 100 = " << scoreboard[w] << endl;
      }
   }
}


