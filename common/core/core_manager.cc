#include <sched.h>
#include <linux/unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "core_manager.h"
#include "core.h"
#include "config.h"

#include "log.h"
#define LOG_DEFAULT_RANK -1
#define LOG_DEFAULT_MODULE CORE_MANAGER

using namespace std;

CoreManager::CoreManager()
   :
   tid_to_core_map(3*g_config->getNumLocalCores()),
   tid_to_core_index_map(3*g_config->getNumLocalCores()),
   shmem_tid_to_core_map(3*g_config->getNumLocalCores()),
   shmem_tid_to_core_index_map(3*g_config->getNumLocalCores()),
   prev_rank(0) 
{
   LOG_PRINT("Starting CoreManager Constructor.");

   maps_lock = Lock::create();

   tid_map = new UInt32 [g_config->getNumLocalCores()];
   core_to_shmem_tid_map = new UInt32 [g_config->getNumLocalCores()];

   m_cores = new Core[g_config->getNumLocalCores()];

   // Need to subtract 1 for the MCP
   for(UInt32 i = 0; i < g_config->getNumLocalCores(); i++) 
   {
      tid_map[i] = UINT_MAX;
      core_to_shmem_tid_map[i] = UINT_MAX;
      m_cores[i].coreInit(g_config->getCoreListForProcess(g_config->getCurrentProcessNum())[i], g_config->getNumLocalCores());
   }

   LOG_PRINT("Finished CoreManager Constructor.");
}

CoreManager::~CoreManager()
{
   delete [] m_cores;

   delete [] core_to_shmem_tid_map;
   delete [] tid_map;

   delete maps_lock;
}

void CoreManager::initializeThread(int rank)
{
   UInt32 tid = getCurrentTID();

   pair<bool, UINT64> e = tid_to_core_map.find(tid);

   //FIXME: Check to see if two cores try to grab the same rank

   if ( e.first == false ) {
      tid_map[rank] = tid;
      tid_to_core_map.insert( tid, rank );
   }
   else
   {
      LOG_PRINT("initializeThread : Error initializing core twice: %d", rank);
   }

}

void CoreManager::initializeThreadFree(int *rank)
{
   UInt32 tid = getCurrentTID();

   maps_lock->acquire();

   pair<bool, UINT64> e = tid_to_core_map.find(tid);

   //FIXME: Check to see if two cores try to grab the same rank

   if ( e.first == false ) {
      // Don't allow free initializion of the MCP which claimes the
      // highest core.
      for(unsigned int i = 0; i < g_config->getNumLocalCores() - 1; i++)
      {
         if (tid_map[i] == UINT_MAX)
         {
            tid_map[i] = tid;    
            tid_to_core_map.insert( tid, i );
            *rank = i;

            maps_lock->release();
         }
      }
      
      LOG_PRINT("*WARNING* initializeThreadFree - No free cores");
      LOG_NOTIFY_WARNING();
   }
   else
   {
   }

   maps_lock->release();
}

UInt32 CoreManager::getCurrentCoreID()
{
   UInt32 id;
   UInt32 tid = getCurrentTID();

   pair<bool, UINT64> e = tid_to_core_map.find(tid);
   id = (e.first == false) ? -1 : e.second;

   ASSERT(!e.first || id < g_config->getTotalCores(), "Illegal rank value returned by getCurrentCoreID!\n");

   return id;
}

Core *CoreManager::getCurrentCore()
{
   Core *core;
   UInt32 tid = getCurrentTID();

   pair<bool, UINT64> e = tid_to_core_index_map.find(tid);
   core = (e.first == false) ? NULL : &m_cores[e.second];

   ASSERT(!e.first || e.second < g_config->getTotalCores(), "Illegal rank value returned by getCurrentCore!\n");
   return core;
}

Core *CoreManager::getCoreFromID(unsigned int id)
{
   Core *core = NULL;
   // Look up the index from the core list
   // FIXME: make this more cached
   const Config::CoreList & cores (g_config->getCoreListForProcess(g_config->getCurrentProcessNum()));
   UInt32 idx = 0;
   for(Config::CLCI i = cores.begin(); i != cores.end(); i++)
   {
      if(*i == id)
      {
         core = &m_cores[idx];
         break;
      }

      idx++;
   }

   ASSERT(!core || idx < g_config->getNumLocalCores(), "Illegal index in getCoreFromID!\n");

   return core;
}

void CoreManager::fini(int code, void *v)
{
   LOG_PRINT("Starting CoreManager::fini");

   ofstream out( g_config->getOutputFileName() );

   for(UInt32 i = 0; i < g_config->getNumLocalCores(); i++)
   {
      LOG_PRINT("Output summary core %i", i);

      out << "*** Core[" << i << "] summary ***" << endl;
      m_cores[i].fini(code, v, out); 
      out << endl;
   }

   out.close();

   LOG_PRINT("Finish core_manager::fini");
}

int CoreManager::registerSharedMemThread()
{
   // FIXME: I need to lock because there is a race condition
   UInt32 tid = getCurrentTID();

   maps_lock->acquire();
   pair<bool, UINT64> e = shmem_tid_to_core_map.find(tid);

   // If this thread isn't registered
   if ( e.first == false ) {

      // Search for an unused core to map this shmem thread to
      // one less to account for the MCP
      for(UInt32 i = 0; i < g_config->getNumLocalCores(); i++)
      {
         // Unused slots are set to UINT_MAX
         // FIXME: Use a different constant than UINT_MAX
         if(core_to_shmem_tid_map[i] == UINT_MAX)
         {
            core_to_shmem_tid_map[i] = tid;    
            shmem_tid_to_core_map.insert( tid, i );
            maps_lock->release();
            return g_config->getCoreListForProcess(g_config->getCurrentProcessNum())[i];
         }
         else
         {
            LOG_PRINT("core_to_shmem_tid_map[%d] = %d\n", i, core_to_shmem_tid_map[i]);
         }
      }

      LOG_PRINT("*WARNING* registerSharedMemThread - No free cores");
      LOG_NOTIFY_WARNING();
   }
   else
   {
      LOG_PRINT("*WARNING* registerSharedMemThread - Initialized thread twice");
      LOG_NOTIFY_WARNING();
      maps_lock->release();
      // FIXME: I think this is OK
      return shmem_tid_to_core_map.find(tid).second;
   }

   maps_lock->release();
   return -1;
}

UInt32 CoreManager::getCurrentTID()
{
   return  syscall( __NR_gettid );
}

