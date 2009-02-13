#include <sched.h>
#include <linux/unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "core_manager.h"
#include "core.h"
#include "network.h"
#include "ocache.h"
#include "config.h"

#include "log.h"
#define LOG_DEFAULT_RANK -1
#define LOG_DEFAULT_MODULE CORE_MANAGER

using namespace std;

CoreManager::CoreManager()
      :
      tid_to_core_map(3*g_config->getNumLocalCores()),
      tid_to_core_index_map(3*g_config->getNumLocalCores()),
      simthread_tid_to_core_map(3*g_config->getNumLocalCores()),
      simthread_tid_to_core_index_map(3*g_config->getNumLocalCores())
{
   LOG_PRINT("Starting CoreManager Constructor.");

   m_maps_lock = Lock::create();

   tid_map = new UInt32 [g_config->getNumLocalCores()];
   core_to_simthread_tid_map = new UInt32 [g_config->getNumLocalCores()];

   // Need to subtract 1 for the MCP
   for (UInt32 i = 0; i < g_config->getNumLocalCores(); i++)
   {
      tid_map[i] = UINT_MAX;
      core_to_simthread_tid_map[i] = UINT_MAX;
      m_cores.push_back(new Core(g_config->getCoreListForProcess(g_config->getCurrentProcessNum())[i]));
   }

   LOG_PRINT("Finished CoreManager Constructor.");
}

CoreManager::~CoreManager()
{
   for (std::vector<Core *>::iterator i = m_cores.begin(); i != m_cores.end(); i++)
      delete *i;

   delete [] core_to_simthread_tid_map;
   delete [] tid_map;

   delete m_maps_lock;
}

void CoreManager::initializeThread(UInt32 core_id)
{
   UInt32 tid = getCurrentTID();

   pair<bool, UINT64> e = tid_to_core_map.find(tid);

   //FIXME: Check to see if two threads try to grab the same core_id
   const Config::CoreList & cores(g_config->getCoreListForProcess(g_config->getCurrentProcessNum()));
   UInt32 idx = 0;
   Config::CLCI i;
   for (i = cores.begin(); i != cores.end(); i++)
   {
      if (*i == core_id)
         break;
      idx++;
   }

   if (i == cores.end())
      LOG_PRINT("Tried to claim a core not assigned to this process.");


   if (e.first == false)
   {
      LOG_ASSERT_ERROR(idx < g_config->getNumLocalCores(), "Invalid tid_map index in initializeThread!\n");
      tid_map[idx] = tid;
      tid_to_core_map.insert(tid, core_id);
   }
   else
   {
      LOG_PRINT("initializeThread : Error initializing core twice: %d", core_id);
   }

}

void CoreManager::initializeThreadFree(int *core_id)
{
   UInt32 tid = getCurrentTID();

   m_maps_lock->acquire();

   pair<bool, UINT64> e = tid_to_core_map.find(tid);

   //FIXME: Check to see if two threads try to grab the same core_id

   if (e.first == false)
   {
      // Don't allow free initializion of the MCP which claimes the
      // highest core.
      for (unsigned int i = 0; i < g_config->getNumLocalCores() - 1; i++)
      {
         if (tid_map[i] == UINT_MAX)
         {
            tid_map[i] = tid;
            tid_to_core_map.insert(tid, i);
            *core_id = i;

            m_maps_lock->release();
         }
      }
      LOG_PRINT("*WARNING* initializeThreadFree - No free cores");
      LOG_NOTIFY_WARNING();
   }
   else
   {
   }

   m_maps_lock->release();
}

UInt32 CoreManager::getCurrentCoreID()
{
   UInt32 id;
   UInt32 tid = getCurrentTID();

   pair<bool, UINT64> e = tid_to_core_map.find(tid);
   id = (e.first == false) ? -1 : e.second;

   ASSERT(!e.first || id < g_config->getTotalCores(), "Illegal core_id value returned by getCurrentCoreID!\n");

   return id;
}

Core *CoreManager::getCurrentCore()
{
   Core *core;
   UInt32 tid = getCurrentTID();

   pair<bool, UINT64> e = tid_to_core_index_map.find(tid);
   core = (e.first == false) ? NULL : m_cores[e.second];

   LOG_ASSERT_ERROR(!e.first || e.second < g_config->getTotalCores(), "Illegal core_id value returned by getCurrentCore!\n");
   return core;
}

Core *CoreManager::getCoreFromID(unsigned int id)
{
   Core *core = NULL;
   // Look up the index from the core list
   // FIXME: make this more cached
   const Config::CoreList & cores(g_config->getCoreListForProcess(g_config->getCurrentProcessNum()));
   UInt32 idx = 0;
   for (Config::CLCI i = cores.begin(); i != cores.end(); i++)
   {
      if (*i == id)
      {
         core = m_cores[idx];
         break;
      }

      idx++;
   }

   LOG_ASSERT_ERROR(!core || idx < g_config->getNumLocalCores(), "Illegal index in getCoreFromID!\n");

   return core;
}

void CoreManager::outputSummary()
{
   LOG_PRINT("Starting CoreManager::fini");

   ofstream out(g_config->getOutputFileName());

   for (UInt32 i = 0; i < g_config->getNumLocalCores(); i++)
   {
      LOG_PRINT("Output summary core %i", i);

      out << "*** Core[" << i << "] summary ***" << endl;
      if (g_config->getEnablePerformanceModeling())
      {
         m_cores[i]->getPerfModel()->outputSummary(out);
         m_cores[i]->getNetwork()->outputSummary(out);
      }

      if (g_config->getEnableDCacheModeling() || g_config->getEnableICacheModeling())
         m_cores[i]->getOCache()->outputSummary(out);

      delete m_cores[i];
      m_cores[i] = NULL;

      out << endl;
   }

   out.close();

   LOG_PRINT("Finish core_manager::fini");
}

int CoreManager::registerSharedMemThread()
{
   UInt32 tid = getCurrentTID();

   ScopedLock sl(*m_maps_lock);
   pair<bool, UINT64> e = simthread_tid_to_core_map.find(tid);

   // If this thread isn't registered
   if (e.first == false)
   {
      // Search for an unused core to map this simthread thread to
      // one less to account for the MCP
      for (UInt32 i = 0; i < g_config->getNumLocalCores(); i++)
      {
         // Unused slots are set to UINT_MAX
         // FIXME: Use a different constant than UINT_MAX
         if (core_to_simthread_tid_map[i] == UINT_MAX)
         {
            core_to_simthread_tid_map[i] = tid;
            simthread_tid_to_core_map.insert(tid, i);
            return g_config->getCoreListForProcess(g_config->getCurrentProcessNum())[i];
         }
      }

      LOG_PRINT("*ERROR* registerSharedMemThread - No free cores for thread: %d", tid);
      for (UInt32 j = 0; j < g_config->getNumLocalCores(); j++)
         LOG_PRINT("core_to_simthread_tid_map[%d] = %d\n", j, core_to_simthread_tid_map[j]);

      LOG_NOTIFY_ERROR();
   }
   else
   {
      LOG_PRINT("*WARNING* registerSharedMemThread - Initialized thread twice");
      LOG_NOTIFY_WARNING();
      // FIXME: I think this is OK
      return simthread_tid_to_core_map.find(tid).second;
   }

   return -1;
}

UInt32 CoreManager::getCurrentTID()
{
   return  syscall(__NR_gettid);
}

