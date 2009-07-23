#include "cache_set.h"
#include "log.h"

CacheSet::CacheSet(UInt32 associativity, UInt32 blocksize):
      m_associativity(associativity), m_blocksize(blocksize)
{
   m_cache_block_info_array = new CacheBlockInfo[m_associativity];
   m_blocks = new char[m_associativity * m_blocksize];
   
   memset(m_blocks, 0x00, m_associativity * m_blocksize);
}

CacheSet::~CacheSet()
{
   delete [] m_cache_block_info_array;
   delete [] m_blocks;
}

void 
CacheSet::read_line(UInt32 line_index, UInt32 offset, Byte *out_buff, UInt32 bytes)
{
   assert(offset + bytes <= m_blocksize);
   assert((out_buff == NULL) == (bytes == 0));

   if (out_buff != NULL)
      memcpy((void*) out_buff, &m_blocks[line_index * m_blocksize + offset], bytes);

   updateReplacementIndex(line_index);
}

void 
CacheSet::write_line(UInt32 line_index, UInt32 offset, Byte *in_buff, UInt32 bytes)
{
   assert(offset + bytes <= m_blocksize);
   assert((in_buff == NULL) == (bytes == 0));

   if (in_buff != NULL)
      memcpy(&m_blocks[line_index * m_blocksize + offset], (void*) in_buff, bytes);

   // Set the Dirty bit
   m_cache_block_info_array[line_index].setDirty();

   updateReplacementIndex(line_index);
}

CacheBlockInfo* 
CacheSet::find(IntPtr tag, UInt32* line_index)
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

bool 
CacheSet::invalidate(IntPtr& tag)
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

void 
CacheSet::insert(CacheBlockInfo* cache_block_info, Byte* fill_buff, bool* eviction, CacheBlockInfo* evict_block_info, Byte* evict_buff)
{
   // This replacement strategy does not take into account the fact that
   // cache blocks can be voluntarily flushed or invalidated due to another write request
   const UInt32 index = getReplacementIndex();
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
}

CacheSet* 
CacheSet::createCacheSet (ReplacementPolicy replacement_policy,
      UInt32 associativity, UInt32 blocksize)
{
   switch(replacement_policy)
   {
      case ROUND_ROBIN:
         return new CacheSetRoundRobin(associativity, blocksize);               
              
      case LRU:
         return new CacheSetLRU(associativity, blocksize);

      default:
         LOG_PRINT_ERROR("Unrecognized Cache Replacement Policy: %i",
               replacement_policy);
         break;
   }

   return (CacheSet*) NULL;
}

CacheSet::ReplacementPolicy 
CacheSet::parsePolicyType(string policy)
{
   if (policy == "round_robin")
      return ROUND_ROBIN;
   if (policy == "lru")
      return LRU;
   else
      return (ReplacementPolicy) -1;
}
