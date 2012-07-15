#pragma once

#include "fixed_types.h"
#include "cache.h"
#include "cache_utils.h"
#include "caching_protocol_type.h"

#include "common_defines.h"
#ifdef TRACK_DETAILED_CACHE_COUNTERS
#include "cache_line_utilization.h"
#endif

class CacheLineInfo
{
// This can be extended to include other information
// for different cache coherence protocols and cache types
public:
   CacheLineInfo(IntPtr tag = ~0, CacheState::Type cstate = CacheState::INVALID, UInt64 curr_time = 0);
   virtual ~CacheLineInfo();

   static CacheLineInfo* create(CachingProtocolType caching_protocol_type, SInt32 cache_level);

   virtual void invalidate();
   virtual void assign(CacheLineInfo* cache_line_info);

   bool isValid() const                        
   { return (_tag != ((IntPtr) ~0)); }
   IntPtr getTag() const                        
   { return _tag; }
   CacheState::Type getCState() const           
   { return _cstate; }

#ifdef TRACK_DETAILED_CACHE_COUNTERS
   CacheLineUtilization getUtilization() const  
   { return _utilization; }
   UInt64 getBirthTime() const                  
   { return _birth_time; }
   UInt64 getLifetime(UInt64 curr_time) const
   { return computeLifetime(_birth_time, curr_time); }
#endif

   void setTag(IntPtr tag)                     
   { _tag = tag; }
   void setCState(CacheState::Type cstate)      
   { _cstate = cstate; }

#ifdef TRACK_DETAILED_CACHE_COUNTERS
   void setUtilization(CacheLineUtilization utilization)
   { _utilization = utilization; }
   void incrReadUtilization()                   
   { _utilization.read ++; }
   void incrWriteUtilization()                  
   { _utilization.write ++; }
   void setBirthTime(UInt64 birth_time)
   { _birth_time = birth_time; }
#endif

protected:
   IntPtr _tag;
   CacheState::Type _cstate;

#ifdef TRACK_DETAILED_CACHE_COUNTERS
   CacheLineUtilization _utilization;
   UInt64 _birth_time;
#endif
};
