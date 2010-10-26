#include "clock_skew_minimization.h"
#include "simulator.h"
#include "core_manager.h"
#include "core.h"
#include "clock_skew_minimization_object.h"

static bool enabled()
{
   std::string scheme = Sim()->getCfg()->getString("clock_skew_minimization/scheme", "none");
   return (scheme != "none");
}

void handlePeriodicSync()
{
   Tile* core = Sim()->getCoreManager()->getCurrentCore();
   assert(core);
   if (core->getId() >= (core_id_t) Sim()->getConfig()->getApplicationCores())
   {
      // Thread Spawner Tile / MCP
      return;
   }

   ClockSkewMinimizationClient *client = core->getClockSkewMinimizationClient();
   assert(client);

   client->synchronize();
}

void addPeriodicSync(INS ins)
{
   if (!enabled())
      return;

   INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(handlePeriodicSync), IARG_END);
}
