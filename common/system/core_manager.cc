#include <sched.h>
#include <linux/unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <limits.h>
#include <algorithm>
#include <vector>

#include "core_manager.h"
#include "core.h"
#include "network.h"
#include "cache.h"
#include "config.h"
#include "packetize.h"
#include "message_types.h"

#include "log.h"

using namespace std;

TileManager::TileManager()
      : m_core_tls(TLS::create())
      , m_core_index_tls(TLS::create())
      , m_thread_type_tls(TLS::create())
      , m_num_registered_sim_threads(0)
{
   LOG_PRINT("Starting TileManager Constructor.");

   UInt32 num_local_cores = Config::getSingleton()->getNumLocalCores();

   UInt32 proc_id = Config::getSingleton()->getCurrentProcessNum();
   const Config::CoreList &local_cores = Config::getSingleton()->getCoreListForProcess(proc_id);

   for (UInt32 i = 0; i < num_local_cores; i++)
   {
      LOG_PRINT("Tile[%u] == %d", i, local_cores.at(i));
      m_tiles.push_back(new Tile(local_cores.at(i)));
      m_initialized_tiles.push_back(false);
   }

   LOG_PRINT("Finished TileManager Constructor.");
}

TileManager::~TileManager()
{
   for (std::vector<Tile *>::iterator i = m_tiles.begin(); i != m_tiles.end(); i++)
      delete *i;

   delete m_core_tls;
   delete m_core_index_tls;
   delete m_thread_type_tls;
}

void TileManager::initializeCommId(SInt32 comm_id)
{
   LOG_PRINT("initializeCommId - current core (id) = %p (%d)", getCurrentTile(), getCurrentCoreID());

   core_id_t core_id = getCurrentCoreID();

   LOG_ASSERT_ERROR(core_id != INVALID_CORE_ID, "Unexpected invalid core id : %d", core_id);

   UnstructuredBuffer send_buff;
   send_buff << (SInt32) LCP_MESSAGE_COMMID_UPDATE << comm_id << core_id;

   LOG_PRINT("Initializing comm_id: %d to core_id: %d", comm_id, core_id);

   // Broadcast this update to other processes

   UInt32 idx = getCurrentTileIndex();

   LOG_ASSERT_ERROR(idx < Config::getSingleton()->getNumLocalCores(),
         "TileManager got and index [%d] out of range (0-%d).", 
         idx, Config::getSingleton()->getNumLocalCores());

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
   ScopedLock sl(m_initialized_tiles_lock);

   for (core_id_t i = 0; i < (core_id_t)m_initialized_tiles.size(); i++)
   {
       if (!m_initialized_tiles.at(i))
       {
           doInitializeThread(i);
           return;
       }
   }

   LOG_PRINT_ERROR("initializeThread - No free cores out of %d total.", Config::getSingleton()->getNumLocalCores());
}

void TileManager::initializeThread(core_id_t core_id)
{
   ScopedLock sl(m_initialized_tiles_lock);

   const Config::CoreList &core_list = Config::getSingleton()->getCoreListForProcess(Config::getSingleton()->getCurrentProcessNum());
   LOG_ASSERT_ERROR(core_list.size() == Config::getSingleton()->getNumLocalCores(),
                    "Tile list size different from num local cores? %d != %d", core_list.size(), Config::getSingleton()->getNumLocalCores());

   for (UInt32 i = 0; i < core_list.size(); i++)
   {
      core_id_t local_core_id = core_list.at(i);
      if (local_core_id == core_id)
      {
          if (m_initialized_tiles.at(i))
              LOG_PRINT_ERROR("initializeThread -- %d/%d already mapped", i, Config::getSingleton()->getNumLocalCores());

          doInitializeThread(i);
          return;
      }
   }

   LOG_PRINT_ERROR("initializeThread - Requested core %d does not live on process %d.", core_id, Config::getSingleton()->getCurrentProcessNum());
}

