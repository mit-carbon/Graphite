#include "cache_set.h"

CacheSetRoundRobin::CacheSetRoundRobin(Cache::Type cache_type, UInt32 associativity, UInt32 blocksize)
   : CacheSet(cache_type, associativity, blocksize)
{
   _replacement_index = _associativity - 1;
}

CacheSetRoundRobin::~CacheSetRoundRobin()
{}

UInt32
CacheSetRoundRobin::getReplacementIndex()
{
   UInt32 curr_replacement_index = _replacement_index;
   _replacement_index = (_replacement_index == 0) ? (_associativity-1) : (_replacement_index-1);
   return curr_replacement_index; 
}

void
CacheSetRoundRobin::updateReplacementIndex(UInt32 accessed_index)
{
   return;
}
