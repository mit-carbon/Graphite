#include "ocache.h"
#include "cache.h"
#include "memory_manager.h"
#include "core.h"

#include "pin.H"
#include "log.h"

/* ===================================================================== */
/* Externally defined variables */
/* ===================================================================== */

extern LEVEL_BASE::KNOB<bool> g_knob_icache_ignore_size;
extern LEVEL_BASE::KNOB<bool> g_knob_dcache_ignore_size;
extern LEVEL_BASE::KNOB<bool> g_knob_dcache_track_loads;
extern LEVEL_BASE::KNOB<bool> g_knob_dcache_track_stores;
extern LEVEL_BASE::KNOB<bool> g_knob_icache_track_insts;

extern LEVEL_BASE::KNOB<UInt32> g_knob_cache_size;
extern LEVEL_BASE::KNOB<UInt32> g_knob_line_size;
extern LEVEL_BASE::KNOB<UInt32> g_knob_associativity;
extern LEVEL_BASE::KNOB<UInt32> g_knob_mutation_interval;
extern LEVEL_BASE::KNOB<UInt32> g_knob_dcache_threshold_hit;
extern LEVEL_BASE::KNOB<UInt32> g_knob_dcache_threshold_miss;
extern LEVEL_BASE::KNOB<UInt32> g_knob_dcache_size;
extern LEVEL_BASE::KNOB<UInt32> g_knob_dcache_associativity;
extern LEVEL_BASE::KNOB<UInt32> g_knob_dcache_max_search_depth;
extern LEVEL_BASE::KNOB<UInt32> g_knob_icache_threshold_hit;
extern LEVEL_BASE::KNOB<UInt32> g_knob_icache_threshold_miss;
extern LEVEL_BASE::KNOB<UInt32> g_knob_icache_size;
extern LEVEL_BASE::KNOB<UInt32> g_knob_icache_associativity;
extern LEVEL_BASE::KNOB<UInt32> g_knob_icache_max_search_depth;

/* =================================================== */
/* OCache method definitions */
/* =================================================== */

OCache::OCache(std::string name, Core *core)
      :
      m_dl1(new RRSACache(name + "_dl1", g_knob_dcache_size.Value() * k_KILO, g_knob_line_size.Value(), g_knob_dcache_associativity.Value(), g_knob_dcache_max_search_depth.Value())),
      m_il1(new RRSACache(name + "_il1", g_knob_icache_size.Value() * k_KILO, g_knob_line_size.Value(), g_knob_icache_associativity.Value(), g_knob_icache_max_search_depth.Value())),
      m_cache_size(g_knob_cache_size.Value() * k_KILO),
      m_line_size(g_knob_line_size.Value()),
      m_associativity(g_knob_associativity.Value()),
      m_mutation_interval(g_knob_mutation_interval.Value()),
      m_dcache_total_accesses(0), m_dcache_total_misses(0),
      m_icache_total_accesses(0), m_icache_total_misses(0),
      m_total_resize_evictions(0),
      m_last_dcache_misses(0), m_last_icache_misses(0), m_name(name), m_core(core)
{

   // limitations due to RRSACache typedef RoundRobin set template parameters
   ASSERTX(m_associativity <= 16);
   ASSERTX(m_line_size <= 128);
   ASSERTX(m_dl1->getNumSets() <= 1024 && m_il1->getNumSets() <= 1024);

   ASSERTX((m_cache_size & 1) == 0);
   ASSERTX((m_associativity & 1) == 0);
   ASSERTX(((g_knob_dcache_size.Value() * k_KILO) + (g_knob_icache_size.Value() * k_KILO)) == m_cache_size);
   ASSERTX((g_knob_dcache_associativity.Value() + g_knob_icache_associativity.Value()) == m_associativity);

   resetIntervalCounters();

   dcache_profile.SetKeyName("d_addr          ");
   dcache_profile.SetCounterName("dcache:miss        dcache:hit");

   icache_profile.SetKeyName("i_addr          ");
   icache_profile.SetCounterName("icache:miss        icache:hit");

   CounterArray dcache_threshold;
   dcache_threshold[k_COUNTER_HIT] = g_knob_dcache_threshold_hit.Value();
   dcache_threshold[k_COUNTER_MISS] = g_knob_dcache_threshold_miss.Value();
   dcache_profile.SetThreshold(dcache_threshold);

   CounterArray icache_threshold;
   icache_threshold[k_COUNTER_HIT] = g_knob_icache_threshold_hit.Value();
   icache_threshold[k_COUNTER_MISS] = g_knob_icache_threshold_hit.Value();
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

   if (g_knob_dcache_track_loads || g_knob_dcache_track_stores)
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

   if (g_knob_icache_track_insts)
   {
      out << "#" << endl
      << "# INST stats" << endl
      << "#" << endl;

      out << icache_profile.StringLong();
   }

}

