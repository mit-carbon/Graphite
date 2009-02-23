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
#include "packetize.h"
#include "message_types.h"

#include "log.h"
#define LOG_DEFAULT_RANK -1
#define LOG_DEFAULT_MODULE CORE_MANAGER

using namespace std;

CoreManager::CoreManager()
      :
      tid_to_core_map(3*Config::getSingleton()->getNumLocalCores()),
      tid_to_core_index_map(3*Config::getSingleton()->getNumLocalCores()),
      simthread_tid_to_core_map(3*Config::getSingleton()->getNumLocalCores())
{
   LOG_PRINT("Starting CoreManager Constructor.");

   m_maps_lock = Lock::create();

   UInt32 num_local_cores = Config::getSingleton()->getNumLocalCores();

   tid_map = new UInt32 [num_local_cores];
   core_to_simthread_tid_map = new UInt32 [num_local_cores];

   UInt32 proc_id = Config::getSingleton()->getCurrentProcessNum();
   const Config::CoreList &local_cores = Config::getSingleton()->getCoreListForProcess(proc_id);

   for (UInt32 i = 0; i < num_local_cores; i++)
   {
      tid_map[i] = UINT_MAX;
      core_to_simthread_tid_map[i] = UINT_MAX;
      m_cores.push_back(new Core(local_cores[i]));
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

void CoreManager::initializeCommId(UInt32 comm_id)
{
   UInt32 tid = getCurrentTID();
   pair<bool, UINT64> e = tid_to_core_map.find(tid);

   LOG_ASSERT_ERROR(e.first, "*ERROR* initializeCommId: Called without binding thread to a core.");
   UInt32 core_id = e.second;

   UnstructuredBuffer send_buff;
   send_buff << (SInt32)LCP_MESSAGE_COMMID_UPDATE << comm_id << core_id;

   LOG_PRINT("Initializing comm_id: %d to core_id: %d", comm_id, core_id);

   // Broadcast this update to other processes
   Transport::Node *global_node = Transport::getSingleton()->getGlobalNode();
   UInt32 num_procs = Config::getSingleton()->getProcessCount();

   for (UInt32 i = 0; i < num_procs; i++)
   {
      global_node->globalSend(i,
                              send_buff.getBuffer(),
                              send_buff.size());
   }
}

void CoreManager::initializeThread()
{
   ScopedLock scoped_maps_lock(*m_maps_lock);
   UInt32 tid = getCurrentTID();
   pair<bool, UInt64> e = tid_to_core_map.find(tid);

   LOG_ASSERT_WARNING(e.first == false, "*WARNING* Thread: %d already mapped to core: %d", tid, e.second);

   for (UInt32 i = 0; i < Config::getSingleton()->getNumLocalCores(); i++)
   {
      if (tid_map[i] == UINT_MAX)
      {
         UInt32 core_id = Config::getSingleton()->getCoreListForProcess(Config::getSingleton()->getCurrentProcessNum())[i];
         tid_map[i] = tid;
         tid_to_core_index_map.insert(tid, i);
         tid_to_core_map.insert(tid, core_id);
         LOG_PRINT("Initialize thread : index %d mapped to: thread %d, core_id: %d", i, tid_map[i], core_id);
         return;
      }
      else
      {
         // LOG_PRINT("%d/%d already mapped to: %d", i, Config::getSingleton()->getNumLocalCores(), tid_map[i]);
      }
   }

   LOG_PRINT("*ERROR* initializeThread - No free cores out of %d total.", Config::getSingleton()->getNumLocalCores());
   LOG_NOTIFY_ERROR();
}

void CoreManager::initializeThread(UInt32 core_id)
{
   ScopedLock scoped_maps_lock(*m_maps_lock);
   UInt32 tid = getCurrentTID();
   pair<bool, UInt64> e = tid_to_core_map.find(tid);

   LOG_ASSERT_ERROR(e.first == false, "Tried to initialize core %d twice.", core_id);

   for (unsigned int i = 0; i < Config::getSingleton()->getNumLocalCores(); i++)
   {
      UInt32 local_core_id = Config::getSingleton()->getCoreListForProcess(Config::getSingleton()->getCurrentProcessNum())[i];
      if(local_core_id == core_id)
      {
         if (tid_map[i] == UINT_MAX)
         {
            tid_map[i] = tid;
            tid_to_core_index_map.insert(tid, i);
            tid_to_core_map.insert(tid, core_id);
            LOG_PRINT("Initialize thread : index %d mapped to: thread %d, core_id: %d", i, tid_map[i], core_id);
            return;
         }
         else
         {
            LOG_PRINT("*ERROR* initializeThread -- %d/%d already mapped to thread %d", i, Config::getSingleton()->getNumLocalCores(), tid_map[i]);
            LOG_NOTIFY_ERROR();
         }
      }
   }

   LOG_PRINT("*ERROR* initializeThread - Requested core %d does not live on process %d.", core_id, Config::getSingleton()->getCurrentProcessNum());
   LOG_NOTIFY_ERROR();
}

void CoreManager::terminateThread()
{
   ScopedLock scoped_maps_lock(*m_maps_lock);
   UInt32 tid = getCurrentTID();
   LOG_PRINT("CoreManager::terminating thread: %d", tid);
   pair<bool, UInt64> e = tid_to_core_map.find(tid);

   LOG_ASSERT_WARNING(e.first == true, "*WARNING* Thread: %d not initialized while terminating.", e.second);

   // If it's not in the tid_to_core_map, well then we don't need to remove it
   if(e.first == false)
       return;

   for (UInt32 i = 0; i < Config::getSingleton()->getNumLocalCores(); i++)
   {
      if (tid_map[i] == tid)
      {
         tid_map[i] = UINT_MAX;
         tid_to_core_index_map.remove(tid);
         tid_to_core_map.remove(tid);
         LOG_PRINT("Terminate thread : removed %d", e.second);
         return;
      }
      else
      {
         // LOG_PRINT("%d/%d already mapped to: %d", i, Config::getSingleton()->getNumLocalCores(), tid_map[i]);
      }
   }

   LOG_PRINT("*ERROR* terminateThread - Thread tid: %d not found in list.", e.second);
   LOG_NOTIFY_ERROR();
}

UInt32 CoreManager::getCurrentCoreID()
{
   UInt32 id;
   UInt32 tid = getCurrentTID();

   pair<bool, UINT64> e = tid_to_core_map.find(tid);
   id = (e.first == false) ? -1 : e.second;

   LOG_ASSERT_ERROR(!e.first || id < Config::getSingleton()->getTotalCores(), "Illegal core_id value returned by getCurrentCoreID!\n");

   return id;
}

Core *CoreManager::getCurrentCore()
{
   Core *core;
   UInt32 tid = getCurrentTID();

   pair<bool, UINT64> e = tid_to_core_index_map.find(tid);
   core = (e.first == false) ? NULL : m_cores[e.second];

   LOG_ASSERT_ERROR(!e.first || e.second < Config::getSingleton()->getTotalCores(), "Illegal core_id value returned by getCurrentCore!\n");
   return core;
}

Core *CoreManager::getCoreFromID(UInt32 id)
{
   Core *core = NULL;
   // Look up the index from the core list
   // FIXME: make this more cached
   const Config::CoreList & cores(Config::getSingleton()->getCoreListForProcess(Config::getSingleton()->getCurrentProcessNum()));
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

   LOG_ASSERT_ERROR(!core || idx < Config::getSingleton()->getNumLocalCores(), "Illegal index in getCoreFromID!\n");

   return core;
}

Core *CoreManager::getCoreFromIndex(UInt32 index)
{
   LOG_ASSERT_ERROR(index < Config::getSingleton()->getNumLocalCores(), "getCoreFromIndex -- invalid index %d", index);

   return m_cores[index];
}

void CoreManager::outputSummary()
{
   LOG_PRINT("Starting CoreManager::outputSummary");

   ofstream out(Config::getSingleton()->getOutputFileName());

   for (UInt32 i = 0; i < Config::getSingleton()->getNumLocalCores(); i++)
   {
      LOG_PRINT("Output summary core %i", i);

      out << "*** Core[" << i << "] summary ***" << endl;
      if (Config::getSingleton()->getEnablePerformanceModeling())
      {
         m_cores[i]->getPerfModel()->outputSummary(out);
         m_cores[i]->getNetwork()->outputSummary(out);
      }

      if (Config::getSingleton()->getEnableDCacheModeling() || Config::getSingleton()->getEnableICacheModeling())
         m_cores[i]->getOCache()->outputSummary(out);

      out << endl;
   }

   out.close();
}

int CoreManager::registerSimMemThread()
{
   UInt32 tid = getCurrentTID();

   ScopedLock sl(*m_maps_lock);
   pair<bool, UINT64> e = simthread_tid_to_core_map.find(tid);

   // If this thread isn't registered
   if (e.first == false)
   {
      // Search for an unused core to map this simthread thread to
      // one less to account for the MCP
      for (UInt32 i = 0; i < Config::getSingleton()->getNumLocalCores(); i++)
      {
         // Unused slots are set to UINT_MAX
         // FIXME: Use a different constant than UINT_MAX
         if (core_to_simthread_tid_map[i] == UINT_MAX)
         {
            core_to_simthread_tid_map[i] = tid;
            simthread_tid_to_core_map.insert(tid, i);
            return Config::getSingleton()->getCoreListForProcess(Config::getSingleton()->getCurrentProcessNum())[i];
         }
      }

      LOG_PRINT("*ERROR* registerSimMemThread - No free cores for thread: %d", tid);
      for (UInt32 j = 0; j < Config::getSingleton()->getNumLocalCores(); j++)
         LOG_PRINT("core_to_simthread_tid_map[%d] = %d\n", j, core_to_simthread_tid_map[j]);

      LOG_NOTIFY_ERROR();
   }
   else
   {
      LOG_PRINT("*WARNING* registerSimMemThread - Initialized thread twice");
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

