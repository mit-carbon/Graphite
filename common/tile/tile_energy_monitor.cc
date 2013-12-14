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
   : _tile(tile)
{
   // Get Parts of Tile Energy Monitor
   _tile_id = _tile->getId();
   _core_model = _tile->getCore()->getModel();
   _network = _tile->getNetwork();
   if (Config::getSingleton()->isSimulatingSharedMemory())
      _memory_manager = _tile->getMemoryManager();

   // Initialize Delta T Variable
   try
   {
      // Cfg file specifies interval in nanosec -> convert into picosec first
      _delta_t = Time(Sim()->getCfg()->getInt("runtime_energy_modeling/interval") * 1000);
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Cound not read [runtime_energy_modeling/interval] from cfg file");
   }

   // Initialize Time Counters
   initializeTimeCounters();

   // Initialize Next Time Variable
   _next_time = _delta_t;

   // Initialize Energy Counters
   initializeCoreEnergyCounters();
   initializeCacheEnergyCounters();
   initializeNetworkEnergyCounters();

   // Initialize First Time Variable
   _first_time = _core_model->getCurrTime();

   // Initialize Power Trace
   _power_trace_enabled = Sim()->getCfg()->getBool("runtime_energy_modeling/power_trace/enabled");
   if (_power_trace_enabled)
   {
      if (_tile_id < (tile_id_t) Sim()->getConfig()->getApplicationTiles())
      {
         // Not Thread Spawner Tile / MCP
         char _filename[256];
         sprintf(_filename, "%s_%d.csv", "power_trace_tile", _tile_id);
         _power_trace_file = fopen(Config::getSingleton()->formatOutputFileName(_filename).c_str(),"w");
         fprintf(_power_trace_file, "Time %d, "
                 "Core Static Energy %d, Core Static Power %d, Core Dynamic Energy %d, Core Dynamic Power %d, Core Total Energy %d, Core Total Power %d, "
                 "Cache Static Energy %d, Cache Static Power %d, Cache Dynamic Energy %d, Cache Dynamic Power %d, Cache Total Energy %d, Cache Total Power %d, "
                 "Network (Memory) Static Energy %d, Network (Memory) Static Power %d, Network (Memory) Dynamic Energy %d, Network (Memory) Dynamic Power %d, Network (Memory) Total Energy %d, Network (Memory) Total Power %d\n",
                  _tile_id,
                  _tile_id, _tile_id, _tile_id, _tile_id, _tile_id, _tile_id,
                  _tile_id, _tile_id, _tile_id, _tile_id, _tile_id, _tile_id,
                  _tile_id, _tile_id, _tile_id, _tile_id, _tile_id, _tile_id);
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
   if (_power_trace_enabled)
   {
      if (_tile_id < (tile_id_t) Sim()->getConfig()->getApplicationTiles())
      {
         // Not Thread Spawner Tile / MCP
         fclose(_power_trace_file);
      }
   }
}

//---------------------------------------------------------------------------
// Return Tile Energy Information
//---------------------------------------------------------------------------
double TileEnergyMonitor::getTileEnergy()
{
   double tile_energy = _core_current_total_energy + _cache_current_total_energy;
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      tile_energy += _network_current_total_energy[i];
   }
   return tile_energy;
}

//---------------------------------------------------------------------------
// Periodically Collect Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::periodicallyCollectEnergy()
{
   // Check Core Cycle Count
   Time current_time = _core_model->getCurrTime();

   // Check if the Next Time has been reached
   if (current_time >= _next_time)
   {
      collectEnergy(current_time);
   }
}

//---------------------------------------------------------------------------
// Collect Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::collectEnergy(const Time& current_time)
{
   // Check Core Cycle Count
   _current_time = current_time;

   // Calculate Time Elapsed from Previous Time
   assert(_current_time >= _previous_time);
   _time_elapsed = _current_time - _previous_time;
   // Increment Counter
   _counter = _counter + 1;

   // Compute Energy
   computeCoreEnergy();
   computeCacheEnergy();
   computeNetworkEnergy();

   // Collect Energy
   collectCoreEnergy();
   collectCacheEnergy();
   collectNetworkEnergy();

   // Calculate Power
   calculateCorePower();
   calculateCachePower();
   calculateNetworkPower();

   // Update the Previous Time
   _previous_time = _current_time;
   // Update the Next Time
   _next_time = _current_time + _delta_t;

   // Power Trace
   if (_power_trace_enabled)
   {
      if (_tile_id < (tile_id_t) Sim()->getConfig()->getApplicationTiles())
      {
         // Not Thread Spawner Tile / MCP
         // Log Current Total Energy and Power
         logCurrentTotalEnergyAndPower();
      }
   }
}

//---------------------------------------------------------------------------
// Log Current Total Energy and Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::logCurrentTotalEnergyAndPower()
{
   fprintf(_power_trace_file,\
      "%g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g\n", \
      (double) _current_time.toSec(),\
      (double) _core_current_static_energy,\
      (double) _core_current_static_power,\
      (double) _core_current_dynamic_energy,\
      (double) _core_current_dynamic_power,\
      (double) _core_current_total_energy,\
      (double) _core_current_total_power,\
      (double) _cache_current_static_energy,\
      (double) _cache_current_static_power,\
      (double) _cache_current_dynamic_energy,\
      (double) _cache_current_dynamic_power,\
      (double) _cache_current_total_energy,\
      (double) _cache_current_total_power,\
      (double) _network_current_static_energy[1],\
      (double) _network_current_static_power[1],\
      (double) _network_current_dynamic_energy[1],\
      (double) _network_current_dynamic_power[1],\
      (double) _network_current_total_energy[1],\
      (double) _network_current_total_power[1]);
}

//---------------------------------------------------------------------------
// Initialize Time Counters
//---------------------------------------------------------------------------
void TileEnergyMonitor::initializeTimeCounters()
{
   _first_time = Time(0);
   _previous_time = Time(0);
   _current_time = Time(0);
   _next_time = Time(0);
   _last_time = Time(0);
   _time_elapsed = Time(0);
   _counter = 0;
}

//---------------------------------------------------------------------------
// Initialize Core Energy Counters
//---------------------------------------------------------------------------
void TileEnergyMonitor::initializeCoreEnergyCounters()
{
   //    Total
   _core_previous_total_energy = 0;
   _core_current_total_energy = 0;
   _core_current_total_power = 0;
   //    Dynamic
   _core_previous_dynamic_energy = 0;
   _core_current_dynamic_energy = 0;
   _core_current_static_power = 0;
   //    Static
   _core_previous_static_energy = 0;
   _core_current_static_energy = 0;
   _core_current_static_power = 0;
}

//---------------------------------------------------------------------------
// Compute Core Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::computeCoreEnergy()
{
   // Compute Energy
   _core_model->computeEnergy(_current_time);
}

//---------------------------------------------------------------------------
// Collect Core Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::collectCoreEnergy()
{
   getCoreDynamicEnergy();
   getCoreStaticEnergy();

   // Store the Previous Total Energy
   _core_previous_total_energy = _core_current_total_energy;
   // Calculate the Current Total Energy
   _core_current_total_energy = _core_current_dynamic_energy + _core_current_static_energy;
}

//---------------------------------------------------------------------------
// Calculate Core Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateCorePower()
{
   calculateCoreDynamicPower();
   calculateCoreStaticPower();

   _core_current_total_power = (_core_current_total_energy -
                                _core_previous_total_energy)/
                                (_time_elapsed.toSec());
}

//---------------------------------------------------------------------------
// Get Core Dynamic Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getCoreDynamicEnergy()
{
   _core_previous_dynamic_energy = _core_current_dynamic_energy;

   _core_current_dynamic_energy = _core_model->getDynamicEnergy();

   if (std::isnan(_core_current_dynamic_energy)) 
      _core_current_dynamic_energy = _core_previous_dynamic_energy;
}

//---------------------------------------------------------------------------
// Calculate Core Dynamic Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateCoreDynamicPower()
{
   _core_current_dynamic_power = (_core_current_dynamic_energy -
                                  _core_previous_dynamic_energy)/
                                  (_time_elapsed.toSec());
}

//---------------------------------------------------------------------------
// Get Core Static Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getCoreStaticEnergy()
{
   _core_previous_static_energy = _core_current_static_energy;

   _core_current_static_energy = _core_model->getLeakageEnergy();

   if (std::isnan(_core_current_static_energy))
      _core_current_static_energy = _core_previous_static_energy;
}

//---------------------------------------------------------------------------
// Calculate Core Static Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateCoreStaticPower()
{
   _core_current_static_power = (_core_current_static_energy -
                                 _core_previous_static_energy)/
                                 (_time_elapsed.toSec());
}

//---------------------------------------------------------------------------
// Initialize Cache Energy Counters
//---------------------------------------------------------------------------
void TileEnergyMonitor::initializeCacheEnergyCounters()
{
   //    Total
   _cache_previous_total_energy = 0;
   _cache_current_total_energy = 0;
   _cache_current_total_power = 0;
   //    Dynamic
   _cache_previous_dynamic_energy = 0;
   _cache_current_dynamic_energy = 0;
   _cache_current_dynamic_power = 0;
   //    Static
   _cache_previous_static_energy = 0;
   _cache_current_static_energy = 0;
   _cache_current_static_power = 0;
}

//---------------------------------------------------------------------------
// Compute Cache Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::computeCacheEnergy()
{
   // Compute Energy
   if (_memory_manager)
      _memory_manager->computeEnergy(_current_time);
}

//---------------------------------------------------------------------------
// Collect Cache Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::collectCacheEnergy()
{
   getCacheDynamicEnergy();
   getCacheStaticEnergy();

   // Store the Previous Total Energy
   _cache_previous_total_energy = _cache_current_total_energy;
   // Calculate the Current Total Energy
   _cache_current_total_energy = _cache_current_dynamic_energy + _cache_current_static_energy;
}

//---------------------------------------------------------------------------
// Calculate Cache Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateCachePower()
{
   calculateCacheDynamicPower();
   calculateCacheStaticPower();

   _cache_current_total_power = (_cache_current_total_energy -
                                 _cache_previous_total_energy)/
                                 (_time_elapsed.toSec());
}

//---------------------------------------------------------------------------
// Get Cache Dynamic Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getCacheDynamicEnergy()
{
   _cache_previous_dynamic_energy = _cache_current_dynamic_energy;

   _cache_current_dynamic_energy = _memory_manager->getDynamicEnergy();

   if (std::isnan(_cache_current_dynamic_energy))
      _cache_current_dynamic_energy = _cache_previous_dynamic_energy;
}

//---------------------------------------------------------------------------
// Calculate Cache Dynamic Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateCacheDynamicPower()
{
   _cache_current_dynamic_power = (_cache_current_dynamic_energy -
                                   _cache_previous_dynamic_energy)/
                                   (_time_elapsed.toSec());
}

//---------------------------------------------------------------------------
// Get Cache Static Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getCacheStaticEnergy()
{
   _cache_previous_static_energy = _cache_current_static_energy;

   _cache_current_static_energy = _memory_manager->getLeakageEnergy();

   if (std::isnan(_cache_current_static_energy))
      _cache_current_static_energy = _cache_previous_static_energy;
}

//---------------------------------------------------------------------------
// Calculate Cache Static Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateCacheStaticPower()
{
   _cache_current_static_power = (_cache_current_static_energy -
                                  _cache_previous_static_energy)/
                                  (_time_elapsed.toSec());
}

//---------------------------------------------------------------------------
// Initialize Network Energy Counters
//---------------------------------------------------------------------------
void TileEnergyMonitor::initializeNetworkEnergyCounters()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      //    Total
      _network_previous_total_energy[i] = 0;
      _network_current_total_energy[i] = 0;
      _network_current_total_power[i] = 0;
      //    Dynamic
      _network_previous_dynamic_energy[i] = 0;
      _network_current_dynamic_energy[i] = 0;
      _network_current_dynamic_power[i] = 0;
      //    Static
      _network_previous_static_energy[i] = 0;
      _network_current_static_energy[i] = 0;
      _network_current_static_power[i] = 0;
   }
}

