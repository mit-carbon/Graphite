#include "simulator.h"
#include "cache.h"
#include "log.h"

using namespace std;

// Cache class
// constructors/destructors
Cache::Cache(string name, 
      UInt32 cache_size,
      UInt32 associativity, UInt32 cache_block_size,
      string replacement_policy,
      cache_t cache_type,
      bool track_detailed_cache_counters,
      UInt32 cache_data_access_time,
      UInt32 cache_tags_access_time,
      string cache_perf_model_type,
      ShmemPerfModel* shmem_perf_model) :

      CacheBase(name, cache_size, associativity, cache_block_size),
      m_num_accesses(0),
      m_num_hits(0),
      m_num_cold_misses(0),
      m_num_capacity_misses(0),
      m_num_upgrade_misses(0),
      m_num_sharing_misses(0),
      m_track_detailed_cache_counters(track_detailed_cache_counters),
      m_cache_counters_enabled(false),
      m_cache_type(cache_type),
      m_shmem_perf_model(shmem_perf_model),
      m_cache_perf_model(NULL)
{
   m_sets = new CacheSet*[m_num_sets];
   for (UInt32 i = 0; i < m_num_sets; i++)
   {
      m_sets[i] = CacheSet::createCacheSet(replacement_policy, m_cache_type, m_associativity, m_blocksize);
   }

   // Cache Perf Model
   if (Config::getSingleton()->getEnablePerformanceModeling())
   {
      m_cache_perf_model = CachePerfModel::create(cache_perf_model_type, 
            cache_data_access_time, cache_tags_access_time);
   }

   // Detailed Cache Counters
   if (m_track_detailed_cache_counters)
   {
      m_invalidated_set = new HashMapSet<IntPtr>(m_num_sets, &moduloHashFn<IntPtr>, m_log_blocksize);
      m_evicted_set = new HashMapSet<IntPtr>(m_num_sets, &moduloHashFn<IntPtr>, m_log_blocksize);
   }
}

Cache::~Cache()
{
   for (SInt32 i = 0; i < (SInt32) m_num_sets; i++)
      delete m_sets[i];
   delete [] m_sets;

   if (m_cache_perf_model)
      delete m_cache_perf_model;

   if (m_track_detailed_cache_counters)
   {
      delete m_invalidated_set;
      delete m_evicted_set;
   }
}

bool 
Cache::invalidateSingleLine(IntPtr addr)
{
   IntPtr tag;
   UInt32 set_index;

   splitAddress(addr, tag, set_index);
   assert(set_index < m_num_sets);

   // Update sets for maintaining cache counters
   if (m_track_detailed_cache_counters 
         && m_shmem_perf_model 
         && m_shmem_perf_model->isEnabled())
      m_invalidated_set->insert(addr);

   return m_sets[set_index]->invalidate(tag);
}

CacheBlockInfo* 
Cache::accessSingleLine(IntPtr addr, access_t access_type, 
      Byte* buff, UInt32 bytes)
{
   if (m_shmem_perf_model && m_cache_perf_model)
      getShmemPerfModel()->incrCycleCount(getCachePerfModel()->getLatency(CachePerfModel::ACCESS_CACHE_DATA));

   assert((buff == NULL) == (bytes == 0));

   IntPtr tag;
   UInt32 set_index;
   UInt32 line_index = -1;
   UInt32 block_offset;

   splitAddress(addr, tag, set_index, block_offset);

   CacheSet* set = m_sets[set_index];
   CacheBlockInfo* cache_block_info = set->find(tag, &line_index);

   if (cache_block_info == NULL)
      return NULL;

   if (access_type == LOAD)
      set->read_line(line_index, block_offset, buff, bytes);
   else
      set->write_line(line_index, block_offset, buff, bytes);

   return cache_block_info;
}

