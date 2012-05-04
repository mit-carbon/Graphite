#include "lru_replacement_policy.h"
#include "cache_line_info.h"
#include "log.h"

LRUReplacementPolicy::LRUReplacementPolicy(UInt32 cache_size, UInt32 associativity, UInt32 cache_line_size)
   : CacheReplacementPolicy(cache_size, associativity, cache_line_size)
{
   _lru_bits_vec.resize(_num_sets);
   for (UInt32 set_num = 0; set_num < _num_sets; set_num ++)
   {
      vector<UInt8>& lru_bits = _lru_bits_vec[set_num];
      lru_bits.resize(_associativity);
      for (UInt32 way_num = 0; way_num < _associativity; way_num ++)
      {
         lru_bits[way_num] = way_num;
      }
   }
}

LRUReplacementPolicy::~LRUReplacementPolicy()
{}

UInt32 
LRUReplacementPolicy::getReplacementWay(CacheLineInfo** cache_line_info_array, UInt32 set_num)
{
   const vector<UInt8>& lru_bits = _lru_bits_vec[set_num];
   // Invalidations may mess up the LRU bits
   UInt32 way = _associativity;
   for (UInt32 i = 0; i < _associativity; i++)
   {
      if (!cache_line_info_array[i]->isValid())
         return i;
      else if (lru_bits[i] == (_associativity-1))
         way = i;
   }
   LOG_ASSERT_ERROR(way < _associativity, "Error Finding LRU bits");
   return way;
}

void
LRUReplacementPolicy::update(CacheLineInfo** cache_line_info_array, UInt32 set_num, UInt32 accessed_way)
{
   vector<UInt8>& lru_bits = _lru_bits_vec[set_num];
   for (UInt32 i = 0; i < _associativity; i++)
   {
      if (lru_bits[i] < lru_bits[accessed_way])
         lru_bits[i] ++;
   }
   lru_bits[accessed_way] = 0;
}