//---------------------------------------------------------------------------
// Compute Network Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::computeNetworkEnergy()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      // Compute Energy
      _network->getNetworkModel(i)->computeEnergy(_current_time);
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
      _network_previous_total_energy[i] = _network_current_total_energy[i];
      // Calculate the Current Total Energy
      _network_current_total_energy[i] = _network_current_dynamic_energy[i] +
                                          _network_current_static_energy[i];
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
      _network_current_total_power[i] = (_network_current_total_energy[i] -
                                         _network_previous_total_energy[i])/
                                         (_time_elapsed.toSec());
   }
}

//---------------------------------------------------------------------------
// Get Network Dynamic Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getNetworkDynamicEnergy()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      _network_previous_dynamic_energy[i] = _network_current_dynamic_energy[i];

      _network_current_dynamic_energy[i] = _network->getNetworkModel(i)->getDynamicEnergy();
   }
}

//---------------------------------------------------------------------------
// Calculate Network Dynamic Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateNetworkDynamicPower()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      _network_current_dynamic_power[i] = (_network_current_dynamic_energy[i] -
                                           _network_previous_dynamic_energy[i])/
                                           (_time_elapsed.toSec());
   }
}

//---------------------------------------------------------------------------
// Get Network Static Energy
//---------------------------------------------------------------------------
void TileEnergyMonitor::getNetworkStaticEnergy()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      _network_previous_static_energy[i] = _network_current_static_energy[i];

      _network_current_static_energy[i] = _network->getNetworkModel(i)->getStaticEnergy();
   }
}

