#include "cache_line_info.h"
#include "l2_directory_cfg.h"
#include "log.h"

namespace PrL1ShL2MSI
{

CacheLineInfo::CacheLineInfo(IntPtr tag, CacheState::Type cstate, UInt64 curr_time)
   : ::CacheLineInfo(tag, cstate, curr_time)
{}

CacheLineInfo::~CacheLineInfo()
{}

CacheLineInfo*
CacheLineInfo::create(SInt32 cache_level)
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
   , _caching_component(MemComponent::INVALID_MEM_COMPONENT)
{}

ShL2CacheLineInfo::~ShL2CacheLineInfo()
{}

void
ShL2CacheLineInfo::assign(CacheLineInfo* cache_line_info)
{
   CacheLineInfo::assign(cache_line_info);
   _directory_entry = ((ShL2CacheLineInfo*) cache_line_info)->getDirectoryEntry();
   _caching_component = ((ShL2CacheLineInfo*) cache_line_info)->getCachingComponent();
}

}
