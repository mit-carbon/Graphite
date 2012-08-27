#pragma once

#include "fixed_types.h"
#include "cache_line_info.h"
#include "cache_replacement_policy.h"

// Everything related to cache sets
class CacheSet
{
public:
   CacheSet(UInt32 set_num, CachingProtocolType caching_protocol_type, SInt32 cache_level,
            CacheReplacementPolicy* replacement_policy, UInt32 associativity, UInt32 line_size);
   ~CacheSet();

   void read_line(UInt32 line_index, UInt32 offset, Byte *out_buf, UInt32 bytes);
   void write_line(UInt32 line_index, UInt32 offset, Byte *in_buf, UInt32 bytes);
   CacheLineInfo* find(IntPtr tag, UInt32* line_index = NULL);
   void insert(CacheLineInfo* inserted_cache_line_info, Byte* fill_buf,
               bool* eviction, CacheLineInfo* evicted_cache_line_info, Byte* writeback_buf);

private:
   CacheLineInfo** _cache_line_info_array;
   char* _lines;
   UInt32 _set_num;
   CacheReplacementPolicy* _replacement_policy;
   UInt32 _associativity;
   UInt32 _line_size;
};
