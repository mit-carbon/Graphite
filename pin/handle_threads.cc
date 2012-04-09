#include "handle_threads.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "opcodes.h"
#include "thread_scheduler.h"

static bool enabled()
{
   std::string scheme = Sim()->getCfg()->getString("thread_scheduling/scheme", "none");
   return (scheme != "none");
}

void handleYield()
{
   Tile* tile = Sim()->getTileManager()->getCurrentTile();
   assert(tile);
   if (tile->getId() >= (tile_id_t) Sim()->getConfig()->getApplicationTiles())
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

   // Only yield on arithmetic instructions for now, I don't want it to yield
   // at atomic memory operations.
   // FIXME
   switch(INS_Opcode(ins))
   {
      case OPCODE_DIV:
      case OPCODE_MUL:
      case OPCODE_FDIV:
      case OPCODE_FMUL:
         INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(handleYield), IARG_END);
         break;
   }
}
