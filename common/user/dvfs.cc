#include "dvfs.h"
#include "simulator.h"
#include "core_manager.h"
#include "core.h"
#include "performance_model.h"
#include "fxsupport.h"

void CarbonGetCoreFrequency(volatile float* frequency)
{
   // Floating Point Save/Restore
   FloatingPointHandler floating_point_handler;

   *frequency = Sim()->getTileManager()->getCurrentTile()->getPerformanceModel()->getFrequency();
}

void CarbonSetCoreFrequency(volatile float* frequency)
{
   // Floating Point Save/Restore
   FloatingPointHandler floating_point_handler;

   // Stuff to change
   // 1) Tile Performance Model
   // 2) Shared Memory Performance Model
   // 3) Cache Performance Model
   Tile* core = Sim()->getTileManager()->getCurrentTile();
   core->updateInternalVariablesOnFrequencyChange(*frequency);
   Config::getSingleton()->setCoreFrequency(core->getId(), *frequency);
}
