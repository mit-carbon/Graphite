#include "cache_line_info.h"
#include "pr_l1_pr_l2_dram_directory_msi/cache_line_info.h"
#include "pr_l1_pr_l2_dram_directory_mosi/cache_line_info.h"
#include "pr_l1_sh_l2_msi/cache_line_info.h"
#include "log.h"

CacheLineInfo::CacheLineInfo(IntPtr tag, CacheState::Type cstate)
   : _tag(tag)
   , _cstate(cstate)
{}

CacheLineInfo::~CacheLineInfo()
{}

CacheLineInfo*
CacheLineInfo::create(CachingProtocolType caching_protocol_type, SInt32 cache_level)
{
   switch (caching_protocol_type)
   {
   case PR_L1_PR_L2_DRAM_DIRECTORY_MSI:
      return PrL1PrL2DramDirectoryMSI::createCacheLineInfo(cache_level);

   case PR_L1_PR_L2_DRAM_DIRECTORY_MOSI:
      return PrL1PrL2DramDirectoryMOSI::createCacheLineInfo(cache_level);

   case PR_L1_SH_L2_MSI:
      return PrL1ShL2MSI::createCacheLineInfo(cache_level);

   default:
      LOG_PRINT_ERROR("Unrecognized caching protocol type(%u)", caching_protocol_type);
      return NULL;
   }
}

void
CacheLineInfo::invalidate()
{
   _tag = ~0;
   _cstate = CacheState::INVALID;
}

void
CacheLineInfo::assign(CacheLineInfo* cache_line_info)
{
   _tag = cache_line_info->getTag();
   _cstate = cache_line_info->getCState();
}
