#include "ocache.h"
#include "cache.h"

/* =================================================== */
/* OCache method definitions */
/* =================================================== */


// cache evolution related

VOID OCache::evolveNaive()
{
   // gives more associativity (and thus cachesize) to the cache with more misses
   if ( dcache_misses > (icache_misses * 1) )
   {
      //cout << dec << "dcacheMisses = " << dcache_misses << " icacheMisses = " 
      //     << icache_misses << endl;
      if ( il1->getNumWays() >= 2 )
      {
         il1->resize(il1->getNumWays() - 1);
         dl1->resize(dl1->getNumWays() + 1);
         //assume we don't have self-modifying code so no need to flush
      }  
   } 
   else 
   {
      if ( icache_misses > (dcache_misses * 1) ) 
      {
         //cout << dec << "dcacheMisses = " << dcache_misses << " icacheMisses = " 
         //     << icache_misses << endl;
         if ( dl1->getNumWays() >= 2 )
	 {
            dl1->resize(dl1->getNumWays() - 1);
            il1->resize(il1->getNumWays() + 1);
            total_resize_evictions += 1 * dl1->getNumSets();
         }
      }
   }
}

VOID OCache::evolveDataIntensive()
{
   //shrink icache so long as shrinking saves more dcache misses than icache misses it adds
   if ( (last_dcache_misses == 0 && last_icache_misses == 0) )
   { 
      // initial action
      if ( il1->getNumWays() >= 2 ) {
         il1->resize(il1->getNumWays() - 1);
         dl1->resize(dl1->getNumWays() + 1);
      }
   } 
   else 
   {
      if ( (dcache_misses + icache_misses) < (last_dcache_misses + last_icache_misses) )
      { 
         //we got improvement
         if ( il1->getNumWays() >= 2 ) {
            il1->resize(il1->getNumWays() - 1);
            dl1->resize(dl1->getNumWays() + 1);
         }
      }
   }

   last_dcache_misses = dcache_misses;
   last_icache_misses = icache_misses;
}  

VOID OCache::mutationRuntime()
{
   //if ( mutation_interval && ((icache_accesses + dcacheAccesses) >= mutation_interval) )
   //cout << dec << icache_misses << " " << dcache_misses << " " << endl;

   if ( mutation_interval && ((icache_misses + dcache_misses) >= mutation_interval) )
   {
      cout << "Mutation Interval Elapsed" << endl
           << "config before mutation:" << endl
           << statsLong() << endl;
      evolveNaive();
      //evolveDataIntensive();

      resetIntervalCounters();
   } 
   else 
   {
      if ( mutation_interval==0 && ((icache_misses + dcache_misses) >= 1000) ) {
         cout << "Mutation Interval Elapsed" << endl
              << "config before mutation:" << endl
              << statsLong() << endl;
         resetIntervalCounters();
      }
   }
}


// cache access related


pair<bool, CacheTag*> OCache::dCacheLoadSingle(ADDRINT addr, UINT32 inst_id)
{
   // @todo we may access several cache lines for 
   // first level D-cache
   pair<bool, CacheTag*> res = dl1->accessSingleLine(addr, CacheBase::k_ACCESS_TYPE_LOAD);
   const bool dl1_hit = res.first;
   const Counter counter = dl1_hit ? k_COUNTER_HIT : k_COUNTER_MISS;

   dcache_profile[inst_id][counter]++;
   dcacheAccesses++; 
   dcache_total_accesses++;

   if ( !dl1_hit ) 
   {
      dcache_misses++; 
      dcache_total_misses++;
   }      
   mutationRuntime();

   return res;
}

pair<bool, CacheTag*> OCache::dCacheLoadSingleFast(ADDRINT addr)
{
   pair<bool, CacheTag*> res = dl1->accessSingleLine(addr, CacheBase::k_ACCESS_TYPE_LOAD);
   const bool dl1_hit = res.first;

   dcacheAccesses++; 
   dcache_total_accesses++;

   if ( !dl1_hit ) 
   {
      dcache_misses++; 
      dcache_total_misses++;
   }      
   mutationRuntime();

   return res;
}


