#include <cassert>

#define k_PERFMDL_CYCLE_INVALID  (~ ((UInt64) 0) )
#define k_PERFMDL_INT_STATE_SIZE 5

class PerfModelIntervalStat
{
   private:

      // keeps track of miss status for icache and dcache loads and stores
      // set true for miss

      bool icache_load_miss_history[k_PERFMDL_INT_STATE_SIZE];
      bool dcache_load_miss_history[k_PERFMDL_INT_STATE_SIZE];
      bool dcache_store_miss_history[k_PERFMDL_INT_STATE_SIZE];
      UInt32 icache_load_miss_history_index;
      UInt32 dcache_load_miss_history_index;
      UInt32 dcache_store_miss_history_index;

   public:
      // holds instruction addresses and sizes
      std::vector< std::pair<IntPtr, UInt32> > inst_trace;

      // set when instrumenting the code to add calls to analysis
      UInt32 microops_count;
      UInt32 cycles_subtotal;

      // set true if interval had branch mispredict
      bool branch_mispredict;

      // set for use in debugging
      std::string parent_routine;

      // methods
      PerfModelIntervalStat(const std::string& parent, const std::vector< std::pair<IntPtr, UInt32> >& trace,
                            UInt32 uops, UInt32 cyc_subtotal):
            icache_load_miss_history_index(0), dcache_load_miss_history_index(0), dcache_store_miss_history_index(0),
            inst_trace(trace),
            microops_count(uops), cycles_subtotal(cyc_subtotal),
            branch_mispredict(false), parent_routine(parent)
      {
      }

      void logICacheLoadAccess(bool hit)
      {
         assert(icache_load_miss_history_index < k_PERFMDL_INT_STATE_SIZE);
         icache_load_miss_history[icache_load_miss_history_index++] = !hit;
      }

      UInt32 getICacheLoadAccessCount()
      {  return icache_load_miss_history_index; }

      bool getICacheLoadAccessMissStatus(UInt32 which)
      {
         assert(which < k_PERFMDL_INT_STATE_SIZE);
         return icache_load_miss_history[which];
      }


      void logDCacheLoadAccess(bool hit)
      {
         assert(dcache_load_miss_history_index < k_PERFMDL_INT_STATE_SIZE);
         dcache_load_miss_history[dcache_load_miss_history_index++] = !hit;
      }

      UInt32 getDCacheLoadAccessCount()
      {  return dcache_load_miss_history_index; }

      bool getDCacheLoadAccessMissStatus(UInt32 which)
      {
         assert(which < k_PERFMDL_INT_STATE_SIZE);
         return dcache_load_miss_history[which];
      }


      void logDCacheStoreAccess(bool hit)
      {
         assert(dcache_store_miss_history_index < k_PERFMDL_INT_STATE_SIZE);
         dcache_store_miss_history[dcache_store_miss_history_index++] = !hit;
      }

      UInt32 getDCacheStoreAccessCount()
      {  return dcache_store_miss_history_index; }

      bool getDCacheStoreAccessMissStatus(UInt32 which)
      {
         assert(which < k_PERFMDL_INT_STATE_SIZE);
         return dcache_store_miss_history[which];
      }

      void logBranchPrediction(bool correct)
      {
         branch_mispredict = !correct;
      }

      void reset()
      {
         // resets everything but inst_trace, parent,
         // microops_count and cyc_subtotal
         // (all the dynamic stuff)

         icache_load_miss_history_index = 0;
         dcache_load_miss_history_index = 0;
         dcache_store_miss_history_index = 0;

         branch_mispredict = false;
      }

};
