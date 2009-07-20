#include "simulator.h"
#include "cache.h"
#include "log.h"

using namespace std;

CacheBase::CacheBase(string name)
   : m_name(name)
{
   try
   {
      m_cache_size = k_KILO * (Sim()->getCfg()->getInt("cache/size"));
      m_blocksize = Sim()->getCfg()->getInt("cache/line_size");
      m_associativity = Sim()->getCfg()->getInt("cache/associativity");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error while reading cache parameters from the config file");
   }
   m_num_sets = m_cache_size / (m_associativity * m_blocksize);
   m_log_blocksize = ceilLog2(m_blocksize);
}

CacheBase::~CacheBase() 
{}

CacheStats 
CacheBase::sumAccess(bool hit) const
{
   CacheStats sum = 0;
   for (UInt32 access_type = 0; access_type < NUM_ACCESS_TYPES; access_type++)
   {
      sum += m_access[access_type][hit];
   }
   return sum;
}

// stats
CacheStats 
CacheBase::getHits(AccessType access_type) const
{
   assert(access_type < NUM_ACCESS_TYPES);
   return m_access[access_type][true];
}

CacheStats 
CacheBase::getMisses(AccessType access_type) const
{
   assert(access_type < NUM_ACCESS_TYPES);
   return m_access[access_type][false];
}

CacheStats 
CacheBase::getAccesses(AccessType access_type) const
{
   return getHits(access_type) + getMisses(access_type); 
}

// utilities
IntPtr 
CacheBase::tagToAddress(IntPtr tag)
{
   return (tag << m_log_blocksize);
}

void 
CacheBase::splitAddress(const IntPtr addr, IntPtr& tag, UInt32& set_index) const
{
   tag = addr >> m_log_blocksize;
   set_index = tag & (m_num_sets-1);
}

void 
CacheBase::splitAddress(const IntPtr addr, IntPtr& tag, UInt32& set_index,
                  UInt32& block_offset) const
{
   block_offset = addr & (m_blocksize-1);
   splitAddress(addr, tag, set_index);
}


// Cache class
// constructors/destructors
Cache::Cache(string name, ShmemPerfModel* shmem_perf_model) :
      CacheBase(name),
      m_num_accesses(0),
      m_num_hits(0),
      m_num_cold_misses(0),
      m_num_capacity_misses(0),
      m_num_upgrade_misses(0),
      m_num_sharing_misses(0)
{
   CacheSetBase::ReplacementPolicy replacement_policy;
   try
   {
      replacement_policy = CacheSetBase::parsePolicyType(Sim()->getCfg()->getString("cache/replacement_policy"));
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Error reading cache replacement policy from the config file");
      return;
   }

   m_sets = new CacheSetBase*[m_num_sets];
   for (UInt32 i = 0; i < m_num_sets; i++)
   {
      m_sets[i] = CacheSetBase::createCacheSet(replacement_policy, m_associativity, m_blocksize);
   }

   // Performance Models
   CachePerfModelBase::CacheModel_t model_type;
   try
   {
      model_type = CachePerfModelBase::parseModelType(Sim()->getCfg()->getString("perf_model/l2_cache/model_type"));
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error reading perf_model/l2_cache/model_type from config file");
   }
   m_cache_perf_model = CachePerfModelBase::createModel(model_type);
   m_shmem_perf_model = shmem_perf_model;

   // Detailed Cache Counters
   try
   {
      m_track_detailed_cache_counters = Sim()->getCfg()->getBool("cache/track_detailed_cache_counters");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error reading cache/track_detailed_cache_counters from the config file");
   }
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
   m_shmem_perf_model->updateCycleCount(m_cache_perf_model->getLatency(CachePerfModelBase::ACCESS_CACHE_TAGS));

   IntPtr tag;
   UInt32 set_index;

   splitAddress(addr, tag, set_index);
   assert(set_index < m_num_sets);

   // Update sets for maintaining cache counters
   if (m_track_detailed_cache_counters && m_shmem_perf_model->isEnabled())
      invalidated_set.insert(addr);

   return m_sets[set_index]->invalidate(tag);
}

CacheBlockInfo* 
Cache::accessSingleLine(IntPtr addr, AccessType access_type, 
      Byte* buff, UInt32 bytes)
{
   m_shmem_perf_model->updateCycleCount(m_cache_perf_model->getLatency(CachePerfModelBase::ACCESS_CACHE_DATA_AND_TAGS));

   assert((buff == NULL) == (bytes == 0));

   IntPtr tag;
   UInt32 set_index;
   UInt32 line_index = -1;
   UInt32 block_offset;

   splitAddress(addr, tag, set_index, block_offset);

   CacheSetBase* set = m_sets[set_index];
   CacheBlockInfo* cache_block_info = set->find(tag, &line_index);

   if (cache_block_info == NULL)
      return NULL;

   if (access_type == ACCESS_TYPE_LOAD)
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
   m_shmem_perf_model->updateCycleCount(m_cache_perf_model->getLatency(CachePerfModelBase::ACCESS_CACHE_DATA_AND_TAGS));

   IntPtr tag;
   UInt32 set_index;
   splitAddress(addr, tag, set_index);

   CacheBlockInfo cache_block_info(tag);

   m_sets[set_index]->insert(&cache_block_info, fill_buff, 
         eviction, evict_block_info, evict_buff);
   *evict_addr = tagToAddress(evict_block_info->getTag());
  
   // Update sets for the purpose of maintaining cache counters
   if (m_track_detailed_cache_counters && m_shmem_perf_model->isEnabled())
   {
      evicted_set.erase(addr);
      invalidated_set.erase(addr);

      if (*eviction)
         evicted_set.insert(*evict_addr);
   }
}


// Single line cache access at addr
CacheBlockInfo* 
Cache::peekSingleLine(IntPtr addr)
{
   m_shmem_perf_model->updateCycleCount(m_cache_perf_model->getLatency(CachePerfModelBase::ACCESS_CACHE_TAGS));

   IntPtr tag;
   UInt32 set_index;
   splitAddress(addr, tag, set_index);

   return m_sets[set_index]->find(tag);
}

void
Cache::updateCounters(IntPtr addr, CacheState cache_state, AccessType access_type)
{
   if (access_type == ACCESS_TYPE_LOAD)
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
   out << "    num cache accesses: " << m_num_accesses;
   out << "    miss rate: " <<
      ((float) (m_num_accesses - m_num_hits) / m_num_accesses) * 100;
   
   if (m_track_detailed_cache_counters)
   {
      out << "    cold miss rate: " <<
         ((float) m_num_cold_misses / m_num_accesses) * 100;
      out << "    capacity miss rate: " <<
         ((float) m_num_capacity_misses / m_num_accesses) * 100;
      out << "    upgrade miss rate: " <<
         ((float) m_num_upgrade_misses / m_num_accesses) * 100;
      out << "    sharing miss rate: " <<
         ((float) m_num_sharing_misses / m_num_accesses) * 100;
   }
}
