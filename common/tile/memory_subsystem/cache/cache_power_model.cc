#include <iostream>
using namespace std;

#include "cache.h"
#include "cache_power_model.h"
#include "mcpat_cache.h"
#include "config.h"
#include "log.h"

CachePowerModel::CachePowerModel(string type, UInt32 size, UInt32 blocksize,
                                 UInt32 associativity, UInt32 delay, volatile float frequency)
{
   LOG_ASSERT_ERROR(Config::getSingleton()->getEnablePowerModeling(), "Power Modeling Disabled");
   CacheParams cache_params(type, size, blocksize, associativity, delay, frequency);
   McPATCache::getSingleton()->getPower(&cache_params, &_cache_power);

   _total_static_power = _cache_power._subthreshold_leakage_power + _cache_power._gate_leakage_power;
   _total_dynamic_energy = 0;
}

void
CachePowerModel::updateDynamicEnergy(UInt32 op)
{
   Cache::OperationType operation_type = (Cache::OperationType) op;
   switch (operation_type)
   {
   case Cache::TAG_ARRAY_READ:
      _total_dynamic_energy += _cache_power._tag_array_read_energy;
      break;
   case Cache::TAG_ARRAY_WRITE:
      _total_dynamic_energy += _cache_power._tag_array_write_energy;
      break;
   case Cache::DATA_ARRAY_READ:
      _total_dynamic_energy += _cache_power._data_array_read_energy;
      break;
   case Cache::DATA_ARRAY_WRITE:
      _total_dynamic_energy += _cache_power._data_array_write_energy;
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized Operation Type: %u", operation_type);
      break;
   }
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
