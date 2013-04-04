/*****************************************************************************
 * Graphite-McPAT Cache Interface
 ***************************************************************************/

#pragma once

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

   // Compute Energy from McPAT
   void computeEnergy(Cache* cache);

   // Collect Energy from McPAT
   double getDynamicEnergy();
   double getStaticPower();

   // Output energy/area summary from McPAT
   void outputSummary(ostream& os);

private:
   // McPAT Objects
   McPAT::ParseXML* _xml;
   McPAT::CacheWrapper* _cache_wrapper;
   // McPAT Output Data Structure
   mcpat_cache_output _mcpat_cache_out;
   
   // Initialize XML Object
   void fillCacheParamsIntoXML(Cache* cache, UInt32 technology_node, UInt32 temperature);
   void fillCacheStatsIntoXML(Cache* cache);
};
