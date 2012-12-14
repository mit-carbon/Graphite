#include "round_robin_replacement_policy.h"
#include "cache_line_info.h"

RoundRobinReplacementPolicy::RoundRobinReplacementPolicy(UInt32 cache_size, UInt32 associativity, UInt32 cache_line_size)
   : CacheReplacementPolicy(cache_size, associativity, cache_line_size)
{
   _replacement_index_vec.resize(_num_sets, _associativity - 1);
}

RoundRobinReplacementPolicy::~RoundRobinReplacementPolicy()
{}

UInt32
RoundRobinReplacementPolicy::getReplacementWay(CacheLineInfo** cache_line_info_array, UInt32 set_num)
{
   assert(set_num < _num_sets);
   UInt32 curr_replacement_index = _replacement_index_vec[set_num];
   _replacement_index_vec[set_num] = (_replacement_index_vec[set_num] == 0) ?
                                     (_associativity-1) : (_replacement_index_vec[set_num] - 1);
   return curr_replacement_index; 
}

void
RoundRobinReplacementPolicy::update(CacheLineInfo** cache_line_info_array, UInt32 set_num, UInt32 accessed_way)
{
   return;
}
