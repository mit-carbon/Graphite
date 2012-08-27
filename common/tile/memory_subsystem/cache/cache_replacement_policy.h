#pragma once

#include <string>
using std::string;

#include "fixed_types.h"
#include "caching_protocol_type.h"

class CacheLineInfo;

class CacheReplacementPolicy
{
public:
   enum Type
   {
      ROUND_ROBIN = 0,
      LRU,
      NUM_TYPES
   };

   CacheReplacementPolicy(UInt32 cache_size, UInt32 associativity, UInt32 cache_line_size);
   virtual ~CacheReplacementPolicy();

   static CacheReplacementPolicy* create(string policy_str, UInt32 cache_size, UInt32 associativity, UInt32 cache_line_size);
   static Type parse(string policy_str);
   
   virtual UInt32 getReplacementWay(CacheLineInfo** cache_line_info_array, UInt32 set_num) = 0;
   virtual void update(CacheLineInfo** cache_line_info_array, UInt32 set_num, UInt32 accessed_way) = 0;

protected:
   UInt32 _num_sets;
   UInt32 _associativity;
};
