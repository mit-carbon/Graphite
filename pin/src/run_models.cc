#include "run_models.h"

#include "core.h"
#include "ocache.h"
#include "core_manager.h"
#include "log.h"
#include "simulator.h"

static void runICacheModels(PerfModelIntervalStat *stats, Core *core, bool do_perf_modeling);
static void runDCacheReadModels(PerfModelIntervalStat *stats, Core *core, bool do_perf_modeling,
                                bool is_dual_read, IntPtr dcache_ld_addr, IntPtr dcache_ld_addr2, 
                                UINT32 dcache_ld_size,
                                REG *reads, UINT32 num_reads);

static void runDCacheWriteModels(PerfModelIntervalStat *stats, Core *core, bool do_perf_modeling,
                                 IntPtr dcache_st_addr, UINT32 dcache_st_size,
                                 REG *writes, UINT32 num_writes);

// For instrumentation / modeling
void runModels(IntPtr dcache_ld_addr, IntPtr dcache_ld_addr2, UINT32 dcache_ld_size,
               IntPtr dcache_st_addr, UINT32 dcache_st_size,
               PerfModelIntervalStat* *stats,
               REG *reads, UINT32 num_reads, REG *writes, UINT32 num_writes,
               bool do_icache_modeling, bool do_dcache_read_modeling, bool is_dual_read,
               bool do_dcache_write_modeling, bool do_perf_modeling, bool check_scoreboard)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   UInt32 core_index;

   if (!core)
      return;

   // Look up core index
   for (core_index = 0; core_index < Config::getSingleton()->getNumLocalCores(); core_index++)
   {
      if (Sim()->getCoreManager()->getCoreFromIndex(core_index) == core)
         break;
   }

   LOG_ASSERT_ERROR(core_index < Config::getSingleton()->getNumLocalCores(),
                    "No core index found for current core?! %p", core);

   PerfModelIntervalStat *interval_stats = stats[core_index];
   // This must be consistent with the behavior of
   // insertInstructionModelingCall.

   // Trying to prevent using NULL stats. This happens when
   // instrumenting portions of the main thread.
   bool skip_modeling = ((check_scoreboard || do_perf_modeling || do_icache_modeling) && stats == NULL);

   if (skip_modeling)
      return;

   // Account for cycles spent performing computation
   core->getPerfModel()->runComputationModel(interval_stats);

   // Account for cycles spent stalled on memory accesses
   // as well as perform the memory accesses in the icache model
   if (do_icache_modeling)
      runICacheModels(interval_stats, core, do_perf_modeling);

   // this check must go before everything but the icache check
   assert(!check_scoreboard || do_perf_modeling);

   // it's not possible to delay the evaluation of the performance impact for these.
   // get the cycle counter up to date then account for dependency stalls
   if (check_scoreboard)
      core->getPerfModel()->runDCacheWriteModel(stats[core_index], reads, num_reads);

   // Perform the dcache_read_modeling
   if (do_dcache_read_modeling)
      runDCacheReadModels(interval_stats, core, do_perf_modeling, is_dual_read, 
              dcache_ld_addr, dcache_ld_addr2, dcache_ld_size,
              writes, num_writes);
   else
      assert(dcache_ld_addr == (IntPtr) NULL && dcache_ld_addr2 == (IntPtr) NULL && dcache_ld_size == 0);

   // Perform the dcache write_modeling
   if (do_dcache_write_modeling)
      runDCacheWriteModels(interval_stats, core, do_perf_modeling, dcache_st_addr, dcache_st_size,
              reads, num_reads);
   else
      assert(dcache_st_addr == (IntPtr) NULL && dcache_st_size == 0);

} //end of runModels


static void runICacheModels(PerfModelIntervalStat *stats, Core *core, bool do_perf_modeling)
{
    for (UINT32 i = 0; i < (stats->inst_trace.size()); i++)
    {
        // first = PC, second = size
        bool i_hit = core->getOCache()->runICacheLoadModel(stats->inst_trace[i].first,
                stats->inst_trace[i].second).first;
        if (do_perf_modeling)
            stats->logICacheLoadAccess(i_hit);
    }
    core->getPerfModel()->runICacheModel(stats);;
}

static void runDCacheReadModels(PerfModelIntervalStat *stats, Core *core, bool do_perf_modeling,
                                bool is_dual_read, IntPtr dcache_ld_addr, IntPtr dcache_ld_addr2, 
                                UINT32 dcache_ld_size,
                                REG *reads, UINT32 num_reads)
{
    // FIXME: This should actually be a UINT32 which tells how many read misses occured
    char data_ld_buffer[dcache_ld_size];
    //TODO HARSHAD sharedmemory will fill ld_buffer
    bool d_hit = core->getOCache()->runDCacheModel(CacheBase::k_ACCESS_TYPE_LOAD, dcache_ld_addr, data_ld_buffer, dcache_ld_size);

    if (do_perf_modeling)
        core->getPerfModel()->runDCacheReadModel(stats, d_hit, reads, num_reads);

    if (is_dual_read)
    {
        char data_ld_buffer_2[dcache_ld_size];
        //TODO HARSHAD sharedmemory will fill ld_buffer
        bool d_hit2 = core->getOCache()->runDCacheModel(CacheBase::k_ACCESS_TYPE_LOAD, dcache_ld_addr2, data_ld_buffer_2, dcache_ld_size);
        // bool d_hit2 = dcacheRunLoadModel(dcache_ld_addr2, dcache_ld_size);
        if (do_perf_modeling)
            core->getPerfModel()->runDCacheReadModel(stats, d_hit2, reads, num_reads);
    }
}

static void runDCacheWriteModels(PerfModelIntervalStat *stats, Core *core, bool do_perf_modeling,
                                 IntPtr dcache_st_addr, UINT32 dcache_st_size,
                                 REG *writes, UINT32 num_writes)
{
   // FIXME: This should actually be a UINT32 which tells how many write misses occurred
   char data_st_buffer[dcache_st_size];

   //TODO Harshad: shared memory expects all data_buffers to be pre-allocated
   core->getOCache()->runDCacheModel(CacheBase::k_ACCESS_TYPE_STORE, dcache_st_addr, data_st_buffer, dcache_st_size);

   // This breaks the code
   /*
   bool d_hit = core->getOCache()->runDCacheModel(CacheBase::k_ACCESS_TYPE_STORE, dcache_st_addr, data_st_buffer, dcache_st_size);
   
   if (do_perf_modeling)
      stats->logDCacheStoreAccess(d_hit);
    */
}
