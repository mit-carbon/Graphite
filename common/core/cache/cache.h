#ifndef CACHE_H
#define CACHE_H

#include <string>
#include <cassert>

#include "simulator.h"
#include "utils.h"
#include "cache_set.h"
#include "cache_line.h"
#include "cache_perf_model_base.h"
#include "shmem_perf_model.h"
#include "log.h"

#define k_KILO 1024
#define k_MEGA (k_KILO*k_KILO)
#define k_GIGA (k_KILO*k_MEGA)

using namespace std;

// type of cache hit/miss counters
typedef UInt64 CacheStats;

typedef enum
{
   STORE_ALLOCATE,
   STORE_NO_ALLOCATE
} StoreAllocation;

// Generic cache base class; no allocate specialization, no cache set specialization
class CacheBase
{
   public:
      // types, constants
      typedef enum
      {
         ACCESS_TYPE_LOAD,
         ACCESS_TYPE_STORE,
         NUM_ACCESS_TYPES
      } AccessType;

      typedef enum
      {
         CACHE_TYPE_ICACHE,
         CACHE_TYPE_DCACHE,
         NUM_CACHE_TYPES
      } CacheType;

   protected:
      //1 counter for hit==true, 1 counter for hit==false
      CacheStats m_access[NUM_ACCESS_TYPES][2];

      // input params
      string m_name;
      UInt32 m_cache_size;
      UInt32 m_blocksize;
      UInt32 m_associativity;
      UInt32 m_num_sets;

      // computed params
      UInt32 m_log_blocksize;

   private:
      CacheStats sumAccess(bool hit) const
      {
         CacheStats sum = 0;
         for (UInt32 access_type = 0; access_type < NUM_ACCESS_TYPES; access_type++)
         {
            sum += m_access[access_type][hit];
         }
         return sum;
      }

   public:
      // constructors/destructors
      CacheBase(string name)
         : m_name(name)
      {
         try
         {
            m_cache_size = k_KILO * (Sim()->getCfg()->getInt("cache/dcache_size"));
            m_blocksize = Sim()->getCfg()->getInt("cache/dcache_line_size");
            m_associativity = Sim()->getCfg()->getInt("cache/dcache_associativity");
         }
         catch(...)
         {
            LOG_PRINT_ERROR("Error while reading cache parameters from the config file");
         }
         m_num_sets = m_cache_size / (m_associativity * m_blocksize);
         m_log_blocksize = ceilLog2(m_blocksize);
      }

      virtual ~CacheBase() {}

      // accessors
      UInt32 getCacheSize() const { return m_cache_size; }
      UInt32 getBlockSize() const { return m_blocksize; }
      UInt32 dCacheLineSize() const { return m_blocksize; }
      UInt32 getNumWays() const { return m_associativity; }
      UInt32 getNumSets() const { return m_num_sets; }

      // stats
      CacheStats getHits(AccessType access_type) const
      {
         assert(access_type < NUM_ACCESS_TYPES);
         return m_access[access_type][true];
      }
      CacheStats getMisses(AccessType access_type) const
      {
         assert(access_type < NUM_ACCESS_TYPES);
         return m_access[access_type][false];
      }
      CacheStats getAccesses(AccessType access_type) const
      {
         return getHits(access_type) + getMisses(access_type); 
      }
      CacheStats getHits() const { return sumAccess(true); }
      CacheStats getMisses() const { return sumAccess(false); }
      CacheStats getAccesses() const { return getHits() + getMisses(); }

      // utilities
      IntPtr tagToAddress(IntPtr tag)
      {
         return (tag << m_log_blocksize);
      }

      void splitAddress(const IntPtr addr, IntPtr& tag, UInt32& set_index) const
      {
         tag = addr >> m_log_blocksize;
         set_index = tag & (m_num_sets-1);
      }

      void splitAddress(const IntPtr addr, IntPtr& tag, UInt32& set_index,
                        UInt32& block_offset) const
      {
         block_offset = addr & (m_blocksize-1);
         splitAddress(addr, tag, set_index);
      }

      void outputSummary(ostream& out)
      {
         // FIXME: Fill this in later
         return;
      }

};

//  Templated cache class with specific cache set allocation policies
//  All that remains to be done here is allocate and deallocate the right
//  type of cache sets.

class Cache : public CacheBase
{
   private:

      CacheSetBase**  m_sets;
      CachePerfModelBase* m_cache_perf_model;
      ShmemPerfModel* m_shmem_perf_model;

   public:

      // constructors/destructors
      Cache(string name, ShmemPerfModel* shmem_perf_model) :
            CacheBase(name)
      {
         CacheSetBase::ReplacementPolicy replacement_policy;
         try
         {
            replacement_policy = CacheSetBase::parsePolicyType(Sim()->getCfg()->getString("cache/dcache_replacement_policy"));
         }
         catch (...)
         {
            LOG_PRINT_ERROR("Error reading cache replacement policy from the config file");
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
            model_type = CachePerfModelBase::parseModelType(Sim()->getCfg()->getString("perf_model/cache/model_type"));
         }
         catch(...)
         {
            LOG_PRINT_ERROR("Error reading perf_model/cache/model_type from config file");
         }
         m_cache_perf_model = CachePerfModelBase::createModel(model_type);
         m_shmem_perf_model = shmem_perf_model;
      }

      ~Cache()
      {
         for (SInt32 i = 0; i < (SInt32) m_num_sets; i++)
            delete m_sets[i];
         delete [] m_sets;
      }

      

      bool invalidateSingleLine(IntPtr addr)
      {
         m_shmem_perf_model->updateCycleCount(m_cache_perf_model->getLatency(CachePerfModelBase::ACCESS_CACHE_TAGS));

         IntPtr tag;
         UInt32 set_index;

         splitAddress(addr, tag, set_index);
         assert(set_index < m_num_sets);

         return m_sets[set_index]->invalidate(tag);
      }

      CacheBlockInfo* accessSingleLine(IntPtr addr, AccessType access_type, 
            Byte* buff = NULL, UInt32 bytes = 0)
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

      void insertSingleLine(IntPtr addr, Byte* fill_buff,
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
      }
      

      // Single line cache access at addr
      CacheBlockInfo* peekSingleLine(IntPtr addr)
      {
         m_shmem_perf_model->updateCycleCount(m_cache_perf_model->getLatency(CachePerfModelBase::ACCESS_CACHE_TAGS));

         IntPtr tag;
         UInt32 set_index;
         splitAddress(addr, tag, set_index);

         return m_sets[set_index]->find(tag);
      }

};

#endif /* CACHE_H */
