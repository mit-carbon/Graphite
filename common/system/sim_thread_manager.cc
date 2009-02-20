#include "sim_thread_manager.h"

#include "lock.h"
#include "log.h"
#include "config.h"
#include "simulator.h"
#include "mcp.h"

#define LOG_DEFAULT_RANK -1
#define LOG_DEFAULT_MODULE SIM_THREAD_MANAGER

SimThreadManager::SimThreadManager()
   : m_active_threads_lock(Lock::create())
   , m_active_threads(0)
{
}

SimThreadManager::~SimThreadManager()
{
   LOG_ASSERT_WARNING(m_active_threads == 0,
                      "*WARNING* Threads still active when SimThreadManager exits.");
   delete m_active_threads_lock;
}

void SimThreadManager::spawnSimThreads()
{
   UInt32 num_sim_threads = Config::getSingleton()->getNumLocalCores();

   LOG_PRINT("Starting %d threads on proc: %d\n.", num_sim_threads, Config::getSingleton()->getCurrentProcessNum());

   m_sim_threads = new SimThread [num_sim_threads];

   for (UInt32 i = 0; i < num_sim_threads; i++)
   {
      LOG_PRINT("Starting thread %i", i);
      m_sim_threads[i].spawn();
   }

   while (m_active_threads < num_sim_threads)
      sched_yield();

   LOG_PRINT("Threads started.");
}

void SimThreadManager::quitSimThreads()
{
   // FIXME: This should use the LCP to communicate to all the local
   // cores so that each process handles termination itself.

   if (Config::getSingleton()->getCurrentProcessNum() 
       == 
       Config::getSingleton()->getProcessNumForCore(Config::getSingleton()->getMCPCoreNum()))
   {
      NetPacket pkt;
      pkt.type = SIM_THREAD_TERMINATE_THREADS;
      pkt.length = 0;
      pkt.data = 0;

      LOG_PRINT("Sending quit messages.");

      Sim()->getMCP()->broadcastPacket(pkt);
   }

   LOG_PRINT("Waiting for local sim threads to exit.");

   while (m_active_threads > 0)
      sched_yield();

   Transport::getSingleton()->barrier();
}

void SimThreadManager::simThreadStartCallback()
{
   m_active_threads_lock->acquire();
   ++m_active_threads;
   m_active_threads_lock->release();
}

void SimThreadManager::simThreadExitCallback()
{
   m_active_threads_lock->acquire();
   --m_active_threads;
   m_active_threads_lock->release();
}
