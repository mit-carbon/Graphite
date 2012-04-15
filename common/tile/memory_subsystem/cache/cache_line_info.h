#pragma once

#include "fixed_types.h"
#include "cache_state.h"
#include "cache_line_utilization.h"
#include "cache.h"
#include "cache_utils.h"

class CacheLineInfo
{
// This can be extended to include other information
// for different cache coherence protocols and cache types
public:
   CacheLineInfo(IntPtr tag = ~0, CacheState::Type cstate = CacheState::INVALID, UInt64 curr_time = 0);
   virtual ~CacheLineInfo();

   static CacheLineInfo* create(Cache::Type cache_type);

   virtual void invalidate(CacheLineUtilization& utilization, UInt64 curr_time);
   virtual void assign(CacheLineInfo* cache_line_info);

   bool isValid() const { return (_tag != ((IntPtr) ~0)); }
   
   IntPtr getTag() const                        { return _tag; }
   CacheState::Type getCState() const           { return _cstate; }
   CacheLineUtilization getUtilization() const  { return _utilization; }
   UInt64 getBirthTime() const                  { return _birth_time; }
   UInt64 getLifetime(UInt64 curr_time) const   { return computeLifetime(_birth_time, curr_time); }

   void setTag(IntPtr tag)                      { _tag = tag; }
   void setCState(CacheState::Type cstate)      { _cstate = cstate; }
   void incrReadUtilization()                   { _utilization.read ++; }
   void incrWriteUtilization()                  { _utilization.write ++; }

private:
   IntPtr _tag;
   CacheState::Type _cstate;

protected:
   CacheLineUtilization _utilization;
   UInt64 _birth_time;
};
