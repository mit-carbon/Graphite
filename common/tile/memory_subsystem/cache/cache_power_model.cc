#include <iostream>
using namespace std;

#include "cache_power_model.h"
#include "mcpat_cache.h"
#include "cache_info.h"
#include "config.h"
#include "log.h"

CachePowerModel::CachePowerModel(string type, UInt32 size, UInt32 blocksize,
                                 UInt32 associativity, UInt32 delay, volatile float frequency)
{
   LOG_ASSERT_ERROR(Config::getSingleton()->getEnablePowerModeling(), "Power Modeling Disabled");
   CacheParams cache_params(type, size, blocksize, associativity, delay, frequency);
   CachePower cache_power;
   McPATCache::getSingleton()->getPower(&cache_params, &cache_power);

   _dynamic_energy = cache_power._dynamic_energy;
   _total_static_power = cache_power._subthreshold_leakage_power + cache_power._gate_leakage_power;
}

void
CachePowerModel::outputSummary(ostream& out)
{
   out << "    Static Power (in W): " << _total_static_power << endl;
   out << "    Dynamic Energy (in J): " << _total_dynamic_energy << endl;
}

void
CachePowerModel::dummyOutputSummary(ostream& out)
{
   out << "    Static Power (in W): " << endl;
   out << "    Dynamic Energy (in J): " << endl;
}
