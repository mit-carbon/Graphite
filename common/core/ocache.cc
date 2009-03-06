#include "ocache.h"
#include "cache.h"
#include "memory_manager.h"
#include "core.h"
#include "assert.h"
#include "simulator.h"

#include "log.h"

/* ===================================================================== */
/* Externally defined variables */
/* ===================================================================== */


/* =================================================== */
/* OCache method definitions */
/* =================================================== */

OCache::OCache(std::string name, Core *core)
      :
      m_knob_dcache_track_loads(Sim()->getCfg()->GetBool("ocache/track_dcache_loads")),
      m_knob_dcache_track_stores(Sim()->getCfg()->GetBool("ocache/track_dcache_stores")),
      m_knob_icache_track_insts(Sim()->getCfg()->GetBool("ocache/icache_tracK_insts")),
      m_knob_cache_size(Sim()->getCfg()->GetInt("ocache/cache_size")),
      m_knob_line_size(Sim()->getCfg()->GetInt("ocache/line_size")),
      m_knob_associativity(Sim()->getCfg()->GetInt("ocache/associativity")),
      m_knob_mutation_interval(Sim()->getCfg()->GetInt("ocache/mutation_interval")),
      m_knob_dcache_threshold_hit(Sim()->getCfg()->GetInt("ocache/dcache_threshold_hit")),
      m_knob_dcache_threshold_miss(Sim()->getCfg()->GetInt("ocache/dcache_threshold_miss")),
      m_knob_dcache_size(Sim()->getCfg()->GetInt("ocache/dcache_size")),
      m_knob_dcache_associativity(Sim()->getCfg()->GetInt("ocache/dcache_associativity")),
      m_knob_dcache_max_search_depth(Sim()->getCfg()->GetInt("ocache/dcache_max_search_depth")),
      m_knob_icache_threshold_hit(Sim()->getCfg()->GetInt("ocache/icache_threshold_hit")),
      m_knob_icache_threshold_miss(Sim()->getCfg()->GetInt("ocache/icache_threshold_miss")),
      m_knob_icache_size(Sim()->getCfg()->GetInt("ocache/icache_size")),
      m_knob_icache_associativity(Sim()->getCfg()->GetInt("ocache/icache_associativity")),
      m_knob_icache_max_search_depth(Sim()->getCfg()->GetInt("ocache/icache_max_search_depth")),
      m_dl1(new RRSACache(name + "_dl1", m_knob_dcache_size * k_KILO, m_knob_line_size, m_knob_dcache_associativity, m_knob_dcache_max_search_depth)),
      m_il1(new RRSACache(name + "_il1", m_knob_icache_size * k_KILO, m_knob_line_size, m_knob_icache_associativity, m_knob_icache_max_search_depth)),
      m_cache_size(m_knob_cache_size * k_KILO),
      m_line_size(m_knob_line_size),
      m_associativity(m_knob_associativity),
      m_mutation_interval(m_knob_mutation_interval),
      m_dcache_total_accesses(0), m_dcache_total_misses(0),
      m_icache_total_accesses(0), m_icache_total_misses(0),
      m_total_resize_evictions(0),
      m_last_dcache_misses(0), m_last_icache_misses(0), m_name(name), m_core(core)
{

   // limitations due to RRSACache typedef RoundRobin set template parameters
   assert(m_associativity <= 16);
   assert(m_line_size <= 128);
   assert(m_dl1->getNumSets() <= 1024 && m_il1->getNumSets() <= 1024);

   assert((m_cache_size & 1) == 0);
   assert((m_associativity & 1) == 0);
   assert(((m_knob_dcache_size * k_KILO) + (m_knob_icache_size * k_KILO)) == m_cache_size);
   assert((m_knob_dcache_associativity + m_knob_icache_associativity) == m_associativity);

   resetIntervalCounters();

   dcache_profile.SetKeyName("d_addr          ");
   dcache_profile.SetCounterName("dcache:miss        dcache:hit");

   icache_profile.SetKeyName("i_addr          ");
   icache_profile.SetCounterName("icache:miss        icache:hit");

   CounterArray dcache_threshold;
   dcache_threshold[k_COUNTER_HIT] = m_knob_dcache_threshold_hit;
   dcache_threshold[k_COUNTER_MISS] = m_knob_dcache_threshold_miss;
   dcache_profile.SetThreshold(dcache_threshold);

   CounterArray icache_threshold;
   icache_threshold[k_COUNTER_HIT] = m_knob_icache_threshold_hit;
   icache_threshold[k_COUNTER_MISS] = m_knob_icache_threshold_hit;
   icache_profile.SetThreshold(icache_threshold);
}

