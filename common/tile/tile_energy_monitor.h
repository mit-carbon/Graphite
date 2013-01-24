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
   Tile* m_tile;
   Core* m_core;
   CoreModel* m_core_model;
   Network* m_network;
   MemoryManager* m_memory_manager;

   // Sampling Period
   static const double m_delta_t = 1000; // (in ns)

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
};
