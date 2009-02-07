#include <sched.h>

#include "shared_mem.h"
#include "lock.h"
#include "net_thread_runner.h"
#include "mcp.h"
#include "core_manager.h"
#include "core.h"

#include "log.h"
#define LOG_DEFAULT_RANK   -1
#define LOG_DEFAULT_MODULE SHAREDMEM

// FIXME: Right now the termination code using shared globals only
// works within a single process. For distribution this will not
// work. Fortunately, it *SHOULD* be possible to remove
// synchronization, as the race condition with MCP termination is
// already handled in MCP::finish().

UInt32 g_shared_mem_active_threads;
Lock* g_shared_mem_threads_lock;

extern MCP *g_MCP;

NetThreadRunner *SimSharedMemStartThreads()
{
   g_shared_mem_threads_lock = Lock::create();
   g_shared_mem_active_threads = 0;

   unsigned int num_shared_mem_threads = g_config->getNumLocalCores();

   LOG_PRINT("Starting %d threads on proc: %d\n.", num_shared_mem_threads, g_config->getCurrentProcessNum());

   NetThreadRunner * runners = new NetThreadRunner[num_shared_mem_threads];
   for(unsigned int i = 0; i < num_shared_mem_threads; i++)
   {
      LOG_PRINT("Starting thread %i", i);

      OS_SERVICES::ITHREAD *my_thread_p;
      my_thread_p = OS_SERVICES::ITHREADS::GetSingleton()->Spawn(4096, &runners[i]);
      assert(my_thread_p);
   }

   return runners;
}

void SimSharedMemQuit()
{
   NetPacket pkt;
   pkt.type = SHARED_MEM_TERMINATE_THREADS;
   pkt.length = 0;
   pkt.data = 0;

   g_MCP->broadcastPacket(pkt);

   LOG_PRINT("SimSharedMemQuit : Send quit message.");

   // Only quit when all threads have terminated. Lock shouldn't be
   // necessary here.
   while (g_shared_mem_active_threads > 0)
      sched_yield();

   LOG_PRINT("SimSharedMemQuit : All shared mem threads have terminated.");

   delete g_shared_mem_threads_lock;
}

void SimSharedMemTerminateFunc(void *vp, NetPacket pkt)
{
   bool *pcont = (bool*) vp;
   *pcont = false;
}

void* SimSharedMemThreadFunc(void *)
{
   int core_id = g_core_manager->registerSharedMemThread();
   Network *net = g_core_manager->getCoreFromID(core_id)->getNetwork();
   bool cont = true;

   // Bookkeeping for SimSharedMemQuit
   g_shared_mem_threads_lock->acquire();
   ++g_shared_mem_active_threads;
   g_shared_mem_threads_lock->release();

   // Turn off cont when we receive a quit message
   net->registerCallback(SHARED_MEM_TERMINATE_THREADS,
                         SimSharedMemTerminateFunc,
                         &cont);

   // Actual work gets done here
   while(cont)
      net->netPullFromTransport();

   // Bookkeeping for SimSharedMemQuit
   g_shared_mem_threads_lock->acquire();
   --g_shared_mem_active_threads;
   g_shared_mem_threads_lock->release();

   return 0;
}
