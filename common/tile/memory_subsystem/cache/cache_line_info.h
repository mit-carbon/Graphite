#pragma once

#include "fixed_types.h"
#include "cache_state.h"
#include "cache.h"

class CacheLineInfo
{
// This can be extended to include other information
// for different cache coherence protocols and cache types
public:
   CacheLineInfo(IntPtr tag = ~0, CacheState::CState cstate = CacheState::INVALID);
   virtual ~CacheLineInfo();

   static CacheLineInfo* create(Cache::Type cache_type);

   virtual void invalidate(void);
   virtual void assign(CacheLineInfo* cache_line_info);

   bool isValid() const { return (_tag != ((IntPtr) ~0)); }
   
   IntPtr getTag() const { return _tag; }
   CacheState::CState getCState() const { return _cstate; }

   void setTag(IntPtr tag) { _tag = tag; }
   void setCState(CacheState::CState cstate) { _cstate = cstate; }

private:
   IntPtr _tag;
   CacheState::CState _cstate;
};
