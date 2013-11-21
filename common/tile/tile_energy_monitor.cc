#include "tile.h"
#include "network.h"
#include "network_model.h"
#include "network_types.h"
#include "memory_manager.h"
#include "main_core.h"
#include "simulator.h"
#include "log.h"
#include "tile_energy_monitor.h"
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
   m_delta_t = (Sim()->getCfg()->getInt("runtime_energy_modeling/interval"))*1E-9;

   // Initialize Time Counters
   initializeTimeCounters();

   // Initialize Next Time Variable
   m_next_time = m_delta_t;

   // Initialize Energy Counters
   initializeCoreEnergyCounters();
   initializeCacheEnergyCounters();
   initializeNetworkEnergyCounters();

   // Initialize First Time Variable
   m_first_time = m_core_model->getCurrTime().toSec();

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
         // Log Initial Energy and Power
         logCurrentTotalEnergyAndPower();
      }
   }
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
// Return Tile Energy Information
//---------------------------------------------------------------------------
double TileEnergyMonitor::getTileEnergy()
{
   double tile_energy = m_core_current_total_energy + m_cache_current_total_energy;
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      tile_energy += m_network_current_total_energy[i];
   }
   return tile_energy;
}

//---------------------------------------------------------------------------
// Periodically Collect Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::periodicallyCollectEnergy()
{
   // Check Core Cycle Count
   m_current_time = m_core_model->getCurrTime().toSec();

   // Check if the Next Time has been reached
   if (m_current_time >= m_next_time)
   {
      // Calculate Time Elapsed from Previous Time
      m_time_elapsed = m_current_time - m_previous_time;
      // Increment Counter
      m_counter = m_counter + 1;

      // Compute Energy
      Time curr_time = m_core_model->getCurrTime();
      computeCoreEnergy(curr_time);
      computeCacheEnergy(curr_time);

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
            // Not Thread Spawner Tile / MCP
            // Log Current Total Energy and Power
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
      (double) m_network_current_total_energy[1],\
      (double) m_network_current_total_power[1]);
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
   //    Total
   m_core_previous_total_energy = 0;
   m_core_current_total_energy = 0;
   m_core_current_total_power = 0;
   //    Dynamic
   m_core_previous_dynamic_energy = 0;
   m_core_current_dynamic_energy = 0;
   m_core_current_static_power = 0;
   //    Static
   m_core_previous_static_energy = 0;
   m_core_current_static_energy = 0;
   m_core_current_static_power = 0;
}

//---------------------------------------------------------------------------
// Compute Core Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::computeCoreEnergy(Time &curr_time)
{
   // Compute Energy
   m_core_model->computeEnergy(curr_time);
}

//---------------------------------------------------------------------------
// Collect Core Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::collectCoreEnergy()
{
   getCoreDynamicEnergy();
   getCoreStaticEnergy();

   // Store the Previous Total Energy
   m_core_previous_total_energy = m_core_current_total_energy;
   // Calculate the Current Total Energy
   m_core_current_total_energy = m_core_current_dynamic_energy + m_core_current_static_energy;
}

//---------------------------------------------------------------------------
// Calculate Core Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateCorePower()
{
   calculateCoreDynamicPower();
   calculateCoreStaticPower();

   m_core_current_total_power = (m_core_current_total_energy -
                                 m_core_previous_total_energy)/
                                (m_time_elapsed);
}

//---------------------------------------------------------------------------
// Get Core Dynamic Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getCoreDynamicEnergy()
{
   m_core_previous_dynamic_energy = m_core_current_dynamic_energy;

   m_core_current_dynamic_energy = m_core_model->getDynamicEnergy();

   if (std::isnan(m_core_current_dynamic_energy)) 
      m_core_current_dynamic_energy = m_core_previous_dynamic_energy;
}

//---------------------------------------------------------------------------
// Calculate Core Dynamic Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateCoreDynamicPower()
{
   m_core_current_dynamic_power = (m_core_current_dynamic_energy -
                                   m_core_previous_dynamic_energy)/
                                  (m_time_elapsed);
}