//---------------------------------------------------------------------------
// Calculate Network Static Power
//---------------------------------------------------------------------------
void TileEnergyMonitor::calculateNetworkStaticPower()
{
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      _network_current_static_power[i] = (_network_current_static_energy[i] -
                                          _network_previous_static_energy[i])/
                                          (_time_elapsed.toSec());
   }
}

//---------------------------------------------------------------------------
// Output Summary
//---------------------------------------------------------------------------
void TileEnergyMonitor::outputSummary(std::ostream &out, const Time& target_completion_time)
{
   // Set Last Time Variable
   // Check Core Cycle Count
   _last_time = target_completion_time;
   assert (_tile_id < (tile_id_t) Config::getSingleton()->getApplicationTiles());
      
   // Not Thread Spawner Tile / MCP
   collectEnergy(_last_time);

   out << "Tile Energy Monitor Summary: " << endl;
   /*
   out << "  First Time (in nanoseconds): " << _first_time.toNanosec() << endl;
   out << "  Last Time (in nanoseconds): " << _last_time.toNanosec() << endl;
   out << "  Delta-T (in nanoseconds): " << _delta_t.toNanosec() << endl;
   out << "  Counter: " << _counter << endl;
   out << "  Current Time (in nanoseconds): " << _current_time.toNanosec() << endl;
   out << "  Previous Time (in nanoseconds): " << _previous_time.toNanosec() << endl;
   out << "  Time Elapsed (in nanoseconds): " << _time_elapsed.toNanosec() << endl;
    */
   out << "  Core: " << endl;
   out << "    Static Energy (in J): " << _core_current_static_energy << endl;
   out << "    Dynamic Energy (in J): " << _core_current_dynamic_energy << endl;
   out << "    Total Energy (in J): " << _core_current_total_energy << endl;
   out << "  Cache Hierarchy (L1-I, L1-D, L2): " << endl;
   out << "    Static Energy (in J): " << _cache_current_static_energy << endl;
   out << "    Dynamic Energy (in J): " << _cache_current_dynamic_energy << endl;
   out << "    Total Energy (in J): " << _cache_current_total_energy << endl;
   out << "  Networks (User, Memory): " << endl;
   out << "    Static Energy (in J): " << _network_current_static_energy[STATIC_NETWORK_USER] +
                                          _network_current_static_energy[STATIC_NETWORK_MEMORY] << endl;
   out << "    Dynamic Energy (in J): " << _network_current_dynamic_energy[STATIC_NETWORK_USER] +
                                           _network_current_dynamic_energy[STATIC_NETWORK_MEMORY] << endl;
   out << "    Total Energy (in J): " << _network_current_total_energy[STATIC_NETWORK_USER] +
                                         _network_current_total_energy[STATIC_NETWORK_MEMORY] << endl;
}
