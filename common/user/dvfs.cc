#include "dvfs.h"
#include "simulator.h"
#include "core_manager.h"
#include "core.h"
#include "performance_model.h"
#include "fxsupport.h"

void getCoreFrequency(volatile float* frequency)
{
   *frequency = Sim()->getCoreManager()->getCurrentCore()->getPerformanceModel()->getFrequency();
}

void setCoreFrequency(volatile float* frequency)
{
   if (Fxsupport::isInitialized())
      Fxsupport::getSingleton()->fxsave();

   // Stuff to change
   // 1) Core Performance Model
   // 2) Shared Memory Performance Model
   // 3) Cache Performance Model
   Core* core = Sim()->getCoreManager()->getCurrentCore();
   core->updateInternalVariablesOnFrequencyChange(*frequency);
   Config::getSingleton()->setCoreFrequency(core->getId(), *frequency);
   
   if (Fxsupport::isInitialized())
      Fxsupport::getSingleton()->fxrstor();
}
