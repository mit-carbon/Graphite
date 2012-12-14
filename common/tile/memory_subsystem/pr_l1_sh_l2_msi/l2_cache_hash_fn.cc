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
   _log_stack_size = floorLog2(Sim()->getCfg()->getInt("stack/stack_size_per_core"));
}

L2CacheHashFn::~L2CacheHashFn()
{}

UInt32
L2CacheHashFn::compute(IntPtr address)
{
   UInt32 num_tile_id_bits = (_log_num_application_tiles <= _log_num_sets) ? _log_num_application_tiles : _log_num_sets;
   IntPtr tile_id_bits = (address >> _log_stack_size) & ((1 << num_tile_id_bits) - 1);
 
   UInt32 num_sub_block_bits = _log_num_sets - num_tile_id_bits;

   IntPtr sub_block_id = (address >> (_log_cache_line_size + _log_num_application_tiles))
                         & ((1 << num_sub_block_bits) - 1);

   IntPtr super_block_id = (address >> (_log_cache_line_size + _log_num_application_tiles + num_sub_block_bits))
                           & ((1 << num_tile_id_bits) - 1);

   return ((tile_id_bits ^ super_block_id) << num_sub_block_bits) + sub_block_id;
}

}
