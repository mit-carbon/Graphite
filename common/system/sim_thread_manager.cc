#include "sim_thread_manager.h"

#include "lock.h"
#include "log.h"
#include "config.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "mcp.h"

SimThreadManager::SimThreadManager()
   : m_active_threads(0)
{
}

SimThreadManager::~SimThreadManager()
{
   LOG_ASSERT_WARNING(m_active_threads == 0,
                      "Threads still active when SimThreadManager exits.");
}

void SimThreadManager::spawnSimThreads()
{
   UInt32 num_sim_threads = Config::getSingleton()->getNumLocalTiles();

   LOG_PRINT("Starting %d threads on proc: %d.", num_sim_threads, Config::getSingleton()->getCurrentProcessNum());

   m_sim_threads = new SimThread [num_sim_threads];

   for (UInt32 i = 0; i < num_sim_threads; i++)
   {
      LOG_PRINT("Starting thread %i", i);
      m_sim_threads[i].spawn();
   }

   LOG_PRINT("Threads started: %d.", m_active_threads);
}

void SimThreadManager::quitSimThreads()
{
   LOG_PRINT("Sending quit messages.");

   Transport::Node *global_node = Transport::getSingleton()->getGlobalNode();
   UInt32 num_local_tiles = Config::getSingleton()->getNumLocalTiles();

   // This is something of a hard-wired emulation of Network::netSend
   // ... not the greatest thing to do, but whatever.
   //NetPacket pkt(0, SIM_THREAD_TERMINATE_THREADS, 0, 0, 0, NULL);
   NetPacket pkt(Time(0), SIM_THREAD_TERMINATE_THREADS, (core_id_t) {0,0}, (core_id_t) {0,0}, 0, NULL);
   const Config::TileList &tile_list = Config::getSingleton()->getTileListForProcess(Config::getSingleton()->getCurrentProcessNum());

   for (UInt32 i = 0; i < num_local_tiles; i++)
   {
      tile_id_t tile_id = tile_list[i];
      core_id_t receiver = Tile::getMainCoreId(tile_id);
      pkt.receiver.tile_id = receiver.tile_id;
      pkt.receiver.core_type = receiver.core_type;
      global_node->send(tile_id, &pkt, pkt.bufferSize());
   }

   LOG_PRINT("All threads have exited.");
}

void SimThreadManager::simThreadStartCallback()
{
   m_active_threads_lock.acquire();
   ++m_active_threads;
   m_active_threads_lock.release();
}

void SimThreadManager::simThreadExitCallback()
{
   m_active_threads_lock.acquire();
   --m_active_threads;
   m_active_threads_lock.release();
}
