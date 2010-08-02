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

   *frequency = Sim()->getCoreManager()->getCurrentCore()->getPerformanceModel()->getFrequency();
}

void CarbonSetCoreFrequency(volatile float* frequency)
{
   // Floating Point Save/Restore
   FloatingPointHandler floating_point_handler;

   // Stuff to change
   // 1) Core Performance Model
   // 2) Shared Memory Performance Model
   // 3) Cache Performance Model
   Core* core = Sim()->getCoreManager()->getCurrentCore();
   core->updateInternalVariablesOnFrequencyChange(*frequency);
   Config::getSingleton()->setCoreFrequency(core->getId(), *frequency);
}