// cache evolution related

void OCache::evolveNaive()
{
   // gives more associativity (and thus cachesize) to the cache with more misses
   if (m_dcache_misses > (m_icache_misses * 1))
   {
      //cout << dec << "dcacheMisses = " << dcache_misses << " icacheMisses = "
      //     << m_icache_misses << endl;
      if (m_il1->getNumWays() >= 2)
      {
         m_il1->resize(m_il1->getNumWays() - 1);
         m_dl1->resize(m_dl1->getNumWays() + 1);
         //assume we don't have self-modifying code so no need to flush
      }
   }
   else
   {
      if (m_icache_misses > (m_dcache_misses * 1))
      {
         //cout << dec << "dcacheMisses = " << m_dcache_misses << " icacheMisses = "
         //     << m_icache_misses << endl;
         if (m_dl1->getNumWays() >= 2)
         {
            m_dl1->resize(m_dl1->getNumWays() - 1);
            m_il1->resize(m_il1->getNumWays() + 1);
            m_total_resize_evictions += 1 * m_dl1->getNumSets();
         }
      }
   }
}

void OCache::evolveDataIntensive()
{
   //shrink icache so long as shrinking saves more dcache misses than icache misses it adds
   if ((m_last_dcache_misses == 0 && m_last_icache_misses == 0))
   {
      // initial action
      if (m_il1->getNumWays() >= 2)
      {
         m_il1->resize(m_il1->getNumWays() - 1);
         m_dl1->resize(m_dl1->getNumWays() + 1);
      }
   }
   else
   {
      if ((m_dcache_misses + m_icache_misses) < (m_last_dcache_misses + m_last_icache_misses))
      {
         //we got improvement
         if (m_il1->getNumWays() >= 2)
         {
            m_il1->resize(m_il1->getNumWays() - 1);
            m_dl1->resize(m_dl1->getNumWays() + 1);
         }
      }
   }

   m_last_dcache_misses = m_dcache_misses;
   m_last_icache_misses = m_icache_misses;
}

void OCache::mutationRuntime()
{
   //if ( m_mutation_interval && ((m_icache_accesses + m_dcacheAccesses) >= m_mutation_interval) )
   //cout << dec << m_icache_misses << " " << dcache_misses << " " << endl;

   if (m_mutation_interval && ((m_icache_misses + m_dcache_misses) >= m_mutation_interval))
   {
//       cout << "Mutation Interval Elapsed" << endl
//            << "config before mutation:" << endl
//            << statsLong() << endl;
      evolveNaive();
      //evolveDataIntensive();

      resetIntervalCounters();
   }
   else
   {
      if (m_mutation_interval==0 && ((m_icache_misses + m_dcache_misses) >= 1000))
      {
//          cout << "Mutation Interval Elapsed" << endl
//               << "config before mutation:" << endl
//               << statsLong() << endl;
         resetIntervalCounters();
      }
   }
}


// cache access related
pair<bool, CacheTag*> OCache::dCacheLoadSingle(IntPtr addr, UInt32 inst_id)
{
   // @todo we may access several cache lines for
   // first level D-cache
   pair<bool, CacheTag*> res = m_dl1->accessSingleLine(addr, CacheBase::k_ACCESS_TYPE_LOAD);
   const bool m_dl1_hit = res.first;
   const Counter counter = m_dl1_hit ? k_COUNTER_HIT : k_COUNTER_MISS;

   dcache_profile[inst_id][counter]++;
   m_dcacheAccesses++;
   m_dcache_total_accesses++;

   if (!m_dl1_hit)
   {
      m_dcache_misses++;
      m_dcache_total_misses++;
   }
   mutationRuntime();

   return res;
}

