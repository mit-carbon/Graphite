#include <sched.h>
#include <linux/unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <limits.h>
#include <algorithm>
#include <vector>

#include "tile_manager.h"
#include "tile.h"
#include "network.h"
#include "cache.h"
#include "config.h"
#include "packetize.h"
#include "message_types.h"

#include "log.h"

using namespace std;

TileManager::TileManager()
      : m_tile_tls(TLS::create())
      , m_tile_index_tls(TLS::create())
      , m_thread_type_tls(TLS::create())
      , m_num_registered_sim_threads(0)
{
   LOG_PRINT("Starting TileManager Constructor.");

   UInt32 num_local_tiles = Config::getSingleton()->getNumLocalTiles();

   UInt32 proc_id = Config::getSingleton()->getCurrentProcessNum();
   const Config::TileList &local_tiles = Config::getSingleton()->getTileListForProcess(proc_id);

   for (UInt32 i = 0; i < num_local_tiles; i++)
   {
      LOG_PRINT("Tile[%u] == %d", i, local_tiles.at(i));
      m_tiles.push_back(new Tile(local_tiles.at(i)));
      m_initialized_cores.push_back(false);
   }

   LOG_PRINT("Finished TileManager Constructor.");
}

TileManager::~TileManager()
{
   for (std::vector<Tile *>::iterator i = m_tiles.begin(); i != m_tiles.end(); i++)
      delete *i;
   delete m_tile_tls;
   m_tile_tls = NULL;
   delete m_tile_index_tls;
   m_tile_index_tls = NULL;
   delete m_thread_type_tls;
   m_thread_type_tls = NULL;
}

void TileManager::initializeCommId(SInt32 comm_id)
{
   LOG_PRINT("initializeCommId - current tile (id) = %p (%d)", getCurrentTile(), getCurrentTileID());

   tile_id_t tile_id = getCurrentTileID();

   LOG_ASSERT_ERROR(tile_id != INVALID_TILE_ID, "Unexpected invalid tile id : %d", tile_id);

   UnstructuredBuffer send_buff;
   send_buff << (SInt32) LCP_MESSAGE_COMMID_UPDATE << comm_id << tile_id;

   LOG_PRINT("Initializing comm_id: %d to tile_id: %d", comm_id, tile_id);

   // Broadcast this update to other processes

   UInt32 idx = getCurrentTileIndex();

   LOG_ASSERT_ERROR(idx < Config::getSingleton()->getNumLocalTiles(),
         "TileManager got and index [%d] out of range (0-%d).", 
         idx, Config::getSingleton()->getNumLocalTiles());

   Network *network = m_tiles.at(idx)->getNetwork();
   Transport::Node *transport = Transport::getSingleton()->getGlobalNode();
   UInt32 num_procs = Config::getSingleton()->getProcessCount();

   for (UInt32 i = 0; i < num_procs; i++)
   {
      transport->globalSend(i,
                            send_buff.getBuffer(),
                            send_buff.size());
   }

   LOG_PRINT("Waiting for replies from LCPs.");

   for (UInt32 i = 0; i < num_procs; i++)
   {
      network->netRecvType(LCP_COMM_ID_UPDATE_REPLY);
      LOG_PRINT("Received reply from proc: %d", i);
   }

   LOG_PRINT("Finished.");
}

void TileManager::initializeThread()
{
   // This is for main threads only!
   ScopedLock sl(m_initialized_cores_lock);

   for (tile_id_t i = 0; i < (tile_id_t)m_initialized_cores.size(); i++)
   {
       if (!m_initialized_cores.at(i))
       {
           doInitializeThread(i);
           return;
       }
   }

   LOG_PRINT_ERROR("initializeThread - No free tiles out of %d total.", Config::getSingleton()->getNumLocalTiles());
}

void TileManager::initializeThread(core_id_t core_id)
{
   ScopedLock sl(m_initialized_cores_lock);

   const Config::TileList &tile_list = Config::getSingleton()->getTileListForProcess(Config::getSingleton()->getCurrentProcessNum());
   LOG_ASSERT_ERROR(tile_list.size() == Config::getSingleton()->getNumLocalTiles(),
                    "Tile list size different from num local tiles? %d != %d", tile_list.size(), Config::getSingleton()->getNumLocalTiles());

   for (UInt32 i = 0; i < tile_list.size(); i++)
   {
      tile_id_t local_tile_id = tile_list.at(i);
      if (local_tile_id == core_id.tile_id)
      {
         if (m_initialized_cores.at(i))
            LOG_PRINT_ERROR("initializeThread -- main core at %d/%d already mapped", i, Config::getSingleton()->getNumLocalTiles());

         doInitializeThread(i);
         return;
      }
   }

   LOG_PRINT_ERROR("initializeThread - Requested tile %d does not live on process %d.", core_id.tile_id, Config::getSingleton()->getCurrentProcessNum());
}

void TileManager::doInitializeThread(UInt32 tile_index)
{
    m_tile_tls->set(m_tiles.at(tile_index));
    m_tile_index_tls->setInt(tile_index);
    m_thread_type_tls->setInt(APP_THREAD);
    m_initialized_cores.at(tile_index) = true;
    LOG_PRINT("Initialize app thread : index %d mapped to tile (id): %p (%d)", tile_index, m_tiles.at(tile_index), m_tiles.at(tile_index)->getId());
    LOG_ASSERT_ERROR(m_tile_tls->get() == (void*)(m_tiles.at(tile_index)),
                     "TLS appears to be broken. %p != %p", m_tile_tls->get(), (void*)(m_tiles.at(tile_index)));
}

