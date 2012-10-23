#include "perf_counter_support.h"
#include "tile_manager.h"
#include "tile.h"
#include "core.h"
#include "simulator.h"
#include "packet_type.h"
#include "message_types.h"
#include "network.h"
#include "sync_api.h"
#include "fxsupport.h"
#include "log.h"

carbon_barrier_t models_barrier;

void CarbonInitModels() 
{
   // Initialize the barrier for Carbon[Enable/Disable/Reset]Models
   if (Config::getSingleton()->getCurrentProcessNum() == 0)
      CarbonBarrierInit(&models_barrier, Config::getSingleton()->getApplicationTiles());
}

void CarbonEnableModels()
{
   if (Sim()->getCfg()->getBool("general/trigger_models_within_application", false))
   {
      // Acquire & Release a barrier
      CarbonBarrierWait(&models_barrier);

      if (Sim()->getTileManager()->getCurrentTileIndex() == 0)
      {
         fprintf(stderr, "[[Graphite]] --> [ Enabling Performance and Power Models ]\n");
         // Enable the models of the cores in the current process
         Simulator::enablePerformanceModelsInCurrentProcess();
      }

      // Acquire & Release a barrier again
      CarbonBarrierWait(&models_barrier);
   }
}

void CarbonDisableModels()
{
   if (Sim()->getCfg()->getBool("general/trigger_models_within_application", false))
   {
      // Acquire & Release a barrier
      CarbonBarrierWait(&models_barrier);

      if (Sim()->getTileManager()->getCurrentTileIndex() == 0)
      {
         fprintf(stderr, "[[Graphite]] --> [ Disabling Performance and Power Models ]\n");
         // Disable performance models of cores in this process
         Simulator::disablePerformanceModelsInCurrentProcess();
      }

      // Acquire & Release a barrier again
      CarbonBarrierWait(&models_barrier);
   }
} 

void CarbonResetCacheCounters()
{
   UInt32 msg = MCP_MESSAGE_RESET_CACHE_COUNTERS;
   // Send a message to the MCP asking it to reset all the cache counters
   Network *net = Sim()->getTileManager()->getCurrentTile()->getNetwork();
   net->netSend(Sim()->getConfig()->getMCPCoreId(), MCP_SYSTEM_TYPE,
         (const void*) &msg, sizeof(msg));

   NetPacket recv_pkt;
   Core *core = Sim()->getTileManager()->getCurrentCore();
   recv_pkt = net->netRecv(Sim()->getConfig()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);
   
   assert(recv_pkt.length == sizeof(UInt32));
   delete [](Byte*)recv_pkt.data;
}

void CarbonDisableCacheCounters()
{
   UInt32 msg = MCP_MESSAGE_DISABLE_CACHE_COUNTERS;
   // Send a message to the MCP asking it to reset all the cache counters
   Network *net = Sim()->getTileManager()->getCurrentTile()->getNetwork();
   net->netSend(Sim()->getConfig()->getMCPCoreId(), MCP_SYSTEM_TYPE,
         (const void*) &msg, sizeof(msg));

   NetPacket recv_pkt;
   Core *core = Sim()->getTileManager()->getCurrentCore();
   recv_pkt = net->netRecv(Sim()->getConfig()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);
   
   assert(recv_pkt.length == sizeof(UInt32));
   delete [](Byte*)recv_pkt.data;
}
