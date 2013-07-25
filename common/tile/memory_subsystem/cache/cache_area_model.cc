#include <iostream>
using namespace std;

#include "cache_area_model.h"
#include "mcpat_cache.h"
#include "cache_info.h"
#include "config.h"
#include "log.h"

CacheAreaModel::CacheAreaModel(string type, UInt32 size, UInt32 blocksize,
      UInt32 associativity, UInt32 delay, float frequency)
{
   LOG_ASSERT_ERROR(Config::getSingleton()->getEnableAreaModeling(), "Area Modeling Disabled");
   CacheParams cache_params(type, size, blocksize, associativity, delay, frequency);
   CacheArea cache_area;
   McPATCache::getSingleton()->getArea(&cache_params, &cache_area);

   _area = cache_area._area;
}

void
CacheAreaModel::outputSummary(ostream& out)
{
   out << "    Area (in mm^2): " << _area << endl;
}

void
CacheAreaModel::dummyOutputSummary(ostream& out)
{
   out << "    Area (in mm^2): " << endl;
}
