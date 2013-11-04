#include "clock_skew_management.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "core.h"
#include "clock_skew_management_object.h"
#include "hash_map.h"

extern HashMap core_map;

static bool enabled()
{
   std::string scheme = Sim()->getCfg()->getString("clock_skew_management/scheme", "lax");
   return (scheme != "lax");
}

void handlePeriodicSync(THREADID thread_id)
{
   if (!Sim()->isEnabled())
      return;

   Core* core = core_map.get<Core>(thread_id);
   assert(core);
   if (core->getTile()->getId() >= (tile_id_t) Sim()->getConfig()->getApplicationTiles())
   {
      // Thread Spawner Tile / MCP
      return;
   }

   ClockSkewManagementClient *client = core->getClockSkewManagementClient();
   if (client)
      client->synchronize();
}

void addPeriodicSync(INS ins)
{
   if (!enabled())
      return;

   INS_InsertCall(ins, IPOINT_BEFORE,
                  AFUNPTR(handlePeriodicSync),
                  IARG_THREAD_ID,
                  IARG_END);
}
