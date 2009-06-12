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

      static CacheSetBase* createModel(ReplacementPolicy replacement_policy, UInt32 associativity, UInt32 blocksize);

      static ReplacementPolicy parsePolicyType(string policy);

   protected:
      CacheBlockInfo* m_cache_block_info_array;
      char* m_blocks;
      UInt32 m_associativity;
      UInt32 m_blocksize;
      UInt32 m_next_replace_index;

   public:

      CacheSetBase(UInt32 associativity, UInt32 blocksize):
            m_associativity(associativity), m_blocksize(blocksize)
      {
         m_next_replace_index = m_associativity-1;

         m_cache_block_info_array = new CacheBlockInfo[m_associativity];
         m_blocks = new char[m_associativity * m_blocksize];
         
         memset(m_blocks, 0x00, m_associativity * m_blocksize);
      }

      virtual ~CacheSetBase()
      {
         delete [] m_cache_block_info_array;
         delete [] m_blocks;
      }

      UInt32 getBlockSize() { return m_blocksize; }

      UInt32 getAssociativity() { return m_associativity; }

      void read_line(UInt32 line_index, UInt32 offset, Byte *out_buff, UInt32 bytes)
      {
         assert(offset + bytes <= m_blocksize);
         assert((out_buff == NULL) == (bytes == 0));

         if (out_buff != NULL)
            memcpy((void*) out_buff, &m_blocks[line_index * m_blocksize + offset], bytes);
      }

      void write_line(UInt32 line_index, UInt32 offset, Byte *in_buff, UInt32 bytes)
      {
         assert(offset + bytes <= m_blocksize);
         assert((in_buff == NULL) == (bytes == 0));

         if (in_buff != NULL)
            memcpy(&m_blocks[line_index * m_blocksize + offset], (void*) in_buff, bytes);
      }

      CacheBlockInfo* find(IntPtr tag, UInt32* line_index = NULL)
      {
         CacheBlockInfo cache_block_info;

         for (SInt32 index = m_associativity-1; index >= 0; index--)
         {
            if (m_cache_block_info_array[index].getTag() == tag)
            {
               if (line_index != NULL)
                  *line_index = index;
               return (&m_cache_block_info_array[index]);
            }
         }
         return NULL;
      }

      bool invalidate(IntPtr& tag)
      {
         for (SInt32 index = m_associativity-1; index >= 0; index--)
         {
            if (m_cache_block_info_array[index].getTag() == tag)
            {
               m_cache_block_info_array[index] = CacheBlockInfo();
               return true;
            }
         }
         return false;
      }

      void insert(CacheBlockInfo* cache_block_info, Byte* fill_buff, bool* eviction, CacheBlockInfo* evict_block_info, Byte* evict_buff)
      {
         // This replacement strategy does not take into account the fact that
         // cache blocks can be voluntarily flushed or invalidated due to another write request
         const UInt32 index = m_next_replace_index;
         assert(index < m_associativity);

         assert(eviction != NULL);
               
         if (m_cache_block_info_array[index].isValid())
         {
            *eviction = true;
            *evict_block_info = m_cache_block_info_array[index];
            if (evict_buff != NULL)
               memcpy((void*) evict_buff, &m_blocks[index * m_blocksize], m_blocksize);
         }
         else
         {
            *eviction = false;
         }

         m_cache_block_info_array[index] = *cache_block_info;

         if (fill_buff != NULL)
            memcpy(&m_blocks[index * m_blocksize], (void*) fill_buff, m_blocksize);

         // condition typically faster than modulo
         m_next_replace_index = getNextReplaceIndex();
      }

      virtual UInt32 getNextReplaceIndex()
      {
         return 0;
      }

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
