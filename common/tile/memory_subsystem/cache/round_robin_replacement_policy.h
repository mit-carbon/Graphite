#pragma once

#include <vector>
using std::vector;

#include "cache_replacement_policy.h"

class RoundRobinReplacementPolicy : public CacheReplacementPolicy
{
public:
   RoundRobinReplacementPolicy(UInt32 cache_size, UInt32 associativity, UInt32 cache_line_size);
   ~RoundRobinReplacementPolicy();

   UInt32 getReplacementWay(CacheLineInfo** cache_line_info_array, UInt32 set_num);
   void update(CacheLineInfo** cache_line_info_array, UInt32 set_num, UInt32 accessed_way);
  
private: 
   vector<UInt32> _replacement_index_vec;
};