pair<bool, CacheTag*> OCache::dCacheStoreSingle(ADDRINT addr, UINT32 inst_id)
{
   // @todo we may access several cache lines for 
   // first level D-cache; we only model stores to dcache
   pair<bool, CacheTag*> res = dl1->accessSingleLine(addr, CacheBase::k_ACCESS_TYPE_STORE);
   const bool dl1_hit = res.first;
   const Counter counter = dl1_hit ? k_COUNTER_HIT : k_COUNTER_MISS;

   dcache_profile[inst_id][counter]++;
   dcacheAccesses++; 
   dcache_total_accesses++;

   if ( !dl1_hit ) 
   {
      dcache_misses++; 
      dcache_total_misses++;
   }      
   mutationRuntime();

   return res;
}

pair<bool, CacheTag*> OCache::dCacheStoreSingleFast(ADDRINT addr)
{
   // we only model stores for dcache
   pair<bool, CacheTag*> res = dl1->accessSingleLine(addr, CacheBase::k_ACCESS_TYPE_STORE);
   const bool dl1_hit = res.first;    

   dcacheAccesses++; 
   dcache_total_accesses++;

   if ( !dl1_hit ) 
   {
      dcache_misses++; 
      dcache_total_misses++;
   }      
   mutationRuntime();

   return res;
}


pair<bool, CacheTag*> OCache::iCacheLoadSingle(ADDRINT addr, UINT32 inst_id)
{
   // @todo we may access several cache lines for 
   // first level I-cache
   pair<bool, CacheTag*> res = il1->accessSingleLine(addr, CacheBase::k_ACCESS_TYPE_LOAD);
   const bool il1_hit = res.first;
   const Counter counter = il1_hit ? k_COUNTER_HIT : k_COUNTER_MISS;

   icache_profile[inst_id][counter]++;
   icache_accesses++; 
   icache_total_accesses++;

   if ( !il1_hit ) 
   {
      icache_misses++; 
      icache_total_misses++;
   }           
   mutationRuntime();

   return res;
}

pair<bool, CacheTag*> OCache::iCacheLoadSingleFast(ADDRINT addr)
{
   pair<bool, CacheTag*> res = il1->accessSingleLine(addr, CacheBase::k_ACCESS_TYPE_LOAD);
   const bool il1_hit = res.first;    

   icache_accesses++; 
   icache_total_accesses++;

   if ( !il1_hit ) 
   {
      icache_misses++; 
      icache_total_misses++;
   }           
   mutationRuntime();

   return res;
}


pair<bool, CacheTag*> OCache::runICacheLoadModel(ADDRINT i_addr, UINT32 size)
{
   ASSERTX(size <= 4);
   return iCacheLoadSingleFast(i_addr);
}

pair<bool, CacheTag*> OCache::runDCacheLoadModel(ADDRINT d_addr, UINT32 size)
{
   ASSERTX(size <= 4);
   return dCacheLoadSingleFast(d_addr);
}

pair<bool, CacheTag*> OCache::runDCacheStoreModel(ADDRINT d_addr, UINT32 size)
{
   ASSERTX(size <= 4);
   return dCacheStoreSingleFast(d_addr);     
}

pair<bool, CacheTag*> OCache::runICacheLoadModelPeek(ADDRINT i_addr, UINT32 size)
{
   ASSERTX(size <= 4);
   return il1->accessSingleLinePeek(i_addr);
}

pair<bool, CacheTag*> OCache::runDCacheLoadModelPeek(ADDRINT d_addr, UINT32 size)
{
   ASSERTX(size <= 4);
   return dl1->accessSingleLinePeek(d_addr);
}

pair<bool, CacheTag*> OCache::runDCacheStoreModelPeek(ADDRINT d_addr, UINT32 size)
{
   ASSERTX(size <= 4);
   return dl1->accessSingleLinePeek(d_addr);     
}


// constructor