//---------------------------------------------------------------------------
// Get Core Static Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getCoreStaticEnergy()
{
   m_core_previous_static_energy = m_core_current_static_energy;

   m_core_current_static_energy = m_core_model->getLeakageEnergy();

   if (std::isnan(m_core_current_static_energy))
      m_core_current_static_energy = m_core_previous_static_energy;
}

//---------------------------------------------------------------------------
// Calculate Core Static Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateCoreStaticPower()
{
   m_core_current_static_power = (m_core_current_static_energy -
                                  m_core_previous_static_energy)/
                                 (m_time_elapsed);
}

//---------------------------------------------------------------------------
// Initialize Cache Energy Counters
//---------------------------------------------------------------------------
void TileEnergyMonitor::initializeCacheEnergyCounters()
{
   //    Total
   m_cache_previous_total_energy = 0;
   m_cache_current_total_energy = 0;
   m_cache_current_total_power = 0;
   //    Dynamic
   m_cache_previous_dynamic_energy = 0;
   m_cache_current_dynamic_energy = 0;
   m_cache_current_dynamic_power = 0;
   //    Static
   m_cache_previous_static_energy = 0;
   m_cache_current_static_energy = 0;
   m_cache_current_static_power = 0;
}

//---------------------------------------------------------------------------
// Compute Cache Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::computeCacheEnergy(Time &curr_time)
{
   // Compute Energy
   if (m_memory_manager)
   {
      m_tile->getMemoryManager()->computeEnergy(curr_time);
   }
}

//---------------------------------------------------------------------------
// Collect Cache Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::collectCacheEnergy()
{
   getCacheDynamicEnergy();
   getCacheStaticEnergy();

   // Store the Previous Total Energy
   m_cache_previous_total_energy = m_cache_current_total_energy;
   // Calculate the Current Total Energy
   m_cache_current_total_energy = m_cache_current_dynamic_energy + m_cache_current_static_energy;
}

//---------------------------------------------------------------------------
// Calculate Cache Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateCachePower()
{
   calculateCacheDynamicPower();
   calculateCacheStaticPower();

   m_cache_current_total_power = (m_cache_current_total_energy -
                                  m_cache_previous_total_energy)/
                                 (m_time_elapsed);
}

//---------------------------------------------------------------------------
// Get Cache Dynamic Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getCacheDynamicEnergy()
{
   m_cache_previous_dynamic_energy = m_cache_current_dynamic_energy;

   m_cache_current_dynamic_energy = m_tile->getMemoryManager()->getDynamicEnergy();

   if (std::isnan(m_cache_current_dynamic_energy))
      m_cache_current_dynamic_energy = m_cache_previous_dynamic_energy;
}

//---------------------------------------------------------------------------
// Calculate Cache Dynamic Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateCacheDynamicPower()
{
   m_cache_current_dynamic_power = (m_cache_current_dynamic_energy -
                                    m_cache_previous_dynamic_energy)/
                                   (m_time_elapsed);
}

//---------------------------------------------------------------------------
// Get Cache Static Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getCacheStaticEnergy()
{
   m_cache_previous_static_energy = m_cache_current_static_energy;

   m_cache_current_static_energy = m_tile->getMemoryManager()->getLeakageEnergy();

   if (std::isnan(m_cache_current_static_energy))
      m_cache_current_static_energy = m_cache_previous_static_energy;
}

//---------------------------------------------------------------------------
// Calculate Cache Static Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateCacheStaticPower()
{
   m_cache_current_static_power = (m_cache_current_static_energy -
                                   m_cache_previous_static_energy)/
                                  (m_time_elapsed);
}

//---------------------------------------------------------------------------
// Initialize Network Energy Counters
//---------------------------------------------------------------------------
void TileEnergyMonitor::initializeNetworkEnergyCounters()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      //    Total
      m_network_previous_total_energy[i] = 0;
      m_network_current_total_energy[i] = 0;
      m_network_current_total_power[i] = 0;
      //    Dynamic
      m_network_previous_dynamic_energy[i] = 0;
      m_network_current_dynamic_energy[i] = 0;
      m_network_current_dynamic_power[i] = 0;
      //    Static
      m_network_previous_static_energy[i] = 0;
      m_network_current_static_energy[i] = 0;
      m_network_current_static_power[i] = 0;
   }
}