void TileManager::terminateThread()
{
   LOG_ASSERT_WARNING(m_tile_tls->get() != NULL, "Thread not initialized while terminating.");

   tile_id_t tile_index = m_tile_index_tls->getInt();
   m_initialized_cores.at(tile_index) = false;

   m_tile_tls->set(NULL);
   m_tile_index_tls->setInt(-1);
}

core_id_t TileManager::getCurrentCoreID()
{
   Tile *tile = getCurrentTile();
   if (!tile)
       return INVALID_CORE_ID;
   else
       return tile->getCurrentCore()->getCoreId();
}

tile_id_t TileManager::getCurrentTileID()
{
   Tile *tile = getCurrentTile();
   if (!tile)
       return INVALID_TILE_ID;
   else
       return tile->getId();
}

Tile *TileManager::getCurrentTile()
{
    return m_tile_tls ? m_tile_tls->getPtr<Tile>() : NULL;
}

UInt32 TileManager::getCurrentTileIndex()
{
    UInt32 idx = m_tile_index_tls ? (m_tile_index_tls->getInt()) : -1;
    // LOG_ASSERT_ERROR(idx < m_tiles.size(),
    //       "Invalid tile index, idx(%u) >= m_tiles.size(%u)",
    //       idx, m_tiles.size());
    return idx;
}

Tile *TileManager::getTileFromID(tile_id_t id)
{
   Tile *tile = NULL;
   // Look up the index from the tile list
   // FIXME: make this more cached
   const Config::TileList & tiles(Config::getSingleton()->getTileListForProcess(Config::getSingleton()->getCurrentProcessNum()));
   UInt32 idx = 0;
   for (Config::TLCI i = tiles.begin(); i != tiles.end(); i++)
   {
      if (*i == id)
      {
         tile = m_tiles.at(idx);
         break;
      }

      idx++;
   }

   LOG_ASSERT_ERROR(!tile || idx < Config::getSingleton()->getNumLocalTiles(), "Illegal index in getTileFromID!");

   return tile;
}

Tile *TileManager::getTileFromIndex(UInt32 index)
{
   LOG_ASSERT_ERROR(index < Config::getSingleton()->getNumLocalTiles(), "getTileFromIndex -- invalid index %d", index);

   return m_tiles.at(index);
}

UInt32 TileManager::getTileIndexFromID(tile_id_t tile_id)
{
   // Look up the index from the tile list
   // FIXME: make this more cached
   const Config::TileList & tiles(Config::getSingleton()->getTileListForProcess(Config::getSingleton()->getCurrentProcessNum()));
   UInt32 idx = 0;
   for (Config::TLCI i = tiles.begin(); i != tiles.end(); i++)
   {
      if (*i == tile_id)
         return idx;

      idx++;
   }

   LOG_ASSERT_ERROR(false, "Tile lookup failed for tile id: %d!", tile_id);
   return INVALID_TILE_ID;
}

Core *TileManager::getCurrentCore()
{
   return this->getCore(getCurrentTile());
}

core_id_t TileManager::getMainCoreId(tile_id_t tile_id)
{
   return (core_id_t) {tile_id, MAIN_CORE_TYPE};
}


Core *TileManager::getCoreFromID(core_id_t id)
{
   Tile *tile = NULL;
   tile_id_t tile_id = id.tile_id;

   // Look up the index from the tile list
   // FIXME: make this more cached
   const Config::TileList & tiles(Config::getSingleton()->getTileListForProcess(Config::getSingleton()->getCurrentProcessNum()));
   UInt32 idx = 0;
   for (Config::TLCI i = tiles.begin(); i != tiles.end(); i++)
   {
      if (*i == tile_id)
      {
         tile = m_tiles.at(idx);
         break;
      }

      idx++;
   }

   LOG_ASSERT_ERROR(!tile || idx < Config::getSingleton()->getNumLocalTiles(), "Illegal index in getTileFromID!");

   return tile->getCore(id);
}

Core *TileManager::getCore(Tile *tile)
{
   assert(m_thread_type_tls);
   assert(tile);

   if (m_thread_type_tls->getInt() == APP_THREAD)
      return tile->getCore();
   else
   {
      LOG_PRINT_ERROR("Incorrect thread type!");
      exit(0);
   }
      
}


tile_id_t TileManager::registerSimThread()
{
    if (getCurrentTile() != NULL)
    {
        LOG_PRINT_ERROR("registerSimMemThread - Initialized thread twice");
        return getCurrentTile()->getId();
    }

    ScopedLock sl(m_num_registered_sim_threads_lock);

    LOG_ASSERT_ERROR(m_num_registered_sim_threads < Config::getSingleton()->getNumLocalTiles(),
                     "All sim threads already registered. %d > %d",
                     m_num_registered_sim_threads+1, Config::getSingleton()->getNumLocalTiles());

    Tile *tile = m_tiles.at(m_num_registered_sim_threads);

    m_tile_tls->set(tile);
    m_tile_index_tls->setInt(m_num_registered_sim_threads);
    m_thread_type_tls->setInt(SIM_THREAD);

    ++m_num_registered_sim_threads;

    return tile->getId();
}

bool TileManager::amiSimThread()
{
    return m_thread_type_tls ? (m_thread_type_tls->getInt() == SIM_THREAD) : false;
}

bool TileManager::amiAppThread()
{
    return m_thread_type_tls ? (m_thread_type_tls->getInt() == APP_THREAD) : false;
}

