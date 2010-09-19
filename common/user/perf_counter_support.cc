#include "perf_counter_support.h"
#include "core.h"
#include "core_manager.h"
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
      CarbonBarrierInit(&models_barrier, Config::getSingleton()->getApplicationCores());
}

void CarbonEnableModels()
{
   if (! Sim()->getCfg()->getBool("general/enable_models_at_startup", true))
   {
      // Acquire & Release a barrier
      CarbonBarrierWait(&models_barrier);

      if (Sim()->getCoreManager()->getCurrentCoreIndex() == 0)
      {
         fprintf(stderr, "---> [[  Enabling Performance and Power Models  ]]\n");
         // Enable the models of the cores in the current process
         Simulator::enablePerformanceModelsInCurrentProcess();
      }

      // Acquire & Release a barrier again
      CarbonBarrierWait(&models_barrier);
   }
}

void CarbonDisableModels()
{
   // Acquire & Release a barrier
   CarbonBarrierWait(&models_barrier);

   if (Sim()->getCoreManager()->getCurrentCoreIndex() == 0)
   {
      fprintf(stderr, "---> [[  Disabling Performance and Power Models  ]]\n");
      // Disable performance models of cores in this process
      Simulator::disablePerformanceModelsInCurrentProcess();
   }

   // Acquire & Release a barrier again
   CarbonBarrierWait(&models_barrier);
} 

void CarbonResetModels()
{
   FloatingPointHandler floating_point_handler;

   // Acquire & Release a barrier
   CarbonBarrierWait(&models_barrier);

   if (Sim()->getCoreManager()->getCurrentCoreIndex() == 0)
   {
      fprintf(stderr, "---> [[  Reset Performance and Power Models  ]]\n");
      // Reset performance models of cores in this process
      Simulator::resetPerformanceModelsInCurrentProcess();
   }

   // Acquire & Release a barrier again
   CarbonBarrierWait(&models_barrier);
} 

void CarbonResetCacheCounters()
{
   UInt32 msg = MCP_MESSAGE_RESET_CACHE_COUNTERS;
   // Send a message to the MCP asking it to reset all the cache counters
   Network *net = Sim()->getCoreManager()->getCurrentCore()->getNetwork();
   net->netSend(Sim()->getConfig()->getMCPCoreNum(), MCP_SYSTEM_TYPE,
         (const void*) &msg, sizeof(msg));

   NetPacket recv_pkt;
   recv_pkt = net->netRecv(Sim()->getConfig()->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   
   assert(recv_pkt.length == sizeof(UInt32));
   delete [](Byte*)recv_pkt.data;
}

void CarbonDisableCacheCounters()
{
   UInt32 msg = MCP_MESSAGE_DISABLE_CACHE_COUNTERS;
   // Send a message to the MCP asking it to reset all the cache counters
   Network *net = Sim()->getCoreManager()->getCurrentCore()->getNetwork();
   net->netSend(Sim()->getConfig()->getMCPCoreNum(), MCP_SYSTEM_TYPE,
         (const void*) &msg, sizeof(msg));

   NetPacket recv_pkt;
   recv_pkt = net->netRecv(Sim()->getConfig()->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   
   assert(recv_pkt.length == sizeof(UInt32));
   delete [](Byte*)recv_pkt.data;
}