void 
Cache::insertSingleLine(IntPtr addr, Byte* fill_buff,
      bool* eviction, IntPtr* evict_addr, 
      CacheBlockInfo* evict_block_info, Byte* evict_buff)
{
   if (m_shmem_perf_model && m_cache_perf_model)
      getShmemPerfModel()->incrCycleCount(getCachePerfModel()->getLatency(CachePerfModel::ACCESS_CACHE_DATA_AND_TAGS));

   IntPtr tag;
   UInt32 set_index;
   splitAddress(addr, tag, set_index);

   CacheBlockInfo* cache_block_info = CacheBlockInfo::create(m_cache_type);
   cache_block_info->setTag(tag);

   m_sets[set_index]->insert(cache_block_info, fill_buff, 
         eviction, evict_block_info, evict_buff);
   *evict_addr = tagToAddress(evict_block_info->getTag());

   delete cache_block_info;
  
   // Update sets for the purpose of maintaining cache counters
   if (m_track_detailed_cache_counters 
         && m_shmem_perf_model 
         && m_shmem_perf_model->isEnabled())
   {
      m_evicted_set->erase(addr);
      m_invalidated_set->erase(addr);

      if (*eviction)
         m_evicted_set->insert(*evict_addr);
   }
}


// Single line cache access at addr
CacheBlockInfo* 
Cache::peekSingleLine(IntPtr addr)
{
   if (m_shmem_perf_model && m_cache_perf_model)
      getShmemPerfModel()->incrCycleCount(getCachePerfModel()->getLatency(CachePerfModel::ACCESS_CACHE_TAGS));

   IntPtr tag;
   UInt32 set_index;
   splitAddress(addr, tag, set_index);

   return m_sets[set_index]->find(tag);
}

void
Cache::updateCounters(IntPtr addr, CacheState cache_state, access_t access_type)
{
   if (!m_cache_counters_enabled)
      return;

   if (access_type == LOAD)
   {
      incrNumAccesses();
      if (cache_state.readable())
         incrNumHits();
      else if (m_track_detailed_cache_counters)
      {
         if (isInvalidated(addr))
            incrNumSharingMisses();
         else if (isEvicted(addr))
            incrNumCapacityMisses();
         else
            incrNumColdMisses();
      }
   }
   else /* access_type == ACCESS_TYPE_STORE */
   {
      incrNumAccesses();
      if (cache_state.writable())
         incrNumHits();
      else if (m_track_detailed_cache_counters)
      {
         if (cache_state.readable())
            incrNumUpgradeMisses();
         else if (isInvalidated(addr))
            incrNumSharingMisses();
         else if (isEvicted(addr))
            incrNumCapacityMisses();
         else
            incrNumColdMisses();
      }
   }
}

void 
Cache::outputSummary(ostream& out)
{
   out << "Cache summary: " << endl;
   out << "    num cache accesses: " << m_num_accesses << endl;
   out << "    miss rate: " <<
      ((float) (m_num_accesses - m_num_hits) / m_num_accesses) * 100 << endl;
   out << "    num cache misses: " << m_num_accesses - m_num_hits << endl;
   
   if (m_track_detailed_cache_counters)
   {
      out << "    cold miss rate: " <<
         ((float) m_num_cold_misses / m_num_accesses) * 100 << endl;
      out << "    num cold misses: " << m_num_cold_misses << endl;

      out << "    capacity miss rate: " <<
         ((float) m_num_capacity_misses / m_num_accesses) * 100 << endl;
      out << "    num capacity misses: " << m_num_capacity_misses << endl;

      out << "    upgrade miss rate: " <<
         ((float) m_num_upgrade_misses / m_num_accesses) * 100 << endl;
      out << "    num upgrade misses: " << m_num_upgrade_misses << endl;

      out << "    sharing miss rate: " <<
         ((float) m_num_sharing_misses / m_num_accesses) * 100 << endl;
      out << "    num sharing misses: " << m_num_sharing_misses << endl;
   }
}
