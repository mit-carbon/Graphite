#include "cache_set.h"
#include "log.h"

CacheSetLRU::CacheSetLRU(CachingProtocolType caching_protocol_type, SInt32 cache_level, UInt32 associativity, UInt32 linesize)
   : CacheSet(caching_protocol_type, cache_level, associativity, linesize)
{
   _lru_bits = new UInt8[_associativity];
   for (UInt32 i = 0; i < _associativity; i++)
      _lru_bits[i] = i;
}

CacheSetLRU::~CacheSetLRU()
{
   delete [] _lru_bits;
}

UInt32 
CacheSetLRU::getReplacementIndex()
{
   // Invalidations may mess up the LRU bits
   UInt32 index = _associativity;
   for (UInt32 i = 0; i < _associativity; i++)
   {
      if (!_cache_line_info_array[i]->isValid())
         return i;
      else if (_lru_bits[i] == (_associativity-1))
         index = i;
   }
   LOG_ASSERT_ERROR(index < _associativity, "Error Finding LRU bits");
   return index;
}

void
CacheSetLRU::updateReplacementIndex(UInt32 accessed_index)
{
   for (UInt32 i = 0; i < _associativity; i++)
   {
      if (_lru_bits[i] < _lru_bits[accessed_index])
         _lru_bits[i] ++;
   }
   _lru_bits[accessed_index] = 0;
}
