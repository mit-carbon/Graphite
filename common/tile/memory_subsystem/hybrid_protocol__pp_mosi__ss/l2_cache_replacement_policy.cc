#include "l2_cache_replacement_policy.h"
#include "cache_line_info.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

L2CacheReplacementPolicy::L2CacheReplacementPolicy(UInt32 cache_size, UInt32 associativity, UInt32 cache_line_size)
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

L2CacheReplacementPolicy::~L2CacheReplacementPolicy()
{}

UInt32 
L2CacheReplacementPolicy::getReplacementWay(CacheLineInfo** cache_line_info_array, UInt32 set_num)
{
   const vector<UInt8>& lru_bits = _lru_bits_vec[set_num];
   // Invalidations may mess up the LRU bits
   UInt32 lru_way = _associativity;
   UInt32 lru_val = 0;
   for (UInt32 i = 0; i < _associativity; i++)
   {
      HybridL2CacheLineInfo* l2_cache_line_info = dynamic_cast<HybridL2CacheLineInfo*>(cache_line_info_array[i]);
      if (!l2_cache_line_info->isValid())
         return i;
      else if ((lru_bits[i] >= lru_val) && (!l2_cache_line_info->isLocked()))
      {
         lru_way = i;
         lru_val = lru_bits[i];
      }
   }
   LOG_ASSERT_ERROR(lru_way < _associativity, "Error Finding LRU bits, num_sets(%u), _associativity(%u)", _num_sets, _associativity);
   return lru_way;
}

void
L2CacheReplacementPolicy::update(CacheLineInfo** cache_line_info_array, UInt32 set_num, UInt32 accessed_way)
{
   vector<UInt8>& lru_bits = _lru_bits_vec[set_num];
   for (UInt32 i = 0; i < _associativity; i++)
   {
      if (lru_bits[i] < lru_bits[accessed_way])
         lru_bits[i] ++;
   }
   lru_bits[accessed_way] = 0;
}

}
