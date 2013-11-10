#include "handle_threads.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "thread_scheduler.h"
#include "hash_map.h"

extern HashMap core_map;

static bool enabled()
{
   // WARNING: Do not change this parameter. Hard-coded until multi-threading bug is fixed
   std::string scheme = "none"; // Sim()->getCfg()->getString("thread_scheduling/scheme", "none");
   return (scheme != "none");
}

void handleYield(THREADID thread_id)
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

   ThreadScheduler * thread_scheduler = Sim()->getThreadScheduler();
   assert(thread_scheduler);
   thread_scheduler->yieldThread();
}

void addYield(INS ins)
{ 
   if (!enabled())
      return;

   // Do not yield on branch or memory instructions because we deal with them asynchronously.
   if (INS_IsMemoryRead (ins) || INS_IsMemoryWrite (ins) || INS_IsBranch(ins))
      return ;
   
   INS_InsertCall(ins, IPOINT_BEFORE,
                  AFUNPTR(handleYield),
                  IARG_THREAD_ID,
                  IARG_END);
}
