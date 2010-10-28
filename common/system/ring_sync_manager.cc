#define __STDC_LIMIT_MACROS   1
#include <stdint.h>

#include "ring_sync_client.h"
#include "ring_sync_manager.h"
#include "simulator.h"
#include "config.h"
#include "tile_manager.h"
#include "packetize.h"
#include "message_types.h"
#include "utils.h"

// There is one RingSyncManager per process
RingSyncManager::RingSyncManager():
   _transport(Transport::getSingleton()->getGlobalNode()),
   _slack(0)
{
   // Cache the tile pointers corresponding to all the application cores
   UInt32 num_cores = Config::getSingleton()->getNumLocalCores();
   UInt32 num_app_cores = (Config::getSingleton()->getCurrentProcessNum() == 0) ? num_cores - 2 : num_cores - 1;

   Config::CoreList core_list = Config::getSingleton()->getCoreListForCurrentProcess();
   Config::CoreList::iterator it;

   for (it = core_list.begin(); it != core_list.end(); it++)
   {
      if ((*it) < (core_id_t) Sim()->getConfig()->getApplicationCores())
      {
         Tile* tile = Sim()->getTileManager()->getTileFromID(*it);
         assert(tile != NULL);
         _core_list.push_back(tile);
      }
   }
   assert(_core_list.size() == num_app_cores);

   // Has Fields
   try
   {
      _slack = (UInt64) Sim()->getCfg()->getInt("clock_skew_minimization/ring/slack");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Could not read 'clock_skew_minimization/ring/slack' from the config file");
   }
}

RingSyncManager::~RingSyncManager()
{}

void
RingSyncManager::generateSyncMsg()
{
   Config* config = Config::getSingleton();
   if (config->getCurrentProcessNum() == 0)
   {
      CycleCountUpdate cycle_count_update;
      cycle_count_update.min_cycle_count = UINT64_MAX;
      cycle_count_update.max_cycle_count = _slack;
      
      UnstructuredBuffer send_msg;
      send_msg << LCP_MESSAGE_CLOCK_SKEW_MINIMIZATION;
      send_msg.put<CycleCountUpdate>(cycle_count_update);

      // Send it to process '0' itself
      _transport->globalSend(0, send_msg.getBuffer(), send_msg.size());
   }
}

void
RingSyncManager::processSyncMsg(Byte* msg)
{
   // How do I model the slight time delay that should be put in 
   // before sending the message to another tile
   CycleCountUpdate* cycle_count_update = (CycleCountUpdate*) msg;

   Config* config = Config::getSingleton();
   if (config->getCurrentProcessNum() == 0)
   {
      if (cycle_count_update->min_cycle_count != UINT64_MAX)
      {
         cycle_count_update->max_cycle_count = cycle_count_update->min_cycle_count + _slack;
         cycle_count_update->min_cycle_count = UINT64_MAX;
      }
   }

   // 1) Compute the min_cycle_count of all the cores in this process 
   //    and update the message
   // 2) Update the max_cycle_count of all the cores
   // 3) Wake up any sleeping cores
   updateClientObjectsAndRingMsg(cycle_count_update);

   // I could easily prevent 1 copy here but I am just avoiding it now
   UnstructuredBuffer send_msg;
   send_msg << LCP_MESSAGE_CLOCK_SKEW_MINIMIZATION;
   send_msg.put<CycleCountUpdate>(*cycle_count_update);
   
   SInt32 next_process_num = (config->getCurrentProcessNum() + 1) % config->getProcessCount();
   _transport->globalSend(next_process_num, send_msg.getBuffer(), send_msg.size());
}

void
RingSyncManager::updateClientObjectsAndRingMsg(CycleCountUpdate* cycle_count_update)
{
   // Get the min cycle counts of all the application cores in this process
   UInt64 min_cycle_count = UINT64_MAX;

   std::vector<Tile*>::iterator it;
   for (it = _core_list.begin(); it != _core_list.end(); it++)
   {
      // Read the Cycle Count and State of the tile
      // May need locks around this
      Tile::State tile_state = (*it)->getState();

      RingSyncClient* ring_sync_client = (RingSyncClient*) (*it)->getClockSkewMinimizationClient();
      ring_sync_client->getLock()->acquire();

      UInt64 cycle_count = ring_sync_client->getCycleCount();
      if ((tile_state == Tile::RUNNING) && (cycle_count < min_cycle_count))
      {
         // Dont worry about the cycle counts of threads that are not running
         min_cycle_count = cycle_count;
      }

      // Update the max cycle count of each client
      ring_sync_client->setMaxCycleCount(cycle_count_update->max_cycle_count);

      ring_sync_client->getLock()->release();
   }
   
   cycle_count_update->min_cycle_count = getMin<UInt64>(cycle_count_update->min_cycle_count, min_cycle_count);
}