pair<bool, CacheTag*> OCache::dCacheLoadSingleFast(IntPtr addr)
{
   pair<bool, CacheTag*> res = m_dl1->accessSingleLine(addr, CacheBase::k_ACCESS_TYPE_LOAD);
   const bool m_dl1_hit = res.first;

   m_dcacheAccesses++;
   m_dcache_total_accesses++;

   if (!m_dl1_hit)
   {
      m_dcache_misses++;
      m_dcache_total_misses++;
   }
   mutationRuntime();

   return res;
}

pair<bool, CacheTag*> OCache::dCacheLoadMultiFast(IntPtr addr, UInt32 size)
{
   //NOTE: only returns pointer to cachetag of first line spanned
   bool hit = true;
   CacheTag* tag = NULL;

   for (IntPtr a = addr; a < addr + size; a += m_line_size)
   {
      pair<bool, CacheTag*> res = dCacheLoadSingleFast(a);
      if (hit && !res.first)
         hit = false;
      if (a == addr)
         tag = res.second;
   }

   return make_pair(hit, tag);
}

pair<bool, CacheTag*> OCache::dCacheStoreSingle(IntPtr addr, UInt32 inst_id)
{
   // @todo we may access several cache lines for
   // first level D-cache; we only model stores to dcache
   pair<bool, CacheTag*> res = m_dl1->accessSingleLine(addr, CacheBase::k_ACCESS_TYPE_STORE);
   const bool m_dl1_hit = res.first;
   const Counter counter = m_dl1_hit ? k_COUNTER_HIT : k_COUNTER_MISS;

   dcache_profile[inst_id][counter]++;
   m_dcacheAccesses++;
   m_dcache_total_accesses++;

   if (!m_dl1_hit)
   {
      m_dcache_misses++;
      m_dcache_total_misses++;
   }
   mutationRuntime();

   return res;
}

pair<bool, CacheTag*> OCache::dCacheStoreSingleFast(IntPtr addr)
{
   // we only model stores for dcache
   pair<bool, CacheTag*> res = m_dl1->accessSingleLine(addr, CacheBase::k_ACCESS_TYPE_STORE);
   const bool m_dl1_hit = res.first;

   m_dcacheAccesses++;
   m_dcache_total_accesses++;

   if (!m_dl1_hit)
   {
      m_dcache_misses++;
      m_dcache_total_misses++;
   }
   mutationRuntime();

   return res;
}

pair<bool, CacheTag*> OCache::dCacheStoreMultiFast(IntPtr addr, UInt32 size)
{
   //NOTE: only returns pointer to cachetag of first line spanned
   bool hit = true;
   CacheTag *tag = NULL;

   for (IntPtr a = addr; a < addr + size; a += m_line_size)
   {
      pair<bool, CacheTag*> res = dCacheStoreSingleFast(a);
      if (hit && !res.first)
         hit = false;
      if (a == addr)
         tag = res.second;
   }

   return make_pair(hit, tag);
}


pair<bool, CacheTag*> OCache::iCacheLoadSingle(IntPtr addr, UInt32 inst_id)
{
   // @todo we may access several cache lines for
   // first level I-cache
   pair<bool, CacheTag*> res = m_il1->accessSingleLine(addr, CacheBase::k_ACCESS_TYPE_LOAD);
   const bool m_il1_hit = res.first;
   const Counter counter = m_il1_hit ? k_COUNTER_HIT : k_COUNTER_MISS;

   icache_profile[inst_id][counter]++;
   m_icache_accesses++;
   m_icache_total_accesses++;

   if (!m_il1_hit)
   {
      m_icache_misses++;
      m_icache_total_misses++;
   }
   mutationRuntime();

   return res;
}

pair<bool, CacheTag*> OCache::iCacheLoadSingleFast(IntPtr addr)
{
   pair<bool, CacheTag*> res = m_il1->accessSingleLine(addr, CacheBase::k_ACCESS_TYPE_LOAD);
   const bool m_il1_hit = res.first;

   m_icache_accesses++;
   m_icache_total_accesses++;

   if (!m_il1_hit)
   {
      m_icache_misses++;
      m_icache_total_misses++;
   }
   mutationRuntime();

   return res;
}

