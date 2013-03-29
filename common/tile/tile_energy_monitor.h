/*****************************************************************************
 * Tile Energy Monitor
 ***************************************************************************/

#pragma once

using namespace std;

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
   double m_delta_t; // (in ns)

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
   void computeCoreEnergy();
   void collectCoreEnergy();
   void calculateCorePower();
   void getCoreDynamicEnergy();
   void getCoreStaticPower();
   void getCoreStaticEnergy();

   // Core Energy Counters
   double m_core_current_total_energy;
   double m_core_previous_total_energy;
   double m_core_current_total_power;
   double m_core_total_dynamic_energy;
   double m_core_static_power;
   double m_core_total_static_energy;

   // Cache Energy
   void initializeCacheEnergyCounters();
   void computeCacheEnergy();
   void collectCacheEnergy();
   void calculateCachePower();
   void getCacheDynamicEnergy();
   void getCacheStaticPower();
   void getCacheStaticEnergy();

   // Cache Energy Counters
   double m_cache_current_total_energy;
   double m_cache_previous_total_energy;
   double m_cache_current_total_power;
   double m_cache_total_dynamic_energy;
   double m_cache_static_power;
   double m_cache_total_static_energy;

   // L1-I Cache Energy Counters
   double m_l1i_cache_current_total_energy;
   double m_l1i_cache_previous_total_energy;
   double m_l1i_cache_current_total_power;
   double m_l1i_cache_total_dynamic_energy;
   double m_l1i_cache_static_power;
   double m_l1i_cache_total_static_energy;

   // L1-D Cache Energy Counters
   double m_l1d_cache_current_total_energy;
   double m_l1d_cache_previous_total_energy;
   double m_l1d_cache_current_total_power;
   double m_l1d_cache_total_dynamic_energy;
   double m_l1d_cache_static_power;
   double m_l1d_cache_total_static_energy;

   // L2 Cache Energy Counters
   double m_l2_cache_current_total_energy;
   double m_l2_cache_previous_total_energy;
   double m_l2_cache_current_total_power;
   double m_l2_cache_total_dynamic_energy;
   double m_l2_cache_static_power;
   double m_l2_cache_total_static_energy;

   // Nework Energy
   void initializeNetworkEnergyCounters();
   void collectNetworkEnergy();
   void calculateNetworkPower();
   void getNetworkDynamicEnergy();
   void getNetworkStaticPower();
   void getNetworkStaticEnergy();

   // Network Energy Counters
   double m_network_current_total_energy[NUM_STATIC_NETWORKS];
   double m_network_previous_total_energy[NUM_STATIC_NETWORKS];
   double m_network_current_total_power[NUM_STATIC_NETWORKS];
   double m_network_total_dynamic_energy[NUM_STATIC_NETWORKS];
   double m_network_static_power[NUM_STATIC_NETWORKS];
   double m_network_total_static_energy[NUM_STATIC_NETWORKS];
};
