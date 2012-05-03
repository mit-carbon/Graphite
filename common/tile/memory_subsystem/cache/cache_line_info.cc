#include "cache_line_info.h"
#include "pr_l1_pr_l2_dram_directory_msi/cache_line_info.h"
#include "pr_l1_pr_l2_dram_directory_mosi/cache_line_info.h"
#include "sh_l1_sh_l2/cache_line_info.h"
#include "pr_l1_sh_l2_msi/cache_line_info.h"
#include "log.h"

CacheLineInfo::CacheLineInfo(IntPtr tag, CacheState::Type cstate, UInt64 curr_time)
   : _tag(tag)
   , _cstate(cstate)
#ifdef TRACK_DETAILED_CACHE_COUNTERS
   , _birth_time(curr_time)
#endif
{}

CacheLineInfo::~CacheLineInfo()
{}

CacheLineInfo*
CacheLineInfo::create(CachingProtocolType caching_protocol_type, SInt32 cache_type)
{
   switch (caching_protocol_type)
   {
   case PR_L1_PR_L2_DRAM_DIRECTORY_MSI:
      return PrL1PrL2DramDirectoryMSI::CacheLineInfo::create(cache_type);

   case PR_L1_PR_L2_DRAM_DIRECTORY_MOSI:
      return PrL1PrL2DramDirectoryMOSI::CacheLineInfo::create(cache_type);

   case SH_L1_SH_L2:
      return ShL1ShL2::CacheLineInfo::create(cache_type);

   case PR_L1_SH_L2_MSI:
      return PrL1ShL2MSI::CacheLineInfo::create(cache_type);

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
#ifdef TRACK_DETAILED_CACHE_COUNTERS
   _utilization = CacheLineUtilization();
   _birth_time = UINT64_MAX_;
#endif
}

void
CacheLineInfo::assign(CacheLineInfo* cache_line_info)
{
   _tag = cache_line_info->getTag();
   _cstate = cache_line_info->getCState();
#ifdef TRACK_DETAILED_CACHE_COUNTERS
   _utilization = cache_line_info->getUtilization();
   _birth_time = cache_line_info->getBirthTime();
#endif
}
