#include <sstream>
#include <fstream>
#include <stdlib.h>
using namespace std;

#include "mcpat_cache.h"
#include "simulator.h"
#include "config.h"
#include "log.h"

McPATCache* McPATCache::_singleton = (McPATCache*) NULL;

void
McPATCache::allocate()
{
   assert(!_singleton);
   _singleton = new McPATCache();
}

void
McPATCache::release()
{
   assert(_singleton);
   delete _singleton;
}

McPATCache::McPATCache()
{
   try
   {
      _mcpat_home = Sim()->getCfg()->getString("general/McPAT_home");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read McPAT home location");
   }
   if (_mcpat_home == "/path/to/McPAT")
   {
      LOG_PRINT_ERROR("Enter Correct Path to McPAT installation (or) Set [general/enable_power_modeling] to false");
   }
}

McPATCache::~McPATCache()
{
   // Delete the Cache Information
   CacheInfoMap::iterator it = _cache_info_map.begin();
   for ( ; it != _cache_info_map.end(); it++)
   {
      delete (it->second).first;
      delete (it->second).second;
      delete it->first;
   }
}

void
McPATCache::getArea(CacheParams* cache_params, CacheArea* cache_area)
{
   bool found = false;
   CacheInfo cache_info = findCached(cache_params, found);
   if (found)
      *cache_area = *(cache_info.first);
   else
      *cache_area = *(runMcPAT(cache_params).first);
}

void
McPATCache::getPower(CacheParams* cache_params, CachePower* cache_power)
{
   bool found = false;
   CacheInfo cache_info = findCached(cache_params, found);
   if (found)
      *cache_power = *(cache_info.second);
   else
      *cache_power = *(runMcPAT(cache_params).second);
}

McPATCache::CacheInfo
McPATCache::findCached(CacheParams* cache_params, bool& found)
{
   CacheInfoMap::iterator it = _cache_info_map.begin();
   for ( ; it != _cache_info_map.end(); it++)
   {
      if ( (*(it->first)) == (*cache_params) )
      {
         found = true;
         return it->second;
      }
   }
   found = false;
   return make_pair<CacheArea*,CachePower*>(NULL,NULL);
}

McPATCache::CacheInfo
McPATCache::runMcPAT(CacheParams* cache_params_)
{
   CacheParams* cache_params = new CacheParams(*cache_params_);
   CacheArea* cache_area = new CacheArea();
   CachePower* cache_power = new CachePower();
 
   UInt32 num_read_accesses = 100000;
   UInt64 total_cycles = 100000;

   ostringstream cmd; 
   cmd << Sim()->getGraphiteHome() << "/common/mcpat/mcpat_cache_parser.py "
       << " --mcpat-home " << _mcpat_home
       << " --graphite-home " << Sim()->getGraphiteHome()
       << " --type " << cache_params->_type
       << " --size " << cache_params->_size
       << " --blocksize " << cache_params->_blocksize
       << " --associativity " << cache_params->_associativity
       << " --delay " << cache_params->_delay
       << " --frequency " << cache_params->_frequency
       << " --input-file default_input.xml" 
       << " --output-file mcpat.out"
       << " --read-accesses " << num_read_accesses
       << " --total-cycles " << total_cycles;

   // cerr << cmd.str() << endl;

   system((cmd.str()).c_str());

   ifstream mcpat_output((Sim()->getGraphiteHome() + "/common/mcpat/mcpat.out").c_str());

   mcpat_output >> cache_area->_area;
   
   __attribute(__unused__) double peak_dynamic_power;
   mcpat_output >> peak_dynamic_power;
   
   mcpat_output >> cache_power->_subthreshold_leakage_power;
   mcpat_output >> cache_power->_gate_leakage_power;
   
   double runtime_dynamic_power;
   mcpat_output >> runtime_dynamic_power;
   cache_power->_dynamic_energy = (runtime_dynamic_power / ((double)num_read_accesses)) *
                                  (((double)total_cycles) / (1e9 * cache_params->_frequency));

   // cerr << cache_area->_area << ", " << cache_power->_subthreshold_leakage_power << ", "
   //      << cache_power->_gate_leakage_power << ", " << cache_power->_dynamic_energy << endl;
   
   _cache_info_map.insert(make_pair<CacheParams*, CacheInfo>(
            cache_params, make_pair<CacheArea*, CachePower*>(cache_area, cache_power) ) );

   return make_pair<CacheArea*,CachePower*>(cache_area, cache_power);
}
