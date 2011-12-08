#include "cache_line_info.h"
#include "pr_l1_cache_line_info.h"
#include "pr_l2_cache_line_info.h"
#include "log.h"

CacheLineInfo::CacheLineInfo(IntPtr tag, CacheState::CState cstate)
   : _tag(tag)
   , _cstate(cstate)
{}

CacheLineInfo::~CacheLineInfo()
{}

CacheLineInfo*
CacheLineInfo::create(Cache::Type cache_type)
{
   switch (cache_type)
   {
   case Cache::PR_L1_CACHE:
      return new PrL1CacheLineInfo();

   case Cache::PR_L2_CACHE:
      return new PrL2CacheLineInfo();

   default:
      LOG_PRINT_ERROR("Unrecognized cache type (%u)", cache_type);
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
