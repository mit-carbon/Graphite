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
   double leakage_energy;                 // (Subthreshold + Gate) Leakage Energy
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
   void setDVFS(double old_frequency, double new_voltage, double new_frequency, const Time& curr_time);

   // Compute Energy from McPAT
   void computeEnergy(const Time& curr_time, double frequency);

   // Collect Energy from McPAT
   double getDynamicEnergy();
   double getLeakageEnergy();

   // Output energy/area summary from McPAT
   void outputSummary(ostream& os, const Time& target_completion_time, double frequency);

private:
   // McPAT Objects
   typedef map<double,McPAT::CacheWrapper*> CacheWrapperMap;
   CacheWrapperMap _cache_wrapper_map;
   McPAT::CacheWrapper* _cache_wrapper;
   McPAT::ParseXML* _xml;
   // Performance model of cache
   Cache* _cache;
   // McPAT Output Data Structure
   mcpat_cache_output _mcpat_cache_out;
   // Last energy compute time
   Time _last_energy_compute_time;
   
   // Previous event counters
   UInt64 _prev_read_accesses;
   UInt64 _prev_write_accesses;
   UInt64 _prev_read_misses;
   UInt64 _prev_write_misses;
   UInt64 _prev_event_counters[Cache::NUM_OPERATION_TYPES];
   
   // Create core wrapper
   McPAT::CacheWrapper* createCacheWrapper(double voltage, double max_frequency_at_voltage);
   // Initialize XML Object
   void fillCacheParamsIntoXML(UInt32 technology_node, UInt32 temperature);
   void fillCacheStatsIntoXML(UInt64 interval_cycles);

   // Initialize event counters
   void initializeEventCounters();
   // Initialize/Update leakage/dynamic energy counters
   void initializeOutputDataStructure();
   void updateOutputDataStructure(double time_interval);

   // Display energy
   void displayEnergy(ostream& os, const Time& target_completion_time);
};
