#ifndef CACHE_SET_H
#define CACHE_SET_H

#include <string>

#include "cache_line.h"
#include "fixed_types.h"

using namespace std;

// Everything related to cache sets
class CacheSetBase
{
   public:
      enum ReplacementPolicy
      {
         ROUND_ROBIN = 0,
         LEAST_RECENTLY_USED,
         NUM_REPLACEMENT_POLICIES
      };

      static CacheSetBase* createCacheSet(ReplacementPolicy replacement_policy, UInt32 associativity, UInt32 blocksize);
      static ReplacementPolicy parsePolicyType(string policy);

   protected:
      CacheBlockInfo* m_cache_block_info_array;
      char* m_blocks;
      UInt32 m_associativity;
      UInt32 m_blocksize;
      UInt32 m_next_replace_index;

   public:

      CacheSetBase(UInt32 associativity, UInt32 blocksize);
      virtual ~CacheSetBase();

      UInt32 getBlockSize() { return m_blocksize; }
      UInt32 getAssociativity() { return m_associativity; }

      void read_line(UInt32 line_index, UInt32 offset, Byte *out_buff, UInt32 bytes);
      void write_line(UInt32 line_index, UInt32 offset, Byte *in_buff, UInt32 bytes);
      CacheBlockInfo* find(IntPtr tag, UInt32* line_index = NULL);
      bool invalidate(IntPtr& tag);
      void insert(CacheBlockInfo* cache_block_info, Byte* fill_buff, bool* eviction, CacheBlockInfo* evict_block_info, Byte* evict_buff);

      virtual UInt32 getNextReplaceIndex() { return 0; }

};

// Cache set with round robin replacement
class RoundRobin : public CacheSetBase
{
   public:
      RoundRobin(UInt32 associativity, UInt32 blocksize) :
         CacheSetBase(associativity, blocksize)
      {}
      ~RoundRobin()
      {}

      UInt32 getNextReplaceIndex()
      {
         return ( (m_next_replace_index == 0) ? (m_associativity-1) : (m_next_replace_index-1) );
      }
};


#endif /* CACHE_SET_H */
