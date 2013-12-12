/*****************************************************************************
 * Tile Energy Monitor
 ***************************************************************************/

#pragma once

#include "packet_type.h"

// Forward Declarations
class Network;
class Core;
class MemoryManager;

//---------------------------------------------------------------------------
// Tile Energy Monitor
//---------------------------------------------------------------------------
class TileEnergyMonitor
{
public:
   // Tile Energy Monitor Constructor
   TileEnergyMonitor(Tile *tile);
   // Tile Energy Monitor Destructor
   ~TileEnergyMonitor();

   // Get Energy Information
   double getTileEnergy();

   // Periodically Collect Energy
   void periodicallyCollectEnergy();

   // Output Summary
   void outputSummary(ostream &out, const Time& target_completion_time);

private:
   // Parts of Tile Energy Monitor
   Tile* _tile;
   CoreModel* _core_model;
   Network* _network;
   MemoryManager* _memory_manager;
   tile_id_t _tile_id;

   // Collect Energy
   void collectEnergy(const Time& curr_time);

   // Power Trace
   void logCurrentTotalEnergyAndPower();

   // Power Trace Variables
   bool _power_trace_enabled;
   FILE *_power_trace_file;

   // Sampling Period
   Time _delta_t; // (in nanoseconds)

   // Time
   void initializeTimeCounters();

   // Time Counters
   Time _first_time;
   Time _previous_time;
   Time _current_time;
   Time _next_time;
   Time _last_time;
   Time _time_elapsed;
   UInt64 _counter;

   // Core Energy
   void initializeCoreEnergyCounters();
   //    Total
   void computeCoreEnergy();
   void collectCoreEnergy();
   void calculateCorePower();
   //    Dynamic
   void getCoreDynamicEnergy();
   void calculateCoreDynamicPower();
   //    Static
   void getCoreStaticEnergy();
   void calculateCoreStaticPower();

   // Core Energy Counters
   //    Total
   double _core_previous_total_energy;
   double _core_current_total_energy;
   double _core_current_total_power;
   //    Dynamic
   double _core_previous_dynamic_energy;
   double _core_current_dynamic_energy;
   double _core_current_dynamic_power;
   //    Static
   double _core_previous_static_energy;
   double _core_current_static_energy;
   double _core_current_static_power;

   // Cache Energy
   void initializeCacheEnergyCounters();
   //    Total
   void computeCacheEnergy();
   void collectCacheEnergy();
   void calculateCachePower();
   //    Dynamic
   void getCacheDynamicEnergy();
   void calculateCacheDynamicPower();
   //    Static
   void getCacheStaticEnergy();
   void calculateCacheStaticPower();

   // Cache Energy Counters
   //    Total
   double _cache_previous_total_energy;
   double _cache_current_total_energy;
   double _cache_current_total_power;
   //    Dynamic
   double _cache_previous_dynamic_energy;
   double _cache_current_dynamic_energy;
   double _cache_current_dynamic_power;
   //    Static
   double _cache_previous_static_energy;
   double _cache_current_static_energy;
   double _cache_current_static_power;

   // Nework Energy
   void initializeNetworkEnergyCounters();
   //    Total
   void computeNetworkEnergy();
   void collectNetworkEnergy();
   void calculateNetworkPower();
   //    Dynamic
   void getNetworkDynamicEnergy();
   void calculateNetworkDynamicPower();
   //    Static
   void getNetworkStaticEnergy();
   void calculateNetworkStaticPower();

   // Network Energy Counters
   //    Total
   double _network_previous_total_energy[NUM_STATIC_NETWORKS];
   double _network_current_total_energy[NUM_STATIC_NETWORKS];
   double _network_current_total_power[NUM_STATIC_NETWORKS];
   //    Dynamic
   double _network_previous_dynamic_energy[NUM_STATIC_NETWORKS];
   double _network_current_dynamic_energy[NUM_STATIC_NETWORKS];
   double _network_current_dynamic_power[NUM_STATIC_NETWORKS];
   //    Static
   double _network_previous_static_energy[NUM_STATIC_NETWORKS];
   double _network_current_static_energy[NUM_STATIC_NETWORKS];
   double _network_current_static_power[NUM_STATIC_NETWORKS];
};
