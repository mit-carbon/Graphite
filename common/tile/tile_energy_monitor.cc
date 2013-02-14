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
#include <stdio.h>

//---------------------------------------------------------------------------
// Tile Energy Monitor Constructor
//---------------------------------------------------------------------------
TileEnergyMonitor::TileEnergyMonitor(Tile *tile)
   : m_tile(tile)
   , m_memory_manager(NULL)
{
   // Get Parts of Tile Energy Monitor
   m_tile_id = tile->getId();
   m_core = m_tile->getCore();
   m_core_model = m_core->getModel();
   m_network = m_tile->getNetwork();
   if (Config::getSingleton()->isSimulatingSharedMemory())
      m_memory_manager = m_tile->getMemoryManager();

   // Initialize Delta T Variable
   m_delta_t = Sim()->getCfg()->getInt("runtime_energy_modeling/interval");

   // Initialize Power Trace
   m_power_trace_enabled = Sim()->getCfg()->getBool("runtime_energy_modeling/power_trace/enabled");
   if (m_power_trace_enabled)
   {
      if (m_tile_id < (tile_id_t) Sim()->getConfig()->getApplicationTiles())
      {
         // Not Thread Spawner Tile / MCP
         char m_filename[256];
         sprintf(m_filename, "%s_%d.csv", "power_trace_tile", m_tile_id);
         m_power_trace_file = fopen(Config::getSingleton()->formatOutputFileName(m_filename).c_str(),"w");
         fprintf(m_power_trace_file,\
            "Time %d, Core Energy %d, Core Power %d, Cache Energy %d, Cache Power %d, Network Energy %d, Network Power %d\n",\
            m_tile_id, m_tile_id, m_tile_id, m_tile_id, m_tile_id, m_tile_id, m_tile_id);
      }
   }

   // Initialize Next Time Variable
   m_next_time = m_delta_t;

   // Initialize Time Counters
   initializeTimeCounters();

   // Initialize Energy Counters
   initializeCoreEnergyCounters();
   initializeCacheEnergyCounters();
   initializeNetworkEnergyCounters();

   // Set First Time Variable
   // Check Core Cycle Count
   UInt64 m_cycle_count = m_core_model->getCycleCount();
   // Convert from Tile Clock to Global Clock
   m_first_time = convertCycleCount(m_cycle_count, m_tile->getFrequency(), 1.0);
}

//---------------------------------------------------------------------------
// Tile Energy Monitor Destructor
//---------------------------------------------------------------------------
TileEnergyMonitor::~TileEnergyMonitor() 
{
   if (m_power_trace_enabled)
   {
      if (m_tile_id < (tile_id_t) Sim()->getConfig()->getApplicationTiles())
      {
         // Not Thread Spawner Tile / MCP
         fclose(m_power_trace_file);
      }
   }
}

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
      computeCacheEnergy();

      // Collect Energy
      collectCoreEnergy();
      collectCacheEnergy();
      collectNetworkEnergy();

      // Calculate Power
      calculateCorePower();
      calculateCachePower();
      calculateNetworkPower();

      // Update the Previous Time
      m_previous_time = m_current_time;
      // Update the Next Time
      m_next_time = m_current_time + m_delta_t;

      // Power Trace
      if (m_power_trace_enabled)
      {
         if (m_tile_id < (tile_id_t) Sim()->getConfig()->getApplicationTiles())
         {
            // Not Thread Spawner Tile / MCP// Log Current Total Energy and Power
            logCurrentTotalEnergyAndPower();
         }
      }
   }
}

