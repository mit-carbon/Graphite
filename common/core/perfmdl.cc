#include "perfmdl.h"
#include "config.h"
#include <cassert>

#include "pin.H"

PerfModel::PerfModel(string name)
      :
      m_microop_issue_count(0),
      m_cycle_count(0),
      m_scoreboard(LEVEL_BASE::REG_LAST, k_PERFMDL_CYCLE_INVALID),
      m_name(name)
{
}

PerfModel::~PerfModel()
{
}

void PerfModel::runComputationModel(PerfModelIntervalStat *interval_stats)
{
   addToCycleCount(interval_stats->cycles_subtotal);
   m_microop_issue_count += interval_stats->microops_count;
}

void PerfModel::runICacheModel(PerfModelIntervalStat *interval_stats)
{
   UInt32 interval_cycle_count = 0;
   interval_cycle_count += (interval_stats->branch_mispredict ? 10 : 0);

   // icache miss penalty
   for (UInt32 i = 0; i < interval_stats->getICacheLoadAccessCount(); i++)
   {
      interval_cycle_count += interval_stats->getICacheLoadAccessMissStatus(i) ? 10 : 0;
   }

   addToCycleCount(interval_cycle_count);
   interval_stats->reset();
}

// run method which accounts for load data dependency stalls
void PerfModel::runDCacheWriteModel(PerfModelIntervalStat *interval_stats, REG *reads, UInt32 numReads)
{
   interval_stats->reset();
   UInt64 max = m_cycle_count;
   REG max_reg = LEVEL_BASE::REG_LAST;

   for (UInt32 i = 0; i < numReads; i++)
   {
      REG r = reads[i];
      assert((UInt32)r < m_scoreboard.size());
      UInt64 cycle = m_scoreboard[r];

      if (cycle != k_PERFMDL_CYCLE_INVALID)
      {
         if (cycle > max)
         {
            max = cycle;
            max_reg = r;
         }

         // first use encountered so release scoreboard slot
         m_scoreboard[r] = k_PERFMDL_CYCLE_INVALID;
         // removed REG_StringShort(r)from scoreboard.
      }
   }

   if (max != m_cycle_count)
   {
      // cout << "stalled from " << m_cycle_count << " to " << max << " on "
      //     << REG_StringShort(max_reg) << endl;
   }

   // stall for latest load dependency if needed
   updateCycleCount(max);
}

void PerfModel::runDCacheReadModel(PerfModelIntervalStat *interval_stats, bool dcache_load_hit, REG *writes, UInt32 numWrites)
{
   interval_stats->reset();
   interval_stats->logDCacheLoadAccess(dcache_load_hit);

   if (Config::getSingleton()->getEnablePerformanceModeling() && !dcache_load_hit)
   {
      for (UInt32 i = 0; i < numWrites; i++)
      {
         REG w = writes[i];
         assert((UInt32)w < m_scoreboard.size());
         m_scoreboard[w] = m_cycle_count + 100;  //FIXME: make this parameterizable
      }
   }
}


void PerfModel::updateCycleCount(UInt64 new_cycle_count)
{
   m_clock_lock.acquire();
   m_cycle_count = max(m_cycle_count, new_cycle_count);
   m_clock_lock.release();
}
void PerfModel::addToCycleCount(UInt64 cycles)
{
   m_clock_lock.acquire();
   m_cycle_count += cycles;
   m_clock_lock.release();
}

void PerfModel::outputSummary(ostream& out)
{
   out << "  Total cycles: " << getCycleCount() << endl;
}
