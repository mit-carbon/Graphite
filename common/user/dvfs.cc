#include "dvfs.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "core.h"
#include "core_model.h"
#include "fxsupport.h"

void CarbonGetCoreFrequency(volatile float* frequency)
{
   // Floating Point Save/Restore
   FloatingPointHandler floating_point_handler;

   *frequency = Sim()->getTileManager()->getCurrentCore()->getModel()->getFrequency();
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
   Config::getSingleton()->setCoreFrequency(tile->getCore()->getId(), *frequency);
}
