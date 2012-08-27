#include "cache_line_info.h"
#include "log.h"

namespace ShL1ShL2
{

CacheLineInfo* createCacheLineInfo(SInt32 cache_level)
{
   switch (cache_level)
   {
   case L1:
      return new ShL1CacheLineInfo();
   case L2:
      return new ShL2CacheLineInfo();
   default:
      LOG_PRINT_ERROR("Unrecognized Cache Level(%u)", cache_level);
      return (CacheLineInfo*) NULL;
   }
}

}
