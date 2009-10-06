#include "simulation_barrier.h"
#include "simulator.h"
#include "core_manager.h"
#include "core.h"
#include "simulation_barrier_client.h"

static bool enabled()
{
   return Sim()->getCfg()->getBool("simulation_barrier/enabled", false);
}

void handlePeriodicSimulationBarrier()
{
   Core* core = Sim()->getCoreManager()->getCurrentCore();
   assert(core);
   if (core->getId() >= (core_id_t) Sim()->getConfig()->getApplicationCores())
   {
      // Thread Spawner Core / MCP
      return;
   }

   SimulationBarrierClient *barrier_client = core->getSimulationBarrierClient();
   assert(barrier_client);

   barrier_client->handlePeriodicBarrier();
}

void addPeriodicSimulationBarrier(INS ins)
{
   if (!enabled())
      return;

   INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(handlePeriodicSimulationBarrier), IARG_END);
}
