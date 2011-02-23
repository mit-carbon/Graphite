#include "dvfs.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "core_perf_model.h"
#include "fxsupport.h"

void CarbonGetCoreFrequency(volatile float* frequency)
{
   // Floating Point Save/Restore
   FloatingPointHandler floating_point_handler;

   *frequency = Sim()->getTileManager()->getCurrentCore()->getPerformanceModel()->getFrequency();
}

void CarbonSetCoreFrequency(volatile float* frequency)
{
   // Floating Point Save/Restore
   FloatingPointHandler floating_point_handler;

   // Stuff to change
   // 1) Tile Performance Model
   // 2) Shared Memory Performance Model
   // 3) Cache Performance Model
   Tile* tile = Sim()->getTileManager()->getCurrentTile();
   tile->updateInternalVariablesOnFrequencyChange(*frequency);
   Config::getSingleton()->setCoreFrequency(tile->getCurrentCore()->getCoreId(), *frequency);
}
