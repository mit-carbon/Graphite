#pragma once

#include <map>
#include "cache_info.h"

class McPATCache
{
   public:
      static void allocate();
      static void release();

      static McPATCache* getSingleton() { return _singleton; }

      void getArea(CacheParams* cache_params, CacheArea* cache_area);
      void getPower(CacheParams* cache_params, CachePower* cache_power);

   private:
      McPATCache();
      ~McPATCache();
     
      static McPATCache* _singleton;
     
      std::string _mcpat_home;

      typedef std::pair<CacheArea*,CachePower*> CacheInfo;
      typedef std::map<CacheParams*, CacheInfo> CacheInfoMap;
      CacheInfoMap _cache_info_map;

      CacheInfo findCached(CacheParams* cache_params, bool& found);
      CacheInfo runMcPAT(CacheParams* cache_params_);
};