//---------------------------------------------------------------------------
// Collect Network Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::collectNetworkEnergy()
{
   getNetworkDynamicEnergy();
   getNetworkStaticEnergy();

   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      // Store the Previous Total Energy
      m_network_previous_total_energy[i] = m_network_current_total_energy[i];
      // Calculate the Current Total Energy
      m_network_current_total_energy[i] = m_network_current_dynamic_energy[i] +
                                          m_network_current_static_energy[i];
   }
}

//---------------------------------------------------------------------------
// Calculate Network Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateNetworkPower()
{
   calculateNetworkDynamicPower();
   calculateNetworkStaticPower();

   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      m_network_current_total_power[i] = (m_network_current_total_energy[i] -
                                          m_network_previous_total_energy[i])/
                                         (m_time_elapsed);
   }
}

//---------------------------------------------------------------------------
// Get Network Dynamic Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getNetworkDynamicEnergy()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      m_network_previous_dynamic_energy[i] = m_network_current_dynamic_energy[i];

      m_network_current_dynamic_energy[i] = m_network->getNetworkModel(i)->getDynamicEnergy();
   }
}

//---------------------------------------------------------------------------
// Calculate Network Dynamic Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateNetworkDynamicPower()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      m_network_current_dynamic_power[i] = (m_network_current_dynamic_energy[i] -
                                            m_network_previous_dynamic_energy[i])/
                                           (m_time_elapsed);
   }
}

//---------------------------------------------------------------------------
// Get Network Static Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getNetworkStaticEnergy()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      m_network_previous_static_energy[i] = m_network_current_static_energy[i];

      m_network_current_static_energy[i] = m_network->getNetworkModel(i)->getStaticEnergy();
   }
}

//---------------------------------------------------------------------------
// Calculate Network Static Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateNetworkStaticPower()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      m_network_current_static_power[i] = (m_network_current_static_energy[i] -
                                           m_network_previous_static_energy[i])/
                                          (m_time_elapsed);
   }
}

//---------------------------------------------------------------------------
// Output Summary
//---------------------------------------------------------------------------
void TileEnergyMonitor::outputSummary(std::ostream &out)
{
   // Set Last Time Variable
   // Check Core Cycle Count
   m_last_time = m_core_model->getCurrTime().toSec();

   if (m_tile_id < (tile_id_t) Sim()->getConfig()->getApplicationTiles())
   {
      // Not Thread Spawner Tile / MCP
      periodicallyCollectEnergy();

      out << "Tile Energy Monitor Summary: " << endl;
      //out << "  First Time (in nanoseconds): " << m_first_time << std::endl;
      //out << "  Last Time (in nanoseconds): " << m_last_time << std::endl;
      //out << "  Delta t (in nanoseconds): " << m_delta_t << std::endl;
      //out << "  Counter: " << m_counter << std::endl;
      //out << "  Current Time (in nanoseconds): " << m_current_time << std::endl;
      //out << "  Previous Time (in nanoseconds): " << m_previous_time << std::endl;
      //out << "  Time Elapsed (in nanoseconds): " << m_time_elapsed << std::endl;
      out << "  Core: " << endl;
      out << "    Static Energy (in J): " << m_core_current_static_energy << std::endl;
      out << "    Dynamic Energy (in J): " << m_core_current_dynamic_energy << std::endl;
      out << "    Total Energy (in J): " << m_core_current_total_energy << std::endl;
      out << "  Cache: " << endl;
      out << "    Static Energy (in J): " << m_cache_current_static_energy << std::endl;
      out << "    Dynamic Energy (in J): " << m_cache_current_dynamic_energy << std::endl;
      out << "    Total Energy (in J): " << m_cache_current_total_energy << std::endl;
      for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
      {
         if (i > STATIC_NETWORK_SYSTEM) break;
         out << "  Network (" <<  m_network->getNetworkModel(i)->getNetworkName() << "): " << endl;
         out << "    Static Energy (in J): " << m_network_current_static_energy[i] << std::endl;
         out << "    Dynamic Energy (in J): " << m_network_current_dynamic_energy[i] << std::endl;
         out << "    Total Energy (in J): " << m_network_current_total_energy[i] << std::endl;
      }
   }
}
