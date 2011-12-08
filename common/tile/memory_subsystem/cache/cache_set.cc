#include <cstring>
#include "cache_set.h"
#include "cache.h"
#include "log.h"

CacheSet::CacheSet(Cache::Type cache_type, UInt32 associativity, UInt32 line_size)
   : _associativity(associativity)
   , _line_size(line_size)
{
   _cache_line_info_array = new CacheLineInfo*[_associativity];
   for (UInt32 i = 0; i < _associativity; i++)
   {
      _cache_line_info_array[i] = CacheLineInfo::create(cache_type);
   }
   _lines = new char[_associativity * _line_size];
   
   memset(_lines, 0x00, _associativity * _line_size);
}

CacheSet::~CacheSet()
{
   for (UInt32 i = 0; i < _associativity; i++)
      delete _cache_line_info_array[i];
   delete [] _cache_line_info_array;
   delete [] _lines;
}

void 
CacheSet::read_line(UInt32 line_index, UInt32 offset, Byte *out_buf, UInt32 bytes)
{
   assert(offset + bytes <= _line_size);
   assert((out_buf == NULL) == (bytes == 0));

   if (out_buf != NULL)
      memcpy((void*) out_buf, &_lines[line_index * _line_size + offset], bytes);

   updateReplacementIndex(line_index);
}

void 
CacheSet::write_line(UInt32 line_index, UInt32 offset, Byte *in_buf, UInt32 bytes)
{
   assert(offset + bytes <= _line_size);
   assert((in_buf == NULL) == (bytes == 0));

   if (in_buf != NULL)
      memcpy(&_lines[line_index * _line_size + offset], (void*) in_buf, bytes);

   updateReplacementIndex(line_index);
}

CacheLineInfo* 
CacheSet::find(IntPtr tag, UInt32* line_index)
{
   for (SInt32 index = _associativity-1; index >= 0; index--)
   {
      if (_cache_line_info_array[index]->getTag() == tag)
      {
         if (line_index != NULL)
            *line_index = index;
         return (_cache_line_info_array[index]);
      }
   }
   return NULL;
}

void 
CacheSet::insert(CacheLineInfo* inserted_cache_line_info, Byte* fill_buf,
                 bool* eviction, CacheLineInfo* evicted_cache_line_info, Byte* writeback_buf)
{
   // This replacement strategy does not take into account the fact that
   // cache lines can be voluntarily flushed or invalidated due to another write request
   const UInt32 index = getReplacementIndex();
   assert(index < _associativity);

   assert(eviction != NULL);
         
   if (_cache_line_info_array[index]->isValid())
   {
      *eviction = true;
      evicted_cache_line_info->assign(_cache_line_info_array[index]);
      if (writeback_buf != NULL)
         memcpy((void*) writeback_buf, &_lines[index * _line_size], _line_size);
   }
   else
   {
      *eviction = false;
   }

   _cache_line_info_array[index]->assign(inserted_cache_line_info);
   if (fill_buf != NULL)
      memcpy(&_lines[index * _line_size], (void*) fill_buf, _line_size);
}

CacheSet* 
CacheSet::createCacheSet(Cache::ReplacementPolicy replacement_policy, Cache::Type cache_type,
                         UInt32 associativity, UInt32 line_size)
{
   switch(replacement_policy)
   {
   case Cache::ROUND_ROBIN:
      return new CacheSetRoundRobin(cache_type, associativity, line_size);

   case Cache::LRU:
      return new CacheSetLRU(cache_type, associativity, line_size);

   default:
      LOG_PRINT_ERROR("Unrecognized Cache Replacement Policy: %i", replacement_policy);
      return (CacheSet*) NULL;
   }
}
