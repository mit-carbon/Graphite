#ifndef CACHE_SET_H
#define CACHE_SET_H

#include <string>

#include "cache_line.h"
#include "fixed_types.h"

using namespace std;

// Everything related to cache sets
class CacheSet
{
   public:
      enum ReplacementPolicy
      {
         ROUND_ROBIN = 0,
         LRU,
         NUM_REPLACEMENT_POLICIES
      };

      static CacheSet* createCacheSet(ReplacementPolicy replacement_policy, UInt32 associativity, UInt32 blocksize);
      static ReplacementPolicy parsePolicyType(string policy);

   protected:
      CacheBlockInfo* m_cache_block_info_array;
      char* m_blocks;
      UInt32 m_associativity;
      UInt32 m_blocksize;

   public:

      CacheSet(UInt32 associativity, UInt32 blocksize);
      virtual ~CacheSet();

      UInt32 getBlockSize() { return m_blocksize; }
      UInt32 getAssociativity() { return m_associativity; }

      void read_line(UInt32 line_index, UInt32 offset, Byte *out_buff, UInt32 bytes);
      void write_line(UInt32 line_index, UInt32 offset, Byte *in_buff, UInt32 bytes);
      CacheBlockInfo* find(IntPtr tag, UInt32* line_index = NULL);
      bool invalidate(IntPtr& tag);
      void insert(CacheBlockInfo* cache_block_info, Byte* fill_buff, bool* eviction, CacheBlockInfo* evict_block_info, Byte* evict_buff);

      virtual UInt32 getReplacementIndex() { return 0; }
      virtual void updateReplacementIndex(UInt32 accessed_index) { }

};

// Cache set with round robin replacement
class CacheSetRoundRobin : public CacheSet
{
   private:
      UInt32 m_replacement_index;

   public:
      CacheSetRoundRobin(UInt32 associativity, UInt32 blocksize);
      ~CacheSetRoundRobin();

      UInt32 getReplacementIndex();
      void updateReplacementIndex(UInt32 accessed_index);
};

// Cache Set with LRU replacement
class CacheSetLRU : public CacheSet
{
   private:
      UInt8* m_lru_bits;

   public:
      CacheSetLRU(UInt32 associativity, UInt32 blocksize);
      ~CacheSetLRU();

      UInt32 getReplacementIndex();
      void updateReplacementIndex(UInt32 accessed_index);
};


#endif /* CACHE_SET_H */
