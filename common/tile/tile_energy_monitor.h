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

   // Get Parts of Tile Energy Monitor
   Tile* getTile()               const { return m_tile; }
   Core* getCore()                     { return m_core; }
   CoreModel* getCoreModel()           { return m_core_model; }
   Network* getNetwork()               { return m_network; }
   MemoryManager* getMemoryManager()   { return m_memory_manager; }

   // Get Energy Information
   double getTileEnergy();

   // Periodically Collect Energy
   void periodicallyCollectEnergy();

   // Output Summary
   void outputSummary(std::ostream &out);

private:
   // Parts of Tile Energy Monitor
   tile_id_t m_tile_id;
   Tile* m_tile;
   Core* m_core;
   CoreModel* m_core_model;
   Network* m_network;
   MemoryManager* m_memory_manager;

   // Power Trace
   void logCurrentTotalEnergyAndPower();

   // Power Trace Variables
   bool m_power_trace_enabled;
   FILE *m_power_trace_file;

   // Sampling Period
   double m_delta_t; // (in nanoseconds)

   // Time
   void initializeTimeCounters();

   // Time Counters
   double m_first_time;
   double m_previous_time;
   double m_current_time;
   double m_next_time;
   double m_last_time;
   double m_time_elapsed;
   double m_counter;

   // Core Energy
   void initializeCoreEnergyCounters();
   //    Total
   void computeCoreEnergy(Time& curr_time);
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
   double m_core_previous_total_energy;
   double m_core_current_total_energy;
   double m_core_current_total_power;
   //    Dynamic
   double m_core_previous_dynamic_energy;
   double m_core_current_dynamic_energy;
   double m_core_current_dynamic_power;
   //    Static
   double m_core_previous_static_energy;
   double m_core_current_static_energy;
   double m_core_current_static_power;

   // Cache Energy
   void initializeCacheEnergyCounters();
   //    Total
   void computeCacheEnergy(Time& curr_time);
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
   double m_cache_previous_total_energy;
   double m_cache_current_total_energy;
   double m_cache_current_total_power;
   //    Dynamic
   double m_cache_previous_dynamic_energy;
   double m_cache_current_dynamic_energy;
   double m_cache_current_dynamic_power;
   //    Static
   double m_cache_previous_static_energy;
   double m_cache_current_static_energy;
   double m_cache_current_static_power;

   // Nework Energy
   void initializeNetworkEnergyCounters();
   //    Total
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
   double m_network_previous_total_energy[NUM_STATIC_NETWORKS];
   double m_network_current_total_energy[NUM_STATIC_NETWORKS];
   double m_network_current_total_power[NUM_STATIC_NETWORKS];
   //    Dynamic
   double m_network_previous_dynamic_energy[NUM_STATIC_NETWORKS];
   double m_network_current_dynamic_energy[NUM_STATIC_NETWORKS];
   double m_network_current_dynamic_power[NUM_STATIC_NETWORKS];
   //    Static
   double m_network_previous_static_energy[NUM_STATIC_NETWORKS];
   double m_network_current_static_energy[NUM_STATIC_NETWORKS];
   double m_network_current_static_power[NUM_STATIC_NETWORKS];
};
