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
      UInt32 access_delay,
      volatile float frequency) :
      
   CacheBase(name, cache_size, associativity, cache_block_size),
   m_enabled(false),
   m_cache_type(cache_type),
   m_power_model(NULL),
   m_area_model(NULL)
{
   m_sets = new CacheSet*[m_num_sets];
   for (UInt32 i = 0; i < m_num_sets; i++)
   {
      m_sets[i] = CacheSet::createCacheSet(replacement_policy, m_cache_type, m_associativity, m_blocksize);
   }

   if (Config::getSingleton()->getEnablePowerModeling())
   {
      // Instantiate area and power models
      m_power_model = new CachePowerModel("data", k_KILO * cache_size, cache_block_size,
            associativity, access_delay, frequency);
      m_area_model = new CacheAreaModel("data", k_KILO * cache_size, cache_block_size,
            associativity, access_delay, frequency);
   }

   // Initialize Cache Counters
   initializePerformanceCounters();
}

Cache::~Cache()
{
   for (SInt32 i = 0; i < (SInt32) m_num_sets; i++)
      delete m_sets[i];
   delete [] m_sets;
}

bool 
Cache::invalidateSingleLine(IntPtr addr)
{
   IntPtr tag;
   UInt32 set_index;

   splitAddress(addr, tag, set_index);
   assert(set_index < m_num_sets);

   // FIXME: Need to update power model here but dont have the numbers

   return m_sets[set_index]->invalidate(tag);
}

CacheBlockInfo* 
Cache::accessSingleLine(IntPtr addr, access_t access_type, 
      Byte* buff, UInt32 bytes)
{
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

   if (Config::getSingleton()->getEnablePowerModeling())
   {
      // Update Dynamic Energy Counters
      m_power_model->updateDynamicEnergy();
   }

   return cache_block_info;
}

void 
Cache::insertSingleLine(IntPtr addr, Byte* fill_buff,
      bool* eviction, IntPtr* evict_addr, 
      CacheBlockInfo* evict_block_info, Byte* evict_buff)
{
   IntPtr tag;
   UInt32 set_index;
   splitAddress(addr, tag, set_index);

   CacheBlockInfo* cache_block_info = CacheBlockInfo::create(m_cache_type);
   cache_block_info->setTag(tag);

   m_sets[set_index]->insert(cache_block_info, fill_buff, 
         eviction, evict_block_info, evict_buff);
   *evict_addr = tagToAddress(evict_block_info->getTag());

   if (Config::getSingleton()->getEnablePowerModeling())
   {
      // Update Dynamic Energy Counters
      m_power_model->updateDynamicEnergy();
   }
   
   delete cache_block_info;
}


// Single line cache access at addr
CacheBlockInfo* 
Cache::peekSingleLine(IntPtr addr)
{
   IntPtr tag;
   UInt32 set_index;
   splitAddress(addr, tag, set_index);

   // FIXME: Need to update power model here but dont have the numbers

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
Cache::reset()
{
   initializePerformanceCounters();
}

void
Cache::initializePerformanceCounters()
{
   m_num_accesses = 0;
   m_num_hits = 0;
}

void 
Cache::outputSummary(ostream& out)
{
   out << "  Cache " << m_name << ":\n";
   out << "    num cache accesses: " << m_num_accesses << endl;
   out << "    miss rate: " <<
      ((float) (m_num_accesses - m_num_hits) / m_num_accesses) * 100 << endl;
   out << "    num cache misses: " << m_num_accesses - m_num_hits << endl;
  
   if (Config::getSingleton()->getEnablePowerModeling())
   { 
      // Output Power and Area Summaries
      m_power_model->outputSummary(out);
      m_area_model->outputSummary(out);
   }
}
