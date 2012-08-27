#include "cache_line_info.h"
#include "log.h"

namespace PrL1ShL2MSI
{

CacheLineInfo* createCacheLineInfo(SInt32 cache_level)
{
   switch (cache_level)
   {
   case L1:
      return new PrL1CacheLineInfo();
   case L2:
      return new ShL2CacheLineInfo();
   default:
      LOG_PRINT_ERROR("Unrecognized Cache Level(%u)", cache_level);
      return (CacheLineInfo*) NULL;
   }
}

// ShL2 CacheLineInfo

ShL2CacheLineInfo::ShL2CacheLineInfo(IntPtr tag, DirectoryEntry* directory_entry)
   : CacheLineInfo(tag, CacheState::INVALID)
   , _directory_entry(directory_entry)
   , _caching_component(MemComponent::INVALID)
{}

ShL2CacheLineInfo::~ShL2CacheLineInfo()
{}

void
ShL2CacheLineInfo::assign(CacheLineInfo* cache_line_info)
{
   CacheLineInfo::assign(cache_line_info);
   ShL2CacheLineInfo* L2_cache_line_info = dynamic_cast<ShL2CacheLineInfo*>(cache_line_info);
   _directory_entry = L2_cache_line_info->getDirectoryEntry();
   _caching_component = L2_cache_line_info->getCachingComponent();
}

}