void TileManager::doInitializeThread(UInt32 core_index)
{
    m_core_tls->set(m_tiles.at(core_index));
    m_core_index_tls->setInt(core_index);
    m_thread_type_tls->setInt(APP_THREAD);
    m_initialized_tiles.at(core_index) = true;
    LOG_PRINT("Initialize thread : index %d mapped to core (id): %p (%d)", core_index, m_tiles.at(core_index), m_tiles.at(core_index)->getId());
    LOG_ASSERT_ERROR(m_core_tls->get() == (void*)(m_tiles.at(core_index)),
                     "TLS appears to be broken. %p != %p", m_core_tls->get(), (void*)(m_tiles.at(core_index)));
}

void TileManager::terminateThread()
{
   LOG_ASSERT_WARNING(m_core_tls->get() != NULL, "Thread not initialized while terminating.");

   core_id_t core_index = m_core_index_tls->getInt();
   m_initialized_tiles.at(core_index) = false;

   m_core_tls->set(NULL);
   m_core_index_tls->setInt(-1);
}

core_id_t TileManager::getCurrentCoreID()
{
   Tile *core = getCurrentTile();
   if (!core)
       return INVALID_CORE_ID;
   else
       return core->getId();
}

Tile *TileManager::getCurrentTile()
{
    return m_core_tls->getPtr<Tile>();
}

UInt32 TileManager::getCurrentTileIndex()
{
    UInt32 idx = m_core_index_tls->getInt();
    // LOG_ASSERT_ERROR(idx < m_tiles.size(),
    //       "Invalid core index, idx(%u) >= m_tiles.size(%u)",
    //       idx, m_tiles.size());
    return idx;
}

Tile *TileManager::getTileFromID(core_id_t id)
{
   Tile *core = NULL;
   // Look up the index from the core list
   // FIXME: make this more cached
   const Config::CoreList & cores(Config::getSingleton()->getCoreListForProcess(Config::getSingleton()->getCurrentProcessNum()));
   UInt32 idx = 0;
   for (Config::CLCI i = cores.begin(); i != cores.end(); i++)
   {
      if (*i == id)
      {
         core = m_tiles.at(idx);
         break;
      }

      idx++;
   }

   LOG_ASSERT_ERROR(!core || idx < Config::getSingleton()->getNumLocalCores(), "Illegal index in getTileFromID!");

   return core;
}

Tile *TileManager::getTileFromIndex(UInt32 index)
{
   LOG_ASSERT_ERROR(index < Config::getSingleton()->getNumLocalCores(), "getTileFromIndex -- invalid index %d", index);

   return m_tiles.at(index);
}

UInt32 TileManager::getTileIndexFromID(core_id_t core_id)
{
   // Look up the index from the core list
   // FIXME: make this more cached
   const Config::CoreList & cores(Config::getSingleton()->getCoreListForProcess(Config::getSingleton()->getCurrentProcessNum()));
   UInt32 idx = 0;
   for (Config::CLCI i = cores.begin(); i != cores.end(); i++)
   {
      if (*i == core_id)
         return idx;

      idx++;
   }

   LOG_ASSERT_ERROR(false, "Tile lookup failed for core id: %d!", core_id);
   return INVALID_CORE_ID;
}

core_id_t TileManager::registerSimThread()
{
    if (getCurrentTile() != NULL)
    {
        LOG_PRINT_ERROR("registerSimMemThread - Initialized thread twice");
        return getCurrentTile()->getId();
    }

    ScopedLock sl(m_num_registered_sim_threads_lock);

    LOG_ASSERT_ERROR(m_num_registered_sim_threads < Config::getSingleton()->getNumLocalCores(),
                     "All sim threads already registered. %d > %d",
                     m_num_registered_sim_threads+1, Config::getSingleton()->getNumLocalCores());

    Tile *core = m_tiles.at(m_num_registered_sim_threads);

    m_core_tls->set(core);
    m_core_index_tls->setInt(m_num_registered_sim_threads);
    m_thread_type_tls->setInt(SIM_THREAD);

    ++m_num_registered_sim_threads;

    return core->getId();
}

bool TileManager::amiSimThread()
{
    return m_thread_type_tls->getInt() == SIM_THREAD;
}

bool TileManager::amiUserThread()
{
    return m_thread_type_tls->getInt() == APP_THREAD;
}
