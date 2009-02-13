#include <sched.h>

#include "sim_thread.h"
#include "lock.h"
#include "sim_thread_runner.h"
#include "mcp.h"
#include "core_manager.h"
#include "core.h"

#include "log.h"
#define LOG_DEFAULT_RANK   -1
#define LOG_DEFAULT_MODULE SHAREDMEM

extern MCP *g_MCP;

UInt32 g_sim_thread_active_threads;
Lock* g_sim_thread_threads_lock;

SimThreadRunner *SimThreadStart()
{
   g_sim_thread_threads_lock = Lock::create();
   g_sim_thread_active_threads = 0;

   unsigned int num_sim_thread_threads = g_config->getNumLocalCores();

   LOG_PRINT("Starting %d threads on proc: %d\n.", num_sim_thread_threads, g_config->getCurrentProcessNum());

   SimThreadRunner * runners = new SimThreadRunner[num_sim_thread_threads];
   for(unsigned int i = 0; i < num_sim_thread_threads; i++)
   {
      LOG_PRINT("Starting thread %i", i);

      OS_SERVICES::ITHREAD *my_thread_p;
      my_thread_p = OS_SERVICES::ITHREADS::GetSingleton()->Spawn(4096, &runners[i]);
      assert(my_thread_p);
   }

   return runners;
}

void SimThreadQuit()
{
   if (g_config->getCurrentProcessNum() == g_config->getProcessNumForCore(g_config->getMCPCoreNum()))
   {
      NetPacket pkt;
      pkt.type = SIM_THREAD_TERMINATE_THREADS;
      pkt.length = 0;
      pkt.data = 0;

      LOG_PRINT("Sending quit messages.");

      g_MCP->broadcastPacket(pkt);
   }

   LOG_PRINT("Waiting for local sim threads to exit.");

   // Only quit when all threads have terminated. Lock shouldn't be
   // necessary here.
   while (g_sim_thread_active_threads > 0)
      sched_yield();

   Transport::ptBarrier();

   LOG_PRINT("Done in SimThreadQuit.");
}

void SimThreadTerminateFunc(void *vp, NetPacket pkt)
{
   bool *pcont = (bool*) vp;
   *pcont = false;
}

void* SimThreadFunc(void *)
{
   int core_id = g_core_manager->registerSharedMemThread();
   Network *net = g_core_manager->getCoreFromID(core_id)->getNetwork();
   bool cont = true;

   // Bookkeeping for SimThreadQuit
   g_sim_thread_threads_lock->acquire();
   ++g_sim_thread_active_threads;
   g_sim_thread_threads_lock->release();


   // Turn off cont when we receive a quit message
   net->registerCallback(SIM_THREAD_TERMINATE_THREADS,
                         SimThreadTerminateFunc,
                         &cont);

   // Actual work gets done here
   while(cont)
      net->netPullFromTransport();

   // Bookkeeping for SimThreadQuit
   g_sim_thread_threads_lock->acquire();
   --g_sim_thread_active_threads;
   g_sim_thread_threads_lock->release();

   LOG_PRINT("Sim thread %i exiting", core_id);

   return 0;
}
