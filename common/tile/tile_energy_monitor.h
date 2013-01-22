/*****************************************************************************
 * Tile Energy Monitor
 ***************************************************************************/

#pragma once

using namespace std;

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

   // Periodically Increment Counter
   void periodicallyIncrementCounter();
   // Increment Counter
   void incrementCounter();

   // Output Summary
   void outputSummary(std::ostream &out);

private:
   // Parts of Tile Energy Monitor
   Tile* m_tile;
   Core* m_core;
   CoreModel* m_core_model;
   Network* m_network;
   MemoryManager* m_memory_manager;

   // Time Variables
   UInt64 m_first_time;
   UInt64 m_previous_time;
   UInt64 m_current_time;
   UInt64 m_next_time;
   UInt64 m_last_time;
   static const UInt64 m_delta_t = 1000; // (in ns)

   // Counter to Increment Periodically
   UInt64 m_counter;
};