pair<bool, CacheTag*> OCache::iCacheLoadMultiFast(IntPtr addr, UInt32 size)
{
   //NOTE: only returns pointer to cachetag of first line spanned
   bool hit = true;
   CacheTag *tag = NULL;

   for (IntPtr a = addr; a < addr + size; a += m_line_size)
   {
      pair<bool, CacheTag*> res = iCacheLoadSingleFast(a);
      if (hit && !res.first)
         hit = false;
      if (a == addr)
         tag = res.second;
   }

   return make_pair(hit, tag);
}


pair<bool, CacheTag*> OCache::runICacheLoadModel(IntPtr i_addr, UInt32 size)
{
   UInt32 a1 = (UInt32) i_addr;
   UInt32 a2 = ((UInt32) i_addr) + size - 1;

   if ((a1/m_line_size) == (a2/m_line_size))
      return iCacheLoadSingleFast(i_addr);
   else
      return iCacheLoadMultiFast(i_addr, size);
}

pair<bool, CacheTag*> OCache::runDCacheLoadModel(IntPtr d_addr, UInt32 size)
{
   UInt32 a1 = (UInt32) d_addr;
   UInt32 a2 = ((UInt32) d_addr) + size - 1;

   if ((a1/m_line_size) == (a2/m_line_size))
      return dCacheLoadSingleFast(d_addr);
   else
      return dCacheLoadMultiFast(d_addr, size);
}

pair<bool, CacheTag*> OCache::runDCacheStoreModel(IntPtr d_addr, UInt32 size)
{
   UInt32 a1 = (UInt32) d_addr;
   UInt32 a2 = ((UInt32) d_addr) + size - 1;

   if ((a1/m_line_size) == (a2/m_line_size))
      return dCacheStoreSingleFast(d_addr);
   else
      return dCacheStoreMultiFast(d_addr, size);
}

pair<bool, CacheTag*> OCache::runICachePeekModel(IntPtr i_addr)
{
   return m_il1->accessSingleLinePeek(i_addr);
}

pair<bool, CacheTag*> OCache::runDCachePeekModel(IntPtr d_addr)
{
   return m_dl1->accessSingleLinePeek(d_addr);
}




// miscellaneous

string OCache::statsLong()
{
   ostringstream out;
   out << dec
   << m_name << ":" << endl
   << "  cacheSize            = " << m_cache_size << endl
   << "  associativity        = " << m_associativity << endl
   << "  lineSize             = " << m_line_size << endl
   << "  mutationInterval     = " << m_mutation_interval << endl
   << "  totalResizeEvictions = " << m_total_resize_evictions << endl

   << "  dcache:" << endl
   << "    cacheSize     = " << m_dl1->getCacheSize() << endl
   << "    associativity = " << m_dl1->getNumWays() << endl
   << "    searchDepth   = " << m_dl1->getSearchDepth() << endl
   << "    lineSize      = " << m_dl1->getLineSize() << endl
   << "    dcacheTotalMisses      = " << m_dcache_total_misses << endl
   << "    dcacheIntervalMisses   = " << m_dcache_misses << endl
   << "    dcacheTotalAccesses    = " << m_dcache_total_accesses << endl
   << "    dcacheIntervalAccesses = " << m_dcacheAccesses << endl

   << "  icache:" << endl
   << "    cacheSize     = " << m_il1->getCacheSize() << endl
   << "    associativity = " << m_il1->getNumWays() << endl
   << "    searchDepth   = " << m_il1->getSearchDepth() << endl
   << "    lineSize      = " << m_il1->getLineSize() << endl
   << "    icacheTotalMisses      = " << m_icache_total_misses << endl
   << "    icacheIntervalMisses   = " << m_icache_misses << endl
   << "    icacheTotalAccesses    = " << m_icache_total_accesses << endl
   << "    icacheIntervalAccesses = " << m_icache_accesses << endl;

   return out.str();
}

void OCache::outputSummary(ostream& out)
{
   // this output is ugly as fuck so I'm just going to use the
   // statsLong from above
   out << statsLong();
   return;

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

   if (m_knob_dcache_track_loads || m_knob_dcache_track_stores)
   {
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

   if (m_knob_icache_track_insts)
   {
      out << "#" << endl
      << "# INST stats" << endl
      << "#" << endl;

      out << icache_profile.StringLong();
   }

}


