/*****************************************************************************
 * Runtime Energy Monitoring
 ***************************************************************************/

#include "runtime_energy_monitoring.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "core.h"
#include "tile_energy_monitor.h"
#include "hash_map.h"

extern HashMap core_map;

static bool enabled()
{
   return Sim()->getCfg()->getBool("general/enable_power_modeling");
}

void handleRuntimeEnergyMonitoring(THREADID thread_id)
{
   if (!Sim()->isEnabled())
      return;

   Core* core = core_map.get<Core>(thread_id);
   assert(core);
   Tile* tile = core->getTile();
   if (tile->getId() >= (tile_id_t) Sim()->getConfig()->getApplicationTiles())
   {
      // Thread Spawner Tile / MCP
      return;
   }

   TileEnergyMonitor* energy_monitor = tile->getTileEnergyMonitor();
   assert(energy_monitor);

   energy_monitor->periodicallyCollectEnergy();
}

void addRuntimeEnergyMonitoring(INS ins)
{
   if (!enabled())
      return;

   INS_InsertCall(ins, IPOINT_BEFORE,
                  AFUNPTR(handleRuntimeEnergyMonitoring),
                  IARG_THREAD_ID,
                  IARG_END);
}