//---------------------------------------------------------------------------
// Log Current Total Energy and Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::logCurrentTotalEnergyAndPower()
{
   fprintf(m_power_trace_file,\
      "%g, %g, %g, %g, %g, %g, %g\n", \
      (double) m_current_time,\
      (double) m_core_current_total_energy,\
      (double) m_core_current_total_power,\
      (double) m_cache_current_total_energy,\
      (double) m_cache_current_total_power,\
      (double) m_network_current_total_energy[2],\
      (double) m_network_current_total_power[2]);
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
// Initialize Cache Energy Counters
//---------------------------------------------------------------------------
void TileEnergyMonitor::initializeCacheEnergyCounters()
{
   m_cache_current_total_energy = 0;
   m_cache_previous_total_energy = 0;
   m_cache_current_total_power = 0;
   m_cache_total_dynamic_energy = 0;
   m_cache_static_power = 0;
   m_cache_total_static_energy = 0;

   /*m_l1i_cache_current_total_energy = 0;
   m_l1i_cache_previous_total_energy = 0;
   m_l1i_cache_current_total_power = 0;
   m_l1i_cache_total_dynamic_energy = 0;
   m_l1i_cache_static_power = 0;
   m_l1i_cache_total_static_energy = 0;

   m_l1d_cache_current_total_energy = 0;
   m_l1d_cache_previous_total_energy = 0;
   m_l1d_cache_current_total_power = 0;
   m_l1d_cache_total_dynamic_energy = 0;
   m_l1d_cache_static_power = 0;
   m_l1d_cache_total_static_energy = 0;

   m_l2_cache_current_total_energy = 0;
   m_l2_cache_previous_total_energy = 0;
   m_l2_cache_current_total_power = 0;
   m_l2_cache_total_dynamic_energy = 0;
   m_l2_cache_static_power = 0;
   m_l2_cache_total_static_energy = 0;*/
}

//---------------------------------------------------------------------------
// Compute Cache Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::computeCacheEnergy()
{
   // Compute Energy
   if (m_memory_manager)
   {
      m_tile->getMemoryManager()->computeEnergy();
   }
}

//---------------------------------------------------------------------------
// Collect Cache Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::collectCacheEnergy()
{
   getCacheDynamicEnergy();
   getCacheStaticPower();
   getCacheStaticEnergy();

   // Store the Previous Total Energy
   m_cache_previous_total_energy = m_cache_current_total_energy;
   /*m_l1i_cache_previous_total_energy = m_l1i_cache_current_total_energy;
   m_l1d_cache_previous_total_energy = m_l1d_cache_current_total_energy;
   m_l2_cache_previous_total_energy = m_l2_cache_current_total_energy;*/
   // Calculate the Current Total Energy
   m_cache_current_total_energy = m_cache_total_dynamic_energy + m_cache_total_static_energy;
   /*m_l1i_cache_current_total_energy = m_l1i_cache_total_dynamic_energy + m_l1i_cache_total_static_energy;
   m_l1d_cache_current_total_energy = m_l1d_cache_total_dynamic_energy + m_l1d_cache_total_static_energy;
   m_l2_cache_current_total_energy = m_l2_cache_total_dynamic_energy + m_l2_cache_total_static_energy;*/
}

//---------------------------------------------------------------------------
// Calculate Cache Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateCachePower()
{
   m_cache_current_total_power = (m_cache_current_total_energy -
                                      m_cache_previous_total_energy)/
                                     (m_time_elapsed*1E-9);
   /*m_l1i_cache_current_total_power = (m_l1i_cache_current_total_energy -
                                      m_l1i_cache_previous_total_energy)/
                                     (m_time_elapsed*1E-9);
   m_l1d_cache_current_total_power = (m_l1d_cache_current_total_energy -
                                      m_l1d_cache_previous_total_energy)/
                                     (m_time_elapsed*1E-9);
   m_l2_cache_current_total_power = (m_l2_cache_current_total_energy -
                                     m_l2_cache_previous_total_energy)/
                                    (m_time_elapsed*1E-9);*/
}

//---------------------------------------------------------------------------
// Get Cache Dynamic Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getCacheDynamicEnergy()
{
   m_cache_total_dynamic_energy = m_tile->getMemoryManager()->getDynamicEnergy();
   /*m_l1i_cache_total_dynamic_energy = m_tile->getMemoryManager()->getL1ICache()->getDynamicEnergy();
   m_l1d_cache_total_dynamic_energy = m_tile->getMemoryManager()->getL1DCache()->getDynamicEnergy();
   m_l2_cache_total_dynamic_energy = m_tile->getMemoryManager()->getL2Cache()->getDynamicEnergy();*/

   if (isnan(m_cache_total_dynamic_energy)) m_cache_total_dynamic_energy = 0;
   /*if (isnan(m_l1i_cache_total_dynamic_energy)) m_l1i_cache_total_dynamic_energy = 0;
   if (isnan(m_l1d_cache_total_dynamic_energy)) m_l1d_cache_total_dynamic_energy = 0;
   if (isnan(m_l2_cache_total_dynamic_energy)) m_l2_cache_total_dynamic_energy = 0;*/
}

//---------------------------------------------------------------------------
// Get Cache Static Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::getCacheStaticPower()
{
   m_cache_static_power = m_tile->getMemoryManager()->getStaticPower();
   /*m_l1i_cache_static_power = m_tile->getMemoryManager()->getL1ICache()->getStaticPower();
   m_l1d_cache_static_power = m_tile->getMemoryManager()->getL1DCache()->getStaticPower();
   m_l2_cache_static_power = m_tile->getMemoryManager()->getL2Cache()->getStaticPower();*/

   if (isnan(m_cache_static_power)) m_cache_static_power = 0;
   /*if (isnan(m_l1i_cache_static_power)) m_l1i_cache_static_power = 0;
   if (isnan(m_l1d_cache_static_power)) m_l1d_cache_static_power = 0;
   if (isnan(m_l2_cache_static_power)) m_l2_cache_static_power = 0;*/
}

//---------------------------------------------------------------------------
// Get Cache Static Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getCacheStaticEnergy()
{
   m_cache_total_static_energy = m_cache_total_static_energy + 
                                    (m_cache_static_power*m_time_elapsed*1E-9);
   /*m_l1i_cache_total_static_energy = m_l1i_cache_total_static_energy + 
                                    (m_l1i_cache_static_power*m_time_elapsed*1E-9);
   m_l1d_cache_total_static_energy = m_l1d_cache_total_static_energy + 
                                    (m_l1d_cache_static_power*m_time_elapsed*1E-9);
   m_l2_cache_total_static_energy = m_l2_cache_total_static_energy + 
                                   (m_l2_cache_static_power*m_time_elapsed*1E-9);*/
}

//---------------------------------------------------------------------------
// Initialize Network Energy Counters
//---------------------------------------------------------------------------
void TileEnergyMonitor::initializeNetworkEnergyCounters()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      m_network_current_total_energy[i] = 0;
      m_network_previous_total_energy[i] = 0;
      m_network_current_total_power[i] = 0;
      m_network_total_dynamic_energy[i] = 0;
      m_network_static_power[i] = 0;
      m_network_total_static_energy[i] = 0;
   }
}

