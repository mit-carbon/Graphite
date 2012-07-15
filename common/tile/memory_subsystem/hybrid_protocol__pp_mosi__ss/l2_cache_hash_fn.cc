#include "l2_cache_hash_fn.h"
#include "simulator.h"
#include "config.h"
#include "utils.h"

namespace HybridProtocol_PPMOSI_SS
{

L2CacheHashFn::L2CacheHashFn(UInt32 cache_size, UInt32 associativity, UInt32 cache_line_size)
   : CacheHashFn(cache_size, associativity, cache_line_size)
{
   _log_num_sets = floorLog2(_num_sets);
}

L2CacheHashFn::~L2CacheHashFn()
{}

UInt32
L2CacheHashFn::compute(IntPtr address)
{
   LOG_PRINT("Computing Set for address(%#lx), _log_cache_line_size(%u), _log_num_sets(%u)",
             address, _log_cache_line_size, _log_num_sets);

   IntPtr set = 0;
   for (UInt32 i = _log_cache_line_size; (i + _log_num_sets) <= (sizeof(IntPtr)*8); i += _log_num_sets)
   {
      IntPtr addr_bits = getBits<IntPtr>(address, i + _log_num_sets, i);
      set = set ^ addr_bits;
   }

   return (UInt32) set;
}

}
