/*****************************************************************************
 * Tile Energy Monitor
 ***************************************************************************/

#include "tile.h"
#include "network.h"
#include "network_model.h"
#include "network_types.h"
#include "memory_manager.h"
#include "main_core.h"
#include "simulator.h"
#include "log.h"
#include "tile_energy_monitor.h"
#include "clock_converter.h"
#include "core_model.h"

//---------------------------------------------------------------------------
// Tile Energy Monitor Constructor
//---------------------------------------------------------------------------
TileEnergyMonitor::TileEnergyMonitor(Tile *tile)
   : m_tile(tile)
   , m_memory_manager(NULL)
{
   // Get Parts of Tile Energy Monitor
   m_core = m_tile->getCore();
   m_core_model = m_core->getModel();
   m_network = m_tile->getNetwork();
   if (Config::getSingleton()->isSimulatingSharedMemory())
      m_memory_manager = m_tile->getMemoryManager();

   // Initialize Next Time Variable
   m_next_time = m_delta_t;

   // Initialize Counter
   m_counter = 0;

   // Set First Time Variable
   // Check Core Cycle Count
   UInt64 m_cycle_count = m_core_model->getCycleCount();
   // Convert from Tile Clock to Global Clock
   m_first_time = convertCycleCount(m_cycle_count, m_core_model->getFrequency(), 1.0);
}

//---------------------------------------------------------------------------
// Tile Energy Monitor Destructor
//---------------------------------------------------------------------------
TileEnergyMonitor::~TileEnergyMonitor() {}

//---------------------------------------------------------------------------
// Periodically Increment Counter
//---------------------------------------------------------------------------
void TileEnergyMonitor::periodicallyIncrementCounter()
{
   // Check Core Cycle Count
   UInt64 m_cycle_count = m_core_model->getCycleCount();
   // Convert from Tile Clock to Global Clock
   m_current_time = convertCycleCount(m_cycle_count, m_core_model->getFrequency(), 1.0);

   // Check if the Next Time has been reached
   if (m_current_time >= m_next_time)
   {
      // If so, then Increment Counter
      incrementCounter();
      // Update the Previous Time
      m_previous_time = m_current_time;
      // Update the Next Time
      m_next_time = m_current_time + m_delta_t;
   }
}

//---------------------------------------------------------------------------
// Increment Counter
//---------------------------------------------------------------------------
void TileEnergyMonitor::incrementCounter()
{
   m_counter = m_counter + 1;
}

//---------------------------------------------------------------------------
// Output Summary
//---------------------------------------------------------------------------
void TileEnergyMonitor::outputSummary(std::ostream &out)
{
   // Set Last Time Variable
   // Check Core Cycle Count
   UInt64 m_cycle_count = m_core_model->getCycleCount();
   // Convert from Tile Clock to Global Clock
   m_last_time = convertCycleCount(m_cycle_count, m_core_model->getFrequency(), 1.0);

   out << "Tile Energy Monitor Summary: " << endl;
   out << "  First Time (in ns): " << m_first_time << std::endl;
   out << "  Last Time (in ns): " << m_last_time << std::endl;
   out << "  Delta t (in ns): " << m_delta_t << std::endl;
   out << "  Counter: " << m_counter << std::endl;
}
