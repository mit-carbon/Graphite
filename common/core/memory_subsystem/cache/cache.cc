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
      UInt32 cache_data_access_time,
      UInt32 cache_tags_access_time,
      string cache_perf_model_type,
      ShmemPerfModel* shmem_perf_model) :

      CacheBase(name, cache_size, associativity, cache_block_size),
      m_enabled(false),
      m_num_accesses(0),
      m_num_hits(0),
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
}

Cache::~Cache()
{
   for (SInt32 i = 0; i < (SInt32) m_num_sets; i++)
      delete m_sets[i];
   delete [] m_sets;

   if (m_cache_perf_model)
      delete m_cache_perf_model;
}

bool 
Cache::invalidateSingleLine(IntPtr addr)
{
   IntPtr tag;
   UInt32 set_index;

   splitAddress(addr, tag, set_index);
   assert(set_index < m_num_sets);

   return m_sets[set_index]->invalidate(tag);
}

CacheBlockInfo* 
Cache::accessSingleLine(IntPtr addr, access_t access_type, 
      Byte* buff, UInt32 bytes)
{
   if (m_cache_perf_model)
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
   if (m_cache_perf_model)
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
}


// Single line cache access at addr
CacheBlockInfo* 
Cache::peekSingleLine(IntPtr addr)
{
   if (m_cache_perf_model)
      getShmemPerfModel()->incrCycleCount(getCachePerfModel()->getLatency(CachePerfModel::ACCESS_CACHE_TAGS));

   IntPtr tag;
   UInt32 set_index;
   splitAddress(addr, tag, set_index);

   return m_sets[set_index]->find(tag);
}

void
Cache::updateCounters(bool cache_hit)
{
   if (m_enabled)
   {
      m_num_accesses ++;
      if (cache_hit)
         m_num_hits ++;
   }
}

void 
Cache::outputSummary(ostream& out)
{
   out << "  Cache " << m_name << ":\n";
   out << "    num cache accesses: " << m_num_accesses << endl;
   out << "    miss rate: " <<
      ((float) (m_num_accesses - m_num_hits) / m_num_accesses) * 100 << endl;
   out << "    num cache misses: " << m_num_accesses - m_num_hits << endl;
}
