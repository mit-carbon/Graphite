#include "performance_counter_support.h"
#include "performance_counter_manager.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "core.h"
#include "network.h"
#include "transport.h"
#include "packetize.h"
#include "message_types.h"
#include "log.h"

void CarbonEnableModels()
{
   if (Sim()->getCfg()->getBool("general/trigger_models_within_application", false))
   {
      fprintf(stderr, "[[Graphite]] --> [ Enabling Performance and Power Models ]\n");
      
      __attribute__((unused)) SInt32 curr_proc_num = Config::getSingleton()->getCurrentProcessNum();
      Core* core = Sim()->getTileManager()->getCurrentCore();
      __attribute__((unused)) tile_id_t curr_tile_id = core->getTile()->getId();
      
      LOG_ASSERT_ERROR(curr_proc_num == 0 && curr_tile_id == 0, "Curr Process Num(%i), Curr Tile ID(%i)",
                       curr_proc_num, curr_tile_id);

      Network* network = core->getTile()->getNetwork();
      Transport::Node* transport = Transport::getSingleton()->getGlobalNode();

      UnstructuredBuffer send_buff;
      send_buff << (SInt32) LCP_MESSAGE_TOGGLE_PERFORMACE_COUNTERS << (SInt32) PerformanceCounterManager::ENABLE;
      
      // Send message to all processes to enable models
      for (SInt32 i = 0; i < (SInt32) Config::getSingleton()->getProcessCount(); i++)
      {
         transport->globalSend(i, send_buff.getBuffer(), send_buff.size());
      }
      
      // Collect ACKs from all processes after models have been enabled
      for (SInt32 i = 0; i < (SInt32) Config::getSingleton()->getProcessCount(); i++)
      {
         network->netRecvType(LCP_TOGGLE_PERFORMANCE_COUNTERS_ACK, core->getId());
      }
   }
}

void CarbonDisableModels()
{
   if (Sim()->getCfg()->getBool("general/trigger_models_within_application", false))
   {
      fprintf(stderr, "[[Graphite]] --> [ Disabling Performance and Power Models ]\n");
      
      __attribute__((unused)) SInt32 curr_proc_num = Config::getSingleton()->getCurrentProcessNum();
      Core* core = Sim()->getTileManager()->getCurrentCore();
      __attribute__((unused)) tile_id_t curr_tile_id = core->getTile()->getId();
      
      LOG_ASSERT_ERROR(curr_proc_num == 0 && curr_tile_id == 0, "Curr Process Num(%i), Curr Tile ID(%i)",
                       curr_proc_num, curr_tile_id);

      Network* network = core->getTile()->getNetwork();
      Transport::Node *transport = Transport::getSingleton()->getGlobalNode();

      UnstructuredBuffer send_buff;
      send_buff << (SInt32) LCP_MESSAGE_TOGGLE_PERFORMACE_COUNTERS << (SInt32) PerformanceCounterManager::DISABLE;
      
      // Send message to all processes to disable models
      for (SInt32 i = 0; i < (SInt32) Config::getSingleton()->getProcessCount(); i++)
      {
         transport->globalSend(i, send_buff.getBuffer(), send_buff.size());
      }
      
      // Collect ACKs from all processes after models have been disabled
      for (SInt32 i = 0; i < (SInt32) Config::getSingleton()->getProcessCount(); i++)
      {
         network->netRecvType(LCP_TOGGLE_PERFORMANCE_COUNTERS_ACK, core->getId());
      }
   }
} 
