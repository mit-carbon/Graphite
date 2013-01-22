/*****************************************************************************
 * Runtime Energy Monitoring
 ***************************************************************************/

#include "runtime_energy_monitoring.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "core.h"
#include "tile_energy_monitor.h"

void handleRuntimeEnergyMonitoring()
{
   Tile* tile = Sim()->getTileManager()->getCurrentTile();
   assert(tile);
   if (tile->getId() >= (tile_id_t) Sim()->getConfig()->getApplicationTiles())
   {
      // Thread Spawner Tile / MCP
      return;
   }

   TileEnergyMonitor* energy_monitor = tile->getTileEnergyMonitor();

   if (energy_monitor)
      energy_monitor->periodicallyIncrementCounter();
}

void addRuntimeEnergyMonitoring(INS ins)
{
   INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(handleRuntimeEnergyMonitoring), IARG_END);
}
