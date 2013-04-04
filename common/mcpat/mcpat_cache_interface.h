/*****************************************************************************
 * Graphite-McPAT Cache Interface
 ***************************************************************************/

#pragma once

#include <map>
using std::map;
#include "contrib/mcpat/mcpat.h"
#include "cache.h"

//---------------------------------------------------------------------------
// McPAT Cache Interface Data Structures for Area and Power
//---------------------------------------------------------------------------
typedef struct
{
   double area;                           // Area
   double leakage_power;                  // Leakage Power
   double dynamic_energy;                 // Runtime Dynamic Energy
} mcpat_cache_output;

class McPATCacheInterface
{
public:
   // McPAT Cache Interface Constructor
   McPATCacheInterface(Cache* cache);
   // McPAT Cache Interface Destructor
   ~McPATCacheInterface();

   // Set DVFS
   void setDVFS(double voltage, double frequency);

   // Compute Energy from McPAT
   void computeEnergy(Cache* cache);

   // Collect Energy from McPAT
   double getDynamicEnergy();
   double getStaticPower();

   // Output energy/area summary from McPAT
   void outputSummary(ostream& os);

private:
   // McPAT Objects
   map<double,McPAT::CacheWrapper*> _cache_wrapper_map;
   McPAT::CacheWrapper* _cache_wrapper;
   McPAT::ParseXML* _xml;
   // McPAT Output Data Structure
   mcpat_cache_output _mcpat_cache_out;
   
   // Nominal voltage and max frequency at nominal voltage
   double _nominal_voltage;
   double _base_frequency;

   // Create core wrapper
   McPAT::CacheWrapper* createCacheWrapper(double voltage, double max_frequency_at_voltage);
   // Initialize XML Object
   void fillCacheParamsIntoXML(Cache* cache, UInt32 technology_node, UInt32 temperature);
   void fillCacheStatsIntoXML(Cache* cache);
};
