
#define k_PERFMDL_CYCLE_INVALID  (~ ((UINT64) 0) )
#define k_PERFMDL_INT_STATE_SIZE 5

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
         ASSERTX( icache_load_miss_history_index < k_PERFMDL_INT_STATE_SIZE );
         icache_load_miss_history[icache_load_miss_history_index++] = !hit;
      }      

      UINT32 getICacheLoadAccessCount()
      {  return icache_load_miss_history_index; }

      bool getICacheLoadAccessMissStatus(UINT32 which)
      {
         ASSERTX( which < k_PERFMDL_INT_STATE_SIZE );
         return icache_load_miss_history[which];
      }


      VOID logDCacheLoadAccess(bool hit)
      {
         ASSERTX( dcache_load_miss_history_index < k_PERFMDL_INT_STATE_SIZE );
         dcache_load_miss_history[dcache_load_miss_history_index++] = !hit;
      }

      UINT32 getDCacheLoadAccessCount()
      {  return dcache_load_miss_history_index; }

      bool getDCacheLoadAccessMissStatus(UINT32 which)
      {
         ASSERTX( which < k_PERFMDL_INT_STATE_SIZE );
         return dcache_load_miss_history[which]; 
      }


      VOID logDCacheStoreAccess(bool hit)
      {
         ASSERTX( dcache_store_miss_history_index < k_PERFMDL_INT_STATE_SIZE );
         dcache_store_miss_history[dcache_store_miss_history_index++] = !hit;
      }

      UINT32 getDCacheStoreAccessCount()
      {  return dcache_store_miss_history_index; }

      bool getDCacheStoreAccessMissStatus(UINT32 which)
      {
         ASSERTX( which < k_PERFMDL_INT_STATE_SIZE );
         return dcache_store_miss_history[which]; 
      }


      VOID logBranchPrediction(bool correct)
      {
         branch_mispredict = !correct; 
      }

      VOID reset()
      {
         // resets everything but inst_trace, parent, 
         // microops_count and cyc_subtotal
         // (all the dynamic stuff)

         // changed because lists were too memory intensive
         // dcache_load_miss_history.resize(0);
         // dcache_store_miss_history.resize(0);
         // icache_load_miss_history.resize(0);
         icache_load_miss_history_index = 0;
         dcache_load_miss_history_index = 0;
         dcache_store_miss_history_index = 0;

         branch_mispredict = false; 
      }
};