//---------------------------------------------------------------------------
// Collect Network Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::collectNetworkEnergy()
{
   getNetworkDynamicEnergy();
   getNetworkStaticPower();
   getNetworkStaticEnergy();

   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      // Store the Previous Total Energy
      m_network_previous_total_energy[i] = m_network_current_total_energy[i];
      // Calculate the Current Total Energy
      m_network_current_total_energy[i] = m_network_total_dynamic_energy[i] +
                                          m_network_total_static_energy[i];
   }
}

//---------------------------------------------------------------------------
// Calculate Network Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateNetworkPower()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      m_network_current_total_power[i] = (m_network_current_total_energy[i] -
                                          m_network_previous_total_energy[i])/
                                         (m_time_elapsed*1E-9);
   }
}

//---------------------------------------------------------------------------
// Get Network Dynamic Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getNetworkDynamicEnergy()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      m_network_total_dynamic_energy[i] = m_network->getNetworkModel(i)->getDynamicEnergy();
   }
}

//---------------------------------------------------------------------------
// Get Network Static Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::getNetworkStaticPower()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      m_network_static_power[i] = m_network->getNetworkModel(i)->getStaticPower();
   }
}

//---------------------------------------------------------------------------
// Get Network Static Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getNetworkStaticEnergy()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      m_network_total_static_energy[i] = m_network_total_static_energy[i] + 
                                  (m_network_static_power[i]*m_time_elapsed*1E-9);
   }
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

   if (m_tile_id < (tile_id_t) Sim()->getConfig()->getApplicationTiles())
   {
      // Not Thread Spawner Tile / MCP
      periodicallyCollectEnergy();

      out << "Tile Energy Monitor Summary: " << endl;
      out << "  First Time (in ns): " << m_first_time << std::endl;
      out << "  Last Time (in ns): " << m_last_time << std::endl;
      out << "  Delta t (in ns): " << m_delta_t << std::endl;
      out << "  Counter: " << m_counter << std::endl;
      out << "  Current Time (in ns): " << m_current_time << std::endl;
      out << "  Previous Time (in ns): " << m_previous_time << std::endl;
      out << "  Time Elapsed (in ns): " << m_time_elapsed << std::endl;
      out << "  Core: " << endl;
      out << "    Dynamic Energy (in J): " << m_core_total_dynamic_energy << std::endl;
      out << "    Static Power (in W): " << m_core_static_power << std::endl;
      out << "    Static Energy (in J): " << m_core_total_static_energy << std::endl;
      out << "    Total Energy (in J): " << m_core_current_total_energy << std::endl;
      out << "    Current Power (in W): " << m_core_current_total_power << std::endl;
      out << "  Cache: " << endl;
      out << "    Dynamic Energy (in J): " << m_cache_total_dynamic_energy << std::endl;
      out << "    Static Power (in W): " << m_cache_static_power << std::endl;
      out << "    Static Energy (in J): " << m_cache_total_static_energy << std::endl;
      out << "    Total Energy (in J): " << m_cache_current_total_energy << std::endl;
      out << "    Current Power (in W): " << m_cache_current_total_power << std::endl;
      /*out << "  Cache L1-I: " << endl;
      out << "    Dynamic Energy (in J): " << m_l1i_cache_total_dynamic_energy << std::endl;
      out << "    Static Power (in W): " << m_l1i_cache_static_power << std::endl;
      out << "    Static Energy (in J): " << m_l1i_cache_total_static_energy << std::endl;
      out << "    Total Energy (in J): " << m_l1i_cache_current_total_energy << std::endl;
      out << "    Current Power (in W): " << m_l1i_cache_current_total_power << std::endl;
      out << "  Cache L1-D: " << endl;
      out << "    Dynamic Energy (in J): " << m_l1d_cache_total_dynamic_energy << std::endl;
      out << "    Static Power (in W): " << m_l1d_cache_static_power << std::endl;
      out << "    Static Energy (in J): " << m_l1d_cache_total_static_energy << std::endl;
      out << "    Total Energy (in J): " << m_l1d_cache_current_total_energy << std::endl;
      out << "    Current Power (in W): " << m_l1d_cache_current_total_power << std::endl;
      out << "  Cache L2: " << endl;
      out << "    Dynamic Energy (in J): " << m_l2_cache_total_dynamic_energy << std::endl;
      out << "    Static Power (in W): " << m_l2_cache_static_power << std::endl;
      out << "    Static Energy (in J): " << m_l2_cache_total_static_energy << std::endl;
      out << "    Total Energy (in J): " << m_l2_cache_current_total_energy << std::endl;
      out << "    Current Power (in W): " << m_l2_cache_current_total_power << std::endl;*/
      for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
      {
         out << "  Network Model " << i << ": " << endl;
         out << "    Dynamic Energy (in J): " << m_network_total_dynamic_energy[i] << std::endl;
         out << "    Static Power (in W): " << m_network_static_power[i] << std::endl;
         out << "    Static Energy (in J): " << m_network_total_static_energy[i] << std::endl;
         out << "    Total Energy (in J): " << m_network_current_total_energy[i] << std::endl;
         out << "    Current Power (in W): " << m_network_current_total_power[i] << std::endl;
      }
   }
}
