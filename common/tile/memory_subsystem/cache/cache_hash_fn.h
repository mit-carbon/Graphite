#pragma once

#include "fixed_types.h"
#include "utils.h"
#include "constants.h"

class CacheHashFn
{
public:
   CacheHashFn(UInt32 cache_size, UInt32 associativity, UInt32 cache_line_size)
      : _num_sets(cache_size * k_KILO / (cache_line_size * associativity))
      , _log_cache_line_size(floorLog2(cache_line_size))
   {}
   virtual ~CacheHashFn()
   {}

   virtual UInt32 compute(IntPtr address)
   { return (address >> _log_cache_line_size) & (_num_sets-1); }

protected:
   UInt32 _num_sets;
   UInt32 _log_cache_line_size;
};
