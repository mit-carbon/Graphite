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
#include <cmath>

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

   // Initialize Time Counters
   initializeTimeCounters();

   // Initialize Energy Counters
   initializeCoreEnergyCounters();

   // Set First Time Variable
   // Check Core Cycle Count
   UInt64 m_cycle_count = m_core_model->getCycleCount();
   // Convert from Tile Clock to Global Clock
   m_first_time = convertCycleCount(m_cycle_count, m_tile->getFrequency(), 1.0);
}

//---------------------------------------------------------------------------
// Tile Energy Monitor Destructor
//---------------------------------------------------------------------------
TileEnergyMonitor::~TileEnergyMonitor() {}

//---------------------------------------------------------------------------
// Periodically Collect Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::periodicallyCollectEnergy()
{
   // Check Core Cycle Count
   UInt64 m_cycle_count = m_core_model->getCycleCount();
   // Convert from Tile Clock to Global Clock
   m_current_time = convertCycleCount(m_cycle_count, m_tile->getFrequency(), 1.0);

   // Check if the Next Time has been reached
   if (m_current_time >= m_next_time)
   {
      // Calculate Time Elapsed from Previous Time
      m_time_elapsed = m_current_time - m_previous_time;
      // Increment Counter
      m_counter = m_counter + 1;

      // Compute Energy
      computeCoreEnergy();

      // Collect Energy
      collectCoreEnergy();

      // Calculate Power
      calculateCorePower();

      // Update the Previous Time
      m_previous_time = m_current_time;
      // Update the Next Time
      m_next_time = m_current_time + m_delta_t;

      /*cout  << "Tile " << m_tile->getId()
            << " \t| Time Variables:"
            << " \tm_first_time = "      << m_first_time
            << " \tm_previous_time = "   << m_previous_time
            << " \tm_current_time = "    << m_current_time
            << " \tm_next_time = "       << m_next_time
            << " \tm_last_time = "       << m_last_time
            << " \tm_time_elapsed = "    << m_time_elapsed
            << " \t| Counter:"
            << " \tm_counter = "         << m_counter
            << " \t| Core Energy Counters:"
            << " \tm_core_current_total_energy = "   << m_core_current_total_energy
            << " \tm_core_previous_total_energy = "  << m_core_previous_total_energy
            << " \tm_core_current_total_power = "    << m_core_current_total_power
            << " \tm_core_total_dynamic_energy = "   << m_core_total_dynamic_energy
            << " \tm_core_static_power = "           << m_core_static_power
            << " \tm_core_total_static_energy = "    << m_core_total_static_energy
            << endl;*/
   }
}

//---------------------------------------------------------------------------
// Initialize Time Counters
//---------------------------------------------------------------------------
void TileEnergyMonitor::initializeTimeCounters()
{
   m_first_time = 0;
   m_previous_time = 0;
   m_current_time = 0;
   m_next_time = 0;
   m_last_time = 0;
   m_time_elapsed = 0;
   m_counter = 0;
}

//---------------------------------------------------------------------------
// Initialize Core Energy Counters
//---------------------------------------------------------------------------
void TileEnergyMonitor::initializeCoreEnergyCounters()
{
   m_core_current_total_energy = 0;
   m_core_previous_total_energy = 0;
   m_core_current_total_power = 0;
   m_core_total_dynamic_energy = 0;
   m_core_static_power = 0;
   m_core_total_static_energy = 0;
}

//---------------------------------------------------------------------------
// Compute Core Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::computeCoreEnergy()
{
   // Compute Energy
   m_core_model->computeEnergy();
}

//---------------------------------------------------------------------------
// Collect Core Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::collectCoreEnergy()
{
   getCoreDynamicEnergy();
   getCoreStaticPower();
   getCoreStaticEnergy();

   // Store the Previous Total Energy
   m_core_previous_total_energy = m_core_current_total_energy;
   // Calculate the Current Total Energy
   m_core_current_total_energy = m_core_total_dynamic_energy + m_core_total_static_energy;
}

//---------------------------------------------------------------------------
// Calculate Core Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateCorePower()
{
   m_core_current_total_power = (m_core_current_total_energy -
                                 m_core_previous_total_energy)/
                                (m_time_elapsed*1E-9);
}

//---------------------------------------------------------------------------
// Get Core Dynamic Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getCoreDynamicEnergy()
{
   m_core_total_dynamic_energy = m_core_model->getDynamicEnergy();

   if (isnan(m_core_total_dynamic_energy)) m_core_total_dynamic_energy = 0;
}

//---------------------------------------------------------------------------
// Get Core Static Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::getCoreStaticPower()
{
   m_core_static_power = m_core_model->getStaticPower();

   if (isnan(m_core_static_power)) m_core_static_power = 0;
}

//---------------------------------------------------------------------------
// Get Core Static Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getCoreStaticEnergy()
{
   m_core_total_static_energy = m_core_total_static_energy + 
                         (m_core_static_power*m_time_elapsed*1E-9);
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
   m_last_time = convertCycleCount(m_cycle_count, m_tile->getFrequency(), 1.0);

   periodicallyCollectEnergy();

   out << "Tile Energy Monitor Summary: " << endl;
   out << "  First Time (in ns): " << m_first_time << std::endl;
   out << "  Last Time (in ns): " << m_last_time << std::endl;
   out << "  Delta t (in ns): " << m_delta_t << std::endl;
   out << "  Counter: " << m_counter << std::endl;
   out << "  Current Time (in ns): " << m_current_time << std::endl;
   out << "  Previous Time (in ns): " << m_previous_time << std::endl;
   out << "  Time Elapsed (in ns): " << m_time_elapsed << std::endl;
   out << "  Core Model: " << endl;
   out << "    Dynamic Energy (in J): " << m_core_total_dynamic_energy << std::endl;
   out << "    Static Power (in W): " << m_core_static_power << std::endl;
   out << "    Static Energy (in J): " << m_core_total_static_energy << std::endl;
   out << "    Total Energy (in J): " << m_core_current_total_energy << std::endl;
   out << "    Current Power (in W): " << m_core_current_total_power << std::endl;
}
