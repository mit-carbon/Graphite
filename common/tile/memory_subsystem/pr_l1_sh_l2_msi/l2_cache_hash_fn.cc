#include "l2_cache_hash_fn.h"
#include "simulator.h"
#include "config.h"

namespace PrL1ShL2MSI
{

L2CacheHashFn::L2CacheHashFn(UInt32 cache_size, UInt32 associativity, UInt32 cache_line_size)
   : CacheHashFn(cache_size, associativity, cache_line_size)
{
   _log_num_application_tiles = floorLog2(Config::getSingleton()->getApplicationTiles());
   _log_num_sets = floorLog2(_num_sets);
}

L2CacheHashFn::~L2CacheHashFn()
{}

UInt32
L2CacheHashFn::compute(IntPtr address)
{
   LOG_PRINT("Computing Set for address(%#lx), _log_cache_line_size(%u), _log_num_sets(%u)",
             address, _log_cache_line_size, _log_num_sets);

   if (_log_num_sets == 0)
      return 0;

   IntPtr set = 0;
   for (UInt32 i = _log_cache_line_size; (i + _log_num_sets) <= (sizeof(IntPtr)*8); i += _log_num_sets)
   {
      IntPtr addr_bits = getBits<IntPtr>(address, i + _log_num_sets, i);
      set = set ^ addr_bits;
   }

   return (UInt32) set;
}

}