OCache::OCache(std::string name, UINT32 size, UINT32 line_bytes, UINT32 assoc, UINT32 mutate_interval,
               UINT32 dcache_threshold_hit_value, UINT32 dcache_threshold_miss_value, UINT32 dcache_size, 
               UINT32 dcache_associativity, UINT32 dcache_max_search_depth, UINT32 icache_threshold_hit_value, 
               UINT32 icache_threshold_miss_value, UINT32 icache_size, UINT32 icache_associativity, 
               UINT32 icache_max_search_depth): 
   dl1(new RRSACache(name + "_dl1", dcache_size, line_bytes, dcache_associativity, dcache_max_search_depth)),
   il1(new RRSACache(name + "_il1", icache_size, line_bytes, icache_associativity, icache_max_search_depth)),
   cache_size(size), line_size(line_bytes), associativity(assoc), 
   mutation_interval(mutate_interval),
   dcache_total_accesses(0), dcache_total_misses(0),
   icache_total_accesses(0), icache_total_misses(0),
   total_resize_evictions(0),
   last_dcache_misses(0), last_icache_misses(0), name(name)
{   
   ASSERTX( (size & 1) == 0 );
   ASSERTX( (assoc & 1) == 0 );
   ASSERTX( (dcache_size + icache_size) == size );
   ASSERTX( (dcache_associativity + icache_associativity) == assoc );

   resetIntervalCounters();

   dcache_profile.SetKeyName("d_addr          ");
   dcache_profile.SetCounterName("dcache:miss        dcache:hit");

   icache_profile.SetKeyName("i_addr          ");
   icache_profile.SetCounterName("icache:miss        icache:hit");

   CounterArray dcache_threshold;
   dcache_threshold[k_COUNTER_HIT] = dcache_threshold_hit_value;
   dcache_threshold[k_COUNTER_MISS] = dcache_threshold_miss_value;
   dcache_profile.SetThreshold( dcache_threshold );

   CounterArray icache_threshold;
   icache_threshold[k_COUNTER_HIT] = icache_threshold_hit_value;
   icache_threshold[k_COUNTER_MISS] = icache_threshold_miss_value;
   icache_profile.SetThreshold( icache_threshold );
}


// miscellaneous

string OCache::statsLong()
{
   ostringstream out;
   out << dec
       << name << ":" << endl
       << " cacheSize            = " << cache_size << endl
       << " associativity        = " << associativity << endl
       << " lineSize             = " << line_size << endl
       << " mutationInterval     = " << mutation_interval << endl 
       << " totalResizeEvictions = " << total_resize_evictions << endl 
       << endl
       << "  dcache:" << endl
       << "   cacheSize     = " << dl1->getCacheSize() << endl
       << "   associativity = " << dl1->getNumWays() << endl
       << "   searchDepth   = " << dl1->getSearchDepth() << endl
       << "   lineSize      = " << dl1->getLineSize() << endl
       << "   dcacheTotalMisses      = " << dcache_total_misses << endl
       << "   dcacheIntervalMisses   = " << dcache_misses << endl
       << "   dcacheTotalAccesses    = " << dcache_total_accesses << endl
       << "   dcacheIntervalAccesses = " << dcacheAccesses << endl
       << endl
       << "  icache:" << endl
       << "   cacheSize     = " << il1->getCacheSize() << endl
       << "   associativity = " << il1->getNumWays() << endl
       << "   searchDepth   = " << il1->getSearchDepth() << endl
       << "   lineSize      = " << il1->getLineSize() << endl
       << "   icacheTotalMisses      = " << icache_total_misses << endl
       << "   icacheIntervalMisses   = " << icache_misses << endl
       << "   icacheTotalAccesses    = " << icache_total_accesses << endl
       << "   icacheIntervalAccesses = " << icache_accesses << endl;

   return out.str();
} 

void OCache::fini(int code, VOID *v, ofstream& out)
{

   // print D-cache profile
   // @todo what does this print
   // out << "PIN:MEMLATENCIES 1.0. 0x0" << endl;

   out << "# DCACHE configuration ["
       << "c = " << dCacheSize() / 1024 << "KB, "
       << "b = " << dCacheLineSize() << "B, "
       << "a = " << dCacheAssociativity() << "]" << endl;

   out << "#" << endl
       << "# DCACHE stats" << endl 
       << "#" << endl;
    
   out << dCacheStatsLong("# ", CacheBase::k_CACHE_TYPE_DCACHE);

   if( g_knob_dcache_track_loads || g_knob_dcache_track_stores ) {
      out << "#" << endl 
          << "# LOAD stats" << endl 
          << "#" << endl;
        
      out << dcache_profile.StringLong();
   }

   out << endl << endl;

   // print I-cache profile
   // @todo what does this print
   // out << "PIN:MEMLATENCIES 1.0. 0x0" << endl;

   out << "# ICACHE configuration ["
       << "c = " << iCacheSize() / 1024 << "KB, "
       << "b = " << iCacheLineSize() << "B, "
       << "a = " << iCacheAssociativity() << "]" << endl;
    
   out << "#" << endl 
       << "# ICACHE stats" << endl 
       << "#" << endl;

   out << iCacheStatsLong("# ", CacheBase::k_CACHE_TYPE_ICACHE);

   if (g_knob_icache_track_insts) {
      out << "#" << endl 
          << "# INST stats" << endl 
          << "#" << endl;
        
      out << icache_profile.StringLong();
   }

}

