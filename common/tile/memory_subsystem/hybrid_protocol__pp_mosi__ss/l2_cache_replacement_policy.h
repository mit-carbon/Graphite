#pragma once

#include <vector>
using std::vector;

#include "../cache/cache_replacement_policy.h"

namespace HybridProtocol_PPMOSI_SS
{

class L2CacheReplacementPolicy : public CacheReplacementPolicy
{
public:
   L2CacheReplacementPolicy(UInt32 cache_size, UInt32 associativity, UInt32 cache_line_size);
   ~L2CacheReplacementPolicy();

   UInt32 getReplacementWay(CacheLineInfo** cache_line_info_array, UInt32 set_num);
   void update(CacheLineInfo** cache_line_info_array, UInt32 set_num, UInt32 accessed_way);
  
private: 
   vector<vector<UInt8> > _lru_bits_vec;
};

}