/*
 * dcacheRunModel (mem_operation_t operation, IntPtr d_addr, char* data_buffer, UInt32 data_size)
 *
 * Arguments:
 *   d_addr :: address of location we want to access (read or write)
 *   shmem_req_t :: READ or WRITE
 *   data_buffer :: buffer holding data for WRITE or buffer which must be written on a READ
 *   data_size :: size of data we must read/write
 *
 * Return Value:
 *   hit :: Say whether there has been at least one cache hit or not
 */
bool OCache::runDCacheModel(CacheBase::AccessType operation, IntPtr d_addr, char* data_buffer, UInt32 data_size)
{
   shmem_req_t shmem_operation;

   if (operation == CacheBase::k_ACCESS_TYPE_LOAD)
   {
      shmem_operation = READ;
   }
   else
   {
      shmem_operation = WRITE;
   }

   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      LOG_PRINT("%s - ADDR: 0x%x, data_size: %u, END!!", ((operation==CacheBase::k_ACCESS_TYPE_LOAD) ? " READ " : " WRITE "), d_addr, data_size);

      bool all_hits = true;

      if (data_size <= 0)
      {
         return (true);
         // TODO: this is going to affect the statistics even though no shared_mem action is taking place
      }

      IntPtr begin_addr = d_addr;
      IntPtr end_addr = d_addr + data_size;
      IntPtr begin_addr_aligned = begin_addr - (begin_addr % dCacheLineSize());
      IntPtr end_addr_aligned = end_addr - (end_addr % dCacheLineSize());
      char *curr_data_buffer_head = data_buffer;

      //TODO set the size parameter correctly, based on the size of the data buffer
      //TODO does this spill over to another line? should shared_mem test look at other DRAM entries?
      for (IntPtr curr_addr_aligned = begin_addr_aligned ; curr_addr_aligned <= end_addr_aligned /* Note <= */; curr_addr_aligned += dCacheLineSize())
      {
         // Access the cache one line at a time
         UInt32 curr_offset;
         UInt32 curr_size;

         // Determine the offset
         // TODO fix curr_size calculations
         // FIXME: Check if all this is correct
         if (curr_addr_aligned == begin_addr_aligned)
         {
            curr_offset = begin_addr % dCacheLineSize();
         }
         else
         {
            curr_offset = 0;
         }

         // Determine the size
         if (curr_addr_aligned == end_addr_aligned)
         {
            curr_size = (end_addr % dCacheLineSize()) - (curr_offset);
            if (curr_size == 0)
            {
               continue;
            }
         }
         else
         {
            curr_size = dCacheLineSize() - (curr_offset);
         }

         LOG_PRINT("Start InitiateSharedMemReq: ADDR: %x, offset: %u, curr_size: %u", curr_addr_aligned, curr_offset, curr_size);

         if (!m_core->getMemoryManager()->initiateSharedMemReq(shmem_operation, curr_addr_aligned, curr_offset, curr_data_buffer_head, curr_size))
         {
            // If it is a LOAD operation, 'initiateSharedMemReq' causes curr_data_buffer_head to be automatically filled in
            // If it is a STORE operation, 'initiateSharedMemReq' reads the data from curr_data_buffer_head
            all_hits = false;
         }

         LOG_PRINT("End InitiateSharedMemReq: ADDR: %x, offset: %u, curr_size: %u", curr_addr_aligned, curr_offset, curr_size);

         // Increment the buffer head
         curr_data_buffer_head += curr_size;
      }

      LOG_PRINT("%s - ADDR: %x, data_size: %u, END!!", ((operation==CacheBase::k_ACCESS_TYPE_LOAD) ? " READ " : " WRITE "), d_addr, data_size);

      return all_hits;

   }
   else
   {
      // run this if we aren't using shared_memory
      // FIXME: I am not sure this is right
      // What if the initial data for this address is in some other core's DRAM (which is on some other host machine)
      if (operation == CacheBase::k_ACCESS_TYPE_LOAD)
         return runDCacheLoadModel(d_addr, data_size).first;
      else
         return runDCacheStoreModel(d_addr, data_size).first;
   }
}

